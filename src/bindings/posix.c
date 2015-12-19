#ifdef _WIN32

#define fsync(fd) _commit(fd)
#define ftruncate(a,b) _chsize_s(a,b)

size_t pread (int fd, void *buffer, size_t len, long position){
    lseek(fd, position, SEEK_SET);
    size_t nread = read(fd, buffer, len);
    return nread;
}

#endif

int como__stat (duk_context *ctx, const char *filename, int fd, int lst) {
    struct stat st;
    int ret;
    if (filename != NULL){
        if (lst == 1){
            ret = stat(filename, &st);
        } else {
            ret = stat(filename, &st);
        }
    } else {
        ret = fstat(fd, &st);
    }

    if (ret == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }

    duk_push_object(ctx);

    duk_push_int(ctx, st.st_dev);
    duk_put_prop_string(ctx, -2, "dev");

    duk_push_int(ctx, st.st_mode);
    duk_put_prop_string(ctx, -2, "mode");

    duk_push_int(ctx, st.st_uid);
    duk_put_prop_string(ctx, -2, "uid");

    duk_push_int(ctx, st.st_gid);
    duk_put_prop_string(ctx, -2, "gid");

    duk_push_int(ctx, st.st_rdev);
    duk_put_prop_string(ctx, -2, "rdev");

    duk_push_int(ctx, st.st_nlink);
    duk_put_prop_string(ctx, -2, "nlink");

    duk_push_int(ctx, st.st_size);
    duk_put_prop_string(ctx, -2, "size");

    #ifdef _WIN32
    duk_push_undefined(ctx);
    #else
    duk_push_int(ctx, st.st_blksize);
    #endif
    duk_put_prop_string(ctx, -2, "blksize");

    #ifdef _WIN32
    duk_push_undefined(ctx);
    #else
    duk_push_int(ctx, st.st_blocks);
    #endif
    duk_put_prop_string(ctx, -2, "blocks");

    duk_push_int(ctx, st.st_ino);
    duk_put_prop_string(ctx, -2, "ino");

    duk_push_int(ctx, st.st_atime);
    duk_put_prop_string(ctx, -2, "atime");

    duk_push_int(ctx, st.st_ctime);
    duk_put_prop_string(ctx, -2, "ctime");

    duk_push_int(ctx, st.st_mtime);
    duk_put_prop_string(ctx, -2, "mtime");

    return 1;
}

COMO_METHOD(como_posix_stat) {
    const char *file = duk_require_string(ctx, 0);
    return como__stat(ctx, file, 0, 0);
}

COMO_METHOD(como_posix_lstat) {
    const char *file = duk_require_string(ctx, 0);
    return como__stat(ctx, file, 0, 1);
}

COMO_METHOD(como_posix_fstat) {
    int fd = duk_require_int(ctx, 0);
    return como__stat(ctx, NULL, fd, 0);
}

COMO_METHOD(como_posix_open) {
    const char *name = duk_require_string(ctx, 0);
    int flags        = duk_require_int(ctx, 1);
    int fd;

    if (duk_is_number(ctx, 2)){
        fd = open(name, flags, duk_get_int(ctx, 2) /* permission */);
    } else {
       fd = open(name, flags);
    }

    if (fd == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_int(ctx, fd);
    return 1;
}

COMO_METHOD(como_posix_close) {
    int fd   = duk_require_int(ctx, 0);

    if (close(fd) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_dup2) {
    int fd1 = duk_require_int(ctx, 0);
    int fd2 = duk_require_int(ctx, 1);

    int fd = dup2(fd1, fd2);
    if (fd == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_int(ctx, fd);
    return 1;
}

COMO_METHOD(como_posix_unlink) {
    const char *path = duk_require_string(ctx, 0);
    
    //#ifdef _WIN32
    chmod(path, 0666);
    //#endif

    if (unlink(path) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_access) {
    const char *file = duk_require_string(ctx, 0);
    int mode         = duk_require_int(ctx, 1);

    if (access(file, mode) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_mkdir) {
    const char *file = duk_require_string(ctx, 0);
    int ret;

    #ifdef _WIN32
    ret = mkdir(file);
    #else
    int mode  = duk_require_uint(ctx, 1);
    ret = mkdir(file, mode);
    #endif

    if (ret == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_rmdir) {
    const char *file = duk_require_string(ctx, 0);
    if (rmdir(file) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_chmod) {
    const char *file = duk_require_string(ctx, 0);
    int mode         = duk_require_int(ctx, 1);
    if (chmod(file, mode) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_fchmod) {
    #ifndef _WIN32
    int fd     = duk_require_int(ctx, 0);
    int mode   = duk_require_int(ctx, 1);
    if (fchmod(fd, mode) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    #endif
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_posix_fsync) {
    int fd     = duk_require_int(ctx, 0);
    if (fsync(fd) == -1) {
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  posix read
  posix.read(fd, [bytestoread | [buffer [, buffer_offset [, bytestoread] ]]]);

  posix.read(fd, length); => returns string
  posix.read(fd, buffer); => returns number of bytes read
  posix.read(fd, buffer, length); => returns number of bytes read
  posix.read(fd, [buffer, offset]); => returns number of bytes read
  posix.read(fd, [buffer, offset], length); => returns number of bytes read


 ============================================================================*/
COMO_METHOD(como_posix_read) {
    int fd  = duk_require_int(ctx, 0);

    int isPread = 0;
    int isBufferRead = 0;
    void *buffer;
    size_t len;
    long position;

    if (duk_is_number(ctx, 1)){
        len = duk_require_uint(ctx, 1);
        buffer = duk_push_fixed_buffer(ctx, len);
    } else {
        size_t buffer_length;
        size_t off = 0;

        //[buffer, offset]
        if (duk_is_array(ctx, 1)){

            duk_get_prop_index(ctx, 1, 0);
            buffer = duk_require_buffer_data(ctx, -1, &buffer_length);

            duk_get_prop_index(ctx, 1, 1);
            off = duk_require_uint(ctx, -1);

            duk_pop_2(ctx);
            
            if (off >=  buffer_length){
                duk_error(ctx, DUK_ERR_RANGE_ERROR, "Offset is out of bounds");
            }

        } else {
            buffer = duk_require_buffer_data(ctx, 1, &buffer_length);
            off = 0;
        }

        //read length
        if (duk_is_number(ctx, 2)){
            len = duk_require_uint(ctx, 2);
            if (off + len > buffer_length){
                duk_error(ctx, DUK_ERR_RANGE_ERROR, "Length extends beyond buffer");
            }
        } else {
            len = buffer_length - off;
        }

        if (duk_is_number(ctx, 3)){
            isPread = 1;
            position = duk_get_number(ctx, 3);
        }

        buffer = buffer + off;
        isBufferRead = 1; /* read into buffer */
    }

    size_t ret;

    if (isPread){
        ret = pread(fd, buffer, len, position);
    } else {
        ret = read(fd, buffer, len);
    }

    if (ret == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }

    //buffer has been filled just return number of read
    if (isBufferRead == 1){
        duk_push_uint(ctx, ret);
    } else {
        duk_to_lstring(ctx, -1, &ret);
    }
    
    return 1;
}

COMO_METHOD(como_posix_writeBuffer) {
    int fd  = duk_require_int(ctx, 0);

    void *buffer;
    size_t len;
    size_t buffer_length;
    size_t off = 0;

    //[buffer | string, offset]
    if (duk_is_array(ctx, 1)){

        duk_get_prop_index(ctx, 1, 0);
        if (duk_is_string(ctx, -1)){
            buffer = duk_to_buffer(ctx, -1, &buffer_length);
        } else {
            buffer = duk_require_buffer_data(ctx, -1, &buffer_length);
        }
        

        duk_get_prop_index(ctx, 1, 1);
        off = duk_require_uint(ctx, -1);

        duk_pop_2(ctx);
        
        if (off >=  buffer_length){
            duk_error(ctx, DUK_ERR_RANGE_ERROR, "Offset is out of bounds");
        }

    } else {
        buffer = duk_require_buffer_data(ctx, 1, &buffer_length);
        off = 0;
    }

    //read length
    if (duk_is_number(ctx, 2)){
        len = duk_require_uint(ctx, 2);
        if (off + len > buffer_length){
            duk_error(ctx, DUK_ERR_RANGE_ERROR, "Length extends beyond buffer");
        }
    } else {
        len = buffer_length - off;
    }

    buffer = buffer + off;

    size_t ret = write(fd, buffer, len);
    if (ret == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }

    duk_push_uint(ctx, ret);
    
    return 1;
}


int como__readdir(duk_context *ctx, const char *dir, size_t length_of_dir) {

    duk_idx_t arr_idx = duk_push_array(ctx);
    int index = 0;

    #ifdef _WIN32
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    if (length_of_dir > (MAX_PATH - 3)) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "dir name is too long");
        return -1;
    }

    // Find the first file in the directory.
    hFind = FindFirstFile(dir, &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return -1;
    }

    do {
        
        if (strcmp(ffd.cFileName, ".") == 0 ||
            strcmp(ffd.cFileName, "..") == 0) {
            continue;
        }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            duk_push_string(ctx, ffd.cFileName);
        } else {
            duk_push_string(ctx, ffd.cFileName);
        }
        duk_put_prop_index(ctx, arr_idx, index++);
    } while (FindNextFile(hFind, &ffd) != 0);

    errno = GetLastError();
    FindClose(hFind);

    if (errno != ERROR_NO_MORE_FILES) return -1;
    #else
    DIR *dp;
    struct dirent *ep;
    dp = opendir (dir);
    if (dp != NULL) {
        while (ep = readdir (dp)) {
            if (ep->d_name[0] != '.') {
                duk_push_string(ctx, ep->d_name);
                duk_put_prop_index(ctx, arr_idx, index++);
            }
        }
        (void) closedir (dp);
    } else {
        return -1;
    }
    #endif
    return 0;
}

COMO_METHOD(como_posix_readdir) {
    size_t length;
    const char *dir = duk_require_lstring(ctx, 0, &length);
    int ret = como__readdir(ctx, dir, length);
    if (ret == -1){
        duk_pop(ctx);
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    return 1;
}

static const duk_function_list_entry como_posix_funcs[] = {
    {"stat", como_posix_stat,                1},
    {"fstat", como_posix_fstat,              1},
    {"lstat", como_posix_lstat,              1},
    {"open", como_posix_open,                3},
    {"close", como_posix_close,              1},
    {"unlink", como_posix_unlink,            1},
    {"dup2", como_posix_dup2,                2},
    {"read", como_posix_read,                4},
    {"access", como_posix_access,            2},
    {"mkdir", como_posix_mkdir,              2},
    {"chmod", como_posix_chmod,              2},
    {"fchmod", como_posix_fchmod,            2},
    {"rmdir", como_posix_rmdir,              1},
    {"readdir", como_posix_readdir,          1},
    {"fsync", como_posix_fsync,              1},
    {"writeBuffer", como_posix_writeBuffer,  3},
    {NULL, NULL, 0}
};

static const duk_number_list_entry como_posix_constants[] = {

    /* file open flags */
    {"O_RDONLY", O_RDONLY },
    {"O_WRONLY", O_WRONLY},
    {"O_RDWR", O_RDWR},
    {"O_APPEND", O_APPEND},
    {"O_CREAT", O_CREAT},
    {"O_TRUNC", O_TRUNC},
    {"O_EXCL", O_EXCL},

    #ifdef O_SYMLINK
    {"O_SYMLINK", O_SYMLINK},
    #endif

    #ifdef O_DSYNC
    {"O_DSYNC", O_DSYNC},
    #endif

    #ifdef O_NOCTTY
    {"O_NOCTTY", O_NOCTTY},
    #endif

    #ifdef O_NONBLOCK
    {"O_NONBLOCK", O_NONBLOCK},
    #endif

    #ifdef O_RSYNC
    {"O_RSYNC", O_RSYNC},
    #endif

    #ifdef O_SYNC
    {"O_SYNC", O_SYNC},
    #endif

     /* file modes */
    {"S_IFMT", S_IFMT}, 
    {"S_IFDIR", S_IFDIR}, /* directory */
    {"S_IFREG", S_IFREG}, /* File */
    {"S_IFBLK", S_IFBLK}, /* block device */
    {"S_IFCHR", S_IFCHR}, /* charcter device */
    {"S_IFIFO", S_IFIFO}, /* FIFO */

    #ifdef S_IFLNK
    {"S_IFLNK", S_IFLNK}, /* symbolic link */
    #endif

    #ifdef S_IFSOCK
    {"S_IFSOCK", S_IFSOCK}, /* socket */
    #endif

    {"F_OK", F_OK},
    {"W_OK", W_OK},
    {"R_OK", R_OK},
    {"X_OK", X_OK},
    {NULL, 0}
};

static int init_binding_posix(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_posix_funcs);
    duk_put_number_list(ctx, -1, como_posix_constants);
    return 1;
}
