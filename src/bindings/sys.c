#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <string.h>
#else

#endif

COMO_METHOD(como_sys_cloexec) {
    int fd   = duk_require_int(ctx, 0);
    int flag = duk_get_int(ctx, 1);
    
    //FIXME: windows expecting file handle,
    //what about file descriptors?
    #ifdef _WIN32
        flag = flag ? 0 : 1;
        if (SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT | 0x00000002 , flag) == 0){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
    #else
        int err = como__cloexec(fd, flag);
        if (err){
            COMO_SET_ERRNO_AND_RETURN(ctx, err);
        }
    #endif

    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_duplicate_handle) {
    #ifdef _WIN32
        int h  = duk_require_int(ctx, 0);
        HANDLE dupHandle;
        if (!DuplicateHandle((HANDLE)-1, (HANDLE)h, (HANDLE)-1, 
            &dupHandle, 0, 1, DUPLICATE_SAME_ACCESS )){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        duk_push_int(ctx, (int)dupHandle);
    #else
        assert(0 && "DuplicateHandle Not supported on this platform");
    #endif
    return 1;
}

COMO_METHOD(como_get_os_fhandle) {
    #ifdef _WIN32
        int fd  = duk_require_int(ctx, 0);
        int handle = _get_osfhandle(fd);
        if (handle == -1){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        duk_push_int(ctx, handle);
    #else
        assert(0 && "Getosfhandle Not supported on this platform");
    #endif
    return 1;
}

COMO_METHOD(como_create_process) {
    #ifdef _WIN32
        STARTUPINFO startup;
        PROCESS_INFORMATION info;
        DWORD process_flags;

        ZeroMemory( &startup, sizeof(startup) );
        startup.cb = sizeof(startup);
        ZeroMemory( &info, sizeof(info) );

        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;

        startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup.hStdInput  = (HANDLE)duk_require_int(ctx, 0);
        startup.hStdOutput = (HANDLE)duk_require_int(ctx, 1);
        startup.hStdError  = (HANDLE)duk_require_int(ctx, 2);

        startup.wShowWindow = SW_HIDE;
        process_flags = CREATE_UNICODE_ENVIRONMENT;
        // putenv("namexxxxxxxxxxx=value\\;name2=value");
        
        if( !CreateProcess( NULL,   // module name (use command line)
            "perl p.pl",            // Command line
            NULL,                   // Process handle not inheritable
            NULL,                   // Thread handle not inheritable
            1,                      // Set handle inheritance to FALSE
            process_flags,          // No creation flags
            NULL,                   // Use parent's environment block
            NULL,                   // Use parent's starting directory 
            &startup,               // Pointer to STARTUPINFO structure
            &info )                 // Pointer to PROCESS_INFORMATION structure
        ) {
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }

        //push new process handle info [info.hProcess, info.dwProcessId]
        duk_idx_t arr_idx = duk_push_array(ctx);
        duk_push_int(ctx, (int)info.hProcess);
        duk_put_prop_index(ctx, arr_idx, 0);

        duk_push_int(ctx, info.dwProcessId);
        duk_put_prop_index(ctx, arr_idx, 1);
        
        CloseHandle(info.hThread);
    #else
        assert(0 && "CreateProcess Not supported on this platform");
    #endif
    return 1;
}

COMO_METHOD(como_get_exit_code) {
    #ifdef _WIN32
        DWORD exitCode = 0;
        HANDLE handle = (HANDLE)duk_require_int(ctx, 0);
        if (GetExitCodeProcess(handle, &exitCode) == FALSE){
            // Handle GetExitCodeProcess failure
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        duk_push_int(ctx, exitCode);
    #else
        assert(0 && "CreateProcess Not supported on this platform");
    #endif
    return 1;
}

COMO_METHOD(como_sys_fork) {
    #ifdef _WIN32
        assert(0 && "fork Not supported on this platform");
    #else
        int pid = fork();
        if (pid == -1){
            COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
        }
        duk_push_int(ctx, pid);
    #endif
    return 1;
}

static const duk_function_list_entry como_sys_funcs[] = {
    { "cloexec"            , como_sys_cloexec,         2 },
    { "GetExitCodeProcess" , como_get_exit_code, 1},
    { "DuplicateHandle"    , como_duplicate_handle, 1},
    { "GetOsFHandle"       , como_get_os_fhandle, 1},
    { "CreateProcess"      , como_create_process, 3   },
    { "fork"               , como_sys_fork, 0   },
    { NULL                 , NULL, 0 }
};

static const duk_number_list_entry como_sys_constants[] = {
    { NULL, 0 }
};

static int init_binding_sys(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_sys_funcs);
    duk_put_number_list(ctx, -1, como_sys_constants);
    return 1;
}
