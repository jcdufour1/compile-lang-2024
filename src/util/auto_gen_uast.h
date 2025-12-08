#ifndef AUTO_GEN_UAST_H
#define AUTO_GEN_UAST_H

#include <auto_gen_util.h>
#include <auto_gen_uast_common.h>

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

static void gen_all_uasts(const char* file_path, bool implementation) {
    Uast_type uast = uast_gen_uast();
    gen_uasts_common(file_path, implementation, uast);
}

#endif // AUTO_GEN_UAST_H
