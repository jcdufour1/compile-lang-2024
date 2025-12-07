
#ifndef AUTO_GEN_UAST_H
#define AUTO_GEN_UAST_H

#include <auto_gen_util.h>

struct Uast_type_;
typedef struct Uast_type_ Uast_type;

typedef struct {
    Vec_base info;
    Uast_type* buf;
} Uast_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent;
    Strv base;
} Uast_name;

typedef struct Uast_type_ {
    Uast_name name;
    Members members;
    Uast_type_vec sub_types;
} Uast_type;

static void extend_uast_name_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "UAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "uast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Uast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_uast_name_upper(String* output, Uast_name name) {
    todo();
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "UAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        string_extend_cstr(&gen_a, output, "uast");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        string_extend_cstr(&gen_a, output, "Uast");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Uast_name uast_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Uast_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static Uast_type uast_gen_import_path(const char* prefix) {
    Uast_type import = {.name = uast_name_new(prefix, "import_path", false)};

    append_member(&import.members, "Uast_block*", "block");
    append_member(&import.members, "Strv", "mod_path");

    return import;
}

static Uast_type uast_gen_mod_alias(const char* prefix) {
    Uast_type import = {.name = uast_name_new(prefix, "mod_alias", false)};

    append_member(&import.members, "Name", "name");
    append_member(&import.members, "Strv", "mod_path");
    append_member(&import.members, "Scope_id", "mod_path_scope");

    return import;
}

static Uast_type uast_gen_block(const char* prefix) {
    Uast_type block = {.name = uast_name_new(prefix, "block", false)};

    append_member(&block.members, "Uast_stmt_vec", "children");
    append_member(&block.members, "Pos", "pos_end");
    append_member(&block.members, "Scope_id", "scope_id");

    return block;
}

static Uast_type uast_gen_unary(const char* prefix) {
    Uast_type unary = {.name = uast_name_new(prefix, "unary", false)};

    append_member(&unary.members, "Uast_expr*", "child");
    append_member(&unary.members, "UNARY_TYPE", "token_type");
    append_member(&unary.members, "Ulang_type", "lang_type");

    return unary;
}

static Uast_type uast_gen_binary(const char* prefix) {
    Uast_type binary = {.name = uast_name_new(prefix, "binary", false)};

    append_member(&binary.members, "Uast_expr*", "lhs");
    append_member(&binary.members, "Uast_expr*", "rhs");
    append_member(&binary.members, "BINARY_TYPE", "token_type");

    return binary;
}

static Uast_type uast_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Uast_type operator = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &operator.sub_types, uast_gen_unary(base_name));
    vec_append(&gen_a, &operator.sub_types, uast_gen_binary(base_name));

    return operator;
}

static Uast_type uast_gen_symbol(const char* prefix) {
    Uast_type sym = {.name = uast_name_new(prefix, "symbol", false)};

    append_member(&sym.members, "Name", "name");

    return sym;
}

static Uast_type uast_gen_unknown(const char* prefix) {
    Uast_type access = {.name = uast_name_new(prefix, "unknown", false)};

    return access;
}

static Uast_type uast_gen_switch(const char* prefix) {
    Uast_type lang_if = {.name = uast_name_new(prefix, "switch", false)};

    append_member(&lang_if.members, "Uast_expr*", "operand");
    append_member(&lang_if.members, "Uast_case_vec", "cases");

    return lang_if;
}

static Uast_type uast_gen_member_access(const char* prefix) {
    Uast_type access = {.name = uast_name_new(prefix, "member_access", false)};

    append_member(&access.members, "Uast_symbol*", "member_name");
    append_member(&access.members, "Uast_expr*", "callee");

    return access;
}

static Uast_type uast_gen_index(const char* prefix) {
    Uast_type index = {.name = uast_name_new(prefix, "index", false)};

    append_member(&index.members, "Uast_expr*", "index");
    append_member(&index.members, "Uast_expr*", "callee");

    return index;
}

static Uast_type uast_gen_int(const char* prefix) {
    Uast_type number = {.name = uast_name_new(prefix, "int", false)};

    append_member(&number.members, "int64_t", "data");

    return number;
}

static Uast_type uast_gen_float(const char* prefix) {
    Uast_type number = {.name = uast_name_new(prefix, "float", false)};

    append_member(&number.members, "double", "data");

    return number;
}

static Uast_type uast_gen_string(const char* prefix) {
    Uast_type string = {.name = uast_name_new(prefix, "string", false)};

    append_member(&string.members, "Strv", "data");

    return string;
}

static Uast_type uast_gen_void(const char* prefix) {
    Uast_type lang_void = {.name = uast_name_new(prefix, "void", false)};

    return lang_void;
}

static Uast_type uast_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lit.sub_types, uast_gen_int(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_float(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_string(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_void(base_name));

    return lit;
}

static Uast_type uast_gen_function_call(const char* prefix) {
    Uast_type call = {.name = uast_name_new(prefix, "function_call", false)};

    append_member(&call.members, "Uast_expr_vec", "args");
    append_member(&call.members, "Uast_expr*", "callee");
    append_member(&call.members, "bool", "is_user_generated");

    return call;
}

static Uast_type uast_gen_struct_literal(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "struct_literal", false)};

    append_member(&lit.members, "Uast_expr_vec", "members");

    return lit;
}

static Uast_type uast_gen_array_literal(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "array_literal", false)};

    append_member(&lit.members, "Uast_expr_vec", "members");

    return lit;
}

static Uast_type uast_gen_tuple(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "tuple", false)};

    append_member(&lit.members, "Uast_expr_vec", "members");

    return lit;
}

static Uast_type uast_gen_macro(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "macro", false)};

    append_member(&lit.members, "Strv", "name");
    append_member(&lit.members, "Pos", "value");

    return lit;
}

static Uast_type uast_gen_using(const char* prefix) {
    Uast_type using = {.name = uast_name_new(prefix, "using", false)};

    append_member(&using.members, "Name", "sym_name");
    append_member(&using.members, "Strv", "mod_path_to_put_defs");

    return using;
}

static Uast_type uast_gen_defer(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "defer", false)};

    append_member(&lit.members, "Uast_stmt*", "child");

    return lit;
}

static Uast_type uast_gen_enum_access(const char* prefix) {
    const char* base_name = "enum_access";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast_enum_tag_lit*", "tag");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Uast_expr*", "callee");

    return lit;
}

static Uast_type uast_gen_enum_get_tag(const char* prefix) {
    const char* base_name = "enum_get_tag";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Uast_expr*", "callee");

    return lit;
}

static Uast_type uast_gen_orelse(const char* prefix) {
    const char* base_name = "orelse";
    Uast_type orelse = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&orelse.members, "Uast_expr*", "expr_to_unwrap");
    append_member(&orelse.members, "Uast_block*", "if_error");
    append_member(&orelse.members, "Scope_id", "scope_id");
    append_member(&orelse.members, "Name", "break_out_of");
    append_member(&orelse.members, "bool", "is_error_symbol");
    append_member(&orelse.members, "Uast_symbol*", "error_symbol");

    return orelse;
}

static Uast_type uast_gen_question_mark(const char* prefix) {
    const char* base_name = "question_mark";
    Uast_type mark = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&mark.members, "Uast_expr*", "expr_to_unwrap");
    append_member(&mark.members, "Scope_id", "scope_id");
    append_member(&mark.members, "Name", "break_out_of");

    return mark;
}

static Uast_type uast_gen_underscore(const char* prefix) {
    const char* base_name = "underscore";
    Uast_type underscore = {.name = uast_name_new(prefix, base_name, false)};

    return underscore;
}

static Uast_type uast_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Uast_type fn = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&fn.members, "Ulang_type_fn", "ulang_type");

    return fn;
}

static Uast_type uast_gen_if_else_chain(const char* prefix) {
    Uast_type chain = {.name = uast_name_new(prefix, "if_else_chain", false)};

    append_member(&chain.members, "Uast_if_vec", "uasts");

    return chain;
}

static Uast_type uast_gen_expr_removed(const char* prefix) {
    Uast_type removed = {.name = uast_name_new(prefix, "expr_removed", false)};

    append_member(&removed.members, "Strv", "msg_suffix");

    return removed;
}

static Uast_type uast_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Uast_type expr = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &expr.sub_types, uast_gen_if_else_chain(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_block(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_switch(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_unknown(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_operator(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_symbol(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_member_access(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_index(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_function_call(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_struct_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_array_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_tuple(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_macro(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_enum_access(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_enum_get_tag(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_orelse(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_fn(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_question_mark(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_underscore(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_expr_removed(base_name));

    return expr;
}

static Uast_type uast_gen_struct_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "struct_def", false)};

    append_member(&def.members, "Ustruct_def_base", "base");

    return def;
}

static Uast_type uast_gen_raw_union_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "raw_union_def", false)};

    append_member(&def.members, "Ustruct_def_base", "base");

    return def;
}

static Uast_type uast_gen_function_decl(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "function_decl", false)};

    append_member(&def.members, "Uast_generic_param_vec", "generics");
    append_member(&def.members, "Uast_function_params*", "params");
    append_member(&def.members, "Ulang_type", "return_type");
    append_member(&def.members, "Name", "name");

    return def;
}

static Uast_type uast_gen_builtin_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "builtin_def", false)};

    append_member(&def.members, "Name", "name");

    return def;
}

static Uast_type uast_gen_function_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "function_def", false)};

    append_member(&def.members, "Uast_function_decl*", "decl");
    append_member(&def.members, "Uast_block*", "body");

    return def;
}

static Uast_type uast_gen_variable_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "variable_def", false)};

    append_member(&def.members, "Ulang_type", "lang_type");
    append_member(&def.members, "Name", "name");

    return def;
}

static Uast_type uast_gen_enum_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "enum_def", false)};

    append_member(&def.members, "Ustruct_def_base", "base");

    return def;
}

static Uast_type uast_gen_lang_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "lang_def", false)};

    append_member(&def.members, "Name", "alias_name");
    append_member(&def.members, "Uast_expr*", "expr");
    append_member(&def.members, "bool", "is_from_using");

    return def;
}

static Uast_type uast_gen_primitive_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "primitive_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Uast_type uast_gen_void_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "void_def", false)};

    return def;
}

static Uast_type uast_gen_generic_param(const char* prefix) {
    Uast_type param = {.name = uast_name_new(prefix, "generic_param", false)};

    append_member(&param.members, "Name", "name");
    append_member(&param.members, "bool", "is_expr");
    append_member(&param.members, "Ulang_type", "expr_lang_type");

    return param;
}

static Uast_type uast_gen_poison_def(const char* prefix) {
    const char* base_name = "poison_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Name", "name");

    return def;
}

static Uast_type uast_gen_label(const char* prefix) {
    Uast_type bound = {.name = uast_name_new(prefix, "label", false)};

    append_member(&bound.members, "Name", "name");
    append_member(&bound.members, "Name", "block_scope");

    return bound;
}

static Uast_type uast_gen_def(const char* prefix) {
    const char* base_name = "def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, uast_gen_label(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_void_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_poison_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_import_path(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_mod_alias(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_generic_param(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_function_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_variable_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_struct_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_raw_union_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_enum_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_lang_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_primitive_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_function_decl(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_builtin_def(base_name));

    return def;
}

static Uast_type uast_gen_function_params(const char* prefix) {
    Uast_type params = {.name = uast_name_new(prefix, "function_params", false)};

    append_member(&params.members, "Uast_param_vec", "params");

    return params;
}

static Uast_type uast_gen_for_lower_bound(const char* prefix) {
    Uast_type bound = {.name = uast_name_new(prefix, "for_lower_bound", false)};

    append_member(&bound.members, "Uast_expr*", "child");

    return bound;
}

static Uast_type uast_gen_for_upper_bound(const char* prefix) {
    Uast_type bound = {.name = uast_name_new(prefix, "for_upper_bound", false)};

    append_member(&bound.members, "Uast_expr*", "child");

    return bound;
}

static Uast_type uast_gen_condition(const char* prefix) {
    Uast_type bound = {.name = uast_name_new(prefix, "condition", false)};

    append_member(&bound.members, "Uast_operator*", "child");

    return bound;
}

static Uast_type uast_gen_for_with_cond(const char* prefix) {
    Uast_type for_cond = {.name = uast_name_new(prefix, "for_with_cond", false)};

    append_member(&for_cond.members, "Uast_condition*", "condition");
    append_member(&for_cond.members, "Uast_block*", "body");
    append_member(&for_cond.members, "Name", "continue_label");
    append_member(&for_cond.members, "bool", "do_cont_label");

    return for_cond;
}

static Uast_type uast_gen_yield(const char* prefix) {
    Uast_type yield = {.name = uast_name_new(prefix, "yield", false)};

    append_member(&yield.members, "bool", "do_yield_expr");
    append_member(&yield.members, "Uast_expr*", "yield_expr");
    append_member(&yield.members, "Name", "break_out_of");
    append_member(&yield.members, "bool", "is_user_generated");

    return yield;
}

static Uast_type uast_gen_continue(const char* prefix) {
    Uast_type cont = {.name = uast_name_new(prefix, "continue", false)};

    append_member(&cont.members, "Name", "break_out_of");

    return cont;
}

static Uast_type uast_gen_assignment(const char* prefix) {
    Uast_type assign = {.name = uast_name_new(prefix, "assignment", false)};

    append_member(&assign.members, "Uast_expr*", "lhs");
    append_member(&assign.members, "Uast_expr*", "rhs");

    return assign;
}

static Uast_type uast_gen_if(const char* prefix) {
    Uast_type lang_if = {.name = uast_name_new(prefix, "if", false)};

    append_member(&lang_if.members, "Uast_condition*", "condition");
    append_member(&lang_if.members, "Uast_block*", "body");

    return lang_if;
}

static Uast_type uast_gen_case(const char* prefix) {
    Uast_type lang_case = {.name = uast_name_new(prefix, "case", false)};

    append_member(&lang_case.members, "bool", "is_default");
    append_member(&lang_case.members, "Uast_expr*", "expr");
    append_member(&lang_case.members, "Uast_block*", "if_true");
    append_member(&lang_case.members, "Scope_id", "scope_id");

    return lang_case;
}

static Uast_type uast_gen_param(const char* prefix) {
    Uast_type lang_if = {.name = uast_name_new(prefix, "param", false)};

    append_member(&lang_if.members, "Uast_variable_def*", "base");
    append_member(&lang_if.members, "bool", "is_optional");
    append_member(&lang_if.members, "bool", "is_variadic");
    append_member(&lang_if.members, "Uast_expr*", "optional_default");

    return lang_if;
}

static Uast_type uast_gen_return(const char* prefix) {
    Uast_type rtn = {.name = uast_name_new(prefix, "return", false)};

    append_member(&rtn.members, "Uast_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted");

    return rtn;
}

static Uast_type uast_gen_stmt_removed(const char* prefix) {
    Uast_type removed = {.name = uast_name_new(prefix, "stmt_removed", false)};

    return removed;
}

static Uast_type uast_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Uast_type stmt = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &stmt.sub_types, uast_gen_defer(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_using(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_expr(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_def(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_for_with_cond(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_yield(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_continue(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_assignment(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_return(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_stmt_removed(base_name));

    return stmt;
}

static Uast_type uast_gen_uast(void) {
    const char* base_name = "uast";
    Uast_type uast = {.name = uast_name_new("uast", "", true)};

    vec_append(&gen_a, &uast.sub_types, uast_gen_stmt(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_function_params(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_for_lower_bound(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_for_upper_bound(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_condition(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_if(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_case(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_param(base_name));

    return uast;
}

static void uast_gen_uast_forward_decl(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_forward_decl(vec_at(uast.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void uast_gen_uast_struct_as(String* output, Uast_type uast) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_uast_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void uast_gen_uast_struct_enum(String* output, Uast_type uast) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void uast_gen_uast_struct(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_struct(vec_at(uast.sub_types, idx));
    }

    if (uast.sub_types.info.count > 0) {
        uast_gen_uast_struct_as(&output, uast);
        uast_gen_uast_struct_enum(&output, uast);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (uast.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_uast_name_first_upper(&as_member_type, uast.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_uast_name_upper(&enum_member_type, uast.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < uast.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(uast.members, idx));
    }

    if (uast.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void uast_gen_internal_unwrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast__unwrap##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast) {\n");

    //    unwrap(uast->type == upper); 
    string_extend_cstr(&gen_a, &function, "    unwrap(uast->type == ");
    extend_uast_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &uast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &uast->as.");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void uast_gen_internal_wrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_wrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast__unwrap##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast) {\n");

    //    return &uast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) uast;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

void uast_gen_uast_unwrap(Uast_type uast) {
    uast_gen_internal_unwrap(uast, false);
    uast_gen_internal_unwrap(uast, true);
}

void uast_gen_uast_wrap(Uast_type uast) {
    uast_gen_internal_wrap(uast, false);
    uast_gen_internal_wrap(uast, true);
}

static void uast_gen_print_overloading(Uast_type_vec types) {
    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define uast_print(uast) strv_print(uast_print_internal_(uast, 0))\n");

    string_extend_cstr(&gen_a, &function, "#define uast_print_internal_(uast, indent) _Generic ((uast), \\\n");

    vec_foreach(idx, Uast_type, type, types) {
        string_extend_cstr(&gen_a, &function, "    ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_uast_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal%s \\\n", idx + 1 < types.info.count ? "," : "");
    }

    string_extend_cstr(&gen_a, &function, ") (uast, indent)\n");

    gen_gen(FMT"\n", string_print(function));
}

// TODO: deduplicate these functions (use same function for Ir and Uast)
static void uast_gen_print_forward_decl(Uast_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_print_forward_decl(vec_at(type.sub_types, idx));
    }
 
    if (type.name.is_topmost) {
        return;
    }
 
    String function = {0};
 
    string_extend_cstr(&gen_a, &function, "Strv ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast, Indent indent);");
 
    gen_gen(FMT"\n", string_print(function));

    //gen_gen("#define uast_print(uast) strv_print(uast_print_internal(uast, 0))\n");
}

static void uast_gen_new_internal(Uast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_new_internal(vec_at(type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(");

    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    } else {
        string_extend_cstr(&gen_a, &function, "Pos pos");
    }
    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        if (idx < type.members.info.count) {
            string_extend_cstr(&gen_a, &function, ", ");
        }

        Member curr = vec_at(type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        extend_parent_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_uast = ");
        extend_parent_uast_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_uast->type = ");
        extend_uast_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    uast_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, "    uast_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return uast_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap");
        string_extend_cstr(&gen_a, &function, "(base_uast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void uast_gen_internal_get_pos(Uast_type type, bool implementation, bool is_ref) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_get_pos(vec_at(type.sub_types, idx), implementation, is_ref);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");
    if (is_ref) {
        string_extend_cstr(&gen_a, &function, "*");
    }

    if (type.name.is_topmost) {
        if (is_ref) {
            string_extend_cstr(&gen_a, &function, "    uast_get_pos_ref(Uast* uast)");
        } else {
            string_extend_cstr(&gen_a, &function, "    uast_get_pos(const Uast* uast)");
        }
    } else {
        string_extend_cstr(&gen_a, &function, "    uast_");
        extend_strv_lower(&function, type.name.base);
        if (is_ref) {
            string_extend_cstr(&gen_a, &function, "_get_pos_ref(");
        } else {
            string_extend_cstr(&gen_a, &function, "_get_pos(const ");
        }
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* uast)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            if (is_ref) {
                string_extend_cstr(&gen_a, &function, "    return &uast->pos;\n");
            } else {
                string_extend_cstr(&gen_a, &function, "    return uast->pos;\n");
            }
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (uast->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Uast_type curr = vec_at(type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_uast_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return uast_");
                extend_strv_lower(&function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_get_pos_ref(uast_");
                } else {
                    string_extend_cstr(&gen_a, &function, "_get_pos(uast_");
                }
                extend_strv_lower(&function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_unwrap(uast));\n");
                } else {
                    string_extend_cstr(&gen_a, &function, "_const_unwrap(uast));\n");
                }

                string_extend_cstr(&gen_a, &function, "        break;\n");
            }

            string_extend_cstr(&gen_a, &function, "    }\n");
        }

        string_extend_cstr(&gen_a, &function, "unreachable(\"\");\n");
        string_extend_cstr(&gen_a, &function, "}\n");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_uast_new_forward_decl(Uast_type uast) {
    uast_gen_new_internal(uast, false);
}

static void gen_uast_new_define(Uast_type uast) {
    uast_gen_new_internal(uast, true);
}

static void gen_uast_forward_decl_get_pos(Uast_type uast) {
    uast_gen_internal_get_pos(uast, false, false);
    uast_gen_internal_get_pos(uast, false, true);
}

static void gen_uast_define_get_pos(Uast_type uast) {
    uast_gen_internal_get_pos(uast, true, false);
    uast_gen_internal_get_pos(uast, true, true);
}

static void gen_uast_vecs(Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        gen_uast_vecs(vec_at(uast.sub_types, idx));
    }

    String vec_name = {0};
    extend_uast_name_first_upper(&vec_name, uast.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_uast_name_first_upper(&item_name, uast.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void uast_get_type_vec_internal(Uast_type_vec* type_vec, Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_get_type_vec_internal(type_vec, vec_at(uast.sub_types, idx));
    }

    vec_append(&gen_a, type_vec, uast);
}

static Uast_type_vec uast_get_type_vec(Uast_type uast) {
    Uast_type_vec type_vec = {0};
    uast_get_type_vec_internal(&type_vec, uast);
    return type_vec;
}

static void gen_all_uasts(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Uast_type uast = uast_gen_uast();
    Uast_type_vec type_vec = uast_get_type_vec(uast);

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef UAST_H\n");
        gen_gen("#define UAST_H\n");

        gen_gen("#include <uast_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
        gen_gen("#include <ulang_type.h>\n");
    } else {
        gen_gen("#ifndef UAST_FORWARD_DECL_H\n");
        gen_gen("#define UAST_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    uast_gen_uast_forward_decl(uast);

    if (!implementation) {
        gen_uast_vecs(uast);
    }

    if (implementation) {
        uast_gen_uast_struct(uast);

        uast_gen_uast_unwrap(uast);
        uast_gen_uast_wrap(uast);

        gen_gen("%s\n", "static inline Uast* uast_new(void) {");
        gen_gen("%s\n", "    Uast* new_uast = arena_alloc(&a_main, sizeof(*new_uast));");
        gen_gen("%s\n", "    return new_uast;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_uast_new_forward_decl(uast);
    }
    uast_gen_print_forward_decl(uast);
    if (!implementation) {
        uast_gen_print_overloading(type_vec);
    }
    if (implementation) {
        gen_uast_new_define(uast);
    }

    gen_uast_forward_decl_get_pos(uast);
    if (implementation) {
        gen_uast_define_get_pos(uast);
    }

    if (implementation) {
        gen_gen("#endif // UAST_H\n");
    } else {
        gen_gen("#endif // UAST_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_UAST_H
