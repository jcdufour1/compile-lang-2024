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

#define log_type_vec(log_level, types) \
    log_type_vec_internal(log_level, __FILE__, __LINE__, types)

static void log_type_internal(LOG_LEVEL log_level, const char* file, int line, Type type) {
    log_internal(
        log_level, file, line, 0, STR_VIEW_FMT" "STR_VIEW_FMT"\n",
        str_view_print(type.prefix), str_view_print(type.type_name)
    );
}

static void log_type_vec_internal(LOG_LEVEL log_level, const char* file, int line, Type_vec types) {
    for (size_t idx = 0; idx < types.info.count; idx++) {
        log_type_internal(log_level, file, line, vec_at(&types, idx));
    }
}

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
    append_member(&top_level, "Node_expr_as", "as");
    append_member(&top_level, "NODE_EXPR_TYPE", "type");
    return top_level;
}

static Members node_get_top_level(void) {
    Members top_level = {0};
    append_member(&top_level, "Node_as", "as");
    append_member(&top_level, "NODE_TYPE", "type");
    append_member(&top_level, "Pos", "pos");
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

static Type type_new_from_cstr(const char* prefix, const char* type_name) {
    Type type = {.prefix = str_view_from_cstr(prefix), .type_name = str_view_from_cstr(type_name)};
    return type;
}

static Type type_new(Str_view prefix, const char* type_name) {
    Type type = {
        .prefix = prefix,
        .type_name = str_view_from_cstr(type_name),
    };
    return type;
}

static void gen_unwrap(Type type, bool is_const) {
    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    string_extend_strv(&gen_a, &function, strv_first_upper(type.type_name));
    string_extend_cstr(&gen_a, &function, "* node_unwrap_");
    string_extend_strv(&gen_a, &function, 
        str_view_slice(strv_lower(type.type_name), 5, type.type_name.count - 5)
    );
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
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

static void gen_wrap(Type type, bool is_const) {
    String function = {0};

    //static inline Node* node_wrap_##lower(Node_##lower* node) {
    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.prefix));
    string_extend_cstr(&gen_a, &function, "* ");
    string_extend_cstr(&gen_a, &function, "node_wrap_");
    string_extend_strv(&gen_a, &function, 
        str_view_slice(strv_lower(type.type_name), 5, type.type_name.count - 5)
    );
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
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

static void gen_variable_def(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "bool", "is_variadic"); // TODO: : 1
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Llvm_register_sym", "storage_location");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_variable_def"), members);
}

static void gen_block(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "bool", "is_variadic");
    append_member(&members, "Node_ptr_vec", "children");
    append_member(&members, "Symbol_table", "symbol_table");
    append_member(&members, "Pos", "pos_end");
    gen_type(type_vec, member_types, type_new(prefix, "node_block"), members);
}

static void gen_member_symbol_typed(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_ptr_vec", "children");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_member_sym_typed"), members);
}

static void gen_member_symbol_untyped(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_ptr_vec", "children");
    append_member(&members, "Str_view", "name");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "node_member_sym_untyped"), members);
}

static void gen_symbol_typed(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Str_view", "str_data"); // TODO: remove this?
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_symbol_typed"), members);
}

static void gen_symbol_untyped(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_symbol_untyped"), members);
}

static void gen_function_call(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Expr_ptr_vec", "args");
    append_member(&members, "Str_view", "name");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "node_function_call"), members);
}

static void gen_struct_literal(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_ptr_vec", "members");
    append_member(&members, "Str_view", "name");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "node_struct_literal"), members);
}

static void gen_string(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Str_view", "data");
    gen_type(type_vec, member_types, type_new(prefix, "node_string"), members);
}

static void gen_number(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "int64_t", "data");
    gen_type(type_vec, member_types, type_new(prefix, "node_number"), members);
}

static void gen_void(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "int", "dummy");
    gen_type(type_vec, member_types, type_new(prefix, "node_void"), members);
}

static void gen_enum_lit(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "int64_t", "data");
    gen_type(type_vec, member_types, type_new(prefix, "node_enum_lit"), members);
}

static void gen_node_literal_part_1(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    gen_number(prefix, type_vec, member_types);
    gen_string(prefix, type_vec, member_types);
    gen_void(prefix, type_vec, member_types);
    gen_enum_lit(prefix, type_vec, member_types);

    gen_union(str_view_from_cstr("Node_literal_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_LITERAL_TYPE"), members_to_upper(*member_types));
}

static void gen_binary(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "lhs");
    append_member(&members, "struct Node_expr_*", "rhs");
    append_member(&members, "TOKEN_TYPE", "token_type");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "node_binary"), members);
}

static void gen_unary(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "child");
    append_member(&members, "TOKEN_TYPE", "token_type");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "node_unary"), members);
}

static void gen_node_operator_part_1(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    gen_binary(prefix, type_vec, member_types);
    gen_unary(prefix, type_vec, member_types);

    gen_union(str_view_from_cstr("Node_operator_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_OPERATOR_TYPE"), members_to_upper(*member_types));
}

static void gen_literal(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Str_view literal_prefix = str_view_from_cstr("node_literal");
    Members members = {0};
    Members literal_members = {0};
    Type_vec literal_type_vec = {0};

    gen_node_literal_part_1(literal_prefix, &literal_type_vec, &literal_members);
    vec_extend(&gen_a, type_vec, &literal_type_vec);

    append_member(&members, "Node_literal_as", "as");
    append_member(&members, "NODE_LITERAL_TYPE", "type");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_literal"), members);
}

static void gen_operator(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Str_view operator_prefix = str_view_from_cstr("node_operator");
    Members members = {0};
    Members operator_members = {0};
    Type_vec operator_type_vec = {0};

    gen_node_operator_part_1(operator_prefix, &operator_type_vec, &operator_members);
    vec_extend(&gen_a, type_vec, &operator_type_vec);

    append_member(&members, "Node_operator_as", "as");
    append_member(&members, "NODE_OPERATOR_TYPE", "type");
    gen_type(type_vec, member_types, type_new(prefix, "node_operator"), members);
}

static void gen_llvm_placeholder(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_register_sym", "llvm_reg");
    gen_type(type_vec, member_types, type_new(prefix, "node_llvm_placeholder"), members);
}

static void gen_node_expr_part_1(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    gen_function_call(prefix, type_vec, member_types);
    gen_symbol_untyped(prefix, type_vec, member_types);
    gen_symbol_typed(prefix, type_vec, member_types);
    gen_member_symbol_untyped(prefix, type_vec, member_types);
    gen_member_symbol_typed(prefix, type_vec, member_types);
    gen_literal(prefix, type_vec, member_types);
    gen_struct_literal(prefix, type_vec, member_types); // TODO: consider struct_literal to be a literal
    gen_operator(prefix, type_vec, member_types);
    gen_llvm_placeholder(prefix, type_vec, member_types);

    gen_union(str_view_from_cstr("Node_expr_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_EXPR_TYPE"), members_to_upper(*member_types));
    //gen_struct(str_view_from_cstr("Node_expr"), node_expr_get_top_level());
}

static void gen_expr(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Str_view expr_prefix = str_view_from_cstr("node_expr");
    Members members = {0};
    Members expr_members = {0};
    Type_vec expr_type_vec = {0};

    gen_node_expr_part_1(expr_prefix, &expr_type_vec, &expr_members);
    vec_extend(&gen_a, type_vec, &expr_type_vec);

    append_member(&members, "Node_expr_as", "as");
    append_member(&members, "NODE_EXPR_TYPE ", "type");
    gen_type(type_vec, member_types, type_new(prefix, "node_expr"), members);
}

static void gen_label(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_label"), members);
}

static void gen_load_element_ptr(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "size_t", "struct_index");
    append_member(&members, "Llvm_register_sym", "node_src");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_load_element_ptr"), members);
}

static void gen_function_params(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_ptr_vec", "params");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "node_function_params"), members);
}

static void gen_lang_type(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "node_lang_type"), members);
}

static void gen_member_sym_piece_untyped(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_member_sym_piece_untyped"), members);
}

static void gen_member_sym_piece_typed(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "size_t", "struct_index");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_member_sym_piece_typed"), members);
}

static void gen_struct_def(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Struct_def_base", "base");
    gen_type(type_vec, member_types, type_new(prefix, "node_struct_def"), members);
}

static void gen_raw_union_def(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Struct_def_base", "base");
    gen_type(type_vec, member_types, type_new(prefix, "node_raw_union_def"), members);
}

static void gen_function_decl(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_function_params*", "parameters");
    append_member(&members, "Node_lang_type*", "return_type");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "node_function_decl"), members);
}

static void gen_function_def(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_function_decl*", "declaration");
    append_member(&members, "Node_block*", "body");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "node_function_def"), members);
}

// TODO: consider removing for_lower_bound and for_upper_bound node types
static void gen_for_lower_bound(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "child");
    gen_type(type_vec, member_types, type_new(prefix, "node_for_lower_bound"), members);
}

static void gen_for_upper_bound(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "child");
    gen_type(type_vec, member_types, type_new(prefix, "node_for_upper_bound"), members);
}

static void gen_for_range(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_variable_def*", "var_def");
    append_member(&members, "Node_for_lower_bound*", "lower_bound");
    append_member(&members, "Node_for_upper_bound*", "upper_bound");
    append_member(&members, "Node_block*", "body");
    gen_type(type_vec, member_types, type_new(prefix, "node_for_range"), members);
}

static void gen_for_with_cond(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_condition*", "condition");
    append_member(&members, "Node_block*", "body");
    gen_type(type_vec, member_types, type_new(prefix, "Node_for_with_cond"), members);
}

static void gen_condition(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "child");
    gen_type(type_vec, member_types, type_new(prefix, "Node_condition"), members);
}

static void gen_break(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_*", "child");
    gen_type(type_vec, member_types, type_new(prefix, "Node_break"), members);
}

static void gen_assignment(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_*", "lhs");
    append_member(&members, "struct Node_expr_*", "rhs");
    gen_type(type_vec, member_types, type_new(prefix, "Node_assignment"), members);
}

static void gen_if(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_condition*", "condition");
    append_member(&members, "Node_block*", "body");
    gen_type(type_vec, member_types, type_new(prefix, "Node_if"), members);
}

static void gen_return(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "struct Node_expr_*", "child");
    append_member(&members, "bool", "is_auto_inserted"); // TODO: use : 1 size?
    gen_type(type_vec, member_types, type_new(prefix, "Node_return"), members);
}

static void gen_goto(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Str_view", "name");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "Node_goto"), members);
}

static void gen_cond_goto(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_operator*", "node_src");
    append_member(&members, "Node_symbol_untyped*", "if_true");
    append_member(&members, "Node_symbol_untyped*", "if_false");
    append_member(&members, "Llvm_id", "llvm_id");
    gen_type(type_vec, member_types, type_new(prefix, "Node_cond_goto"), members);
}

static void gen_alloca(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Str_view", "name");
    gen_type(type_vec, member_types, type_new(prefix, "Node_alloca"), members);
}

static void gen_llvm_store_literal(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_literal*", "child");
    append_member(&members, "Llvm_register_sym", "node_dest");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "Node_llvm_store_literal"), members);
}

static void gen_load_another_node(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Llvm_register_sym", "node_src");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "Node_load_another_node"), members);
}

static void gen_store_another_node(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Llvm_register_sym", "node_src");
    append_member(&members, "Llvm_register_sym", "node_dest");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    gen_type(type_vec, member_types, type_new(prefix, "Node_store_another_node"), members);
}

static void gen_llvm_store_struct_literal(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_struct_literal*", "child");
    append_member(&members, "Llvm_id", "llvm_id");
    append_member(&members, "Lang_type", "lang_type");
    append_member(&members, "Llvm_register_sym", "node_dest");
    gen_type(type_vec, member_types, type_new(prefix, "Node_llvm_store_struct_literal"), members);
}

static void gen_if_else_chain(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Node_if_ptr_vec", "nodes");
    gen_type(type_vec, member_types, type_new(prefix, "Node_if_else_chain"), members);
}

static void gen_enum_def(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    Members members = {0};
    append_member(&members, "Struct_def_base", "base");
    gen_type(type_vec, member_types, type_new(prefix, "Node_enum_def"), members);
}

static void gen_node_part_1(Str_view prefix, Type_vec* type_vec, Members* member_types) {
    gen_variable_def(prefix, type_vec, member_types);
    gen_block(prefix, type_vec, member_types);
    gen_expr(prefix, type_vec, member_types);
    gen_label(prefix, type_vec, member_types);
    gen_load_element_ptr(prefix, type_vec, member_types);
    gen_function_params(prefix, type_vec, member_types);
    gen_lang_type(prefix, type_vec, member_types);
    gen_member_sym_piece_untyped(prefix, type_vec, member_types);
    gen_member_sym_piece_typed(prefix, type_vec, member_types);
    gen_struct_def(prefix, type_vec, member_types);
    gen_raw_union_def(prefix, type_vec, member_types);
    gen_function_decl(prefix, type_vec, member_types);
    gen_function_def(prefix, type_vec, member_types);
    gen_for_lower_bound(prefix, type_vec, member_types);
    gen_for_upper_bound(prefix, type_vec, member_types);
    gen_for_range(prefix, type_vec, member_types);
    gen_condition(prefix, type_vec, member_types);
    gen_for_with_cond(prefix, type_vec, member_types);
    gen_break(prefix, type_vec, member_types);
    gen_assignment(prefix, type_vec, member_types);
    gen_if(prefix, type_vec, member_types);
    gen_return(prefix, type_vec, member_types);
    gen_goto(prefix, type_vec, member_types);
    gen_cond_goto(prefix, type_vec, member_types);
    gen_alloca(prefix, type_vec, member_types);
    gen_llvm_store_literal(prefix, type_vec, member_types);
    gen_load_another_node(prefix, type_vec, member_types);
    gen_store_another_node(prefix, type_vec, member_types);
    gen_llvm_store_struct_literal(prefix, type_vec, member_types);
    gen_if_else_chain(prefix, type_vec, member_types);
    gen_enum_def(prefix, type_vec, member_types);

    // TODO: use prefix variable here?
    gen_union(str_view_from_cstr("Node_as"), members_to_first_upper(*member_types));
    gen_enum(str_view_from_cstr("NODE_TYPE"), members_to_upper(*member_types));
    gen_struct(str_view_from_cstr("Node"), node_get_top_level());
}

static void gen_node_part_2(Type_vec types) {
    for (size_t idx = 0; idx < types.info.count; idx++) {
        Type type = vec_at(&types, idx);
        gen_unwrap(type, false);
        gen_unwrap(type, true);
        gen_wrap(type, false);
        gen_wrap(type, true);
    }
}

static void gen_new(Type type, bool implementation) {
    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    string_extend_strv(&gen_a, &function, strv_first_upper(type.type_name));
    string_extend_cstr(&gen_a, &function, "* ");
    string_extend_strv(&gen_a, &function, strv_lower(type.type_name));
    string_extend_cstr(&gen_a, &function, "_new(Pos pos)");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        string_extend_strv(&gen_a, &function, strv_first_upper(type.prefix));
        string_extend_cstr(&gen_a, &function, "* base_node = ");
        string_extend_strv(&gen_a, &function, strv_lower(type.prefix));
        string_extend_cstr(&gen_a, &function, "_new(pos);\n");

        string_extend_cstr(&gen_a, &function, "    base_node->type = ");
        string_extend_strv(&gen_a, &function, strv_upper(type.type_name));
        string_extend_cstr(&gen_a, &function, ";\n");

        string_extend_cstr(&gen_a, &function, "    return node_unwrap_");
        string_extend_strv(&gen_a, &function, 
            str_view_slice(strv_lower(type.type_name), 5, type.type_name.count - 5)
        );
        string_extend_cstr(&gen_a, &function, "(base_node);\n");

        string_extend_cstr(&gen_a, &function, "}");
    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_node_part_3(Type_vec types) {
    for (size_t idx = 0; idx < types.info.count; idx++) {
        Type type = vec_at(&types, idx);
        gen_new(type, false);
    }
}

static void gen_node_part_4(Type_vec types) {
    for (size_t idx = 0; idx < types.info.count; idx++) {
        Type type = vec_at(&types, idx);
        gen_new(type, true);
    }
}

static void gen_node(void) {
    Str_view prefix = str_view_from_cstr("node");
    Members member_types = {0};
    Type_vec types = {0};

    gen_node_part_1(prefix, &types, &member_types);
    gen_node_part_2(types);

    gen_gen("%s\n", "static inline Node* node_new(Pos pos) {");
    gen_gen("%s\n", "    Node* new_node = arena_alloc(&a_main, sizeof(*new_node));");
    gen_gen("%s\n", "    new_node->pos = pos;");
    gen_gen("%s\n", "    return new_node;");
    gen_gen("%s\n", "}");

    gen_node_part_3(types);
    gen_node_part_4(types);
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
    gen_gen("#include <token.h>");
    gen_gen("#ifndef NODE_H");
    gen_gen("#define NODE_H");

    gen_node();

    gen_gen("#endif /* NODE_H_ */");

    return 0;
}
