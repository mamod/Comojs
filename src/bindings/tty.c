#include <stdio.h>

#ifdef _WIN32
    #include "../../include/ansi/ANSI.h"
    #include <io.h>
    #include <conio.h>
    #include <Windows.h>
    /* initaite critical section for console see ANSI.c */
    #define COMO_INIT_CONSOLE tty_console_init()
#else
    #define COMO_INIT_CONSOLE
    #include <unistd.h>
#endif

#ifdef _WIN32
    
    struct winsize {
        unsigned short ws_row;  /* rows, in characters */
        unsigned short ws_col;    /* columns, in characters */
        unsigned short ws_xpixel; /* horizontal size, pixels */
        unsigned short ws_ypixel; /* vertical size, pixels */
    };

    static inline int
    getWindowSize(struct winsize *win, int fd) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        HANDLE fileHandle = (HANDLE)_get_osfhandle(fd);
        if(GetConsoleScreenBufferInfo(fileHandle, &info)) {
            win->ws_col = info.srWindow.Right - info.srWindow.Left + 1;
            win->ws_row = info.srWindow.Bottom - info.srWindow.Top + 1;
            win->ws_ypixel = info.dwCursorPosition.Y;
            win->ws_xpixel = info.dwCursorPosition.X;
            return 1;
        }

        return 0;
    }
    
#else

    #include <termios.h>
    #include <sys/ioctl.h>
    #define _isatty isatty

    static inline int
    getWindowSize(struct winsize *win, int fd) {
        if (ioctl(fd, TIOCGWINSZ, win) == -1 || win->ws_col == 0) {
            return 0;
        }

        return 1;
    }
#endif

COMO_METHOD(como_tty_write) {
    int fd = duk_require_int(ctx, 0);
    size_t length;
    const char *str = duk_require_lstring(ctx, 1, &length);
    
    #ifdef _WIN32
        HANDLE fileHandle = (HANDLE)_get_osfhandle(fd);
        if (fileHandle == INVALID_HANDLE_VALUE){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        
        LPDWORD lpNumberOfBytesWritten = 0;
        BOOL ret = MyWriteConsoleW(fileHandle,
                              str,
                              (DWORD)length,
                              (LPDWORD)&lpNumberOfBytesWritten,
                              NULL);
        if (ret == 0){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        duk_push_number(ctx, (size_t)lpNumberOfBytesWritten);
        return 1;
    #endif
    
    size_t n = write(fd, str, length);
    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    
    duk_push_number(ctx, n);
    return 1;
}

COMO_METHOD(como_tty_isatty) {
    int fd = duk_require_int(ctx, 0);
    if (_isatty(fd)){
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }
    return 1;
}

COMO_METHOD(como_tty_getWindowSize) {
    struct winsize ws;

    int fd;
    if (duk_get_type(ctx, 0) == DUK_TYPE_NUMBER) {
        fd = duk_get_int(ctx, 0);
    } else {
        fd = STDOUT_FILENO;
    }

    if (getWindowSize(&ws, fd)){
        duk_idx_t obj_idx = duk_push_object(ctx);
        
        duk_push_string(ctx, "width");
        duk_push_int(ctx, ws.ws_col);
        duk_put_prop(ctx, obj_idx);
        
        duk_push_string(ctx, "height");
        duk_push_int(ctx, ws.ws_row);
        duk_put_prop(ctx, obj_idx);

        // duk_push_string(ctx, "width");
        // duk_push_int(ctx, ws.ws_xpixel);
        // duk_put_prop(ctx, obj_idx);

        // duk_push_string(ctx, "height");
        // duk_push_int(ctx, ws.ws_ypixel);
        // duk_put_prop(ctx, obj_idx);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }

    return 1;
}

COMO_METHOD(como_tty_guess_type) {
    int fd = duk_require_int(ctx, 0);
    if (_isatty(fd)){
        duk_push_string(ctx, "TTY");
    } else {
        assert(0 && "tty unknown handle");
        duk_push_string(ctx, "Unknown");
    }
    return 1;
}

COMO_METHOD(como_tty_kbhit) {
    return 1;
}

COMO_METHOD(como_tty_set_mode) {
    return 1;
}

COMO_METHOD(como_tty_read) {
    return 1;
}

static const duk_function_list_entry tty_funcs[] = {
    { "write"           , como_tty_write,          2 },
    { "istty"           , como_tty_isatty,         1 },
    { "guessHandleType" , como_tty_guess_type,     1 },
    { "getWindowSize"   , como_tty_getWindowSize , 1 },
    { "kbhit"           , como_tty_kbhit,          0 },
    { "read"            , como_tty_read,           0 },
    { "setRawMode"      , como_tty_set_mode,       0 },
    { NULL              , NULL, 0 }
};

static int init_binding_tty(duk_context *ctx) {
    COMO_INIT_CONSOLE;
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, tty_funcs);
    return 1;
}
