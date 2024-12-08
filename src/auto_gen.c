#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "newstring.h"
#include "arena.h"
#include <log_internal.h>
#include <string_vec.h>

FILE* global_output = NULL;
Arena gen_a = {0};

#define FOREACH(item_type, item, vector) \
    for (size_t idx = 0; idx < (vector)->info.count; idx++) { \
        item_type item = vec_at((vector), idx);  \

typedef struct {
    Str_view name;
    Str_view type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

typedef struct {
    Vec_base info;
    Str_view* buf;
} Strv_vec;

//typedef struct {
//    Members members;
//
//} Lang_struct;

#define log_member(members) \
    log_member_internal(__FILE__, __LINE__, members)

#define log_members(members) \
    log_members_internal(__FILE__, __LINE__, members)

static void log_member_internal(const char* file, int line, Member member) {
    log_internal(
        LOG_DEBUG, file, line, 0, STR_VIEW_FMT" "STR_VIEW_FMT"\n",
        str_view_print(member.type), str_view_print(member.name)
    );
}

static void log_members_internal(const char* file, int line, Members members) {
    for (size_t idx = 0; idx < members.info.count; idx++) {
        log_member_internal(file, line, vec_at(&members, idx));
    }
}

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...)
__attribute__((format (printf, 4, 5)));

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...) {
    assert(output);

    va_list args;
    va_start(args, format);

    vfprintf(output, format, args);
    fprintf(output, "    /* %s:%d: */\n", file, line);

    va_end(args);
}

#define gen_gen(...) \
    gen_gen_internal(global_output, __FILE__, __LINE__, __VA_ARGS__)

static void gen_struct_internal(
    const char* file,
    int line,
    bool is_struct /* false means that this is a union */,
    Str_view name,
    Members members
) {
    String buf = {0};
    if (is_struct) {
        string_extend_cstr(&gen_a, &buf, "typedef struct ");
    } else {
        string_extend_cstr(&gen_a, &buf, "typedef union ");
    }
    string_extend_strv(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, "_ {");
    gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
    vec_reset(&buf);

    for (size_t idx = 0; idx < members.info.count; idx++) {
        Member curr_memb = vec_at(&members, idx);
        string_extend_cstr(&gen_a, &buf, "    ");
        string_extend_strv(&gen_a, &buf, curr_memb.type);
        string_extend_cstr(&gen_a, &buf, " ");
        string_extend_strv(&gen_a, &buf, curr_memb.name);
        string_extend_cstr(&gen_a, &buf, ";");
        gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
        vec_reset(&buf);
    }

    string_extend_cstr(&gen_a, &buf, "}");
    string_extend_strv(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, ";");
    gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
}

#define gen_struct(name, members) \
    gen_struct_internal(__FILE__, __LINE__, true, name, members)

#define gen_union(name, members) \
    gen_struct_internal(__FILE__, __LINE__, false, name, members)

static void gen_enum_internal(const char* file, int line, Str_view name, Members members) {
    String buf = {0};
    string_extend_cstr(&gen_a, &buf, "typedef enum ");
    string_extend_strv(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, "_ {");
    gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
    vec_reset(&buf);

    for (size_t idx = 0; idx < members.info.count; idx++) {
        Member curr_memb = vec_at(&members, idx);
        string_extend_cstr(&gen_a, &buf, "    ");
        string_extend_strv(&gen_a, &buf, curr_memb.name);
        string_extend_cstr(&gen_a, &buf, ",");
        gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
        vec_reset(&buf);
    }

    string_extend_cstr(&gen_a, &buf, "}");
    string_extend_strv(&gen_a, &buf, name);
    string_extend_cstr(&gen_a, &buf, ";");
    gen_gen_internal(global_output, file, line, STR_VIEW_FMT, str_view_print(string_to_strv(buf)));
}

#define gen_enum(name, members) \
    gen_enum_internal(__FILE__, __LINE__, name, members)

static void append_node_type(Members* parent_type, Member node_type) {
    vec_append(&gen_a, parent_type, node_type);
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Members get_top_level(void) {
    Members top_level = {0};
    append_member(&top_level, "NODE_TYPE", "type");
    append_member(&top_level, "Node_as", "as");
    return top_level;
}

static String string_to_upper(String string) {
    String upper = {0};
    for (size_t idx = 0; idx < string.info.count; idx++) {
        vec_append(&gen_a, &upper, toupper(vec_at(&string, idx)));
    }
    return upper;
}

static String string_to_first_upper(String string) {
    String upper = {0};
    for (size_t idx = 0; idx < string.info.count; idx++) {
        if (idx < 1) {
            vec_append(&gen_a, &upper, toupper(vec_at(&string, idx)));
        } else {
            vec_append(&gen_a, &upper, tolower(vec_at(&string, idx)));
        }
    }
    return upper;
}

static String string_to_lower(String string) {
    String upper = {0};
    for (size_t idx = 0; idx < string.info.count; idx++) {
        vec_append(&gen_a, &upper, tolower(vec_at(&string, idx)));
    }
    return upper;
}

static Member member_to_upper(Member member) {
    Member upper = {
        .type = string_to_strv(string_to_upper(string_new_from_strv(&gen_a, member.type))),
        .name = string_to_strv(string_to_upper(string_new_from_strv(&gen_a, member.name)))
    };
    return upper;
}

static Member member_to_first_upper(Member member) {
    Member upper = {
        .type = string_to_strv(string_to_first_upper(string_new_from_strv(&gen_a, member.type))),
        .name = string_to_strv(string_to_lower(string_new_from_strv(&gen_a, member.name)))
    };
    return upper;
}

static Members members_to_upper(Members members) {
    Members upper = {0};
    for (size_t idx = 0; idx < members.info.count; idx++) {
        vec_append(&gen_a, &upper, member_to_upper(vec_at(&members, idx)));
    }
    return upper;
}

static Members members_to_first_upper(Members members) {
    Members upper = {0};
    for (size_t idx = 0; idx < members.info.count; idx++) {
        vec_append(&gen_a, &upper, member_to_first_upper(vec_at(&members, idx)));
    }
    return upper;
}

static void gen_unwrap(Str_view type_name) {
    Str_view lower = string_to_strv(string_to_lower(string_new_from_strv(&gen_a, type_name)));
    Str_view upper = string_to_strv(string_to_upper(string_new_from_strv(&gen_a, type_name)));
    Str_view first_upper = string_to_strv(string_to_first_upper(
        string_new_from_strv(&gen_a, type_name)
    ));

    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_strv(&gen_a, &function, first_upper);
    string_extend_cstr(&gen_a, &function, "* node_unwrap_");
    string_extend_strv(&gen_a, &function, lower);
    string_extend_cstr(&gen_a, &function, "(Node* node) {\n");

    //    try(node->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(node->type == ");
    string_extend_strv(&gen_a, &function, upper);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &node->as.");
    string_extend_strv(&gen_a, &function, lower);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_wrap(Str_view type_name) {
    Str_view lower = string_to_strv(string_to_lower(string_new_from_strv(&gen_a, type_name)));
    //Str_view upper = string_to_strv(string_to_upper(string_new_from_strv(&gen_a, type_name)));
    Str_view first_upper = string_to_strv(string_to_first_upper(
        string_new_from_strv(&gen_a, type_name)
    ));

    String function = {0};
    //static inline Node* node_wrap_##lower(Node_##lower* node) {
    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_cstr(&gen_a, &function, "Node* node_wrap_");
    string_extend_strv(&gen_a, &function, str_view_slice(lower, 5, first_upper.count - 5));
    string_extend_cstr(&gen_a, &function, "(");
    string_extend_strv(&gen_a, &function, first_upper);
    string_extend_cstr(&gen_a, &function, "* node");
    string_extend_cstr(&gen_a, &function, ") {\n");

    //    return (Node*)node;
    string_extend_cstr(&gen_a, &function, "    return (Node*)node");
    string_extend_cstr(&gen_a, &function, ";\n");

    //}
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_type(Strv_vec* type_names, Members* types, const char* type_name, Members members) {
    Member member = {
        .type = string_to_strv(string_to_upper(string_new_from_cstr(&gen_a, type_name))),
        .name = string_to_strv(string_to_upper(string_new_from_cstr(&gen_a, type_name))),
    };
    vec_append(&gen_a, types, member);

    gen_struct(string_to_strv(string_to_first_upper(string_new_from_cstr(&gen_a, type_name))), members);

    vec_append(&gen_a, type_names, str_view_from_cstr(type_name));
    //gen_unwrap(str_view_from_cstr(type_name));
}

static void gen_variable_def(Strv_vec* type_names, Members* types) {
    Members members = {0};
    append_member(&members, "bool", "is_variadic");

    log_members(members);
    log(LOG_DEBUG, "hello\n");
    gen_type(type_names, types, "node_variable_def", members);
}

static void gen_block(Strv_vec* type_names, Members* types) {
    Members members = {0};
    append_member(&members, "bool", "is_variadic");
    append_member(&members, "Node_ptr_vec", "children");
    append_member(&members, "Symbol_table", "symbol_table");
    append_member(&members, "Pos", "pos_end");
    gen_type(type_names, types, "node_block", members);
}

static void gen_expr(Strv_vec* type_names, Members* types) {
    Members members = {0};
    append_member(&members, "Node_expr_as", "as");
    append_member(&members, "NODE_EXPR_TYPE ", "type");
    append_member(&members, "Symbol_table", "symbol_table");
    append_member(&members, "Pos", "pos_end");
    gen_type(type_names, types, "node_expr", members);
}


static void gen_node_part_1(Strv_vec* type_names, Members* types) {
    gen_variable_def(type_names, types);
    gen_block(type_names, types);
    gen_expr(type_names, types);

    //gen_enum("Node", members);
    gen_union(str_view_from_cstr("Node_as"), members_to_first_upper(*types));
    gen_enum(str_view_from_cstr("NODE_TYPE"), members_to_upper(*types));
    gen_struct(str_view_from_cstr("Node"), get_top_level());
}

static void gen_node_part_2(Strv_vec type_names) {
    for (size_t idx = 0; idx < type_names.info.count; idx++) {
        gen_unwrap(vec_at(&type_names, idx));
        gen_wrap(vec_at(&type_names, idx));
    }

}

static void gen_node(void) {
    Members types = {0};
    Strv_vec type_names = {0};

    gen_node_part_1(&type_names, &types);
    gen_node_part_2(type_names);
}

int main(int argc, char** argv) {
    assert(argc == 2 && "output file path should be provided");

    global_output = fopen(argv[1], "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    gen_gen("/* autogenerated */");
    gen_gen("#include <node_hand_written.h>");
    gen_gen("#ifndef NODE_H");
    gen_gen("#define NODE_H");

    gen_node();

    gen_gen("#endif /* NODE_H_ */");

    return 0;
}
