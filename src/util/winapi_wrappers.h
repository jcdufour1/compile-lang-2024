#ifdef _WIN32

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

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

const char* winapi_err_print(unsigned long err);

#endif // _WIN32
