
#ifndef AUTO_GEN_TAST_H
#define AUTO_GEN_TAST_H

#include <auto_gen_util.h>

struct Tast_type_;
typedef struct Tast_type_ Tast_type;

typedef struct {
    Vec_base info;
    Tast_type* buf;
} Tast_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Tast_name;

typedef struct Tast_type_ {
    Tast_name name;
    Members members;
    Tast_type_vec sub_types;
} Tast_type;

static void extend_tast_name_upper(String* output, Tast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "TAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_tast_name_lower(String* output, Tast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "tast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_tast_name_first_upper(String* output, Tast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Tast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_tast_name_upper(String* output, Tast_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "TAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_tast_name_lower(String* output, Tast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        string_extend_cstr(&gen_a, output, "tast");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "tast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_tast_name_first_upper(String* output, Tast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast")) {
        string_extend_cstr(&gen_a, output, "Tast");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Tast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Tast_name tast_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Tast_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Tast_type tast_gen_block(const char* prefix) {
    const char* base_name = "block";
    Tast_type block = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Tast_stmt_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Tast_type tast_gen_unary(const char* prefix) {
    const char* base_name = "unary";
    Tast_type unary = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&unary.members, "Tast_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");

    return unary;
}

static Tast_type tast_gen_binary(const char* prefix) {
    const char* base_name = "binary";
    Tast_type binary = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&binary.members, "Tast_expr*", "lhs");
    append_member(&binary.members, "Tast_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");

    return binary;
}

static Tast_type tast_gen_primitive_sym(const char* prefix) {
    const char* base_name = "primitive_sym";
    Tast_type primitive = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Tast_type tast_gen_sum_sym(const char* prefix) {
    const char* base_name = "sum_sym";
    Tast_type sum = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&sum.members, "Sym_typed_base", "base");

    return sum;
}

static Tast_type tast_gen_enum_sym(const char* prefix) {
    const char* base_name = "enum_sym";
    Tast_type lang_enum = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Tast_type tast_gen_struct_sym(const char* prefix) {
    const char* base_name = "struct_sym";
    Tast_type lang_struct = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Tast_type tast_gen_tuple_sym(const char* prefix) {
    const char* base_name = "tuple_sym";
    Tast_type lang_tuple = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_tuple.members, "Sym_typed_base", "base");

    return lang_tuple;
}

static Tast_type tast_gen_raw_union_sym(const char* prefix) {
    const char* base_name = "raw_union_sym";
    Tast_type raw_union = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Tast_type tast_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Tast_type operator = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &operator.sub_types, tast_gen_unary(base_name));
    vec_append(&gen_a, &operator.sub_types, tast_gen_binary(base_name));

    return operator;
}

static Tast_type tast_gen_symbol_typed(const char* prefix) {
    const char* base_name = "symbol_typed";
    Tast_type sym = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &sym.sub_types, tast_gen_primitive_sym(base_name));
    vec_append(&gen_a, &sym.sub_types, tast_gen_struct_sym(base_name));
    vec_append(&gen_a, &sym.sub_types, tast_gen_raw_union_sym(base_name));
    vec_append(&gen_a, &sym.sub_types, tast_gen_enum_sym(base_name));
    vec_append(&gen_a, &sym.sub_types, tast_gen_sum_sym(base_name));

    return sym;
}

static Tast_type tast_gen_member_access_typed(const char* prefix) {
    const char* base_name = "member_access_typed";
    Tast_type access = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Tast_expr*", "callee");

    return access;
}

static Tast_type tast_gen_index_typed(const char* prefix) {
    const char* base_name = "index_typed";
    Tast_type index = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Tast_expr*", "index");
    append_member(&index.members, "Tast_expr*", "callee");

    return index;
}

static Tast_type tast_gen_number(const char* prefix) {
    const char* base_name = "number";
    Tast_type number = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Tast_type tast_gen_string(const char* prefix) {
    const char* base_name = "string";
    Tast_type string = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Tast_type tast_gen_char(const char* prefix) {
    const char* base_name = "char";
    Tast_type lang_char = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Tast_type tast_gen_void(const char* prefix) {
    const char* base_name = "void";
    Tast_type lang_void = {.name = tast_name_new(prefix, base_name, false)};

    return lang_void;
}

static Tast_type tast_gen_enum_lit(const char* prefix) {
    const char* base_name = "enum_lit";
    Tast_type enum_lit = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Tast_type tast_gen_sum_lit(const char* prefix) {
    const char* base_name = "sum_lit";
    Tast_type sum_lit = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&sum_lit.members, "Tast_enum_lit*", "tag");
    append_member(&sum_lit.members, "Tast_expr*", "item");
    append_member(&sum_lit.members, "Lang_type", "lang_type");

    return sum_lit;
}

static Tast_type tast_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Tast_type lit = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lit.sub_types, tast_gen_number(base_name));
    vec_append(&gen_a, &lit.sub_types, tast_gen_string(base_name));
    vec_append(&gen_a, &lit.sub_types, tast_gen_void(base_name));
    vec_append(&gen_a, &lit.sub_types, tast_gen_enum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast_gen_sum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast_gen_char(base_name));

    return lit;
}

static Tast_type tast_gen_function_call(const char* prefix) {
    const char* base_name = "function_call";
    Tast_type call = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&call.members, "Tast_expr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Lang_type_vec", "lang_type");

    return call;
}

static Tast_type tast_gen_struct_literal(const char* prefix) {
    const char* base_name = "struct_literal";
    Tast_type lit = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast_expr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");

    return lit;
}

static Tast_type tast_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Tast_type lit = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast_expr_vec", "members");
    append_member(&lit.members, "Lang_type_vec", "lang_type");

    return lit;
}

static Tast_type tast_gen_sum_callee(const char* prefix) {
    const char* base_name = "sum_callee";
    Tast_type lit = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "sum_lang_type");

    return lit;
}

static Tast_type tast_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Tast_type expr = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &expr.sub_types, tast_gen_operator(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_symbol_typed(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_member_access_typed(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_index_typed(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_function_call(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_struct_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_tuple(base_name));
    vec_append(&gen_a, &expr.sub_types, tast_gen_sum_callee(base_name));

    return expr;
}

static Tast_type tast_gen_struct_def(const char* prefix) {
    const char* base_name = "struct_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast_type tast_gen_raw_union_def(const char* prefix) {
    const char* base_name = "raw_union_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast_type tast_gen_function_decl(const char* prefix) {
    const char* base_name = "function_decl";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast_function_params*", "params");
    append_member(&def.members, "Tast_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast_type tast_gen_function_def(const char* prefix) {
    const char* base_name = "function_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast_function_decl*", "decl");
    append_member(&def.members, "Tast_block*", "body");

    return def;
}

static Tast_type tast_gen_variable_def(const char* prefix) {
    const char* base_name = "variable_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast_type tast_gen_enum_def(const char* prefix) {
    const char* base_name = "enum_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast_type tast_gen_sum_def(const char* prefix) {
    const char* base_name = "sum_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast_type tast_gen_primitive_def(const char* prefix) {
    const char* base_name = "primitive_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast_type tast_gen_string_def(const char* prefix) {
    const char* base_name = "string_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Tast_type tast_gen_struct_lit_def(const char* prefix) {
    const char* base_name = "struct_lit_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast_expr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast_type tast_gen_literal_def(const char* prefix) {
    const char* base_name = "literal_def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast_gen_string_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_struct_lit_def(base_name));

    return def;
}

static Tast_type tast_gen_def(const char* prefix) {
    const char* base_name = "def";
    Tast_type def = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast_gen_function_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_variable_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_struct_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_raw_union_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_enum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_sum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_primitive_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_function_decl(base_name));
    vec_append(&gen_a, &def.sub_types, tast_gen_literal_def(base_name));

    return def;
}

static Tast_type tast_gen_function_params(const char* prefix) {
    const char* base_name = "function_params";
    Tast_type params = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&params.members, "Tast_variable_def_vec", "params");

    return params;
}

static Tast_type tast_gen_lang_type(const char* prefix) {
    const char* base_name = "lang_type";
    Tast_type lang_type = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_type.members, "Lang_type_vec", "lang_type");

    return lang_type;
}

static Tast_type tast_gen_for_lower_bound(const char* prefix) {
    const char* base_name = "for_lower_bound";
    Tast_type bound = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast_expr*", "child");

    return bound;
}

static Tast_type tast_gen_for_upper_bound(const char* prefix) {
    const char* base_name = "for_upper_bound";
    Tast_type bound = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast_expr*", "child");

    return bound;
}

static Tast_type tast_gen_condition(const char* prefix) {
    const char* base_name = "condition";
    Tast_type bound = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast_operator*", "child");

    return bound;
}

static Tast_type tast_gen_for_range(const char* prefix) {
    const char* base_name = "for_range";
    Tast_type range = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&range.members, "Tast_variable_def*", "var_def");
    append_member(&range.members, "Tast_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Tast_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Tast_block*", "body");

    return range;
}

static Tast_type tast_gen_for_with_cond(const char* prefix) {
    const char* base_name = "for_with_cond";
    Tast_type for_cond = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&for_cond.members, "Tast_condition*", "condition");
    append_member(&for_cond.members, "Tast_block*", "body");

    return for_cond;
}

static Tast_type tast_gen_break(const char* prefix) {
    const char* base_name = "break";
    Tast_type lang_break = {.name = tast_name_new(prefix, base_name, false)};

    return lang_break;
}

static Tast_type tast_gen_continue(const char* prefix) {
    const char* base_name = "continue";
    Tast_type lang_cont = {.name = tast_name_new(prefix, base_name, false)};

    return lang_cont;
}

static Tast_type tast_gen_assignment(const char* prefix) {
    const char* base_name = "assignment";
    Tast_type assign = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&assign.members, "Tast_stmt*", "lhs");
    append_member(&assign.members, "Tast_expr*", "rhs");

    return assign;
}

static Tast_type tast_gen_if(const char* prefix) {
    const char* base_name = "if";
    Tast_type lang_if = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&lang_if.members, "Tast_condition*", "condition");
    append_member(&lang_if.members, "Tast_block*", "body");

    return lang_if;
}

static Tast_type tast_gen_return(const char* prefix) {
    const char* base_name = "return";
    Tast_type rtn = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&rtn.members, "Tast_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Tast_type tast_gen_if_else_chain(const char* prefix) {
    const char* base_name = "if_else_chain";
    Tast_type chain = {.name = tast_name_new(prefix, base_name, false)};

    append_member(&chain.members, "Tast_if_vec", "tasts");

    return chain;
}

static Tast_type tast_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Tast_type stmt = {.name = tast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &stmt.sub_types, tast_gen_block(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_expr(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_for_range(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_for_with_cond(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_if_else_chain(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_return(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_break(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_continue(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_assignment(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast_gen_def(base_name));

    return stmt;
}

static Tast_type tast_gen_tast(void) {
    const char* base_name = "tast";
    Tast_type tast = {.name = tast_name_new(base_name, "", true)};

    vec_append(&gen_a, &tast.sub_types, tast_gen_stmt(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_function_params(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_lang_type(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_for_lower_bound(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_for_upper_bound(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_condition(base_name));
    vec_append(&gen_a, &tast.sub_types, tast_gen_if(base_name));

    return tast;
}

static void tast_gen_tast_forward_decl(Tast_type tast) {
    String output = {0};

    for (size_t idx = 0; idx < tast.sub_types.info.count; idx++) {
        tast_gen_tast_forward_decl(vec_at(&tast.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_tast_name_first_upper(&output, tast.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast_name_first_upper(&output, tast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_tast_name_first_upper(&output, tast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast_gen_tast_struct_as(String* output, Tast_type tast) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_tast_name_first_upper(output, tast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast.sub_types.info.count; idx++) {
        Tast_type curr = vec_at(&tast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_tast_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast_name_first_upper(output, tast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast_gen_tast_struct_enum(String* output, Tast_type tast) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_tast_name_upper(output, tast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast.sub_types.info.count; idx++) {
        Tast_type curr = vec_at(&tast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast_name_upper(output, tast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast_gen_tast_struct(Tast_type tast) {
    String output = {0};

    for (size_t idx = 0; idx < tast.sub_types.info.count; idx++) {
        tast_gen_tast_struct(vec_at(&tast.sub_types, idx));
    }

    if (tast.sub_types.info.count > 0) {
        tast_gen_tast_struct_as(&output, tast);
        tast_gen_tast_struct_enum(&output, tast);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast_name_first_upper(&output, tast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (tast.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_tast_name_first_upper(&as_member_type, tast.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_tast_name_upper(&enum_member_type, tast.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < tast.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&tast.members, idx));
    }

    if (tast.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_tast_name_first_upper(&output, tast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast_gen_unwrap_internal(Tast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast_gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast_##lower* tast_unwrap_##lower(Tast* tast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast_unwrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast) {\n");

    //    try(tast->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(tast->type == ");
    extend_tast_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &tast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &tast->as.");
    extend_tast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast_gen_wrap_internal(Tast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast_gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast_##lower* tast_unwrap_##lower(Tast* tast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast_wrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast) {\n");

    //    return &tast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) tast;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void tast_gen_tast_unwrap(Tast_type tast) {
    tast_gen_unwrap_internal(tast, false);
    tast_gen_unwrap_internal(tast, true);
}

void tast_gen_tast_wrap(Tast_type tast) {
    tast_gen_wrap_internal(tast, false);
    tast_gen_wrap_internal(tast, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Tast)
static void tast_gen_print_forward_decl(Tast_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_tast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(tast) str_view_print(");
    extend_tast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(tast, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_tast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void tast_gen_new_internal(Tast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_tast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_tast_name_lower(&function, type.name);
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

        Member curr = vec_at(&type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        extend_parent_tast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_tast = ");
        extend_parent_tast_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_tast->type = ");
        extend_tast_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    tast_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_tast)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(&type.members, idx);

            string_extend_cstr(&gen_a, &function, "    tast_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_tast)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return tast_unwrap_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(base_tast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast_gen_get_pos_internal(Tast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast_gen_get_pos_internal(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    tast_get_pos(const Tast* tast)");
    } else {
        string_extend_cstr(&gen_a, &function, "    tast_get_pos_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(const ");
        extend_tast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* tast)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return tast->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (tast->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Tast_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_tast_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return tast_get_pos_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "(tast_unwrap_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const(tast));\n");

                string_extend_cstr(&gen_a, &function, "        break;\n");
            }

            string_extend_cstr(&gen_a, &function, "    }\n");
        }

        string_extend_cstr(&gen_a, &function, "unreachable(\"\");\n");
        string_extend_cstr(&gen_a, &function, "}\n");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_tast_new_forward_decl(Tast_type tast) {
    tast_gen_new_internal(tast, false);
}

static void gen_tast_new_define(Tast_type tast) {
    tast_gen_new_internal(tast, true);
}

static void gen_tast_get_pos_forward_decl(Tast_type tast) {
    tast_gen_get_pos_internal(tast, false);
}

static void gen_tast_get_pos_define(Tast_type tast) {
    tast_gen_get_pos_internal(tast, true);
}

static void gen_tast_vecs(Tast_type tast) {
    for (size_t idx = 0; idx < tast.sub_types.info.count; idx++) {
        gen_tast_vecs(vec_at(&tast.sub_types, idx));
    }

    String vec_name = {0};
    extend_tast_name_first_upper(&vec_name, tast.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_tast_name_first_upper(&item_name, tast.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_all_tasts(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Tast_type tast = tast_gen_tast();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef TAST_H\n");
        gen_gen("#define TAST_H\n");

        gen_gen("#include <tast_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
    } else {
        gen_gen("#ifndef TAST_FORWARD_DECL_H\n");
        gen_gen("#define TAST_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    tast_gen_tast_forward_decl(tast);

    if (!implementation) {
        gen_tast_vecs(tast);
    }

    if (implementation) {
        tast_gen_tast_struct(tast);

        tast_gen_tast_unwrap(tast);
        tast_gen_tast_wrap(tast);

        gen_gen("%s\n", "static inline Tast* tast_new(void) {");
        gen_gen("%s\n", "    Tast* new_tast = arena_alloc(&a_main, sizeof(*new_tast));");
        gen_gen("%s\n", "    return new_tast;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_tast_new_forward_decl(tast);
    }
    tast_gen_print_forward_decl(tast);
    if (implementation) {
        gen_tast_new_define(tast);
    }

    gen_tast_get_pos_forward_decl(tast);
    if (implementation) {
        gen_tast_get_pos_define(tast);
    }

    if (implementation) {
        gen_gen("#endif // TAST_H\n");
    } else {
        gen_gen("#endif // TAST_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_TAST_H
