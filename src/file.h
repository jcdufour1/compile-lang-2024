#ifndef FILE_H
#define FILE_H

#include "util.h"
#include "newstring.h"

void write_file(const char* file_path, Str_view text_to_write);

bool read_file(String* file_text, const char* input_file_name);

#endif // FILE_H
