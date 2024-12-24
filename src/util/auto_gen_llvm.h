
#ifndef AUTO_GEN_LLVM_H
#define AUTO_GEN_LLVM_H

#include <auto_gen_util.h>

struct Llvm_type_;
typedef struct Llvm_type_ Llvm_type;

typedef struct {
    Vec_base info;
    Llvm_type* buf;
} Llvm_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Llvm_name;

// eg. in Node_symbol_typed
//  prefix = "Node"
//  base = "symbol_typed"
//  sub_types = [{.prefix = "Node_symbol_typed", .base = "primitive", .sub_types = []}, ...]
typedef struct Llvm_type_ {
    Llvm_name name;
    Members members;
    Llvm_type_vec sub_types;
} Llvm_type;

static void extend_llvm_name_upper(String* output, Llvm_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "LLVM");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_llvm_name_lower(String* output, Llvm_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "llvm");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_llvm_name_first_upper(String* output, Llvm_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Llvm");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_llvm_name_upper(String* output, Llvm_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "LLVM");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_llvm_name_lower(String* output, Llvm_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        string_extend_cstr(&gen_a, output, "llvm");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "llvm");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_llvm_name_first_upper(String* output, Llvm_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "llvm")) {
        string_extend_cstr(&gen_a, output, "Llvm");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Llvm");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Llvm_name llvm_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Llvm_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Llvm_type llvm_gen_block(void) {
    Llvm_type block = {.name = llvm_name_new("llvm", "block", false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Llvm_ptr_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Llvm_type llvm_gen_unary(void) {
    Llvm_type unary = {.name = llvm_name_new("operator", "unary", false)};

    append_member(&unary.members, "Llvm_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");
    append_member(&unary.members, "Llvm_id", "llvm_id");

    return unary;
}

static Llvm_type llvm_gen_binary(void) {
    Llvm_type binary = {.name = llvm_name_new("operator", "binary", false)};

    append_member(&binary.members, "Llvm_expr*", "lhs");
    append_member(&binary.members, "Llvm_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");
    append_member(&binary.members, "Llvm_id", "llvm_id");

    return binary;
}

static Llvm_type llvm_gen_primitive_sym(void) {
    Llvm_type primitive = {.name = llvm_name_new("symbol_typed", "primitive_sym", false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Llvm_type llvm_gen_enum_sym(void) {
    Llvm_type lang_enum = {.name = llvm_name_new("symbol_typed", "enum_sym", false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Llvm_type llvm_gen_struct_sym(void) {
    Llvm_type lang_struct = {.name = llvm_name_new("symbol_typed", "struct_sym", false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Llvm_type llvm_gen_raw_union_sym(void) {
    Llvm_type raw_union = {.name = llvm_name_new("symbol_typed", "raw_union_sym", false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Llvm_type llvm_gen_operator(void) {
    Llvm_type operator = {.name = llvm_name_new("expr", "operator", false)};

    vec_append(&gen_a, &operator.sub_types, llvm_gen_unary());
    vec_append(&gen_a, &operator.sub_types, llvm_gen_binary());

    return operator;
}

static Llvm_type llvm_gen_symbol_untyped(void) {
    Llvm_type sym = {.name = llvm_name_new("expr", "symbol_untyped", false)};

    append_member(&sym.members, "Str_view", "name");

    return sym;
}

static Llvm_type llvm_gen_symbol_typed(void) {
    Llvm_type sym = {.name = llvm_name_new("expr", "symbol_typed", false)};

    vec_append(&gen_a, &sym.sub_types, llvm_gen_primitive_sym());
    vec_append(&gen_a, &sym.sub_types, llvm_gen_struct_sym());
    vec_append(&gen_a, &sym.sub_types, llvm_gen_raw_union_sym());
    vec_append(&gen_a, &sym.sub_types, llvm_gen_enum_sym());

    return sym;
}

static Llvm_type llvm_gen_member_access_typed(void) {
    Llvm_type access = {.name = llvm_name_new("expr", "member_access_typed", false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Llvm_expr*", "callee");
    append_member(&access.members, "Llvm_id", "llvm_id");

    return access;
}

static Llvm_type llvm_gen_member_access_untyped(void) {
    Llvm_type access = {.name = llvm_name_new("expr", "member_access_untyped", false)};

    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Llvm_expr*", "callee");

    return access;
}

static Llvm_type llvm_gen_index_untyped(void) {
    Llvm_type index = {.name = llvm_name_new("expr", "index_untyped", false)};

    append_member(&index.members, "Llvm_expr*", "index");
    append_member(&index.members, "Llvm_expr*", "callee");

    return index;
}

static Llvm_type llvm_gen_index_typed(void) {
    Llvm_type index = {.name = llvm_name_new("expr", "index_typed", false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Llvm_expr*", "index");
    append_member(&index.members, "Llvm_expr*", "callee");
    append_member(&index.members, "Llvm_id", "llvm_id");

    return index;
}

static Llvm_type llvm_gen_number(void) {
    Llvm_type number = {.name = llvm_name_new("literal", "number", false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Llvm_type llvm_gen_string(void) {
    Llvm_type string = {.name = llvm_name_new("literal", "string", false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Llvm_type llvm_gen_char(void) {
    Llvm_type lang_char = {.name = llvm_name_new("literal", "char", false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Llvm_type llvm_gen_void(void) {
    Llvm_type lang_void = {.name = llvm_name_new("literal", "void", false)};

    append_member(&lang_void.members, "int", "dummy");

    return lang_void;
}

static Llvm_type llvm_gen_enum_lit(void) {
    Llvm_type enum_lit = {.name = llvm_name_new("literal", "enum_lit", false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Llvm_type llvm_gen_literal(void) {
    Llvm_type lit = {.name = llvm_name_new("expr", "literal", false)};

    vec_append(&gen_a, &lit.sub_types, llvm_gen_number());
    vec_append(&gen_a, &lit.sub_types, llvm_gen_string());
    vec_append(&gen_a, &lit.sub_types, llvm_gen_void());
    vec_append(&gen_a, &lit.sub_types, llvm_gen_enum_lit());
    vec_append(&gen_a, &lit.sub_types, llvm_gen_char());

    return lit;
}

static Llvm_type llvm_gen_function_call(void) {
    Llvm_type call = {.name = llvm_name_new("expr", "function_call", false)};

    append_member(&call.members, "Llvm_expr_ptr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Llvm_id", "llvm_id");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Llvm_type llvm_gen_struct_literal(void) {
    Llvm_type lit = {.name = llvm_name_new("expr", "struct_literal", false)};

    append_member(&lit.members, "Llvm_ptr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Llvm_id", "llvm_id");

    return lit;
}

static Llvm_type llvm_gen_llvm_placeholder(void) {
    Llvm_type placeholder = {.name = llvm_name_new("expr", "llvm_placeholder", false)};

    append_member(&placeholder.members, "Lang_type", "lang_type");
    append_member(&placeholder.members, "Llvm_register_sym", "llvm_reg");

    return placeholder;
}

static Llvm_type llvm_gen_expr(void) {
    Llvm_type expr = {.name = llvm_name_new("llvm", "expr", false)};

    vec_append(&gen_a, &expr.sub_types, llvm_gen_operator());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_symbol_untyped());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_symbol_typed());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_member_access_untyped());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_member_access_typed());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_index_untyped());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_index_typed());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_literal());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_function_call());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_struct_literal());
    vec_append(&gen_a, &expr.sub_types, llvm_gen_llvm_placeholder());

    return expr;
}

static Llvm_type llvm_gen_struct_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "struct_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Llvm_type llvm_gen_raw_union_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "raw_union_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Llvm_type llvm_gen_function_decl(void) {
    Llvm_type def = {.name = llvm_name_new("def", "function_decl", false)};

    append_member(&def.members, "Llvm_function_params*", "parameters");
    append_member(&def.members, "Llvm_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Llvm_type llvm_gen_function_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "function_def", false)};

    append_member(&def.members, "Llvm_function_decl*", "declaration");
    append_member(&def.members, "Llvm_block*", "body");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Llvm_type llvm_gen_variable_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "variable_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic"); // TODO: : 1
    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Llvm_type llvm_gen_enum_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "enum_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Llvm_type llvm_gen_primitive_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "primitive_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Llvm_type llvm_gen_label(void) {
    Llvm_type def = {.name = llvm_name_new("def", "label", false)};

    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Llvm_type llvm_gen_string_def(void) {
    Llvm_type def = {.name = llvm_name_new("literal_def", "string_def", false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Llvm_type llvm_gen_struct_lit_def(void) {
    Llvm_type def = {.name = llvm_name_new("literal_def", "struct_lit_def", false)};

    append_member(&def.members, "Llvm_ptr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Llvm_type llvm_gen_literal_def(void) {
    Llvm_type def = {.name = llvm_name_new("def", "literal_def", false)};

    vec_append(&gen_a, &def.sub_types, llvm_gen_string_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_struct_lit_def());

    return def;
}

static Llvm_type llvm_gen_def(void) {
    Llvm_type def = {.name = llvm_name_new("llvm", "def", false)};

    vec_append(&gen_a, &def.sub_types, llvm_gen_function_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_variable_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_struct_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_raw_union_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_enum_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_primitive_def());
    vec_append(&gen_a, &def.sub_types, llvm_gen_function_decl());
    vec_append(&gen_a, &def.sub_types, llvm_gen_label());
    vec_append(&gen_a, &def.sub_types, llvm_gen_literal_def());

    return def;
}

static Llvm_type llvm_gen_load_element_ptr(void) {
    Llvm_type load = {.name = llvm_name_new("llvm", "load_element_ptr", false)};

    append_member(&load.members, "Lang_type", "lang_type");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Llvm_register_sym", "struct_index");
    append_member(&load.members, "Llvm_register_sym", "llvm_src");
    append_member(&load.members, "Str_view", "name");
    append_member(&load.members, "bool", "is_from_struct");

    return load;
}

static Llvm_type llvm_gen_function_params(void) {
    Llvm_type params = {.name = llvm_name_new("llvm", "function_params", false)};

    append_member(&params.members, "Llvm_var_def_vec", "params");
    append_member(&params.members, "Llvm_id", "llvm_id");

    return params;
}

static Llvm_type llvm_gen_lang_type(void) {
    Llvm_type lang_type = {.name = llvm_name_new("llvm", "lang_type", false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Llvm_type llvm_gen_for_lower_bound(void) {
    Llvm_type bound = {.name = llvm_name_new("llvm", "for_lower_bound", false)};

    append_member(&bound.members, "Llvm_expr*", "child");

    return bound;
}

static Llvm_type llvm_gen_for_upper_bound(void) {
    Llvm_type bound = {.name = llvm_name_new("llvm", "for_upper_bound", false)};

    append_member(&bound.members, "Llvm_expr*", "child");

    return bound;
}

static Llvm_type llvm_gen_condition(void) {
    Llvm_type bound = {.name = llvm_name_new("llvm", "condition", false)};

    append_member(&bound.members, "Llvm_operator*", "child");

    return bound;
}

static Llvm_type llvm_gen_for_range(void) {
    Llvm_type range = {.name = llvm_name_new("llvm", "for_range", false)};

    append_member(&range.members, "Llvm_variable_def*", "var_def");
    append_member(&range.members, "Llvm_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Llvm_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Llvm_block*", "body");

    return range;
}

static Llvm_type llvm_gen_for_with_cond(void) {
    Llvm_type for_cond = {.name = llvm_name_new("llvm", "for_with_cond", false)};

    append_member(&for_cond.members, "Llvm_condition*", "condition");
    append_member(&for_cond.members, "Llvm_block*", "body");

    return for_cond;
}

static Llvm_type llvm_gen_break(void) {
    Llvm_type lang_break = {.name = llvm_name_new("llvm", "break", false)};

    append_member(&lang_break.members, "Llvm*", "child");

    return lang_break;
}

static Llvm_type llvm_gen_continue(void) {
    Llvm_type lang_cont = {.name = llvm_name_new("llvm", "continue", false)};

    append_member(&lang_cont.members, "Llvm*", "child");

    return lang_cont;
}

static Llvm_type llvm_gen_assignment(void) {
    Llvm_type assign = {.name = llvm_name_new("llvm", "assignment", false)};

    append_member(&assign.members, "Llvm*", "lhs");
    append_member(&assign.members, "Llvm_expr*", "rhs");

    return assign;
}

static Llvm_type llvm_gen_if(void) {
    Llvm_type lang_if = {.name = llvm_name_new("llvm", "if", false)};

    append_member(&lang_if.members, "Llvm_condition*", "condition");
    append_member(&lang_if.members, "Llvm_block*", "body");

    return lang_if;
}

static Llvm_type llvm_gen_return(void) {
    Llvm_type rtn = {.name = llvm_name_new("llvm", "return", false)};

    append_member(&rtn.members, "Llvm_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Llvm_type llvm_gen_goto(void) {
    Llvm_type lang_goto = {.name = llvm_name_new("llvm", "goto", false)};

    append_member(&lang_goto.members, "Str_view", "name");
    append_member(&lang_goto.members, "Llvm_id", "llvm_id");

    return lang_goto;
}

static Llvm_type llvm_gen_cond_goto(void) {
    Llvm_type cond_goto = {.name = llvm_name_new("llvm", "cond_goto", false)};

    append_member(&cond_goto.members, "Llvm_register_sym", "llvm_src");
    append_member(&cond_goto.members, "Str_view", "if_true");
    append_member(&cond_goto.members, "Str_view", "if_false");
    append_member(&cond_goto.members, "Llvm_id", "llvm_id");

    return cond_goto;
}

static Llvm_type llvm_gen_alloca(void) {
    Llvm_type lang_alloca = {.name = llvm_name_new("llvm", "alloca", false)};

    append_member(&lang_alloca.members, "Llvm_id", "llvm_id");
    append_member(&lang_alloca.members, "Lang_type", "lang_type");
    append_member(&lang_alloca.members, "Str_view", "name");

    return lang_alloca;
}

static Llvm_type llvm_gen_load_another_llvm(void) {
    Llvm_type load = {.name = llvm_name_new("llvm", "load_another_llvm", false)};

    append_member(&load.members, "Llvm_register_sym", "llvm_src");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Lang_type", "lang_type");

    return load;
}

static Llvm_type llvm_gen_store_another_llvm(void) {
    Llvm_type store = {.name = llvm_name_new("llvm", "store_another_llvm", false)};

    append_member(&store.members, "Llvm_register_sym", "llvm_src");
    append_member(&store.members, "Llvm_register_sym", "llvm_dest");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");

    return store;
}

static Llvm_type llvm_gen_if_else_chain(void) {
    Llvm_type chain = {.name = llvm_name_new("llvm", "if_else_chain", false)};

    append_member(&chain.members, "Llvm_if_ptr_vec", "llvms");

    return chain;
}

static Llvm_type llvm_gen_llvm(void) {
    Llvm_type llvm = {.name = llvm_name_new("llvm", "", true)};

    vec_append(&gen_a, &llvm.sub_types, llvm_gen_block());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_expr());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_load_element_ptr());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_function_params());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_lang_type());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_for_lower_bound());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_for_upper_bound());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_def());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_condition());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_for_range());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_for_with_cond());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_break());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_continue());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_assignment());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_if());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_return());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_goto());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_cond_goto());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_alloca());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_load_another_llvm());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_store_another_llvm());
    vec_append(&gen_a, &llvm.sub_types, llvm_gen_if_else_chain());

    return llvm;
}

static void llvm_gen_llvm_forward_decl(Llvm_type llvm) {
    String output = {0};

    for (size_t idx = 0; idx < llvm.sub_types.info.count; idx++) {
        llvm_gen_llvm_forward_decl(vec_at(&llvm.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_llvm_name_first_upper(&output, llvm.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_llvm_name_first_upper(&output, llvm.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_llvm_name_first_upper(&output, llvm.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void llvm_gen_llvm_struct_as(String* output, Llvm_type llvm) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_llvm_name_first_upper(output, llvm.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < llvm.sub_types.info.count; idx++) {
        Llvm_type curr = vec_at(&llvm.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_llvm_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_llvm_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_llvm_name_first_upper(output, llvm.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void llvm_gen_llvm_struct_enum(String* output, Llvm_type llvm) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_llvm_name_upper(output, llvm.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < llvm.sub_types.info.count; idx++) {
        Llvm_type curr = vec_at(&llvm.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_llvm_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_llvm_name_upper(output, llvm.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void llvm_gen_llvm_struct(Llvm_type llvm) {
    String output = {0};

    for (size_t idx = 0; idx < llvm.sub_types.info.count; idx++) {
        llvm_gen_llvm_struct(vec_at(&llvm.sub_types, idx));
    }

    if (llvm.sub_types.info.count > 0) {
        llvm_gen_llvm_struct_as(&output, llvm);
        llvm_gen_llvm_struct_enum(&output, llvm);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_llvm_name_first_upper(&output, llvm.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (llvm.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_llvm_name_first_upper(&as_member_type, llvm.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_llvm_name_upper(&enum_member_type, llvm.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < llvm.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&llvm.members, idx));
    }

    if (llvm.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_llvm_name_first_upper(&output, llvm.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void llvm_gen_unwrap_internal(Llvm_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        llvm_gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Llvm_##lower* llvm_unwrap_##lower(Llvm* llvm) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* llvm_unwrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* llvm) {\n");

    //    try(llvm->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(llvm->type == ");
    extend_llvm_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &llvm->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &llvm->as.");
    extend_llvm_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void llvm_gen_wrap_internal(Llvm_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        llvm_gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Llvm_##lower* llvm_unwrap_##lower(Llvm* llvm) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* llvm_wrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* llvm) {\n");

    //    return &llvm->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) llvm;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void llvm_gen_llvm_unwrap(Llvm_type llvm) {
    llvm_gen_unwrap_internal(llvm, false);
    llvm_gen_unwrap_internal(llvm, true);
}

void llvm_gen_llvm_wrap(Llvm_type llvm) {
    llvm_gen_wrap_internal(llvm, false);
    llvm_gen_wrap_internal(llvm, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Node)
static void llvm_gen_print_forward_decl(Llvm_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        llvm_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_llvm_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(llvm) str_view_print(");
    extend_llvm_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(llvm))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_llvm_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* llvm);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void llvm_gen_new_internal(Llvm_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        llvm_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_llvm_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_llvm_name_lower(&function, type.name);
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
        extend_parent_llvm_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_llvm = ");
        extend_parent_llvm_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_llvm->type = ");
        extend_llvm_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    llvm_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_llvm)->pos = pos;\n");
        }

        string_extend_cstr(&gen_a, &function, "    return llvm_unwrap_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(base_llvm);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void llvm_gen_get_pos_internal(Llvm_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        llvm_gen_get_pos_internal(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    llvm_get_pos(const Llvm* llvm)");
    } else {
        string_extend_cstr(&gen_a, &function, "    llvm_get_pos_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(const ");
        extend_llvm_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* llvm)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return llvm->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (llvm->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Llvm_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_llvm_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return llvm_get_pos_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "(llvm_unwrap_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const(llvm));\n");

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

static void gen_llvm_new_forward_decl(Llvm_type llvm) {
    llvm_gen_new_internal(llvm, false);
}

static void gen_llvm_new_define(Llvm_type llvm) {
    llvm_gen_new_internal(llvm, true);
}

static void gen_llvm_get_pos_forward_decl(Llvm_type llvm) {
    llvm_gen_get_pos_internal(llvm, false);
}

static void gen_llvm_get_pos_define(Llvm_type llvm) {
    llvm_gen_get_pos_internal(llvm, true);
}

static void gen_all_llvms(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Llvm_type llvm = llvm_gen_llvm();

    gen_gen("/* autogenerated */\n");
    gen_gen("#include <llvm_hand_written.h>\n");
    gen_gen("#include <expr_ptr_vec.h>\n");
    gen_gen("#include <symbol_table_struct.h>\n");
    gen_gen("#include <symbol_table.h>\n");
    gen_gen("#include <token.h>\n");

    if (implementation) {
        gen_gen("#ifndef LLVM_H\n");
        gen_gen("#define LLVM_H\n");

        llvm_gen_llvm_forward_decl(llvm);
        llvm_gen_llvm_struct(llvm);

        llvm_gen_llvm_unwrap(llvm);
        llvm_gen_llvm_wrap(llvm);

        gen_gen("%s\n", "static inline Llvm* llvm_new(void) {");
        gen_gen("%s\n", "    Llvm* new_llvm = arena_alloc(&a_main, sizeof(*new_llvm));");
        gen_gen("%s\n", "    return new_llvm;");
        gen_gen("%s\n", "}");
    }

    gen_llvm_new_forward_decl(llvm);
    llvm_gen_print_forward_decl(llvm);
    if (implementation) {
        gen_llvm_new_define(llvm);
    }

    gen_llvm_get_pos_forward_decl(llvm);
    if (implementation) {
        gen_llvm_get_pos_define(llvm);
    }

    if (implementation) {
        gen_gen("#endif // LLVM_H\n");
    }

    CLOSE_FILE(global_output);
}


#endif // AUTO_GEN_LLVM_H
