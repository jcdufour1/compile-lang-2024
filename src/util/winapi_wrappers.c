#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <shlwapi.h>
#include <sysinfoapi.h>

#include <stdio.h>

#include <winapi_wrappers.h>

bool winapi_CreateDirectoryA(const char* dir_name) {
    return CreateDirectoryA(dir_name, NULL);
}

bool winapi_PathIsDirectoryA(const char* dir_name) {
    return PathIsDirectoryA(dir_name);
}

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
) {
    return CreateProcessA(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
   );
}

unsigned long winapi_GetLastError(void) {
    return GetLastError();
}

#define ERR_PRINT_BUF_COUNT 32000
static char err_print_buf[ERR_PRINT_BUF_COUNT] = {0};

static const char* winapi_err_print(unsigned long err) {
    if (FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        0,
        err_print_buf,
        ERR_PRINT_BUF_COUNT,
        NULL
    ) < 1) {
        fprintf(stderr, "fatal: FormatMessage failed\n");
        abort();
    }
    return err_print_buf;
}

const char* winapi_print_last_error(void) {
    return winapi_err_print(winapi_GetLastError());
}

unsigned long winapi_WaitForSingleObject(Winapi_handle hHandle, unsigned long dwMilliseconds) {
    return WaitForSingleObject(hHandle, dwMilliseconds);
}

unsigned long long winapi_GetTickCount64(void) {
    return GetTickCount64();
}

unsigned long winapi_INFINITE(void) {
    return INFINITE;
}

unsigned long winapi_WAIT_FAILED(void) {
    return WAIT_FAILED;
}

unsigned long winapi_WAIT_ABANDONED(void) {
    return WAIT_ABANDONED;
}

unsigned long winapi_WAIT_OBJECT_0(void) {
    return WAIT_OBJECT_0;
}

unsigned long winapi_WAIT_TIMEOUT(void) {
    return WAIT_TIMEOUT;
}

bool winapi_GetExitCodeProcess(Winapi_handle* hProcess, unsigned long* lpExitCode) {
    return GetExitCodeProcess(hProcess, lpExitCode);
}


