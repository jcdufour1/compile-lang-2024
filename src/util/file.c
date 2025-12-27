
#include <util.h>
#include <local_string.h>
#include <parameters.h>
#include <ast_msg.h>
#include <stdlib.h>
#include <errno.h>
#include <msg.h>
#include <file.h>
#include <env.h>
#include <time.h>

#ifdef _WIN32
#   include <winapi_wrappers.h>
#else
#   include <sys/stat.h>
#   include <sys/sysmacros.h>
#   include <sys/types.h>
#endif // _WIN32

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
        darr_reserve(&a_main, &file_text, buf_size);
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

    if (file_text.info.count < 1 || darr_at(file_text, file_text.info.count - 1) != '\n') {
        darr_append(&a_main, &file_text, '\n'); // tokenizer currently requires newline at the end of the file text
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
        local_exit(EXIT_CODE_FAIL);
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
// TODO: add exe
File_type_pair file_type_pairs[] = {
    {"own", FILE_TYPE_OWN},
    {"a", FILE_TYPE_STATIC_LIB},
    {"so", FILE_TYPE_DYNAMIC_LIB},
    {"c", FILE_TYPE_C},
    {"o", FILE_TYPE_OBJECT},
    {"s", FILE_TYPE_LOWER_S},
    {"S", FILE_TYPE_UPPER_S},
};

bool get_file_type(FILE_TYPE* result, Strv* err_text, Strv file_path) {
    Strv ext = {0};
    if (!get_file_extension(&ext, file_path)) {
        *err_text = sv("executable file passed on the command line");
        return false;
    }

    for (size_t idx = 0; idx < sizeof(file_type_pairs)/sizeof(file_type_pairs[0]); idx++) {
        File_type_pair curr = file_type_pairs[idx];
        if (strv_is_equal(sv(curr.text), ext)) {
            *result = curr.type;
            return true;
        }
    }

    *err_text = strv_from_f(&a_temp, "file with extension ."FMT, strv_print(ext));
    return false;
}

void file_extend_strv(FILE* file, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        if (EOF == fputc(strv_at(strv, idx), file)) {
            msg_todo("error message for fputc failing", POS_BUILTIN);
            return;
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
    while (new_path.count > 0 && strv_at(new_path, new_path.count - 1) != PATH_SEP_CHAR) {
        new_path = strv_slice(new_path, 0, new_path.count - 1);
    }
    return strv_slice(file_path, new_path.count, file_path.count - new_path.count);
}

// TODO: move this function?
NEVER_RETURN void local_exit(int exit_code) {
    print_all_defered_msgs();

    arena_free(&a_temp);
    arena_free(&a_pass);
    arena_free_a_main();
    arena_free(&a_leak);

    exit(exit_code);
}

#ifndef _WIN32
static bool make_dir_posix(Strv dir_path) {
    const char* dir_cstr = strv_dup(&a_temp, dir_path);

    struct stat dir_status = {0};
    if (0 == mkdir(dir_cstr, 0755)) {
        return true;
    }

    if (-1 == stat(dir_cstr, &dir_status)) {
        msg(DIAG_DIR_COULD_NOT_BE_MADE, POS_BUILTIN, "could not stat directory `"FMT"`: %s\n", strv_print(dir_path), strerror(errno));
        return false;
    }

    if (!S_ISDIR(dir_status.st_mode)) {
        msg(DIAG_DIR_COULD_NOT_BE_MADE, POS_BUILTIN, "`"FMT"` exists but is not a directory\n", strv_print(dir_path));
        msg(DIAG_NOTE, POS_BUILTIN, "`"FMT"` is the build-dir specified (or default)\n", strv_print(dir_path));
        return false;
    }
    
    return true;
}
#endif // _WIN32

#ifdef _WIN32
static bool make_dir_win32(Strv dir_path) {
    const char* dir_cstr = strv_dup(&a_temp, dir_path);

    if (winapi_CreateDirectoryA(dir_cstr)) {
        return true;
    }

    if (!winapi_PathIsDirectoryA(dir_cstr)) {
        msg(DIAG_DIR_COULD_NOT_BE_MADE, POS_BUILTIN, "`"FMT"` exists but is not a directory\n", strv_print(dir_path));
        msg(DIAG_NOTE, POS_BUILTIN, "`"FMT"` is the build-dir specified (or default)\n", strv_print(dir_path));
        return false;
    }
    
    return true;
}
#endif // _WIN32

bool make_dir(Strv dir_path) {
#   ifdef _WIN32
        return make_dir_win32(dir_path);
#   else
        return make_dir_posix(dir_path);
#   endif // _WIN32
    unreachable("");
}

