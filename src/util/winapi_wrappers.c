#include <windows.h>
#include <winapi_wrappers.h>

bool winapi_CreateDirectoryA(const char* dir_name) {
    return CreateDirectoryA(dir_name, NULL);
}

bool winapi_PathIsDirectoryA(const char* dir_name) {
    return PathIsDirectoryA(dir_name);
}

bool winapi_CreateProcessA() {
}
