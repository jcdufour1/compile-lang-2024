#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>
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

const char* winapi_err_print(unsigned long err) {
    //unsigned long errs[2] = {err, 0};
    //if (FormatMessage(
    //    FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
    //    NULL,
    //    0,
    //    0,
    //    err_print_buf,
    //    ERR_PRINT_BUF_COUNT,
    //    (char**)&errs
    //) < 1) {
    //    fprintf(stderr, "fatal: FormatMessage failed\n");
    //    abort();
    //}

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
