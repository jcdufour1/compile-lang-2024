#ifndef STR_VIEW_STRUCT_H
#define STR_VIEW_STRUCT_H

#include <stddef.h>

typedef struct {
    const char* str;
    size_t count;
} Strv;

#endif // STR_VIEW_STRUCT_H
