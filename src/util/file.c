
#include "util.h"
#include "newstring.h"
#include "parameters.h"
#include <stdlib.h>
#include <errno.h>
#include <msg.h>
#include <file.h>
#include <newstring.h>

bool read_file(Str_view* result, Str_view file_path) {
    String file_text = {0};
    FILE* file = fopen(str_view_dup(&a_main, file_path), "rb");
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
    if (file_text.info.count < 1 || vec_at(&file_text, file_text.info.count - 1) != '\n') {
        vec_append(&a_main, &file_text, '\n'); // tokenizer currently requires newline at the end of the file text
    }
    *result = string_to_strv(file_text);
    return true;
}

void write_file(const char* file_path, Str_view text_to_write) {
    FILE* file = fopen(file_path, "w");
    if (!file) {
        msg(
            DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN, "could not open file "STR_VIEW_FMT": %s\n",
            str_view_print(params.input_file_path), strerror(errno)
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

Str_view file_path_normalize(Str_view file_path) {
    unwrap(file_path.count > 0);

    if (str_view_front(file_path) == '/') {
        // is absolute path
        return file_path;
    }

    if (file_path.count > 1 && str_view_is_equal(str_view_slice(file_path, 0, 2), str_view_from_cstr("./"))) {
        return file_path;
    }

    String new_path = {0};
    string_extend_cstr(&a_main, &new_path, "./"); 
    string_extend_strv(&a_main, &new_path, file_path); 
    return string_to_strv(new_path);
}

