
#ifndef AUTO_GEN_TAST2_H
#define AUTO_GEN_TAST2_H

#include <auto_gen_util.h>

struct Tast2_type_;
typedef struct Tast2_type_ Tast2_type;

typedef struct {
    Vec_base info;
    Tast2_type* buf;
} Tast2_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Tast2_name;

typedef struct Tast2_type_ {
    Tast2_name name;
    Members members;
    Tast2_type_vec sub_types;
} Tast2_type;

static void extend_tast2_name_upper(String* output, Tast2_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "TAST2");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_tast2_name_lower(String* output, Tast2_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "tast2");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_tast2_name_first_upper(String* output, Tast2_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Tast2");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_tast2_name_upper(String* output, Tast2_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "TAST2");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_tast2_name_lower(String* output, Tast2_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        string_extend_cstr(&gen_a, output, "tast2");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "tast2");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_tast2_name_first_upper(String* output, Tast2_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast2")) {
        string_extend_cstr(&gen_a, output, "Tast2");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Tast2");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Tast2_name tast2_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Tast2_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Tast2_type tast2_gen_block(const char* prefix) {
    const char* base_name = "block";
    Tast2_type block = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Tast2_stmt_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Tast2_type tast2_gen_unary(const char* prefix) {
    const char* base_name = "unary";
    Tast2_type unary = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&unary.members, "Tast2_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");

    return unary;
}

static Tast2_type tast2_gen_binary(const char* prefix) {
    const char* base_name = "binary";
    Tast2_type binary = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&binary.members, "Tast2_expr*", "lhs");
    append_member(&binary.members, "Tast2_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");

    return binary;
}

static Tast2_type tast2_gen_primitive_sym(const char* prefix) {
    const char* base_name = "primitive_sym";
    Tast2_type primitive = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Tast2_type tast2_gen_sum_sym(const char* prefix) {
    const char* base_name = "sum_sym";
    Tast2_type sum = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&sum.members, "Sym_typed_base", "base");

    return sum;
}

static Tast2_type tast2_gen_enum_sym(const char* prefix) {
    const char* base_name = "enum_sym";
    Tast2_type lang_enum = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Tast2_type tast2_gen_struct_sym(const char* prefix) {
    const char* base_name = "struct_sym";
    Tast2_type lang_struct = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Tast2_type tast2_gen_tuple_sym(const char* prefix) {
    const char* base_name = "tuple_sym";
    Tast2_type lang_tuple = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_tuple.members, "Sym_typed_base", "base");

    return lang_tuple;
}

static Tast2_type tast2_gen_raw_union_sym(const char* prefix) {
    const char* base_name = "raw_union_sym";
    Tast2_type raw_union = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Tast2_type tast2_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Tast2_type operator = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &operator.sub_types, tast2_gen_unary(base_name));
    vec_append(&gen_a, &operator.sub_types, tast2_gen_binary(base_name));

    return operator;
}

static Tast2_type tast2_gen_symbol(const char* prefix) {
    const char* base_name = "symbol";
    Tast2_type sym = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Sym_typed_base", "base");

    return sym;
}

static Tast2_type tast2_gen_member_access(const char* prefix) {
    const char* base_name = "member_access";
    Tast2_type access = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Tast2_expr*", "callee");

    return access;
}

static Tast2_type tast2_gen_index(const char* prefix) {
    const char* base_name = "index";
    Tast2_type index = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Tast2_expr*", "index");
    append_member(&index.members, "Tast2_expr*", "callee");

    return index;
}

static Tast2_type tast2_gen_number(const char* prefix) {
    const char* base_name = "number";
    Tast2_type number = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Tast2_type tast2_gen_string(const char* prefix) {
    const char* base_name = "string";
    Tast2_type string = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type_primitive", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Tast2_type tast2_gen_char(const char* prefix) {
    const char* base_name = "char";
    Tast2_type lang_char = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Tast2_type tast2_gen_raw_union_lit(const char* prefix) {
    const char* base_name = "raw_union_lit";
    Tast2_type raw_union_lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&raw_union_lit.members, "Tast2_enum_lit*", "tag");
    append_member(&raw_union_lit.members, "Lang_type", "lang_type");
    append_member(&raw_union_lit.members, "Tast2_expr*", "item");

    return raw_union_lit;
}

static Tast2_type tast2_gen_void(const char* prefix) {
    const char* base_name = "void";
    Tast2_type lang_void = {.name = tast2_name_new(prefix, base_name, false)};

    return lang_void;
}

static Tast2_type tast2_gen_enum_lit(const char* prefix) {
    const char* base_name = "enum_lit";
    Tast2_type enum_lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Tast2_type tast2_gen_sum_lit(const char* prefix) {
    const char* base_name = "sum_lit";
    Tast2_type sum_lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&sum_lit.members, "Tast2_enum_lit*", "tag");
    append_member(&sum_lit.members, "Tast2_expr*", "item");
    append_member(&sum_lit.members, "Lang_type", "lang_type");

    return sum_lit;
}

static Tast2_type tast2_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lit.sub_types, tast2_gen_number(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_string(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_void(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_enum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_sum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_char(base_name));
    vec_append(&gen_a, &lit.sub_types, tast2_gen_raw_union_lit(base_name));

    return lit;
}

static Tast2_type tast2_gen_function_call(const char* prefix) {
    const char* base_name = "function_call";
    Tast2_type call = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&call.members, "Tast2_expr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Tast2_type tast2_gen_struct_literal(const char* prefix) {
    const char* base_name = "struct_literal";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast2_expr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");

    return lit;
}

static Tast2_type tast2_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast2_expr_vec", "members");
    append_member(&lit.members, "Lang_type_tuple", "lang_type");

    return lit;
}

static Tast2_type tast2_gen_sum_callee(const char* prefix) {
    const char* base_name = "sum_callee";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast2_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "sum_lang_type");

    return lit;
}

static Tast2_type tast2_gen_sum_case(const char* prefix) {
    const char* base_name = "sum_case";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast2_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "sum_lang_type");

    return lit;
}

static Tast2_type tast2_gen_sum_access(const char* prefix) {
    const char* base_name = "sum_access";
    Tast2_type lit = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast2_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Tast2_expr*", "callee");

    return lit;
}

static Tast2_type tast2_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Tast2_type expr = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &expr.sub_types, tast2_gen_operator(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_symbol(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_member_access(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_index(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_function_call(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_struct_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_tuple(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_sum_callee(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_sum_case(base_name));
    vec_append(&gen_a, &expr.sub_types, tast2_gen_sum_access(base_name));

    return expr;
}

static Tast2_type tast2_gen_struct_def(const char* prefix) {
    const char* base_name = "struct_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast2_type tast2_gen_raw_union_def(const char* prefix) {
    const char* base_name = "raw_union_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast2_type tast2_gen_function_decl(const char* prefix) {
    const char* base_name = "function_decl";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast2_function_params*", "params");
    append_member(&def.members, "Tast2_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast2_type tast2_gen_function_def(const char* prefix) {
    const char* base_name = "function_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast2_function_decl*", "decl");
    append_member(&def.members, "Tast2_block*", "body");

    return def;
}

static Tast2_type tast2_gen_variable_def(const char* prefix) {
    const char* base_name = "variable_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast2_type tast2_gen_enum_def(const char* prefix) {
    const char* base_name = "enum_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast2_type tast2_gen_sum_def(const char* prefix) {
    const char* base_name = "sum_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast2_type tast2_gen_primitive_def(const char* prefix) {
    const char* base_name = "primitive_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast2_type tast2_gen_string_def(const char* prefix) {
    const char* base_name = "string_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Tast2_type tast2_gen_struct_lit_def(const char* prefix) {
    const char* base_name = "struct_lit_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast2_expr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast2_type tast2_gen_literal_def(const char* prefix) {
    const char* base_name = "literal_def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast2_gen_string_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_struct_lit_def(base_name));

    return def;
}

static Tast2_type tast2_gen_def(const char* prefix) {
    const char* base_name = "def";
    Tast2_type def = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast2_gen_function_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_variable_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_struct_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_raw_union_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_enum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_sum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_primitive_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_function_decl(base_name));
    vec_append(&gen_a, &def.sub_types, tast2_gen_literal_def(base_name));

    return def;
}

static Tast2_type tast2_gen_function_params(const char* prefix) {
    const char* base_name = "function_params";
    Tast2_type params = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&params.members, "Tast2_variable_def_vec", "params");

    return params;
}

static Tast2_type tast2_gen_lang_type(const char* prefix) {
    const char* base_name = "lang_type";
    Tast2_type lang_type = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Tast2_type tast2_gen_for_lower_bound(const char* prefix) {
    const char* base_name = "for_lower_bound";
    Tast2_type bound = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast2_expr*", "child");

    return bound;
}

static Tast2_type tast2_gen_for_upper_bound(const char* prefix) {
    const char* base_name = "for_upper_bound";
    Tast2_type bound = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast2_expr*", "child");

    return bound;
}

static Tast2_type tast2_gen_condition(const char* prefix) {
    const char* base_name = "condition";
    Tast2_type bound = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast2_operator*", "child");

    return bound;
}

static Tast2_type tast2_gen_for_range(const char* prefix) {
    const char* base_name = "for_range";
    Tast2_type range = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&range.members, "Tast2_variable_def*", "var_def");
    append_member(&range.members, "Tast2_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Tast2_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Tast2_block*", "body");

    return range;
}

static Tast2_type tast2_gen_for_with_cond(const char* prefix) {
    const char* base_name = "for_with_cond";
    Tast2_type for_cond = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&for_cond.members, "Tast2_condition*", "condition");
    append_member(&for_cond.members, "Tast2_block*", "body");

    return for_cond;
}

static Tast2_type tast2_gen_break(const char* prefix) {
    const char* base_name = "break";
    Tast2_type lang_break = {.name = tast2_name_new(prefix, base_name, false)};

    return lang_break;
}

static Tast2_type tast2_gen_continue(const char* prefix) {
    const char* base_name = "continue";
    Tast2_type lang_cont = {.name = tast2_name_new(prefix, base_name, false)};

    return lang_cont;
}

static Tast2_type tast2_gen_assignment(const char* prefix) {
    const char* base_name = "assignment";
    Tast2_type assign = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&assign.members, "Tast2_stmt*", "lhs");
    append_member(&assign.members, "Tast2_expr*", "rhs");

    return assign;
}

static Tast2_type tast2_gen_if(const char* prefix) {
    const char* base_name = "if";
    Tast2_type lang_if = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&lang_if.members, "Tast2_condition*", "condition");
    append_member(&lang_if.members, "Tast2_block*", "body");

    return lang_if;
}

static Tast2_type tast2_gen_return(const char* prefix) {
    const char* base_name = "return";
    Tast2_type rtn = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&rtn.members, "Tast2_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Tast2_type tast2_gen_if_else_chain(const char* prefix) {
    const char* base_name = "if_else_chain";
    Tast2_type chain = {.name = tast2_name_new(prefix, base_name, false)};

    append_member(&chain.members, "Tast2_if_vec", "tast2s");

    return chain;
}

static Tast2_type tast2_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Tast2_type stmt = {.name = tast2_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &stmt.sub_types, tast2_gen_block(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_expr(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_for_range(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_for_with_cond(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_if_else_chain(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_return(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_break(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_continue(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_assignment(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast2_gen_def(base_name));

    return stmt;
}

static Tast2_type tast2_gen_tast2(void) {
    const char* base_name = "tast2";
    Tast2_type tast2 = {.name = tast2_name_new(base_name, "", true)};

    vec_append(&gen_a, &tast2.sub_types, tast2_gen_stmt(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_function_params(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_lang_type(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_for_lower_bound(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_for_upper_bound(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_condition(base_name));
    vec_append(&gen_a, &tast2.sub_types, tast2_gen_if(base_name));

    return tast2;
}

static void tast2_gen_tast2_forward_decl(Tast2_type tast2) {
    String output = {0};

    for (size_t idx = 0; idx < tast2.sub_types.info.count; idx++) {
        tast2_gen_tast2_forward_decl(vec_at(&tast2.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_tast2_name_first_upper(&output, tast2.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast2_name_first_upper(&output, tast2.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_tast2_name_first_upper(&output, tast2.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast2_gen_tast2_struct_as(String* output, Tast2_type tast2) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_tast2_name_first_upper(output, tast2.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast2.sub_types.info.count; idx++) {
        Tast2_type curr = vec_at(&tast2.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast2_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_tast2_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast2_name_first_upper(output, tast2.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast2_gen_tast2_struct_enum(String* output, Tast2_type tast2) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_tast2_name_upper(output, tast2.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast2.sub_types.info.count; idx++) {
        Tast2_type curr = vec_at(&tast2.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast2_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast2_name_upper(output, tast2.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast2_gen_tast2_struct(Tast2_type tast2) {
    String output = {0};

    for (size_t idx = 0; idx < tast2.sub_types.info.count; idx++) {
        tast2_gen_tast2_struct(vec_at(&tast2.sub_types, idx));
    }

    if (tast2.sub_types.info.count > 0) {
        tast2_gen_tast2_struct_as(&output, tast2);
        tast2_gen_tast2_struct_enum(&output, tast2);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast2_name_first_upper(&output, tast2.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (tast2.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_tast2_name_first_upper(&as_member_type, tast2.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_tast2_name_upper(&enum_member_type, tast2.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < tast2.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&tast2.members, idx));
    }

    if (tast2.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_tast2_name_first_upper(&output, tast2.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast2_gen_internal_unwrap(Tast2_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast2_gen_internal_unwrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast2_##lower* tast2__unwrap##lower(Tast2* tast2) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast2_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast2) {\n");

    //    unwrap(tast2->type == upper); 
    string_extend_cstr(&gen_a, &function, "    unwrap(tast2->type == ");
    extend_tast2_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &tast2->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &tast2->as.");
    extend_tast2_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast2_gen_internal_wrap(Tast2_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast2_gen_internal_wrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast2_##lower* tast2__unwrap##lower(Tast2* tast2) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast2_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast2) {\n");

    //    return &tast2->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) tast2;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void tast2_gen_tast2_unwrap(Tast2_type tast2) {
    tast2_gen_internal_unwrap(tast2, false);
    tast2_gen_internal_unwrap(tast2, true);
}

void tast2_gen_tast2_wrap(Tast2_type tast2) {
    tast2_gen_internal_wrap(tast2, false);
    tast2_gen_internal_wrap(tast2, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Tast2)
static void tast2_gen_print_forward_decl(Tast2_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast2_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_tast2_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(tast2) str_view_print(");
    extend_tast2_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(tast2, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_tast2_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast2, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void tast2_gen_new_internal(Tast2_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast2_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_tast2_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_tast2_name_lower(&function, type.name);
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
        extend_parent_tast2_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_tast2 = ");
        extend_parent_tast2_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_tast2->type = ");
        extend_tast2_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    tast2_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_tast2)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(&type.members, idx);

            string_extend_cstr(&gen_a, &function, "    tast2_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_tast2)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return tast2_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap(base_tast2);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast2_gen_internal_get_pos(Tast2_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast2_gen_internal_get_pos(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    tast2_get_pos(const Tast2* tast2)");
    } else {
        string_extend_cstr(&gen_a, &function, "    tast2_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_get_pos(const ");
        extend_tast2_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* tast2)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return tast2->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (tast2->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Tast2_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_tast2_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return tast2_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_get_pos(tast2_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const_unwrap(tast2));\n");

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

static void gen_tast2_new_forward_decl(Tast2_type tast2) {
    tast2_gen_new_internal(tast2, false);
}

static void gen_tast2_new_define(Tast2_type tast2) {
    tast2_gen_new_internal(tast2, true);
}

static void gen_tast2_forward_decl_get_pos(Tast2_type tast2) {
    tast2_gen_internal_get_pos(tast2, false);
}

static void gen_tast2_define_get_pos(Tast2_type tast2) {
    tast2_gen_internal_get_pos(tast2, true);
}

static void gen_tast2_vecs(Tast2_type tast2) {
    for (size_t idx = 0; idx < tast2.sub_types.info.count; idx++) {
        gen_tast2_vecs(vec_at(&tast2.sub_types, idx));
    }

    String vec_name = {0};
    extend_tast2_name_first_upper(&vec_name, tast2.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_tast2_name_first_upper(&item_name, tast2.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_all_tast2s(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Tast2_type tast2 = tast2_gen_tast2();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef TAST2_H\n");
        gen_gen("#define TAST2_H\n");

        gen_gen("#include <tast2_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
    } else {
        gen_gen("#ifndef TAST2_FORWARD_DECL_H\n");
        gen_gen("#define TAST2_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    tast2_gen_tast2_forward_decl(tast2);

    if (!implementation) {
        gen_tast2_vecs(tast2);
    }

    if (implementation) {
        tast2_gen_tast2_struct(tast2);

        tast2_gen_tast2_unwrap(tast2);
        tast2_gen_tast2_wrap(tast2);

        gen_gen("%s\n", "static inline Tast2* tast2_new(void) {");
        gen_gen("%s\n", "    Tast2* new_tast2 = arena_alloc(&a_main, sizeof(*new_tast2));");
        gen_gen("%s\n", "    return new_tast2;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_tast2_new_forward_decl(tast2);
    }
    tast2_gen_print_forward_decl(tast2);
    if (implementation) {
        gen_tast2_new_define(tast2);
    }

    gen_tast2_forward_decl_get_pos(tast2);
    if (implementation) {
        gen_tast2_define_get_pos(tast2);
    }

    if (implementation) {
        gen_gen("#endif // TAST2_H\n");
    } else {
        gen_gen("#endif // TAST2_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_TAST2_H
