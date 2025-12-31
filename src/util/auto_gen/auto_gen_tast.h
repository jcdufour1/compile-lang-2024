
#ifndef AUTO_GEN_TAST_H
#define AUTO_GEN_TAST_H

#include <auto_gen_util.h>
#include <auto_gen_uast_common.h>

static Uast_type tast_gen_block(const char* prefix) {
    const char* base_name = "block";
    Uast_type block = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&block.members, "Tast_stmt_darr", "children");
    append_member(&block.members, "Pos", "pos_end");
    append_member(&block.members, "Lang_type", "lang_type");
    append_member(&block.members, "Scope_id", "scope_id");
    append_member(&block.members, "bool", "has_defer");
    append_member(&block.members, "bool", "has_yield");
    append_member(&block.members, "bool", "has_continue");

    return block;
}

static Uast_type tast_gen_unary(const char* prefix) {
    const char* base_name = "unary";
    Uast_type unary = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&unary.members, "Tast_expr*", "child");
    append_member(&unary.members, "UNARY_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");

    return unary;
}

static Uast_type tast_gen_binary(const char* prefix) {
    const char* base_name = "binary";
    Uast_type binary = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&binary.members, "Tast_expr*", "lhs");
    append_member(&binary.members, "Tast_expr*", "rhs");
    append_member(&binary.members, "BINARY_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");

    return binary;
}

static Uast_type tast_gen_primitive_sym(const char* prefix) {
    const char* base_name = "primitive_sym";
    Uast_type primitive = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Uast_type tast_gen_enum_sym(const char* prefix) {
    const char* base_name = "enum_sym";
    Uast_type lang_enum = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Uast_type tast_gen_struct_sym(const char* prefix) {
    const char* base_name = "struct_sym";
    Uast_type lang_struct = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Uast_type tast_gen_tuple_sym(const char* prefix) {
    const char* base_name = "tuple_sym";
    Uast_type lang_tuple = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_tuple.members, "Sym_typed_base", "base");

    return lang_tuple;
}

static Uast_type tast_gen_raw_union_sym(const char* prefix) {
    const char* base_name = "raw_union_sym";
    Uast_type raw_union = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Uast_type tast_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Uast_type operator = {.name = uast_name_new(prefix, base_name, false, "tast")};

    darr_append(&a_gen, &operator.sub_types, tast_gen_unary(base_name));
    darr_append(&a_gen, &operator.sub_types, tast_gen_binary(base_name));

    return operator;
}

static Uast_type tast_gen_symbol(const char* prefix) {
    const char* base_name = "symbol";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&sym.members, "Sym_typed_base", "base");

    return sym;
}

static Uast_type tast_gen_member_access(const char* prefix) {
    const char* base_name = "member_access";
    Uast_type access = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Strv", "member_name");
    append_member(&access.members, "Tast_expr*", "callee");

    return access;
}

static Uast_type tast_gen_index(const char* prefix) {
    const char* base_name = "index";
    Uast_type index = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Tast_expr*", "index");
    append_member(&index.members, "Tast_expr*", "callee");

    return index;
}

static Uast_type tast_gen_function_lit(const char* prefix) {
    const char* base_name = "function_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lit.members, "Name", "name");
    append_member(&lit.members, "Lang_type", "lang_type");

    return lit;
}

static Uast_type tast_gen_int(const char* prefix) {
    const char* base_name = "int";
    Uast_type number = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Uast_type tast_gen_float(const char* prefix) {
    const char* base_name = "float";
    Uast_type number = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&number.members, "double", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Uast_type tast_gen_string(const char* prefix) {
    const char* base_name = "string";
    Uast_type string = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&string.members, "Strv", "data");
    append_member(&string.members, "bool", "is_cstr");

    return string;
}

static Uast_type tast_gen_raw_union_lit(const char* prefix) {
    const char* base_name = "raw_union_lit";
    Uast_type raw_union_lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&raw_union_lit.members, "Tast_enum_tag_lit*", "tag");
    append_member(&raw_union_lit.members, "Lang_type", "lang_type");
    append_member(&raw_union_lit.members, "Tast_expr*", "item");

    return raw_union_lit;
}

static Uast_type tast_gen_void(const char* prefix) {
    const char* base_name = "void";
    Uast_type lang_void = {.name = uast_name_new(prefix, base_name, false, "tast")};

    return lang_void;
}

static Uast_type tast_gen_enum_tag_lit(const char* prefix) {
    const char* base_name = "enum_tag_lit";
    Uast_type enum_tag_lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&enum_tag_lit.members, "int64_t", "data");
    append_member(&enum_tag_lit.members, "Lang_type", "lang_type");

    return enum_tag_lit;
}

static Uast_type tast_gen_enum_lit(const char* prefix) {
    const char* base_name = "enum_lit";
    Uast_type enum_lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&enum_lit.members, "Tast_enum_tag_lit*", "tag");
    append_member(&enum_lit.members, "Tast_expr*", "item");
    append_member(&enum_lit.members, "Lang_type", "enum_lang_type");

    return enum_lit;
}

static Uast_type tast_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    darr_append(&a_gen, &lit.sub_types, tast_gen_function_lit(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_int(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_float(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_string(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_void(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_enum_tag_lit(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_enum_lit(base_name));
    darr_append(&a_gen, &lit.sub_types, tast_gen_raw_union_lit(base_name));

    return lit;
}

static Uast_type tast_gen_function_call(const char* prefix) {
    const char* base_name = "function_call";
    Uast_type call = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&call.members, "Tast_expr_darr", "args");
    append_member(&call.members, "Tast_expr*", "callee");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Uast_type tast_gen_struct_literal(const char* prefix) {
    const char* base_name = "struct_literal";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lit.members, "Tast_expr_darr", "members");
    append_member(&lit.members, "Name", "name");
    append_member(&lit.members, "Lang_type", "lang_type");

    return lit;
}

static Uast_type tast_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lit.members, "Tast_expr_darr", "members");
    append_member(&lit.members, "Lang_type_tuple", "lang_type");

    return lit;
}

static Uast_type tast_gen_enum_callee(const char* prefix) {
    const char* base_name = "enum_callee";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lit.members, "Tast_enum_tag_lit*", "tag");
    append_member(&lit.members, "Lang_type", "enum_lang_type");

    return lit;
}

static Uast_type tast_gen_enum_case(const char* prefix) {
    const char* base_name = "enum_case";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lit.members, "Tast_enum_tag_lit*", "tag");
    append_member(&lit.members, "Lang_type", "enum_lang_type");

    return lit;
}

static Uast_type tast_gen_enum_access(const char* prefix) {
    const char* base_name = "enum_access";
    Uast_type access = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&access.members, "Tast_enum_tag_lit*", "tag");
    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Tast_expr*", "callee");

    return access;
}

static Uast_type tast_gen_enum_get_tag(const char* prefix) {
    const char* base_name = "enum_get_tag";
    Uast_type access = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&access.members, "Tast_expr*", "callee");

    return access;
}

static Uast_type tast_gen_assignment(const char* prefix) {
    const char* base_name = "assignment";
    Uast_type assign = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&assign.members, "Tast_expr*", "lhs");
    append_member(&assign.members, "Tast_expr*", "rhs");

    return assign;
}

static Uast_type tast_gen_if_else_chain(const char* prefix) {
    const char* base_name = "if_else_chain";
    Uast_type chain = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&chain.members, "Tast_if_darr", "tasts");
    append_member(&chain.members, "bool", "is_switch");

    return chain;
}

static Uast_type tast_gen_module_alias(const char* prefix) {
    const char* base_name = "module_alias";
    Uast_type chain = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&chain.members, "Name", "alias_name");
    append_member(&chain.members, "Strv", "mod_path");

    return chain;
}

static Uast_type tast_gen_import_path(const char* prefix) {
    Uast_type import = {.name = uast_name_new(prefix, "import_path", false, "tast")};

    append_member(&import.members, "Tast_block*", "block");
    append_member(&import.members, "Strv", "mod_path");

    return import;
}

static Uast_type tast_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Uast_type expr = {.name = uast_name_new(prefix, base_name, false, "tast")};

    darr_append(&a_gen, &expr.sub_types, tast_gen_block(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_module_alias(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_if_else_chain(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_assignment(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_operator(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_symbol(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_member_access(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_index(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_literal(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_function_call(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_struct_literal(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_tuple(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_enum_callee(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_enum_case(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_enum_get_tag(base_name));
    darr_append(&a_gen, &expr.sub_types, tast_gen_enum_access(base_name));

    return expr;
}

static Uast_type tast_gen_struct_def(const char* prefix) {
    const char* base_name = "struct_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Uast_type tast_gen_raw_union_def(const char* prefix) {
    const char* base_name = "raw_union_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Uast_type tast_gen_function_decl(const char* prefix) {
    const char* base_name = "function_decl";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Tast_function_params*", "params");
    append_member(&def.members, "Lang_type", "return_type");
    append_member(&def.members, "Name", "name");

    return def;
}

static Uast_type tast_gen_function_def(const char* prefix) {
    const char* base_name = "function_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Tast_function_decl*", "decl");
    append_member(&def.members, "Tast_block*", "body");

    return def;
}

static Uast_type tast_gen_variable_def(const char* prefix) {
    const char* base_name = "variable_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Name", "name");
    append_member(&def.members, "Attrs", "attrs");

    return def;
}

static Uast_type tast_gen_enum_def(const char* prefix) {
    const char* base_name = "enum_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Uast_type tast_gen_array_def(const char* prefix) {
    const char* base_name = "array_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Lang_type", "item_type");
    append_member(&def.members, "int64_t", "count");

    return def;
}

static Uast_type tast_gen_primitive_def(const char* prefix) {
    const char* base_name = "primitive_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Uast_type tast_gen_label(const char* prefix) {
    const char* base_name = "label";
    Uast_type lang_label = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_label.members, "Name", "name");
    append_member(&lang_label.members, "Name", "block_scope");
    append_member(&lang_label.members, "bool", "has_continue");

    return lang_label;
}

static Uast_type tast_gen_def(const char* prefix) {
    const char* base_name = "def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false, "tast")};

    darr_append(&a_gen, &def.sub_types, tast_gen_label(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_import_path(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_function_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_variable_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_struct_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_raw_union_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_enum_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_array_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_primitive_def(base_name));
    darr_append(&a_gen, &def.sub_types, tast_gen_function_decl(base_name));

    return def;
}

static Uast_type tast_gen_function_params(const char* prefix) {
    const char* base_name = "function_params";
    Uast_type params = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&params.members, "Tast_variable_def_darr", "params");

    return params;
}

static Uast_type tast_gen_condition(const char* prefix) {
    const char* base_name = "condition";
    Uast_type bound = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&bound.members, "Tast_operator*", "child");

    return bound;
}

static Uast_type tast_gen_for_with_cond(const char* prefix) {
    const char* base_name = "for_with_cond";
    Uast_type for_cond = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&for_cond.members, "Tast_condition*", "condition");
    append_member(&for_cond.members, "Tast_block*", "body");
    append_member(&for_cond.members, "Name", "continue_label");
    append_member(&for_cond.members, "bool", "do_cont_label");

    return for_cond;
}

static Uast_type tast_gen_defer(const char* prefix) {
    const char* base_name = "defer";
    Uast_type defer = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&defer.members, "Tast_stmt*", "child");

    return defer;
}

static Uast_type tast_gen_actual_break(const char* prefix) {
    const char* base_name = "actual_break";
    Uast_type lang_break = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_break.members, "bool", "do_break_expr");
    append_member(&lang_break.members, "Tast_expr*", "break_expr");

    return lang_break;
}

static Uast_type tast_gen_yield(const char* prefix) {
    const char* base_name = "yield";
    Uast_type yield = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&yield.members, "bool", "do_yield_expr");
    append_member(&yield.members, "Tast_expr*", "yield_expr");
    append_member(&yield.members, "Name", "break_out_of");

    return yield;
}

static Uast_type tast_gen_continue(const char* prefix) {
    const char* base_name = "continue";
    Uast_type cont = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&cont.members, "Name", "break_out_of");

    return cont;
}

static Uast_type tast_gen_if(const char* prefix) {
    const char* base_name = "if";
    Uast_type lang_if = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&lang_if.members, "Tast_condition*", "condition");
    append_member(&lang_if.members, "Tast_block*", "body");
    append_member(&lang_if.members, "Lang_type", "yield_type");

    return lang_if;
}

static Uast_type tast_gen_return(const char* prefix) {
    const char* base_name = "return";
    Uast_type rtn = {.name = uast_name_new(prefix, base_name, false, "tast")};

    append_member(&rtn.members, "Tast_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted");

    return rtn;
}

static Uast_type tast_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Uast_type stmt = {.name = uast_name_new(prefix, base_name, false, "tast")};

    darr_append(&a_gen, &stmt.sub_types, tast_gen_defer(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_expr(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_for_with_cond(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_return(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_actual_break(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_yield(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_continue(base_name));
    darr_append(&a_gen, &stmt.sub_types, tast_gen_def(base_name));

    return stmt;
}

static Uast_type tast_gen_tast(void) {
    const char* base_name = "tast";
    Uast_type tast = {.name = uast_name_new(base_name, "", true, "tast")};

    darr_append(&a_gen, &tast.sub_types, tast_gen_stmt(base_name));
    darr_append(&a_gen, &tast.sub_types, tast_gen_function_params(base_name));
    darr_append(&a_gen, &tast.sub_types, tast_gen_condition(base_name));
    darr_append(&a_gen, &tast.sub_types, tast_gen_if(base_name));

    return tast;
}

static void gen_all_tasts(const char* file_path, bool implementation) {
    Uast_type tast = tast_gen_tast();
    unwrap(tast.name.type.count > 0);
    gen_uasts_common(file_path, implementation, tast);
}

#endif // AUTO_GEN_TAST_H
