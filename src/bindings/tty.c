#include <stdio.h>

#ifdef _WIN32
    #include "ansi/ANSI.h"
    #include <io.h>
    #include <conio.h>
    #include <Windows.h>
    /* initaite critical section for console see ANSI.c */
    #define COMO_INIT_CONSOLE tty_console_init()
#else
    #define COMO_INIT_CONSOLE
    #include <unistd.h>
#endif

enum {
    SPECIAL_NONE,
    SPECIAL_UP = -20,
    SPECIAL_DOWN = -21,
    SPECIAL_LEFT = -22,
    SPECIAL_RIGHT = -23,
    SPECIAL_DELETE = -24,
    SPECIAL_HOME = -25,
    SPECIAL_END = -26,
};

#define USE_UTF8

#ifdef _WIN32
    
    static DWORD orig_consolemode = 0;

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

    static int enableRawMode(int fd) {
        DWORD n;
        INPUT_RECORD irec;

        HANDLE inh = (HANDLE)_get_osfhandle(fd);
        if (!PeekConsoleInput(inh, &irec, 1, &n)) {
            return -1;
        }

        if (GetConsoleMode(inh, &orig_consolemode)) {
            SetConsoleMode(inh, ENABLE_WINDOW_INPUT);
        }

        return 0;
    }

    static void disableRawMode(int fd) {
        HANDLE inh = (HANDLE)_get_osfhandle(fd);
        assert(inh != INVALID_HANDLE_VALUE && "unknown handle");
        SetConsoleMode(inh, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    }

    static void como_tty_AtExit() {
        //disableRawMode(0);
    }

    static int como_read_char (int fd) {
        HANDLE inh = (HANDLE)_get_osfhandle(fd);
        if (inh == INVALID_HANDLE_VALUE){
            return -1;
        }

        while (1) {
            INPUT_RECORD irec;
            DWORD n;

            if (WaitForSingleObject(inh, INFINITE) != WAIT_OBJECT_0) {
                break;
            }

            if (!ReadConsoleInput (inh, &irec, 1, &n)) {
                break;
            }

            if (irec.EventType == KEY_EVENT && irec.Event.KeyEvent.bKeyDown) {
                KEY_EVENT_RECORD *k = &irec.Event.KeyEvent;
                if (k->dwControlKeyState & ENHANCED_KEY) {
                    switch (k->wVirtualKeyCode) {
                     case VK_LEFT:
                        return SPECIAL_LEFT;
                     case VK_RIGHT:
                        return SPECIAL_RIGHT;
                     case VK_UP:
                        return SPECIAL_UP;
                     case VK_DOWN:
                        return SPECIAL_DOWN;
                     case VK_DELETE:
                        return SPECIAL_DELETE;
                     case VK_HOME:
                        return SPECIAL_HOME;
                     case VK_END:
                        return SPECIAL_END;
                    }
                }
                /* Note that control characters are already translated in AsciiChar */
                else {
                    #ifdef USE_UTF8
                    return k->uChar.UnicodeChar;
                    #else
                    return k->uChar.AsciiChar;
                    #endif
                }
            }
        }
        return -1;
    }
    
#else

    #include <termios.h>
    #include <sys/ioctl.h>
    #define _isatty isatty

    static struct termios orig_termios; /* In order to restore at exit.*/
    static int rawmode = 0; /* For atexit() function to check if restore is needed*/
    static int mlmode = 0;  /* Multi line mode. Default is single line. */
    static int atexit_registered = 0; /* Register atexit just 1 time. */

    static int enableRawMode(int fd) {
        struct termios raw;

        if (!isatty(fd)) goto fatal;
        if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

        raw = orig_termios;  /* modify the original mode */
        /* input modes: no break, no CR to NL, no parity check, no strip char,
         * no start/stop output control. */
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        /* output modes - disable post processing */
        raw.c_oflag &= ~(OPOST);
        /* control modes - set 8 bit chars */
        raw.c_cflag |= (CS8);
        /* local modes - choing off, canonical off, no extended functions,
         * no signal chars (^Z,^C) */
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        /* control chars - set return condition: min number of bytes and timer.
         * We want read to return every single byte, without timeout. */
        raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

        /* put terminal in raw mode after flushing */
        if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
        rawmode = 1;
        return 0;

        fatal:
        errno = ENOTTY;
        return -1;
    }

    static void disableRawMode(int fd) {
        /* Don't even check the return value as it's too late. */
        if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1){
            rawmode = 0;
        }
    }

    static inline int
    getWindowSize(struct winsize *win, int fd) {
        if (ioctl(fd, TIOCGWINSZ, win) == -1 || win->ws_col == 0) {
            return 0;
        }

        return 1;
    }

    static void como_tty_AtExit(void) {
        disableRawMode(STDIN_FILENO);
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

COMO_METHOD(como_tty_read) {
    int fd = duk_require_int(ctx, 0);
    #ifdef _WIN32
        int ch = como_read_char(fd);

        if (ch == -1){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }

        duk_push_int(ctx, ch);
        return 1;
    #endif

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

COMO_METHOD(como_tty_set_raw_mode) {
    int fd  = duk_require_int(ctx, 0);
    if (enableRawMode(fd) == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_tty_disable_raw_mode) {
    int fd  = duk_require_int(ctx, 0);
    disableRawMode(fd);
    duk_push_true(ctx);
    return 1;
}

static const duk_function_list_entry como_tty_funcs[] = {
    {"write", como_tty_write,                       2},
    {"istty", como_tty_isatty,                      1},
    {"guessHandleType", como_tty_guess_type,        1},
    {"getWindowSize", como_tty_getWindowSize,       1},
    {"read", como_tty_read,                         1},
    {"setRawMode", como_tty_set_raw_mode,           1},
    {"disableRawMode", como_tty_disable_raw_mode,   1},
    {NULL, NULL,                                    0}
};

static const duk_number_list_entry como_tty_constants[] = {
    {"KEY_LEFT", SPECIAL_LEFT},
    {"KEY_RIGHT", SPECIAL_RIGHT},
    {"KEY_DOWN", SPECIAL_DOWN},
    {"KEY_UP", SPECIAL_UP},
    { NULL, 0 }
};

static int init_binding_tty(duk_context *ctx) {
    COMO_INIT_CONSOLE;
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_tty_funcs);
    duk_put_number_list(ctx, -1, como_tty_constants);
    atexit(como_tty_AtExit);
    return 1;
}
