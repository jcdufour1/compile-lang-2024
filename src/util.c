
#include "util.h"

void write_file(const char* file_path, Str_view text_to_write) {
    FILE* file = fopen(file_path, "w");
    if (!file) {
        todo();
    }

    for (size_t idx = 0; idx < text_to_write.count; idx++) {
        if (EOF == fputc(text_to_write.str[idx], file)) {
            todo();
        }
    }

    fclose(file);
}
