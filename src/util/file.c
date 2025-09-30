
#include "util.h"
#include "newstring.h"
#include "parameters.h"
#include <stdlib.h>
#include <errno.h>
#include <msg.h>
#include <file.h>
#include <newstring.h>
#include <msg_todo.h>

bool read_file(Strv* result, Strv file_path) {
    String file_text = {0};
    FILE* file = fopen(strv_dup(&a_main, file_path), "rb");
    if (!file) {
        msg(
            DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN,
            "could not open file `"FMT"`: %s\n",
            strv_print(file_path), strerror(errno)
        );
        return false;
    }

    size_t buf_size = 2024;
    size_t amount_read = 0;
    do {
        vec_reserve(&a_main, &file_text, buf_size);
        amount_read = fread(file_text.buf + file_text.info.count, 1, buf_size, file);
        file_text.info.count += amount_read;
    } while (amount_read > 0);

    if (ferror(file)) {
        msg(
            DIAG_FILE_COULD_NOT_READ, POS_BUILTIN,
            "could not read file `"FMT"`: %s\n",
            strv_print(file_path), strerror(errno)
        );
        fclose(file);
        return false;
    }

    fclose(file);

    if (file_text.info.count < 1 || vec_at(&file_text, file_text.info.count - 1) != '\n') {
        vec_append(&a_main, &file_text, '\n'); // tokenizer currently requires newline at the end of the file text
    }
    *result = string_to_strv(file_text);
    return true;
}

void write_file(const char* file_path, Strv text_to_write) {
    FILE* file = fopen(file_path, "w");
    if (!file) {
        msg(
            DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN, "could not open file %s: %s\n",
            file_path, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    file_extend_strv(file, text_to_write);
    fclose(file);
}

bool get_file_extension(Strv* extension, Strv file_path) {
    for (size_t idx_ = file_path.count; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;
        if (strv_at(file_path, idx) == '.') {
            *extension = strv_slice(file_path, idx + 1, file_path.count - idx - 1);
            return true;
        }
    }
    return false;
}

typedef struct {
    const char* text;
    FILE_TYPE type;
} File_type_pair;

static_assert(FILE_TYPE_COUNT == 7, "exhausive handling of file types");
File_type_pair file_type_pairs[] = {
    {"own", FILE_TYPE_OWN},
    {"a", FILE_TYPE_STATIC_LIB},
    {"so", FILE_TYPE_DYNAMIC_LIB},
    {"c", FILE_TYPE_C},
    {"o", FILE_TYPE_OBJECT},
    {"s", FILE_TYPE_LOWER_S},
    {"S", FILE_TYPE_UPPER_S},
};

FILE_TYPE get_file_type(Strv file_path) {
    Strv ext = {0};
    if (!get_file_extension(&ext, file_path)) {
        // TODO: print what user command line option caused this, etc.
        msg_todo("executable file passed on the command line", POS_BUILTIN);
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < sizeof(file_type_pairs)/sizeof(file_type_pairs[0]); idx++) {
        File_type_pair curr = file_type_pairs[idx];
        if (strv_is_equal(sv(curr.text), ext)) {
            return curr.type;
        }
    }

    String buf = {0};
    string_extend_strv(&a_main, &buf, sv("file with extension ."));
    string_extend_strv(&a_main, &buf, ext);
    msg_todo_strv(string_to_strv(buf), POS_BUILTIN);
    exit(EXIT_CODE_FAIL);
}

void file_extend_strv(FILE* file, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        if (EOF == fputc(strv_at(strv, idx), file)) {
            todo();
        }
    }

}

Strv file_strip_extension(Strv file_path) {
    Strv new_path = file_path;
    while (new_path.count > 0 && strv_at(new_path, new_path.count - 1) != '.') {
        new_path = strv_slice(new_path, 0, new_path.count - 1);
    }
    return new_path.count > 0 ? strv_slice(new_path, 0, new_path.count - 1) : file_path;
}

Strv file_basename(Strv file_path) {
    Strv new_path = file_path;
    while (new_path.count > 0 && strv_at(new_path, new_path.count - 1) != PATH_SEPARATOR) {
        new_path = strv_slice(new_path, 0, new_path.count - 1);
    }
    return strv_slice(file_path, new_path.count, file_path.count - new_path.count);
}

