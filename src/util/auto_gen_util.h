#ifndef AUTO_GEN_UTIL_H
#define AUTO_GEN_UTIL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <newstring.h>
#include <arena.h>
#include <log_internal.h>
#include <string_vec.h>
#include <util.h>

FILE* global_output = NULL;
Arena gen_a = {0};

#define CLOSE_FILE(file) \
    do { \
        if (file) { \
            fprintf(file, "\n"); \
            fclose(file); \
        } \
        (file) = NULL; \
    } while(0)

typedef struct {
    Strv type_name;
    Strv normal_prefix;
    Strv internal_prefix;
    Strv key_fn_get_name;
    Strv symbol_table_name;
    Strv ancesters_type;
    Strv print_fn;
    bool do_primitives;
    bool env_thing;
} Symbol_tbl_type;

typedef struct {
    Vec_base info;
    Symbol_tbl_type* buf;
} Sym_tbl_type_vec;

typedef struct {
    Strv name;
    Strv type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...)
__attribute__((format (printf, 4, 5)));

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...) {
    (void) file;
    (void) line;
    assert(output);

    va_list args;
    va_start(args, format);

    vfprintf(output, format, args);
    //fprintf(output, "    /* %s:%d: */", file, line);

    va_end(args);
}

#define gen_gen(...) \
    gen_gen_internal(global_output, __FILE__, __LINE__, __VA_ARGS__)

static void extend_strv_upper(String* output, Strv name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, toupper(strv_at(name, idx)));
    }
}

static void extend_strv_lower(String* output, Strv name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(strv_at(name, idx)));
    }
}

static void extend_strv_first_upper(String* output, Strv name) {
    if (name.count < 1) {
        return;
    }
    vec_append(&gen_a, output, toupper(strv_front(name)));
    for (size_t idx = 1; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(strv_at(name, idx)));
    }
}

static void extend_struct_member(String* output, Member member) {
    string_extend_cstr(&gen_a, output, "    ");
    string_extend_strv(&gen_a, output, member.type);
    string_extend_cstr(&gen_a, output, " ");
    string_extend_strv(&gen_a, output, member.name);
    string_extend_cstr(&gen_a, output, ";\n");
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = sv(type),
        .name = sv(name)
    };
    vec_append(&gen_a, members, member);
}
#endif // AUTO_GEN_UTIL_H
