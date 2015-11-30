#ifndef _COMO_ERRNO_H
#define _COMO_ERRNO_H

#define COMOEOF -4095

#if !defined(EADDRINUSE) && defined(_WIN32)
# define EADDRINUSE WSAEADDRINUSE
#endif

#if !defined(ENOTSOCK) && defined(_WIN32)
# define ENOTSOCK WSAENOTSOCK
#endif

#if !defined(ECONNRESET) && defined(_WIN32)
# define ECONNRESET WSAECONNRESET
#endif

#if !defined(EWOULDBLOCK) && defined(_WIN32)
# define EWOULDBLOCK  WSAEWOULDBLOCK
#endif

#if !defined(ECONNABORTED) && defined(_WIN32)
# define ECONNABORTED  WSAECONNABORTED
#endif

#if !defined(EAFNOSUPPORT) && defined(_WIN32)
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif

#if !defined(EACCES) && defined(_WIN32)
#define EACCES WSAEACCES
#endif

#if !defined(EMFILE) && defined(_WIN32)
#define EMFILE WSAEMFILE
#endif

#if !defined(ESOCKTNOSUPPORT) && defined(_WIN32)
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#endif

#if !defined(EPROTONOSUPPORT) && defined(_WIN32)
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#endif

#if !defined(EADDRNOTAVAIL) && defined(_WIN32)
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#endif

#if !defined(ECONNREFUSED) && defined(_WIN32)
#define ECONNREFUSED WSAECONNREFUSED
#endif

#if !defined(EINPROGRESS) && defined(_WIN32)
#define EINPROGRESS WSAEINPROGRESS
#endif

#if !defined(EALREADY) && defined(_WIN32)
#define EALREADY WSAEALREADY
#endif

#if !defined(EISCONN) && defined(_WIN32)
#define EISCONN WSAEISCONN
#endif

#if !defined(ENOBUFS) && defined(_WIN32)
#define ENOBUFS WSAENOBUFS
#endif

#if !defined(ENOTCONN) && defined(_WIN32)
# define ENOTCONN  WSAENOTCONN
#endif

#ifdef _WIN32
#define SOCKEINTR WSAEINTR
#else
#define SOCKEINTR EINTR
#endif

#endif /* _COMO_ERRNO_H */
