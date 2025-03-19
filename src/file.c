
#include "util.h"
#include "newstring.h"
#include "parameters.h"
#include <stdlib.h>
#include <errno.h>

bool read_file(Str_view* result, const char* input_file_name) {
    assert(input_file_name);

    String file_text = {0};
    FILE* file = fopen(input_file_name, "rb");
    if (!file) {
        return false;
    }

    size_t buf_size = 2024;
    size_t amount_read;
    // TODO: check for errors?
    do {
        vec_reserve(&a_main, &file_text, buf_size);
        amount_read = fread(file_text.buf + file_text.info.count, 1, buf_size, file);
        file_text.info.count += amount_read;
    } while (amount_read > 0);

    fclose(file);
    *result = string_to_strv(file_text);
    return true;
}

void write_file(const char* file_path, Str_view text_to_write) {
    FILE* file = fopen(file_path, "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_NONE, dummy_file_text, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < text_to_write.count; idx++) {
        if (EOF == fputc(str_view_at(text_to_write, idx), file)) {
            todo();
        }
    }

    fclose(file);
}
