#ifdef _WIN32

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef void* Winapi_handle;

bool winapi_CreateDirectoryA(const char* dir_name);

bool winapi_PathIsDirectoryA(const char* dir_name);

bool winapi_CreateProcessA(
    const char* lpApplicationName,
    char* lpCommandLine,
    void* lpProcessAttributes,
    void* lpThreadAttributes,
    bool bInheritHandles,
    unsigned long dwCreationFlags,
    void* lpEnvironment,
    const char* lpCurrentDirectory,
    void* lpStartupInfo,
    void* lpProcessInformation
);

unsigned long winapi_GetLastError(void);

unsigned long long winapi_GetTickCount64(void);

unsigned long winapi_INFINITE(void);

unsigned long winapi_WaitForSingleObject(Winapi_handle hHandle, unsigned long dwMilliseconds);

unsigned long winapi_WAIT_FAILED(void);

unsigned long winapi_WAIT_ABANDONED(void);

unsigned long winapi_WAIT_OBJECT_0(void);

unsigned long winapi_WAIT_TIMEOUT(void);

bool winapi_GetExitCodeProcess(Winapi_handle* hProcess, unsigned long* lpExitCode);

const char* winapi_print_last_error(void);

#endif // _WIN32
