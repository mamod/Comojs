BOOL
WINAPI MyWriteConsoleA( HANDLE hCon, LPCVOID lpBuffer,
            DWORD nNumberOfCharsToWrite,
            LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved );

BOOL
WINAPI MyWriteConsoleW( HANDLE hCon, LPCVOID lpBuffer,
            DWORD nNumberOfCharsToWrite,
            LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved );

BOOL
WINAPI MyWriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
            LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped );

void tty_console_init();
void tty_enter_critical_section();
void tty_leave_critical_section();
