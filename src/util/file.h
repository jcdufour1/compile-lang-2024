#ifndef FILE_H
#define FILE_H

#include "util.h"
#include "newstring.h"

void write_file(const char* file_path, Str_view text_to_write);

bool read_file(Str_view* result, Str_view input_file_name);

Str_view file_path_normalize(Str_view file_path);

#endif // FILE_H
