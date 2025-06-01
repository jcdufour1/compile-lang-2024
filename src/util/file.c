
#include "util.h"
#include "newstring.h"
#include "parameters.h"
#include <stdlib.h>
#include <errno.h>
#include <msg.h>
#include <file.h>
#include <newstring.h>
#include <msg_todo.h>

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

    file_extend_strv(file, text_to_write);
    fclose(file);
}

bool get_file_extension(Str_view* extension, Str_view file_path) {
    for (size_t idx_ = file_path.count; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;
        if (str_view_at(file_path, idx) == '.') {
            *extension = str_view_slice(file_path, idx + 1, file_path.count - idx - 1);
            return true;
        }
    }
    return false;
}

FILE_TYPE get_file_type(Str_view file_path) {
    Str_view ext = {0};
    if (!get_file_extension(&ext, file_path)) {
        // TODO: print what user command line option caused this, etc.
        msg_todo("executable file passed on the command line", POS_BUILTIN);
        exit(EXIT_CODE_FAIL);
    }

    static_assert(FILE_TYPE_COUNT == 7, "exhausive handling of file types");

    if (str_view_is_equal(ext, sv("own"))) {
        return FILE_TYPE_OWN;
    }
    if (str_view_is_equal(ext, sv("a"))) {
        return FILE_TYPE_STATIC_LIB;
    }
    if (str_view_is_equal(ext, sv("so"))) {
        return FILE_TYPE_DYNAMIC_LIB;
    }
    if (str_view_is_equal(ext, sv("c"))) {
        return FILE_TYPE_C;
    }
    if (str_view_is_equal(ext, sv("o"))) {
        return FILE_TYPE_OBJECT;
    }
    if (str_view_is_equal(ext, sv("s"))) {
        return FILE_TYPE_LOWER_S;
    }
    if (str_view_is_equal(ext, sv("S"))) {
        return FILE_TYPE_UPPER_S;
    }

    String buf = {0};
    string_extend_strv(&a_main, &buf, sv("file with extension ."));
    string_extend_strv(&a_main, &buf, ext);
    msg_todo_strv(string_to_strv(buf), POS_BUILTIN);
    exit(EXIT_CODE_FAIL);
}

void file_extend_strv(FILE* file, Str_view str_view) {
    for (size_t idx = 0; idx < str_view.count; idx++) {
        if (EOF == fputc(str_view_at(str_view, idx), file)) {
            todo();
        }
    }

}
