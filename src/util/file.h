#ifndef FILE_H
#define FILE_H

#include "util.h"
#include "newstring.h"

typedef enum {
    FILE_TYPE_OWN,
    FILE_TYPE_STATIC_LIB,
    FILE_TYPE_DYNAMIC_LIB,
    FILE_TYPE_C,

    // count for static asserts
    FILE_TYPE_COUNT,
} FILE_TYPE;

void write_file(const char* file_path, Str_view text_to_write);

bool read_file(Str_view* result, Str_view input_file_name);

bool get_file_extension(Str_view* extension, Str_view file_path);

FILE_TYPE get_file_type(Str_view file_path);

#endif // FILE_H
