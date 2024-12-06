#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "newstring.h"
#include "arena.h"

FILE* output = NULL;
Arena gen_a = {0};

static void log_gen_internal(const char* file, int line, const char* format, ...)
__attribute__((format (printf, 3, 4)));

static void log_gen_internal(const char* file, int line, const char* format, ...) {
    assert(output);

    va_list args;
    va_start(args, format);

    vfprintf(output, format, args);
    fprintf(output, "    /* %s:%d: */\n", file, line);

    va_end(args);
}

#define log_gen(...) \
    log_gen_internal(__FILE__, __LINE__, __VA_ARGS__)

typedef struct {
    const char* name;
    const char* type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

static void log_struct_internal(const char* file, int line, const char* name, Members members) {
    String buf = {0};
    string_extend_cstr(&gen_a, &buf, "typedef struct ");
    string_extend_cstr(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, "_ {");
    log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
    vec_reset(&buf);

    for (size_t idx = 0; idx < members.info.count; idx++) {
        Member curr_memb = vec_at(&members, idx);
        string_extend_cstr(&gen_a, &buf, "    ");
        string_extend_cstr(&gen_a, &buf, curr_memb.type);
        string_extend_cstr(&gen_a, &buf, " ");
        string_extend_cstr(&gen_a, &buf, curr_memb.name);
        string_extend_cstr(&gen_a, &buf, ";");
        log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
        vec_reset(&buf);
    }

    string_extend_cstr(&gen_a, &buf, "}");
    string_extend_cstr(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, ";");
    log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
}

#define log_struct(name, members) \
    log_struct_internal(__FILE__, __LINE__, name, members)

static void log_enum_internal(const char* file, int line, const char* name, Members members) {
    String buf = {0};
    string_extend_cstr(&gen_a, &buf, "typedef enum ");
    string_extend_cstr(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, "_ {");
    log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
    vec_reset(&buf);

    for (size_t idx = 0; idx < members.info.count; idx++) {
        Member curr_memb = vec_at(&members, idx);
        string_extend_cstr(&gen_a, &buf, "    ");
        string_extend_cstr(&gen_a, &buf, curr_memb.name);
        string_extend_cstr(&gen_a, &buf, ";");
        log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
        vec_reset(&buf);
    }

    string_extend_cstr(&gen_a, &buf, "}");
    string_extend_cstr(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, ";");
    log_gen_internal(file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
}

#define log_enum(name, members) \
    log_enum_internal(__FILE__, __LINE__, name, members)

static void append_node_type(Members* parent_type, Member node_type) {
    vec_append(&gen_a, parent_type, node_type);
}

int main(int argc, char** argv) {
    assert(argc == 2 && "output file path should be provided");

    output = fopen(argv[1], "w");
    if (!output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    log_gen("#ifndef NODE_H");
    log_gen("#define NODE_H");

    Members members = {0};

    {
        Member member = {.type = "NODE_TYPE", .name = "type"};
        vec_append(&gen_a, &members, member);
    }

    {
        Member member = {.type = "NODE_AS", .name = "as"};
        vec_append(&gen_a, &members, member);
    }

    log_enum("Node", members);
    log_struct("Node", members);

    log_gen("#endif /* NODE_H_ */");

    return 0;
}
