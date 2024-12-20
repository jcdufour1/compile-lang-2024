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

// TODO: actually use newline to end statement depending on last token of line of line
// TODO: expected failure case for invalid type in extern "c" function
// TODO: char literal with escape
// TODO: lang_type subtypes for number, etc.
//  lang_type_number should also have subtypes for signed, unsigned, etc.
//
//

#define CLOSE_FILE(file) \
    do { \
        if (file) { \
            fprintf(file, "\n"); \
            fclose(file); \
        } \
        (file) = NULL; \
    } while(0)

FILE* global_output = NULL;
Arena gen_a = {0};

typedef struct {
    Str_view type_name;
    Str_view normal_prefix;
    Str_view internal_prefix;
} Symbol_tbl_type;

typedef struct {
    Vec_base info;
    Symbol_tbl_type* buf;
} Sym_tbl_type_vec;

typedef struct {
    Str_view name;
    Str_view type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

struct Type_;
typedef struct Type_ Type;

typedef struct {
    Vec_base info;
    Type* buf;
} Type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Node_name;

// eg. in Node_symbol_typed
//  prefix = "Node"
//  base = "symbol_typed"
//  sub_types = [{.prefix = "Node_symbol_typed", .base = "primitive", .sub_types = []}, ...]
typedef struct Type_ {
    Node_name name;
    Members members;
    Type_vec sub_types;
} Type;

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...)
__attribute__((format (printf, 4, 5)));

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...) {
    assert(output);

    va_list args;
    va_start(args, format);

    vfprintf(output, format, args);
    fprintf(output, "    /* %s:%d: */", file, line);

    va_end(args);
}

#define gen_gen(...) \
    gen_gen_internal(global_output, __FILE__, __LINE__, __VA_ARGS__)

static void extend_strv_upper(String* output, Str_view name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, toupper(str_view_at(name, idx)));
    }
}

static void extend_strv_lower(String* output, Str_view name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(str_view_at(name, idx)));
    }
}

static void extend_strv_first_upper(String* output, Str_view name) {
    if (name.count < 1) {
        return;
    }
    vec_append(&gen_a, output, toupper(str_view_front(name)));
    for (size_t idx = 1; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(str_view_at(name, idx)));
    }
}

static void extend_node_name_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "NODE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_node_name_lower(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "node");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_node_name_first_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Node");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_node_name_upper(String* output, Node_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "NODE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_node_name_lower(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        string_extend_cstr(&gen_a, output, "node");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "node");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_node_name_first_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        string_extend_cstr(&gen_a, output, "Node");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Node");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Node_name node_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Node_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Type gen_block(void) {
    Type block = {.name = node_name_new("node", "block", false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Node_ptr_vec", "children");
    append_member(&block.members, "Symbol_table", "symbol_table");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Type gen_unary(void) {
    Type unary = {.name = node_name_new("operator", "unary", false)};

    append_member(&unary.members, "Node_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");
    append_member(&unary.members, "Llvm_id", "llvm_id");

    return unary;
}

static Type gen_binary(void) {
    Type binary = {.name = node_name_new("operator", "binary", false)};

    append_member(&binary.members, "Node_expr*", "lhs");
    append_member(&binary.members, "Node_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");
    append_member(&binary.members, "Llvm_id", "llvm_id");

    return binary;
}

static Type gen_primitive_sym(void) {
    Type primitive = {.name = node_name_new("symbol_typed", "primitive_sym", false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Type gen_enum_sym(void) {
    Type lang_enum = {.name = node_name_new("symbol_typed", "enum_sym", false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Type gen_struct_sym(void) {
    Type lang_struct = {.name = node_name_new("symbol_typed", "struct_sym", false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Type gen_raw_union_sym(void) {
    Type raw_union = {.name = node_name_new("symbol_typed", "raw_union_sym", false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Type gen_operator(void) {
    Type operator = {.name = node_name_new("expr", "operator", false)};

    vec_append(&gen_a, &operator.sub_types, gen_unary());
    vec_append(&gen_a, &operator.sub_types, gen_binary());

    return operator;
}

static Type gen_symbol_untyped(void) {
    Type sym = {.name = node_name_new("expr", "symbol_untyped", false)};

    append_member(&sym.members, "Str_view", "name");

    return sym;
}

static Type gen_symbol_typed(void) {
    Type sym = {.name = node_name_new("expr", "symbol_typed", false)};

    vec_append(&gen_a, &sym.sub_types, gen_primitive_sym());
    vec_append(&gen_a, &sym.sub_types, gen_struct_sym());
    vec_append(&gen_a, &sym.sub_types, gen_raw_union_sym());
    vec_append(&gen_a, &sym.sub_types, gen_enum_sym());

    return sym;
}

static Type gen_member_access_typed(void) {
    Type access = {.name = node_name_new("expr", "member_access_typed", false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");
    append_member(&access.members, "Llvm_id", "llvm_id");

    return access;
}

static Type gen_member_access_untyped(void) {
    Type access = {.name = node_name_new("expr", "member_access_untyped", false)};

    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");

    return access;
}

static Type gen_index_untyped(void) {
    Type index = {.name = node_name_new("expr", "index_untyped", false)};

    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");

    return index;
}

static Type gen_index_typed(void) {
    Type index = {.name = node_name_new("expr", "index_typed", false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");
    append_member(&index.members, "Llvm_id", "llvm_id");

    return index;
}

static Type gen_number(void) {
    Type number = {.name = node_name_new("literal", "number", false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Type gen_string(void) {
    Type string = {.name = node_name_new("literal", "string", false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Type gen_char(void) {
    Type lang_char = {.name = node_name_new("literal", "char", false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Type gen_void(void) {
    Type lang_void = {.name = node_name_new("literal", "void", false)};

    append_member(&lang_void.members, "int", "dummy");

    return lang_void;
}

static Type gen_enum_lit(void) {
    Type enum_lit = {.name = node_name_new("literal", "enum_lit", false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Type gen_literal(void) {
    Type lit = {.name = node_name_new("expr", "literal", false)};

    vec_append(&gen_a, &lit.sub_types, gen_number());
    vec_append(&gen_a, &lit.sub_types, gen_string());
    vec_append(&gen_a, &lit.sub_types, gen_void());
    vec_append(&gen_a, &lit.sub_types, gen_enum_lit());
    vec_append(&gen_a, &lit.sub_types, gen_char());

    return lit;
}

static Type gen_function_call(void) {
    Type call = {.name = node_name_new("expr", "function_call", false)};

    append_member(&call.members, "Expr_ptr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Llvm_id", "llvm_id");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Type gen_struct_literal(void) {
    Type lit = {.name = node_name_new("expr", "struct_literal", false)};

    append_member(&lit.members, "Node_ptr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Llvm_id", "llvm_id");

    return lit;
}

static Type gen_llvm_placeholder(void) {
    Type placeholder = {.name = node_name_new("expr", "llvm_placeholder", false)};

    append_member(&placeholder.members, "Lang_type", "lang_type");
    append_member(&placeholder.members, "Llvm_register_sym", "llvm_reg");

    return placeholder;
}

static Type gen_expr(void) {
    Type expr = {.name = node_name_new("node", "expr", false)};

    vec_append(&gen_a, &expr.sub_types, gen_operator());
    vec_append(&gen_a, &expr.sub_types, gen_symbol_untyped());
    vec_append(&gen_a, &expr.sub_types, gen_symbol_typed());
    vec_append(&gen_a, &expr.sub_types, gen_member_access_untyped());
    vec_append(&gen_a, &expr.sub_types, gen_member_access_typed());
    vec_append(&gen_a, &expr.sub_types, gen_index_untyped());
    vec_append(&gen_a, &expr.sub_types, gen_index_typed());
    vec_append(&gen_a, &expr.sub_types, gen_literal());
    vec_append(&gen_a, &expr.sub_types, gen_function_call());
    vec_append(&gen_a, &expr.sub_types, gen_struct_literal());
    vec_append(&gen_a, &expr.sub_types, gen_llvm_placeholder());

    return expr;
}

static Type gen_struct_def(void) {
    Type def = {.name = node_name_new("def", "struct_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_raw_union_def(void) {
    Type def = {.name = node_name_new("def", "raw_union_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_function_decl(void) {
    Type def = {.name = node_name_new("def", "function_decl", false)};

    append_member(&def.members, "Node_function_params*", "parameters");
    append_member(&def.members, "Node_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_function_def(void) {
    Type def = {.name = node_name_new("def", "function_def", false)};

    append_member(&def.members, "Node_function_decl*", "declaration");
    append_member(&def.members, "Node_block*", "body");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Type gen_variable_def(void) {
    Type def = {.name = node_name_new("def", "variable_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic"); // TODO: : 1
    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Llvm_register_sym", "storage_location");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_enum_def(void) {
    Type def = {.name = node_name_new("def", "enum_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_primitive_def(void) {
    Type def = {.name = node_name_new("def", "primitive_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Type gen_label(void) {
    Type def = {.name = node_name_new("def", "label", false)};

    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_string_def(void) {
    Type def = {.name = node_name_new("literal_def", "string_def", false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Type gen_struct_lit_def(void) {
    Type def = {.name = node_name_new("literal_def", "struct_lit_def", false)};

    append_member(&def.members, "Node_ptr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Type gen_literal_def(void) {
    Type def = {.name = node_name_new("def", "literal_def", false)};

    vec_append(&gen_a, &def.sub_types, gen_string_def());
    vec_append(&gen_a, &def.sub_types, gen_struct_lit_def());

    return def;
}

static Type gen_def(void) {
    Type def = {.name = node_name_new("node", "def", false)};

    vec_append(&gen_a, &def.sub_types, gen_function_def());
    vec_append(&gen_a, &def.sub_types, gen_variable_def());
    vec_append(&gen_a, &def.sub_types, gen_struct_def());
    vec_append(&gen_a, &def.sub_types, gen_raw_union_def());
    vec_append(&gen_a, &def.sub_types, gen_enum_def());
    vec_append(&gen_a, &def.sub_types, gen_primitive_def());
    vec_append(&gen_a, &def.sub_types, gen_function_decl());
    vec_append(&gen_a, &def.sub_types, gen_label());
    vec_append(&gen_a, &def.sub_types, gen_literal_def());

    return def;
}

static Type gen_load_element_ptr(void) {
    Type load = {.name = node_name_new("node", "load_element_ptr", false)};

    append_member(&load.members, "Lang_type", "lang_type");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Llvm_register_sym", "struct_index");
    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Str_view", "name");
    append_member(&load.members, "bool", "is_from_struct");

    return load;
}

static Type gen_function_params(void) {
    Type params = {.name = node_name_new("node", "function_params", false)};

    append_member(&params.members, "Node_var_def_vec", "params");
    append_member(&params.members, "Llvm_id", "llvm_id");

    return params;
}

static Type gen_lang_type(void) {
    Type lang_type = {.name = node_name_new("node", "lang_type", false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Type gen_for_lower_bound(void) {
    Type bound = {.name = node_name_new("node", "for_lower_bound", false)};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Type gen_for_upper_bound(void) {
    Type bound = {.name = node_name_new("node", "for_upper_bound", false)};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Type gen_condition(void) {
    Type bound = {.name = node_name_new("node", "condition", false)};

    append_member(&bound.members, "Node_operator*", "child");

    return bound;
}

static Type gen_for_range(void) {
    Type range = {.name = node_name_new("node", "for_range", false)};

    append_member(&range.members, "Node_variable_def*", "var_def");
    append_member(&range.members, "Node_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Node_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Node_block*", "body");

    return range;
}

static Type gen_for_with_cond(void) {
    Type for_cond = {.name = node_name_new("node", "for_with_cond", false)};

    append_member(&for_cond.members, "Node_condition*", "condition");
    append_member(&for_cond.members, "Node_block*", "body");

    return for_cond;
}

static Type gen_break(void) {
    Type lang_break = {.name = node_name_new("node", "break", false)};

    append_member(&lang_break.members, "Node*", "child");

    return lang_break;
}

static Type gen_continue(void) {
    Type lang_cont = {.name = node_name_new("node", "continue", false)};

    append_member(&lang_cont.members, "Node*", "child");

    return lang_cont;
}

static Type gen_assignment(void) {
    Type assign = {.name = node_name_new("node", "assignment", false)};

    append_member(&assign.members, "Node*", "lhs");
    append_member(&assign.members, "Node_expr*", "rhs");

    return assign;
}

static Type gen_if(void) {
    Type lang_if = {.name = node_name_new("node", "if", false)};

    append_member(&lang_if.members, "Node_condition*", "condition");
    append_member(&lang_if.members, "Node_block*", "body");

    return lang_if;
}

static Type gen_return(void) {
    Type rtn = {.name = node_name_new("node", "return", false)};

    append_member(&rtn.members, "Node_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Type gen_goto(void) {
    Type lang_goto = {.name = node_name_new("node", "goto", false)};

    append_member(&lang_goto.members, "Str_view", "name");
    append_member(&lang_goto.members, "Llvm_id", "llvm_id");

    return lang_goto;
}

static Type gen_cond_goto(void) {
    Type cond_goto = {.name = node_name_new("node", "cond_goto", false)};

    append_member(&cond_goto.members, "Llvm_register_sym", "node_src");
    append_member(&cond_goto.members, "Str_view", "if_true");
    append_member(&cond_goto.members, "Str_view", "if_false");
    append_member(&cond_goto.members, "Llvm_id", "llvm_id");

    return cond_goto;
}

static Type gen_alloca(void) {
    Type lang_alloca = {.name = node_name_new("node", "alloca", false)};

    append_member(&lang_alloca.members, "Llvm_id", "llvm_id");
    append_member(&lang_alloca.members, "Lang_type", "lang_type");
    append_member(&lang_alloca.members, "Str_view", "name");

    return lang_alloca;
}

static Type gen_load_another_node(void) {
    Type load = {.name = node_name_new("node", "load_another_node", false)};

    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Lang_type", "lang_type");

    return load;
}

static Type gen_store_another_node(void) {
    Type store = {.name = node_name_new("node", "store_another_node", false)};

    append_member(&store.members, "Llvm_register_sym", "node_src");
    append_member(&store.members, "Llvm_register_sym", "node_dest");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");

    return store;
}

static Type gen_if_else_chain(void) {
    Type chain = {.name = node_name_new("node", "if_else_chain", false)};

    append_member(&chain.members, "Node_if_ptr_vec", "nodes");

    return chain;
}

static Type gen_node(void) {
    Type node = {.name = node_name_new("node", "", true)};

    vec_append(&gen_a, &node.sub_types, gen_block());
    vec_append(&gen_a, &node.sub_types, gen_expr());
    vec_append(&gen_a, &node.sub_types, gen_load_element_ptr());
    vec_append(&gen_a, &node.sub_types, gen_function_params());
    vec_append(&gen_a, &node.sub_types, gen_lang_type());
    vec_append(&gen_a, &node.sub_types, gen_for_lower_bound());
    vec_append(&gen_a, &node.sub_types, gen_for_upper_bound());
    vec_append(&gen_a, &node.sub_types, gen_def());
    vec_append(&gen_a, &node.sub_types, gen_condition());
    vec_append(&gen_a, &node.sub_types, gen_for_range());
    vec_append(&gen_a, &node.sub_types, gen_for_with_cond());
    vec_append(&gen_a, &node.sub_types, gen_break());
    vec_append(&gen_a, &node.sub_types, gen_continue());
    vec_append(&gen_a, &node.sub_types, gen_assignment());
    vec_append(&gen_a, &node.sub_types, gen_if());
    vec_append(&gen_a, &node.sub_types, gen_return());
    vec_append(&gen_a, &node.sub_types, gen_goto());
    vec_append(&gen_a, &node.sub_types, gen_cond_goto());
    vec_append(&gen_a, &node.sub_types, gen_alloca());
    vec_append(&gen_a, &node.sub_types, gen_load_another_node());
    vec_append(&gen_a, &node.sub_types, gen_store_another_node());
    vec_append(&gen_a, &node.sub_types, gen_if_else_chain());

    return node;
}

static void gen_node_forward_decl(Type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        gen_node_forward_decl(vec_at(&node.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void extend_struct_member(String* output, Member member) {
    string_extend_cstr(&gen_a, output, "    ");
    string_extend_strv(&gen_a, output, member.type);
    string_extend_cstr(&gen_a, output, " ");
    string_extend_strv(&gen_a, output, member.name);
    string_extend_cstr(&gen_a, output, ";\n");
}

static void gen_node_struct_as(String* output, Type node) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_node_name_first_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Type curr = vec_at(&node.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_node_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_node_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_node_name_first_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void gen_node_struct_enum(String* output, Type node) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Type curr = vec_at(&node.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_node_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void gen_node_struct(Type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        gen_node_struct(vec_at(&node.sub_types, idx));
    }

    if (node.sub_types.info.count > 0) {
        gen_node_struct_as(&output, node);
        gen_node_struct_enum(&output, node);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (node.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_node_name_first_upper(&as_member_type, node.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_node_name_upper(&enum_member_type, node.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < node.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&node.members, idx));
    }

    if (node.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void gen_unwrap_internal(Type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node_unwrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node) {\n");

    //    try(node->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(node->type == ");
    extend_node_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &node->as.");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_wrap_internal(Type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node_wrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node) {\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) node;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void gen_node_unwrap(Type node) {
    gen_unwrap_internal(node, false);
    gen_unwrap_internal(node, true);
}

void gen_node_wrap(Type node) {
    gen_wrap_internal(node, false);
    gen_wrap_internal(node, true);
}

static void gen_new_internal(Type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(");
    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    } else {
        string_extend_cstr(&gen_a, &function, "Pos pos");
    }
    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        extend_parent_node_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_node = ");
        extend_parent_node_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_node->type = ");
        extend_node_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    node_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_node)->pos = pos;\n");
        }

        string_extend_cstr(&gen_a, &function, "    return node_unwrap_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(base_node);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_get_pos_internal(Type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_get_pos_internal(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    node_get_pos(const Node* node)");
    } else {
        string_extend_cstr(&gen_a, &function, "    node_get_pos_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(const ");
        extend_node_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* node)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return node->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (node->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_node_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return node_get_pos_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "(node_unwrap_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const(node));\n");

                string_extend_cstr(&gen_a, &function, "        break;\n");
            }

            string_extend_cstr(&gen_a, &function, "    }\n");
        }

        string_extend_cstr(&gen_a, &function, "}\n");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_node_new_forward_decl(Type node) {
    gen_new_internal(node, false);
}

static void gen_node_new_define(Type node) {
    gen_new_internal(node, true);
}

static void gen_node_get_pos_forward_decl(Type node) {
    gen_get_pos_internal(node, false);
}

static void gen_node_get_pos_define(Type node) {
    gen_get_pos_internal(node, true);
}

static void gen_all_nodes(const char* file_path) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("/* autogenerated */\n");
    gen_gen("#include <node_hand_written.h>\n");
    gen_gen("#include <expr_ptr_vec.h>\n");
    gen_gen("#include <token.h>\n");
    gen_gen("#ifndef NODE_H\n");
    gen_gen("#define NODE_H\n");

    Type node = gen_node();

    gen_node_forward_decl(node);
    gen_node_struct(node);

    gen_node_unwrap(node);
    gen_node_wrap(node);

    gen_gen("%s\n", "static inline Node* node_new(void) {");
    gen_gen("%s\n", "    Node* new_node = arena_alloc(&a_main, sizeof(*new_node));");
    gen_gen("%s\n", "    return new_node;");
    gen_gen("%s\n", "}");

    gen_node_new_forward_decl(node);
    gen_node_new_define(node);

    gen_node_get_pos_forward_decl(node);
    gen_node_get_pos_define(node);

    gen_gen("#endif // NODE_H\n");

    CLOSE_FILE(global_output);
}

static void gen_symbol_table_c_file_internal(Symbol_tbl_type type) {
    String text = {0};

    // TODO: callback, Str_view or other solution for getting node_name?
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(int log_level, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth, const char* file_path, int line) {\n");
    string_extend_cstr(&gen_a, &text, "    String padding = {0};\n");
    string_extend_cstr(&gen_a, &text, "    int indent_amt = 2*(recursion_depth + 4);\n");
    string_extend_cstr(&gen_a, &text, "    vec_reserve(&print_arena, &padding, indent_amt);\n");
    string_extend_cstr(&gen_a, &text, "    for (int idx = 0; idx < indent_amt; idx++) {\n");
    string_extend_cstr(&gen_a, &text, "        vec_append(&print_arena, &padding, ' ');\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* sym_node = &sym_table.table_nodes[idx];\n");
    string_extend_cstr(&gen_a, &text, "        if (sym_node->status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            log_file_new(log_level, file_path, line, STRING_FMT NODE_FMT\"\\n\", string_print(padding), node_print((Node*)(sym_node->node)));\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol is already added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* sym_tbl_nodes, size_t capacity, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    assert(node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "    Str_view symbol_name = get_def_name(node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "    assert(symbol_name.count > 0 && \"invalid node_of_symbol\");\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    assert(capacity > 0);\n");
    string_extend_cstr(&gen_a, &text, "    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);\n");
    string_extend_cstr(&gen_a, &text, "    size_t init_table_idx = curr_table_idx; \n");
    string_extend_cstr(&gen_a, &text, "    while (sym_tbl_nodes[curr_table_idx].status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "        if (str_view_is_equal(get_def_name(sym_tbl_nodes[curr_table_idx].node), symbol_name)) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "        curr_table_idx = (curr_table_idx + 1) % capacity;\n");
    string_extend_cstr(&gen_a, &text, "        assert(init_table_idx != curr_table_idx && \"hash table is full here, and it should not be\");\n");
    string_extend_cstr(&gen_a, &text, "        (void) init_table_idx;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node node = {.key = symbol_name, .node = node_of_symbol, .status = SYM_TBL_OCCUPIED};\n");
    string_extend_cstr(&gen_a, &text, "    sym_tbl_nodes[curr_table_idx] = node;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_cpy(\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* dest,\n");
    string_extend_cstr(&gen_a, &text, "    const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* src,\n");
    string_extend_cstr(&gen_a, &text, "    size_t capacity,\n");
    string_extend_cstr(&gen_a, &text, "    size_t count_nodes_to_cpy\n");
    string_extend_cstr(&gen_a, &text, ") {\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t bucket_src = 0; bucket_src < count_nodes_to_cpy; bucket_src++) {\n");
    string_extend_cstr(&gen_a, &text, "        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            sym_tbl_add_internal(dest, capacity, src[bucket_src].node);\n");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(dest, capacity, src[bucket_src].node);\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_expand_if_nessessary(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table) {\n");
    string_extend_cstr(&gen_a, &text, "    size_t old_capacity_node_count = sym_table->capacity;\n");
    string_extend_cstr(&gen_a, &text, "    size_t minimum_count_to_reserve = 1;\n");
    string_extend_cstr(&gen_a, &text, "    size_t new_count = sym_table->count + minimum_count_to_reserve;\n");
    string_extend_cstr(&gen_a, &text, "    size_t node_size = sizeof(sym_table->table_nodes[0]);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    bool should_move_elements = false;\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* new_table_nodes;\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    if (sym_table->capacity < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;\n");
    string_extend_cstr(&gen_a, &text, "        should_move_elements = true;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->capacity *= 2;\n");
    string_extend_cstr(&gen_a, &text, "        should_move_elements = true;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    if (should_move_elements) {\n");
    string_extend_cstr(&gen_a, &text, "        new_table_nodes = arena_alloc(&a_main, sym_table->capacity*node_size);\n");
    string_extend_cstr(&gen_a, &text, "        sym_tbl_cpy(new_table_nodes, sym_table->table_nodes, sym_table->capacity, old_capacity_node_count);\n");
    string_extend_cstr(&gen_a, &text, "        if (sym_table->table_nodes) {\n");
    string_extend_cstr(&gen_a, &text, "            // not freeing here currently\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->table_nodes = new_table_nodes;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// return false if symbol is not found\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key) {\n");
    string_extend_cstr(&gen_a, &text, "    if (sym_table->capacity < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    size_t curr_table_idx = ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_calculate_idx(key, sym_table->capacity);\n");
    string_extend_cstr(&gen_a, &text, "    size_t init_table_idx = curr_table_idx; \n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    while (1) {\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* curr_node = &sym_table->table_nodes[curr_table_idx];\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_node->status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            if (str_view_is_equal(curr_node->key, key)) {\n");
    string_extend_cstr(&gen_a, &text, "                *result = curr_node;\n");
    string_extend_cstr(&gen_a, &text, "                return true;\n");
    string_extend_cstr(&gen_a, &text, "            }\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_node->status == SYM_TBL_NEVER_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        curr_table_idx = (curr_table_idx + 1) % sym_table->capacity;\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_table_idx == init_table_idx) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    unreachable(\"\");\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol has already been added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_expand_if_nessessary(sym_table);\n");
    string_extend_cstr(&gen_a, &text, "    assert(sym_table->capacity > 0);\n");
    string_extend_cstr(&gen_a, &text, "    if (!");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(sym_table->table_nodes, sym_table->capacity, node_of_symbol)) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* dummy;\n");
    string_extend_cstr(&gen_a, &text, "    (void) dummy;\n");
    string_extend_cstr(&gen_a, &text, "    assert(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(&dummy, sym_table, get_def_name(node_of_symbol)));\n");
    string_extend_cstr(&gen_a, &text, "    sym_table->count++;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_update(Symbol_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* sym_node;\n");
    string_extend_cstr(&gen_a, &text, "    if (");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(&sym_node, sym_table, get_def_name(node_of_symbol))) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_node->node = node_of_symbol;\n");
    string_extend_cstr(&gen_a, &text, "        return;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    try(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(sym_table, node_of_symbol));\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(int log_level, const Env* env, const char* file_path, int line) {\n");
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        Node* curr_node = vec_at(&env->ancesters, idx);\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_node->type == NODE_BLOCK) {\n");
    string_extend_cstr(&gen_a, &text, "            log(LOG_DEBUG, NODE_FMT\"\\n\", node_print(curr_node));\n");
    string_extend_cstr(&gen_a, &text, "            ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(log_level, node_unwrap_block(curr_node)->symbol_table, 4, file_path, line);\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            return;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const Env* env, Str_view key) {\n");
    string_extend_cstr(&gen_a, &text, "    if (");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(result, &env->primitives, key)) {\n");
    string_extend_cstr(&gen_a, &text, "        return true;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {\n");
    string_extend_cstr(&gen_a, &text, "            const Node_block* block = node_unwrap_block_const(vec_at(&env->ancesters, idx));\n");
    string_extend_cstr(&gen_a, &text, "            if (");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(result, &block->symbol_table, key)) {\n");
    string_extend_cstr(&gen_a, &text, "                return true;\n");
    string_extend_cstr(&gen_a, &text, "            }\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_add(Env* env, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    if (str_view_is_equal(str_view_from_cstr(\"str8\"), get_def_name(node_of_symbol))) {\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* dummy;\n");
    string_extend_cstr(&gen_a, &text, "    if (");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(&dummy, env, get_def_name(node_of_symbol))) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {\n");
    string_extend_cstr(&gen_a, &text, "            Node_block* block = node_unwrap_block(vec_at(&env->ancesters, idx));\n");
    string_extend_cstr(&gen_a, &text, "            try(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(&block->symbol_table, node_of_symbol));\n");
    string_extend_cstr(&gen_a, &text, "            return true;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");

    if (str_view_cstr_is_equal(type.type_name, "Node_def")) {
        string_extend_cstr(&gen_a, &text, "bool symbol_do_add_defered(Node_def** redefined_sym, Env* env) {\n");
        string_extend_cstr(&gen_a, &text, "    for (size_t idx = 0; idx < env->defered_symbols_to_add.info.count; idx++) {\n");
        string_extend_cstr(&gen_a, &text, "        if (!symbol_add(env, vec_at(&env->defered_symbols_to_add, idx))) {\n");
        string_extend_cstr(&gen_a, &text, "            *redefined_sym = vec_at(&env->defered_symbols_to_add, idx);\n");
        string_extend_cstr(&gen_a, &text, "            vec_reset(&env->defered_symbols_to_add);\n");
        string_extend_cstr(&gen_a, &text, "            return false;\n");
        string_extend_cstr(&gen_a, &text, "        }\n");
        string_extend_cstr(&gen_a, &text, "    }\n");
        string_extend_cstr(&gen_a, &text, "\n");
        string_extend_cstr(&gen_a, &text, "    vec_reset(&env->defered_symbols_to_add);\n");
        string_extend_cstr(&gen_a, &text, "    return true;\n");
        string_extend_cstr(&gen_a, &text, "}\n");
        string_extend_cstr(&gen_a, &text, "\n");
        string_extend_cstr(&gen_a, &text, "void log_symbol_table_if_block(Env* env, const char* file_path, int line) {\n");
        string_extend_cstr(&gen_a, &text, "    Node* curr_node = vec_top(&env->ancesters);\n");
        string_extend_cstr(&gen_a, &text, "    if (curr_node->type != NODE_BLOCK)  {\n");
        string_extend_cstr(&gen_a, &text, "        return;\n");
        string_extend_cstr(&gen_a, &text, "    }\n");
        string_extend_cstr(&gen_a, &text, "\n");
        string_extend_cstr(&gen_a, &text, "    Node_block* block = node_unwrap_block(curr_node);\n");
        string_extend_cstr(&gen_a, &text, "    symbol_log_table_internal(LOG_DEBUG, block->symbol_table, env->recursion_depth, file_path, line);\n");
        string_extend_cstr(&gen_a, &text, "}\n");
    }

    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_c_file(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "#define STB_DS_IMPLEMENTATION");
    gen_gen("%s\n", "#include <stb_ds.h>");
    gen_gen("%s\n", "#include \"symbol_table.h\"");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#define SYM_TBL_DEFAULT_CAPACITY 1");
    gen_gen("%s\n", "#define SYM_TBL_MAX_DENSITY (0.6f)");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {");
    gen_gen("%s\n", "    assert(capacity > 0);");
    gen_gen("%s\n", "    return stbds_hash_bytes(key.str, key.count, 0)%capacity;");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_c_file_internal(vec_at(&types, idx));
    }

    CLOSE_FILE(global_output);
}

static void gen_symbol_table_header_internal(Symbol_tbl_type type) {
    String text = {0};

    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(int log_level, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth, const char* file_path, int line);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "#define ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table(log_level, sym_table) \\\n");
    string_extend_cstr(&gen_a, &text, "    do { \\\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__) \\\n");
    string_extend_cstr(&gen_a, &text, "    } while(0)\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol is already added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* sym_tbl_nodes, size_t capacity, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static inline bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* sym_node;\n");
    string_extend_cstr(&gen_a, &text, "    if (!");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(&sym_node, sym_table, key)) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    *result = sym_node->node;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol has already been added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_update(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(int log_level, const Env* env, const char* file_path, int line);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "#define ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log(log_level, env) \\\n");
    string_extend_cstr(&gen_a, &text, "    do { \\\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(log_level, env, __FILE__, __LINE__); \\\n");
    string_extend_cstr(&gen_a, &text, "    } while(0)\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const Env* env, Str_view key);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_add(Env* env, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");

    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_header(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "#ifndef SYMBOL_TABLE_H");
    gen_gen("%s\n", "#define SYMBOL_TABLE_H");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#include \"str_view.h\"");
    gen_gen("%s\n", "#include \"string.h\"");
    gen_gen("%s\n", "#include \"node.h\"");
    gen_gen("%s\n", "#include \"nodes.h\"");
    gen_gen("%s\n", "#include \"node_utils.h\"");
    gen_gen("%s\n", "#include \"env.h\"");
    gen_gen("%s\n", "#include \"symbol_table_struct.h\"");
    gen_gen("%s\n", "#include \"do_passes.h\"");
    gen_gen("%s\n", "");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_header_internal(vec_at(&types, idx));
    }

    gen_gen("%s\n", "// these nodes will be actually added to a symbol table when `symbol_do_add_defered` is called");
    gen_gen("%s\n", "static inline void symbol_add_defer(Env* env, Node_def* node_of_symbol) {");
    gen_gen("%s\n", "    if (str_view_is_equal(str_view_from_cstr(\"str8\"), get_def_name(node_of_symbol))) {");
    gen_gen("%s\n", "    }");
    gen_gen("%s\n", "    assert(node_of_symbol);");
    gen_gen("%s\n", "    vec_append(&a_main, &env->defered_symbols_to_add, node_of_symbol);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "bool symbol_do_add_defered(Node_def** redefined_sym, Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static inline void symbol_ignore_defered(Env* env) {");
    gen_gen("%s\n", "    vec_reset(&env->defered_symbols_to_add);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static inline void symbol_update(Env* env, Node* node_of_symbol) {");
    gen_gen("%s\n", "    Node_def* existing;");
    gen_gen("%s\n", "    if (symbol_lookup(&existing, env, get_node_name(node_of_symbol))) {");
    gen_gen("%s\n", "        todo();");
    gen_gen("%s\n", "    }");
    gen_gen("%s\n", "    todo();");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "Symbol_table* symbol_get_block(Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "void log_symbol_table_if_block(Env* env, const char* file_path, int line);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#define SYM_TBL_STATUS_FMT \"%s\"");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "const char* sym_tbl_status_print(SYM_TBL_STATUS status);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#endif // SYMBOL_TABLE_H");

    CLOSE_FILE(global_output);
}

static void gen_symbol_table_struct_internal(Symbol_tbl_type type) {
    String text = {0};

    string_extend_cstr(&gen_a, &text, "typedef struct {\n");
    string_extend_cstr(&gen_a, &text, "    Str_view key;\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* node;\n");
    string_extend_cstr(&gen_a, &text, "    SYM_TBL_STATUS status;\n");
    string_extend_cstr(&gen_a, &text, "} ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node;\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "typedef struct {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_node* table_nodes;\n");
    string_extend_cstr(&gen_a, &text, "    size_t count; // count elements in symbol_table\n");
    string_extend_cstr(&gen_a, &text, "    size_t capacity; // count buckets in symbol_table\n");
    string_extend_cstr(&gen_a, &text, "} ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table;\n");
    string_extend_cstr(&gen_a, &text, "\n");

    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_struct(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "#ifndef SYMBOL_TABLE_STRUCT_H");
    gen_gen("%s\n", "#define SYMBOL_TABLE_STRUCT_H");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "typedef enum {");
    gen_gen("%s\n", "    SYM_TBL_NEVER_OCCUPIED = 0,");
    gen_gen("%s\n", "    SYM_TBL_PREVIOUSLY_OCCUPIED,");
    gen_gen("%s\n", "    SYM_TBL_OCCUPIED,");
    gen_gen("%s\n", "} SYM_TBL_STATUS;");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "");

    // forward declarations
    gen_gen("%s\n", "struct Node_def_;");
    gen_gen("%s\n", "typedef struct Node_def_ Node_def;");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_struct_internal(vec_at(&types, idx));
    }

    gen_gen("%s\n", "#endif // SYMBOL_TABLE_STRUCT_H");

    CLOSE_FILE(global_output);
}

static Symbol_tbl_type symbol_tbl_type_new(
    const char* type_name,
    const char* normal_prefix,
    const char* internal_prefix
) {
    return (Symbol_tbl_type) {
        .type_name = str_view_from_cstr(type_name),
        .normal_prefix = str_view_from_cstr(normal_prefix),
        .internal_prefix = str_view_from_cstr(internal_prefix)
    };
}

static Sym_tbl_type_vec get_symbol_tbl_types(void) {
    Sym_tbl_type_vec types = {0};

    vec_append(&gen_a, &types, symbol_tbl_type_new("Node_def", "symbol", "sym"));

    return types;
}

int main(int argc, char** argv) {
    assert(argc == 5 && "invalid count of arguments provided");

    Sym_tbl_type_vec symbol_tbl_types = get_symbol_tbl_types();

    gen_symbol_table_struct(argv[1], symbol_tbl_types);
    assert(!global_output);

    gen_all_nodes(argv[2]);
    assert(!global_output);

    gen_symbol_table_header(argv[3], symbol_tbl_types);
    assert(!global_output);

    gen_symbol_table_c_file(argv[4], symbol_tbl_types);
    assert(!global_output);
}

