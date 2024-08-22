
#include "util.h"
#include "newstring.h"

bool read_file(String* file_text, const char* input_file_name) {
    memset(file_text, 0, sizeof(*file_text));
    FILE* file = fopen(input_file_name, "rb");
    if (!file) {
        return false;
    }

    size_t buf_size = 2024;
    size_t amount_read;
    // TODO: check for errors?
    do {
        string_reserve(file_text, buf_size);
        amount_read = fread(file_text->buf, 1, buf_size, file);
        file_text->info.count += amount_read;
    } while (amount_read > 0);

    fclose(file);
    return true;
}

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
