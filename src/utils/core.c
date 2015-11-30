//some core utiliy\ties copied from uv core.c
//TODO: this need to be cleaned up, I just copied it as is

#include <stddef.h> /* NULL */
#include <stdio.h> /* printf */
#include <stdlib.h>
#include <string.h> /* strerror */
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h> /* INT_MAX, PATH_MAX, IOV_MAX */
#include <sys/uio.h> /* writev */
#include <sys/resource.h> /* getrusage */
#include <pwd.h>

#ifdef __linux__
# include <sys/ioctl.h>
#endif

#ifdef __sun
# include <sys/types.h>
# include <sys/wait.h>
#endif

#ifdef __APPLE__
# include <mach-o/dyld.h> /* _NSGetExecutablePath */
# include <sys/filio.h>
# include <sys/ioctl.h>
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/sysctl.h>
# include <sys/filio.h>
# include <sys/ioctl.h>
# include <sys/wait.h>
# define COMO__O_CLOEXEC O_CLOEXEC
# if defined(__FreeBSD__) && __FreeBSD__ >= 10
#  define COMO__accept4 accept4
#  define COMO__SOCK_NONBLOCK SOCK_NONBLOCK
#  define COMO__SOCK_CLOEXEC  SOCK_CLOEXEC
# endif
# if !defined(F_DUP2FD_CLOEXEC) && defined(_F_DUP2FD_CLOEXEC)
#  define F_DUP2FD_CLOEXEC  _F_DUP2FD_CLOEXEC
# endif
#endif

#ifdef _AIX
#include <sys/ioctl.h>
#endif

int como__close(int fd) {
  int saved_errno;
  int rc;

  assert(fd > -1);  /* Catch uninitialized io_watcher.fd bugs. */
  assert(fd > STDERR_FILENO);  /* Catch stdio close bugs. */

  saved_errno = errno;
  rc = close(fd);
  if (rc == -1) {
    rc = errno;
    if (rc == -EINTR)
      rc = -EINPROGRESS;  /* For platform/libc consistency. */
    errno = saved_errno;
  }

  return rc;
}

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || \
    defined(_AIX) || defined(__DragonFly__)

int como__nonblock(int fd, int set) {
  int r;

  do
    r = ioctl(fd, FIONBIO, &set);
  while (r == -1 && errno == EINTR);

  if (r)
    return errno;

  return 0;
}


int como__cloexec(int fd, int set) {
  int r;

  do
    r = ioctl(fd, set ? FIOCLEX : FIONCLEX);
  while (r == -1 && errno == EINTR);

  if (r)
    return errno;

  return 0;
}

#else /* !(defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || \
	   defined(_AIX) || defined(__DragonFly__)) */

int como__nonblock(int fd, int set) {
  int flags;
  int r;

  do
    r = fcntl(fd, F_GETFL);
  while (r == -1 && errno == EINTR);

  if (r == -1)
    return errno;

  /* Bail out now if already set/clear. */
  if (!!(r & O_NONBLOCK) == !!set)
    return 0;

  if (set)
    flags = r | O_NONBLOCK;
  else
    flags = r & ~O_NONBLOCK;

  do
    r = fcntl(fd, F_SETFL, flags);
  while (r == -1 && errno == EINTR);

  if (r)
    return errno;

  return 0;
}


int como__cloexec(int fd, int set) {
  int flags;
  int r;

  do
    r = fcntl(fd, F_GETFD);
  while (r == -1 && errno == EINTR);

  if (r == -1)
    return errno;

  /* Bail out now if already set/clear. */
  if (!!(r & FD_CLOEXEC) == !!set)
    return 0;

  if (set)
    flags = r | FD_CLOEXEC;
  else
    flags = r & ~FD_CLOEXEC;

  do
    r = fcntl(fd, F_SETFD, flags);
  while (r == -1 && errno == EINTR);

  if (r)
    return errno;

  return 0;
}

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || \
	  defined(_AIX) || defined(__DragonFly__) */


/* This function is not execve-safe, there is a race window
 * between the call to dup() and fcntl(FD_CLOEXEC).
 */
int como__dup(int fd) {
  int err;

  fd = dup(fd);

  if (fd == -1)
    return errno;

  err = como__cloexec(fd, 1);
  if (err) {
    como__close(fd);
    return err;
  }

  return fd;
}


ssize_t como__recvmsg(int fd, struct msghdr* msg, int flags) {
  struct cmsghdr* cmsg;
  ssize_t rc;
  int* pfd;
  int* end;
#if defined(__linux__)
  static int no_msg_cmsg_cloexec;
  if (no_msg_cmsg_cloexec == 0) {
    rc = recvmsg(fd, msg, flags | 0x40000000);  /* MSG_CMSG_CLOEXEC */
    if (rc != -1)
      return rc;
    if (errno != EINVAL)
      return errno;
    rc = recvmsg(fd, msg, flags);
    if (rc == -1)
      return errno;
    no_msg_cmsg_cloexec = 1;
  } else {
    rc = recvmsg(fd, msg, flags);
  }
#else
  rc = recvmsg(fd, msg, flags);
#endif
  if (rc == -1)
    return errno;
  if (msg->msg_controllen == 0)
    return rc;
  for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg))
    if (cmsg->cmsg_type == SCM_RIGHTS)
      for (pfd = (int*) CMSG_DATA(cmsg),
           end = (int*) ((char*) cmsg + cmsg->cmsg_len);
           pfd < end;
           pfd += 1)
        como__cloexec(*pfd, 1);
  return rc;
}


int uv_cwd(char* buffer, size_t* size) {
  if (buffer == NULL || size == NULL)
    return -EINVAL;

  if (getcwd(buffer, *size) == NULL)
    return errno;

  *size = strlen(buffer);
  if (*size > 1 && buffer[*size - 1] == '/') {
    buffer[*size-1] = '\0';
    (*size)--;
  }

  return 0;
}


int uv_chdir(const char* dir) {
  if (chdir(dir))
    return errno;

  return 0;
}


void como_disable_stdio_inheritance(void) {
  int fd;

  /* Set the CLOEXEC flag on all open descriptors. Unconditionally try the
   * first 16 file descriptors. After that, bail out after the first error.
   */
  for (fd = 0; ; fd++)
    if (como__cloexec(fd, 1) && fd > 15)
      break;
}
