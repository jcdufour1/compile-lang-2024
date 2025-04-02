#ifndef STR_VIEW_STRUCT_H
#define STR_VIEW_STRUCT_H

#include <stddef.h>

typedef struct {
    const char* str;
    size_t count;
} Str_view;

// TODO: move this struct and function to better place?
typedef struct {
    Str_view mod_path;
    Str_view base;
} Name;

#endif // STR_VIEW_STRUCT_H
