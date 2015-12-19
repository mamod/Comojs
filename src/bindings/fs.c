#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#ifdef _WIN32
    
#else
    
#endif

/*=============================================================================
  stat file by name
  fs.stat('file_name');
  on success returns a stat object
  on error returns null and set process.errno
 ============================================================================*/
COMO_METHOD(como_fs_fstat) {

    int file = duk_require_int(ctx, 0);
    struct stat st;

    if (fstat(file, &st) == 0) {
        duk_push_object(ctx);
        
        duk_push_int(ctx, st.st_size);
        duk_put_prop_string(ctx, -2, "size");

        duk_push_int(ctx, st.st_mode);
        duk_put_prop_string(ctx, -2, "mode");

        duk_push_int(ctx, st.st_uid);
        duk_put_prop_string(ctx, -2, "uid");

        duk_push_int(ctx, st.st_gid);
        duk_put_prop_string(ctx, -2, "gid");

        duk_push_int(ctx, st.st_mtime);
        duk_put_prop_string(ctx, -2, "mtime");

        duk_push_int(ctx, st.st_atime);
        duk_put_prop_string(ctx, -2, "atime");

        duk_push_int(ctx, st.st_ctime);
        duk_put_prop_string(ctx, -2, "ctime");
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }

    return 1;
}

COMO_METHOD(como_fs_stat) {

    const char *file = duk_require_string(ctx, 0);
    struct stat st;

    if (stat(file, &st) == 0) {
        duk_push_object(ctx);

        duk_push_uint(ctx, st.st_dev);
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

        duk_push_uint(ctx, st.st_ino);
        duk_put_prop_string(ctx, -2, "ino");

        duk_push_int(ctx, st.st_atime);
        duk_put_prop_string(ctx, -2, "atime");

        duk_push_int(ctx, st.st_ctime);
        duk_put_prop_string(ctx, -2, "ctime");

        duk_push_int(ctx, st.st_mtime);
        duk_put_prop_string(ctx, -2, "mtime");
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }

    return 1;
}

/*=============================================================================
  open file 
  fs.open('/some/file.ext');
  returns a pointer to file handle on success
  on error returns null and sets process.errno 
 ============================================================================*/
COMO_METHOD (como_fs_open) {
    const char *file = duk_get_string(ctx, 0);
    const char *mode = duk_get_string(ctx, 1);
    FILE *f = fopen(file, mode);
    if (!f) {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }

    duk_push_pointer(ctx, f);
    return 1;
}

/*=============================================================================
  close an already opened file handle 
  fs.close(pointer); => file handle pointer
  on success returns true
  on error returns null and sets process.errno 
 ============================================================================*/
COMO_METHOD (como_fs_close) {
    FILE *file = duk_require_pointer(ctx, 0);
    if (fclose(file) != 0) {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }

    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  flush file read/write
  fs.flush(pointer); => file handle pointer
  on success returns true
  on error returns null and sets process.errno 
 ============================================================================*/
COMO_METHOD (como_fs_flush) {
    FILE *file = duk_require_pointer(ctx, 0);
    if (fflush(file) != 0) {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  write to filehandle
  fs.write(pointer, [string|buffer]); => file handle pointer
  on success returns true
  on error returns null and sets process.errno 
 ============================================================================*/
COMO_METHOD (como_fs_write) {
    FILE *file = duk_require_pointer(ctx, 0);
    size_t size;
    const char *data;

    if (duk_get_type(ctx, 1) == DUK_TYPE_BUFFER){
        size  = duk_get_length(ctx, 1);
        data = duk_to_string(ctx, 1);
    } else {
        data = duk_get_lstring(ctx, 1, &size);
    }

    size_t written = fwrite((char *)data, sizeof(char), size, file);
    if (written < size){
        COMO_SET_ERRNO(ctx, COMO_GET_LAST_ERROR);
    }

    duk_push_number(ctx, (double) written);
    return 1;
}

/*=============================================================================
  read file handle into buffer
  fs.readIntoBuffer(fhandle_pointer, buffer, offset, length, positions);
  on success returns bytes read
  on error sets process.errno to error and returns bytes read if any or zero
 ============================================================================*/
COMO_METHOD (como_fs_readIntoBuffer) {
    duk_size_t size;
    FILE *file    = duk_require_pointer(ctx, 0);
    void *buffer  = duk_require_buffer(ctx, 1, &size);
    int offset    = duk_get_int(ctx, 2);
    int length    = duk_get_int(ctx, 3);
    
    int prevError = errno;
    errno = 0;
    
    if (duk_is_number(ctx, 4)){
        int position    = duk_get_int(ctx, 4);
        fseek(file, position, SEEK_SET);
    }

    buffer += offset;

    size_t bytesRead = fread(buffer, 1, length, file);
    if (bytesRead < length){
        COMO_SET_ERRNO(ctx, ferror(file));
    }

    errno = prevError;
    duk_push_number(ctx, (double)bytesRead);
    return 1;
}

COMO_METHOD (como_fs_tell) {
    return 1;
}

COMO_METHOD (como_fs_seek) {
    return 1;
}

/*=============================================================================
  get file descriptor associated with filehandle
  fs.fileno(pointer); => file handle pointer
  on success returns true
  on error returns null and sets process.errno 
 ============================================================================*/
COMO_METHOD (como_fs_fileno) {
    FILE *file = duk_get_pointer(ctx, 0);
    int fd = fileno(file);
    if (fd == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    duk_push_int(ctx, fd);
    return 1;
}

static const duk_function_list_entry como_fs_funcs[] = {
    { "stat"            , como_fs_stat,             1 },
    { "fstat"           , como_fs_fstat,            1 },
    { "open"            , como_fs_open,             2 },
    { "close"           , como_fs_close,            1 },
    { "flush"           , como_fs_flush,            1 },
    { "write"           , como_fs_write,            2 },
    { "readIntoBuffer"  , como_fs_readIntoBuffer,   5 },

    { "fileno"     , como_fs_fileno, 1 },
    { "tell"  , como_fs_tell,   1 },
    { "seek"  , como_fs_seek,   3 },
    { NULL, NULL, 0 }
};

static const duk_number_list_entry como_fs_constants[] = {

    /* file modes */
    {"S_IFMT"     , S_IFMT}, 
    {"S_IFDIR"    , S_IFDIR}, /* directory */
    {"S_IFREG"    , S_IFREG}, /* File */
    {"S_IFBLK"    , S_IFBLK}, /* block device */
    {"S_IFCHR"    , S_IFCHR}, /* charcter device */
    {"S_IFIFO"    , S_IFIFO}, /* FIFO */

    #ifdef S_IFLNK
        {"S_IFLNK"    , S_IFLNK}, /* symbolic link */
    #endif

    #ifdef S_IFSOCK
        {"S_IFSOCK"   , S_IFSOCK}, /* socket */
    #endif

    { "O_APPEND" , O_APPEND },
    { "O_CREAT"  , O_CREAT },
    { "O_EXCL"   , O_EXCL },
    { "O_RDONLY" , O_RDONLY },
    { "O_RDWR"   , O_RDWR },
    
    #ifdef O_SYNC
        { "O_SYNC"   , O_SYNC },
    #else
        { "O_SYNC"   , 0 },
    #endif
    
    { "O_TRUNC"  , O_TRUNC },
    { "O_WRONLY" , O_WRONLY },
    
    #ifdef O_SYMLINK
        { "O_SYMLINK" , O_SYMLINK },
    #endif
    
    { NULL, 0 }
};

static int init_binding_fs(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_fs_funcs);
    duk_put_number_list(ctx,   -1, como_fs_constants);
    return 1;
}
