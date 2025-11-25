#ifndef FILE_H
#define FILE_H

#include "util.h"
#include "newstring.h"

typedef enum {
    FILE_TYPE_OWN,
    FILE_TYPE_STATIC_LIB,
    FILE_TYPE_DYNAMIC_LIB,
    FILE_TYPE_C,
    FILE_TYPE_OBJECT,
    FILE_TYPE_LOWER_S,
    FILE_TYPE_UPPER_S,

    // count for static asserts
    FILE_TYPE_COUNT,
} FILE_TYPE;

void write_file(const char* file_path, Strv text_to_write);

bool read_file(Strv* result, Strv input_file_name);

bool get_file_extension(Strv* extension, Strv file_path);

FILE_TYPE get_file_type(Strv file_path);

void file_extend_strv(FILE* file, Strv strv);

Strv file_strip_extension(Strv file_path);

Strv file_basename(Strv file_path);

// TODO: move this function and declaration?
NEVER_RETURN void local_exit(int exit_code);

bool make_dir(Strv dir_path);

#endif // FILE_H
