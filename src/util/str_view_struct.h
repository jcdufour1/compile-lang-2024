#ifndef STR_VIEW_STRUCT_H
#define STR_VIEW_STRUCT_H

#include <stddef.h>

typedef struct {
    const char* str;
    size_t count;
} Str_view;

#endif // STR_VIEW_STRUCT_H
