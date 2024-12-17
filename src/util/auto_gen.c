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

FILE* global_output = NULL;
Arena gen_a = {0};

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
    fprintf(output, "    /* %s:%d: */\n", file, line);

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
    todo();
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

static Node_name node_name_new(const char* parent, const char* base) {
    return (Node_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base)};
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Type gen_block(void) {
    Type block = {.name = node_name_new("node", "block")};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Node_ptr_vec", "children");
    append_member(&block.members, "Symbol_table", "symbol_table");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Type gen_unary(void) {
    Type unary = {.name = node_name_new("operator", "unary")};

    append_member(&unary.members, "Node_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");
    append_member(&unary.members, "Llvm_id", "llvm_id");

    return unary;
}

static Type gen_binary(void) {
    Type binary = {.name = node_name_new("operator", "binary")};

    append_member(&binary.members, "Node_expr*", "lhs");
    append_member(&binary.members, "Node_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");
    append_member(&binary.members, "Llvm_id", "llvm_id");

    return binary;
}

static Type gen_primitive_sym(void) {
    Type primitive = {.name = node_name_new("symbol_typed", "primitive_sym")};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Type gen_enum_sym(void) {
    Type lang_enum = {.name = node_name_new("symbol_typed", "enum_sym")};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Type gen_struct_sym(void) {
    Type lang_struct = {.name = node_name_new("symbol_typed", "struct_sym")};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Type gen_raw_union_sym(void) {
    Type raw_union = {.name = node_name_new("symbol_typed", "raw_union_sym")};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Type gen_operator(void) {
    Type operator = {.name = node_name_new("expr", "operator")};

    vec_append(&gen_a, &operator.sub_types, gen_unary());
    vec_append(&gen_a, &operator.sub_types, gen_binary());

    return operator;
}

static Type gen_symbol_untyped(void) {
    Type sym = {.name = node_name_new("expr", "symbol_untyped")};

    append_member(&sym.members, "Str_view", "name");

    return sym;
}

static Type gen_symbol_typed(void) {
    Type sym = {.name = node_name_new("expr", "symbol_typed")};

    vec_append(&gen_a, &sym.sub_types, gen_primitive_sym());
    vec_append(&gen_a, &sym.sub_types, gen_struct_sym());
    vec_append(&gen_a, &sym.sub_types, gen_raw_union_sym());
    vec_append(&gen_a, &sym.sub_types, gen_enum_sym());

    return sym;
}

static Type gen_member_access_typed(void) {
    Type access = {.name = node_name_new("expr", "member_access_typed")};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");
    append_member(&access.members, "Llvm_id", "llvm_id");

    return access;
}

static Type gen_member_access_untyped(void) {
    Type access = {.name = node_name_new("expr", "member_access_untyped")};

    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");

    return access;
}

static Type gen_index_untyped(void) {
    Type index = {.name = node_name_new("expr", "index_untyped")};

    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");

    return index;
}

static Type gen_index_typed(void) {
    Type index = {.name = node_name_new("expr", "index_typed")};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");
    append_member(&index.members, "Llvm_id", "llvm_id");

    return index;
}

static Type gen_number(void) {
    Type number = {.name = node_name_new("literal", "number")};

    append_member(&number.members, "int64_t", "data");

    return number;
}

static Type gen_string(void) {
    Type string = {.name = node_name_new("literal", "string")};

    append_member(&string.members, "Str_view", "data");

    return string;
}

static Type gen_char(void) {
    Type lang_char = {.name = node_name_new("literal", "char")};

    append_member(&lang_char.members, "char", "data");

    return lang_char;
}

static Type gen_void(void) {
    Type lang_void = {.name = node_name_new("literal", "void")};

    append_member(&lang_void.members, "int", "dummy");

    return lang_void;
}

static Type gen_enum_lit(void) {
    Type enum_lit = {.name = node_name_new("literal", "enum_lit")};

    append_member(&enum_lit.members, "int64_t", "data");

    return enum_lit;
}

static Type gen_literal(void) {
    Type lit = {.name = node_name_new("expr", "literal")};

    vec_append(&gen_a, &lit.sub_types, gen_number());
    vec_append(&gen_a, &lit.sub_types, gen_string());
    vec_append(&gen_a, &lit.sub_types, gen_void());
    vec_append(&gen_a, &lit.sub_types, gen_enum_lit());
    vec_append(&gen_a, &lit.sub_types, gen_char());

    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Str_view", "name");

    return lit;
}

static Type gen_function_call(void) {
    Type call = {.name = node_name_new("expr", "function_call")};

    append_member(&call.members, "Expr_ptr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Llvm_id", "llvm_id");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Type gen_struct_literal(void) {
    Type lit = {.name = node_name_new("expr", "struct_literal")};

    append_member(&lit.members, "Node_ptr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Llvm_id", "llvm_id");

    return lit;
}

static Type gen_llvm_placeholder(void) {
    Type placeholder = {.name = node_name_new("expr", "llvm_placeholder")};

    append_member(&placeholder.members, "Lang_type", "lang_type");
    append_member(&placeholder.members, "Llvm_register_sym", "llvm_reg");

    return placeholder;
}

static Type gen_expr(void) {
    Type expr = {.name = node_name_new("node", "expr")};

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
    Type def = {.name = node_name_new("def", "struct_def")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_raw_union_def(void) {
    Type def = {.name = node_name_new("def", "raw_union_def")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_function_decl(void) {
    Type def = {.name = node_name_new("def", "function_decl")};

    append_member(&def.members, "Node_function_params*", "parameters");
    append_member(&def.members, "Node_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_function_def(void) {
    Type def = {.name = node_name_new("def", "function_def")};

    append_member(&def.members, "Node_function_decl*", "declaration");
    append_member(&def.members, "Node_block*", "body");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Type gen_variable_def(void) {
    Type def = {.name = node_name_new("def", "variable_def")};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic"); // TODO: : 1
    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Llvm_register_sym", "storage_location");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_enum_def(void) {
    Type def = {.name = node_name_new("def", "enum_def")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Type gen_primitive_def(void) {
    Type def = {.name = node_name_new("def", "primitive_def")};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Type gen_label(void) {
    Type def = {.name = node_name_new("def", "label")};

    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Type gen_string_def(void) {
    Type def = {.name = node_name_new("literal_def", "string_def")};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Type gen_struct_lit_def(void) {
    Type def = {.name = node_name_new("literal_def", "struct_lit_def")};

    append_member(&def.members, "Node_ptr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Type gen_literal_def(void) {
    Type def = {.name = node_name_new("def", "literal_def")};

    vec_append(&gen_a, &def.sub_types, gen_string_def());
    vec_append(&gen_a, &def.sub_types, gen_struct_lit_def());

    return def;
}

static Type gen_def(void) {
    Type def = {.name = node_name_new("node", "def")};

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
    Type load = {.name = node_name_new("node", "load_element_ptr")};

    append_member(&load.members, "Lang_type", "lang_type");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Llvm_register_sym", "struct_index");
    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Str_view", "name");
    append_member(&load.members, "bool", "is_from_struct");

    return load;
}

static Type gen_function_params(void) {
    Type params = {.name = node_name_new("node", "function_params")};

    append_member(&params.members, "Node_var_def_vec", "params");
    append_member(&params.members, "Llvm_id", "llvm_id");

    return params;
}

static Type gen_lang_type(void) {
    Type lang_type = {.name = node_name_new("node", "lang_type")};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Type gen_for_lower_bound(void) {
    Type bound = {.name = node_name_new("node", "for_lower_bound")};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Type gen_for_upper_bound(void) {
    Type bound = {.name = node_name_new("node", "for_upper_bound")};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Type gen_condition(void) {
    Type bound = {.name = node_name_new("node", "condition")};

    append_member(&bound.members, "Node_operator*", "child");

    return bound;
}

static Type gen_for_range(void) {
    Type range = {.name = node_name_new("node", "for_range")};

    append_member(&range.members, "Node_variable_def*", "var_def");
    append_member(&range.members, "Node_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Node_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Node_block*", "body");

    return range;
}

static Type gen_for_with_cond(void) {
    Type for_cond = {.name = node_name_new("node", "for_with_cond")};

    append_member(&for_cond.members, "Node_condition*", "condition");
    append_member(&for_cond.members, "Node_block*", "body");

    return for_cond;
}

static Type gen_break(void) {
    Type lang_break = {.name = node_name_new("node", "break")};

    append_member(&lang_break.members, "Node*", "child");

    return lang_break;
}

static Type gen_continue(void) {
    Type lang_cont = {.name = node_name_new("node", "continue")};

    append_member(&lang_cont.members, "Node*", "child");

    return lang_cont;
}

static Type gen_assignment(void) {
    Type assign = {.name = node_name_new("node", "assignment")};

    append_member(&assign.members, "Node*", "lhs");
    append_member(&assign.members, "Node_expr*", "rhs");

    return assign;
}

static Type gen_if(void) {
    Type lang_if = {.name = node_name_new("node", "if")};

    append_member(&lang_if.members, "Node_condition*", "condition");
    append_member(&lang_if.members, "Node_block*", "body");

    return lang_if;
}

static Type gen_return(void) {
    Type rtn = {.name = node_name_new("node", "return")};

    append_member(&rtn.members, "Node_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Type gen_goto(void) {
    Type lang_goto = {.name = node_name_new("node", "goto")};

    append_member(&lang_goto.members, "Str_view", "name");
    append_member(&lang_goto.members, "Llvm_id", "llvm_id");

    return lang_goto;
}

static Type gen_cond_goto(void) {
    Type cond_goto = {.name = node_name_new("node", "cond_goto")};

    append_member(&cond_goto.members, "Llvm_register_sym", "node_src");
    append_member(&cond_goto.members, "Str_view", "if_true");
    append_member(&cond_goto.members, "Str_view", "if_false");
    append_member(&cond_goto.members, "Llvm_id", "llvm_id");

    return cond_goto;
}

static Type gen_alloca(void) {
    Type lang_alloca = {.name = node_name_new("node", "alloca")};

    append_member(&lang_alloca.members, "Llvm_id", "llvm_id");
    append_member(&lang_alloca.members, "Lang_type", "lang_type");
    append_member(&lang_alloca.members, "Str_view", "name");

    return lang_alloca;
}

static Type gen_llvm_store_literal(void) {
    Type store = {.name = node_name_new("node", "llvm_store_literal")};

    append_member(&store.members, "Node_literal*", "child");
    append_member(&store.members, "Llvm_register_sym", "node_dest");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");

    return store;
}

static Type gen_load_another_node(void) {
    Type load = {.name = node_name_new("node", "load_another_node")};

    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Lang_type", "lang_type");

    return load;
}

static Type gen_store_another_node(void) {
    Type store = {.name = node_name_new("node", "store_another_node")};

    append_member(&store.members, "Llvm_register_sym", "node_src");
    append_member(&store.members, "Llvm_register_sym", "node_dest");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");

    return store;
}

static Type gen_llvm_store_struct_literal(void) {
    Type store = {.name = node_name_new("node", "llvm_store_struct_literal")};

    append_member(&store.members, "Node_struct_literal*", "child");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");
    append_member(&store.members, "Llvm_register_sym", "node_dest");

    return store;
}

static Type gen_if_else_chain(void) {
    Type chain = {.name = node_name_new("node", "if_else_chain")};

    append_member(&chain.members, "Node_if_ptr_vec", "nodes");

    return chain;
}

static Type gen_node(void) {
    Type node = {.name = node_name_new("node", "")};

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
    vec_append(&gen_a, &node.sub_types, gen_llvm_store_literal());
    vec_append(&gen_a, &node.sub_types, gen_load_another_node());
    vec_append(&gen_a, &node.sub_types, gen_store_another_node());
    vec_append(&gen_a, &node.sub_types, gen_llvm_store_struct_literal());
    vec_append(&gen_a, &node.sub_types, gen_if_else_chain());
    
    append_member(&node.members, "Pos", "pos");

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

    Type node = gen_node();

    gen_node_forward_decl(node);
    gen_node_struct(node);

    gen_node_unwrap(node);
    gen_node_wrap(node);

    gen_gen("#endif // NODE_H");
}

