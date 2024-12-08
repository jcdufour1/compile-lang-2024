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

typedef struct {
    Str_view prefix;
    Str_view type_name;
} Type;

typedef struct {
    Vec_base info;
    Type* buf;
} Type_vec;

//typedef struct {
//    Members members;
//
//} Lang_struct;

#define TYPE_FMT STR_VIEW_FMT" "STR_VIEW_FMT

#define type_print(type) \
    str_view_print((type).prefix), str_view_print((type).type_name)

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

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Members node_expr_get_top_level(void) {
    Members top_level = {0};
    append_member(&top_level, "NODE_EXPR_TYPE", "type");
    append_member(&top_level, "Node_expr_as", "as");
    return top_level;
}

static Members node_get_top_level(void) {
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

static Str_view strv_lower(Str_view str_view) {
    return string_to_strv(string_to_lower(string_new_from_strv(&gen_a, str_view)));
}

static Str_view strv_upper(Str_view str_view) {
    return string_to_strv(string_to_upper(string_new_from_strv(&gen_a, str_view)));
}

static Str_view strv_first_upper(Str_view str_view) {
    return string_to_strv(string_to_first_upper(string_new_from_strv(&gen_a, str_view)));
}

static void gen_unwrap(Type type) {
    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.type_name));
    string_extend_cstr(&gen_a, &function, "* node_unwrap_");
    string_extend_strv(&gen_a, &function, strv_lower(type.type_name));
    string_extend_cstr(&gen_a, &function, "(");
    log(LOG_DEBUG, TYPE_FMT"\n", type_print(type));
    string_extend_strv(&gen_a, &function, strv_first_upper(type.prefix));
    string_extend_cstr(&gen_a, &function, "* node) {\n");

    //    try(node->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(node->type == ");
    string_extend_strv(&gen_a, &function, strv_upper(type.type_name));
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &node->as.");
    string_extend_strv(&gen_a, &function, strv_lower(type.type_name));
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_wrap(Type type) {
    String function = {0};

    //static inline Node* node_wrap_##lower(Node_##lower* node) {
    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.prefix));
    string_extend_cstr(&gen_a, &function, "* ");
    string_extend_strv(&gen_a, &function, strv_lower(type.prefix));
    string_extend_cstr(&gen_a, &function, "_wrap_");
    string_extend_strv(&gen_a, &function, 
        str_view_slice(strv_lower(type.type_name), 5, type.type_name.count - 5)
    );
    string_extend_cstr(&gen_a, &function, "(");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.type_name));
    string_extend_cstr(&gen_a, &function, "* node");
    string_extend_cstr(&gen_a, &function, ") {\n");

    //    return (Node*)node;
    string_extend_cstr(&gen_a, &function, "    return (");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.prefix));
    string_extend_cstr(&gen_a, &function, "    *)node");
    string_extend_cstr(&gen_a, &function, ";\n");

    //}
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_type(Type_vec* type_vec, Members* member_types, Type type, Members members) {
    Member member = {.type = strv_upper(type.type_name), .name = strv_upper(type.type_name)};
    vec_append(&gen_a, member_types, member);
    gen_struct(strv_first_upper(type.type_name), members);
    vec_append(&gen_a, type_vec, type);
}

static void gen_variable_def(Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "bool", "is_variadic");

    log_members(members);
    Type type = {
        .prefix = str_view_from_cstr("node"),
        .type_name = str_view_from_cstr("node_variable_def")
    };
    gen_type(type_vec, member_types, type, members);
}

static Type type_new_from_cstr(const char* prefix, const char* type_name) {
    Type type = {.prefix = str_view_from_cstr(prefix), .type_name = str_view_from_cstr(type_name)};
    return type;
}

static void gen_block(Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "bool", "is_variadic");
    append_member(&members, "Node_ptr_vec", "children");
    append_member(&members, "Symbol_table", "symbol_table");
    append_member(&members, "Pos", "pos_end");
    gen_type(type_vec, member_types, type_new_from_cstr("node", "node_block"), members);
}

static void gen_function_call(Type_vec* type_names, Members* member_types) {
    Members members = {0};
    append_member(&members, "Expr_ptr_vec", "args");
    append_member(&members, "Str_view", "name");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_names, member_types, type_new_from_cstr("node_expr", "node_function_call"), members);
}

static void gen_node_expr_part_1(Type_vec* type_vec, Members* member_types) {
    gen_function_call(type_vec, member_types);
    //gen_unary(type_names, types);

    gen_union(str_view_from_cstr("Node_expr_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_EXPR_TYPE"), members_to_upper(*member_types));
    //gen_struct(str_view_from_cstr("Node_expr"), node_expr_get_top_level());
}

static void gen_expr(Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    Members expr_members = {0};
    Type_vec expr_type_vec = {0};

    gen_node_expr_part_1(&expr_type_vec, &expr_members);
    vec_extend(&gen_a, type_vec, &expr_type_vec);

    append_member(&members, "Node_expr_as", "as");
    append_member(&members, "NODE_EXPR_TYPE ", "type");
    append_member(&members, "Symbol_table", "symbol_table");
    append_member(&members, "Pos", "pos_end");
    gen_type(type_vec, member_types, type_new_from_cstr("node", "node_expr"), members);
}

static void gen_node_part_1(Type_vec* type_vec, Members* member_types) {
    gen_variable_def(type_vec, member_types);
    gen_block(type_vec, member_types);
    gen_expr(type_vec, member_types);

    //gen_enum("Node", members);
    gen_union(str_view_from_cstr("Node_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_TYPE"), members_to_upper(*member_types));
    gen_struct(str_view_from_cstr("Node"), node_get_top_level());
}

static void gen_node_part_2(Type_vec types) {
    for (size_t idx = 0; idx < types.info.count; idx++) {
        Type type = vec_at(&types, idx);
        gen_unwrap(type);
        gen_wrap(type);
    }
}

static void gen_node(void) {
    Members member_types = {0};
    Type_vec types = {0};

    gen_node_part_1(&types, &member_types);
    gen_node_part_2(types);
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
    gen_gen("#include <expr_ptr_vec.h>");
    gen_gen("#ifndef NODE_H");
    gen_gen("#define NODE_H");

    gen_node();

    gen_gen("#endif /* NODE_H_ */");

    return 0;
}
