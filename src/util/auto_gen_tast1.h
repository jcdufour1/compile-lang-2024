
#ifndef AUTO_GEN_TAST1_H
#define AUTO_GEN_TAST1_H

#include <auto_gen_util.h>

struct Tast1_type_;
typedef struct Tast1_type_ Tast1_type;

typedef struct {
    Vec_base info;
    Tast1_type* buf;
} Tast1_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Tast1_name;

typedef struct Tast1_type_ {
    Tast1_name name;
    Members members;
    Tast1_type_vec sub_types;
} Tast1_type;

static void extend_tast1_name_upper(String* output, Tast1_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "TAST1");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_tast1_name_lower(String* output, Tast1_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "tast1");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_tast1_name_first_upper(String* output, Tast1_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Tast1");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_tast1_name_upper(String* output, Tast1_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "TAST1");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_tast1_name_lower(String* output, Tast1_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        string_extend_cstr(&gen_a, output, "tast1");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "tast1");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_tast1_name_first_upper(String* output, Tast1_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "tast1")) {
        string_extend_cstr(&gen_a, output, "Tast1");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Tast1");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Tast1_name tast1_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Tast1_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Tast1_type tast1_gen_block(const char* prefix) {
    const char* base_name = "block";
    Tast1_type block = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Tast1_stmt_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Tast1_type tast1_gen_unary(const char* prefix) {
    const char* base_name = "unary";
    Tast1_type unary = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&unary.members, "Tast1_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");

    return unary;
}

static Tast1_type tast1_gen_binary(const char* prefix) {
    const char* base_name = "binary";
    Tast1_type binary = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&binary.members, "Tast1_expr*", "lhs");
    append_member(&binary.members, "Tast1_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");

    return binary;
}

static Tast1_type tast1_gen_primitive_sym(const char* prefix) {
    const char* base_name = "primitive_sym";
    Tast1_type primitive = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Tast1_type tast1_gen_sum_sym(const char* prefix) {
    const char* base_name = "sum_sym";
    Tast1_type sum = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&sum.members, "Sym_typed_base", "base");

    return sum;
}

static Tast1_type tast1_gen_enum_sym(const char* prefix) {
    const char* base_name = "enum_sym";
    Tast1_type lang_enum = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Tast1_type tast1_gen_struct_sym(const char* prefix) {
    const char* base_name = "struct_sym";
    Tast1_type lang_struct = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Tast1_type tast1_gen_tuple_sym(const char* prefix) {
    const char* base_name = "tuple_sym";
    Tast1_type lang_tuple = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_tuple.members, "Sym_typed_base", "base");

    return lang_tuple;
}

static Tast1_type tast1_gen_raw_union_sym(const char* prefix) {
    const char* base_name = "raw_union_sym";
    Tast1_type raw_union = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Tast1_type tast1_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Tast1_type operator = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &operator.sub_types, tast1_gen_unary(base_name));
    vec_append(&gen_a, &operator.sub_types, tast1_gen_binary(base_name));

    return operator;
}

static Tast1_type tast1_gen_symbol(const char* prefix) {
    const char* base_name = "symbol";
    Tast1_type sym = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Sym_typed_base", "base");

    return sym;
}

static Tast1_type tast1_gen_member_access(const char* prefix) {
    const char* base_name = "member_access";
    Tast1_type access = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Tast1_expr*", "callee");

    return access;
}

static Tast1_type tast1_gen_index(const char* prefix) {
    const char* base_name = "index";
    Tast1_type index = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Tast1_expr*", "index");
    append_member(&index.members, "Tast1_expr*", "callee");

    return index;
}

static Tast1_type tast1_gen_number(const char* prefix) {
    const char* base_name = "number";
    Tast1_type number = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Tast1_type tast1_gen_string(const char* prefix) {
    const char* base_name = "string";
    Tast1_type string = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type_primitive", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Tast1_type tast1_gen_char(const char* prefix) {
    const char* base_name = "char";
    Tast1_type lang_char = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Tast1_type tast1_gen_union_lit(const char* prefix) {
    const char* base_name = "union_lit";
    Tast1_type union_lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&union_lit.members, "Tast1_enum_lit*", "tag");
    append_member(&union_lit.members, "Lang_type", "lang_type");
    append_member(&union_lit.members, "Tast1_expr*", "item");

    return union_lit;
}

static Tast1_type tast1_gen_void(const char* prefix) {
    const char* base_name = "void";
    Tast1_type lang_void = {.name = tast1_name_new(prefix, base_name, false)};

    return lang_void;
}

static Tast1_type tast1_gen_enum_lit(const char* prefix) {
    const char* base_name = "enum_lit";
    Tast1_type enum_lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Tast1_type tast1_gen_sum_lit(const char* prefix) {
    const char* base_name = "sum_lit";
    Tast1_type sum_lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&sum_lit.members, "Tast1_enum_lit*", "tag");
    append_member(&sum_lit.members, "Tast1_expr*", "item");
    append_member(&sum_lit.members, "Lang_type", "lang_type");

    return sum_lit;
}

static Tast1_type tast1_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lit.sub_types, tast1_gen_number(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_string(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_void(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_enum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_sum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_char(base_name));
    vec_append(&gen_a, &lit.sub_types, tast1_gen_union_lit(base_name));

    return lit;
}

static Tast1_type tast1_gen_function_call(const char* prefix) {
    const char* base_name = "function_call";
    Tast1_type call = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&call.members, "Tast1_expr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Tast1_type tast1_gen_struct_literal(const char* prefix) {
    const char* base_name = "struct_literal";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast1_expr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");

    return lit;
}

static Tast1_type tast1_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast1_expr_vec", "members");
    append_member(&lit.members, "Lang_type_tuple", "lang_type");

    return lit;
}

static Tast1_type tast1_gen_sum_callee(const char* prefix) {
    const char* base_name = "sum_callee";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast1_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "sum_lang_type");

    return lit;
}

static Tast1_type tast1_gen_sum_case(const char* prefix) {
    const char* base_name = "sum_case";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast1_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "sum_lang_type");

    return lit;
}

static Tast1_type tast1_gen_sum_access(const char* prefix) {
    const char* base_name = "sum_access";
    Tast1_type lit = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Tast1_enum_lit*", "tag");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Tast1_expr*", "callee");

    return lit;
}

static Tast1_type tast1_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Tast1_type expr = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &expr.sub_types, tast1_gen_operator(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_symbol(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_member_access(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_index(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_function_call(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_struct_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_tuple(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_sum_callee(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_sum_case(base_name));
    vec_append(&gen_a, &expr.sub_types, tast1_gen_sum_access(base_name));

    return expr;
}

static Tast1_type tast1_gen_struct_def(const char* prefix) {
    const char* base_name = "struct_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast1_type tast1_gen_raw_union_def(const char* prefix) {
    const char* base_name = "raw_union_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast1_type tast1_gen_function_decl(const char* prefix) {
    const char* base_name = "function_decl";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast1_function_params*", "params");
    append_member(&def.members, "Tast1_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast1_type tast1_gen_function_def(const char* prefix) {
    const char* base_name = "function_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast1_function_decl*", "decl");
    append_member(&def.members, "Tast1_block*", "body");

    return def;
}

static Tast1_type tast1_gen_variable_def(const char* prefix) {
    const char* base_name = "variable_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Tast1_type tast1_gen_enum_def(const char* prefix) {
    const char* base_name = "enum_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast1_type tast1_gen_sum_def(const char* prefix) {
    const char* base_name = "sum_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Tast1_type tast1_gen_primitive_def(const char* prefix) {
    const char* base_name = "primitive_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast1_type tast1_gen_string_def(const char* prefix) {
    const char* base_name = "string_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Tast1_type tast1_gen_struct_lit_def(const char* prefix) {
    const char* base_name = "struct_lit_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&def.members, "Tast1_expr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Tast1_type tast1_gen_literal_def(const char* prefix) {
    const char* base_name = "literal_def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast1_gen_string_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_struct_lit_def(base_name));

    return def;
}

static Tast1_type tast1_gen_def(const char* prefix) {
    const char* base_name = "def";
    Tast1_type def = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, tast1_gen_function_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_variable_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_struct_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_raw_union_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_enum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_sum_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_primitive_def(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_function_decl(base_name));
    vec_append(&gen_a, &def.sub_types, tast1_gen_literal_def(base_name));

    return def;
}

static Tast1_type tast1_gen_function_params(const char* prefix) {
    const char* base_name = "function_params";
    Tast1_type params = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&params.members, "Tast1_variable_def_vec", "params");

    return params;
}

static Tast1_type tast1_gen_lang_type(const char* prefix) {
    const char* base_name = "lang_type";
    Tast1_type lang_type = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Tast1_type tast1_gen_for_lower_bound(const char* prefix) {
    const char* base_name = "for_lower_bound";
    Tast1_type bound = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast1_expr*", "child");

    return bound;
}

static Tast1_type tast1_gen_for_upper_bound(const char* prefix) {
    const char* base_name = "for_upper_bound";
    Tast1_type bound = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast1_expr*", "child");

    return bound;
}

static Tast1_type tast1_gen_condition(const char* prefix) {
    const char* base_name = "condition";
    Tast1_type bound = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&bound.members, "Tast1_operator*", "child");

    return bound;
}

static Tast1_type tast1_gen_for_range(const char* prefix) {
    const char* base_name = "for_range";
    Tast1_type range = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&range.members, "Tast1_variable_def*", "var_def");
    append_member(&range.members, "Tast1_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Tast1_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Tast1_block*", "body");

    return range;
}

static Tast1_type tast1_gen_for_with_cond(const char* prefix) {
    const char* base_name = "for_with_cond";
    Tast1_type for_cond = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&for_cond.members, "Tast1_condition*", "condition");
    append_member(&for_cond.members, "Tast1_block*", "body");

    return for_cond;
}

static Tast1_type tast1_gen_break(const char* prefix) {
    const char* base_name = "break";
    Tast1_type lang_break = {.name = tast1_name_new(prefix, base_name, false)};

    return lang_break;
}

static Tast1_type tast1_gen_continue(const char* prefix) {
    const char* base_name = "continue";
    Tast1_type lang_cont = {.name = tast1_name_new(prefix, base_name, false)};

    return lang_cont;
}

static Tast1_type tast1_gen_assignment(const char* prefix) {
    const char* base_name = "assignment";
    Tast1_type assign = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&assign.members, "Tast1_stmt*", "lhs");
    append_member(&assign.members, "Tast1_expr*", "rhs");

    return assign;
}

static Tast1_type tast1_gen_if(const char* prefix) {
    const char* base_name = "if";
    Tast1_type lang_if = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&lang_if.members, "Tast1_condition*", "condition");
    append_member(&lang_if.members, "Tast1_block*", "body");

    return lang_if;
}

static Tast1_type tast1_gen_return(const char* prefix) {
    const char* base_name = "return";
    Tast1_type rtn = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&rtn.members, "Tast1_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Tast1_type tast1_gen_if_else_chain(const char* prefix) {
    const char* base_name = "if_else_chain";
    Tast1_type chain = {.name = tast1_name_new(prefix, base_name, false)};

    append_member(&chain.members, "Tast1_if_vec", "tast1s");

    return chain;
}

static Tast1_type tast1_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Tast1_type stmt = {.name = tast1_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &stmt.sub_types, tast1_gen_block(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_expr(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_for_range(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_for_with_cond(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_if_else_chain(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_return(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_break(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_continue(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_assignment(base_name));
    vec_append(&gen_a, &stmt.sub_types, tast1_gen_def(base_name));

    return stmt;
}

static Tast1_type tast1_gen_tast1(void) {
    const char* base_name = "tast1";
    Tast1_type tast1 = {.name = tast1_name_new(base_name, "", true)};

    vec_append(&gen_a, &tast1.sub_types, tast1_gen_stmt(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_function_params(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_lang_type(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_for_lower_bound(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_for_upper_bound(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_condition(base_name));
    vec_append(&gen_a, &tast1.sub_types, tast1_gen_if(base_name));

    return tast1;
}

static void tast1_gen_tast1_forward_decl(Tast1_type tast1) {
    String output = {0};

    for (size_t idx = 0; idx < tast1.sub_types.info.count; idx++) {
        tast1_gen_tast1_forward_decl(vec_at(&tast1.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_tast1_name_first_upper(&output, tast1.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast1_name_first_upper(&output, tast1.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_tast1_name_first_upper(&output, tast1.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast1_gen_tast1_struct_as(String* output, Tast1_type tast1) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_tast1_name_first_upper(output, tast1.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast1.sub_types.info.count; idx++) {
        Tast1_type curr = vec_at(&tast1.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast1_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_tast1_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast1_name_first_upper(output, tast1.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast1_gen_tast1_struct_enum(String* output, Tast1_type tast1) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_tast1_name_upper(output, tast1.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < tast1.sub_types.info.count; idx++) {
        Tast1_type curr = vec_at(&tast1.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_tast1_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_tast1_name_upper(output, tast1.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void tast1_gen_tast1_struct(Tast1_type tast1) {
    String output = {0};

    for (size_t idx = 0; idx < tast1.sub_types.info.count; idx++) {
        tast1_gen_tast1_struct(vec_at(&tast1.sub_types, idx));
    }

    if (tast1.sub_types.info.count > 0) {
        tast1_gen_tast1_struct_as(&output, tast1);
        tast1_gen_tast1_struct_enum(&output, tast1);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_tast1_name_first_upper(&output, tast1.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (tast1.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_tast1_name_first_upper(&as_member_type, tast1.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_tast1_name_upper(&enum_member_type, tast1.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < tast1.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&tast1.members, idx));
    }

    if (tast1.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_tast1_name_first_upper(&output, tast1.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void tast1_gen_internal_unwrap(Tast1_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast1_gen_internal_unwrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast1_##lower* tast1__unwrap##lower(Tast1* tast1) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast1_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast1) {\n");

    //    try(tast1->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(tast1->type == ");
    extend_tast1_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &tast1->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &tast1->as.");
    extend_tast1_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast1_gen_internal_wrap(Tast1_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast1_gen_internal_wrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Tast1_##lower* tast1__unwrap##lower(Tast1* tast1) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast1_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast1) {\n");

    //    return &tast1->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) tast1;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void tast1_gen_tast1_unwrap(Tast1_type tast1) {
    tast1_gen_internal_unwrap(tast1, false);
    tast1_gen_internal_unwrap(tast1, true);
}

void tast1_gen_tast1_wrap(Tast1_type tast1) {
    tast1_gen_internal_wrap(tast1, false);
    tast1_gen_internal_wrap(tast1, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Tast1)
static void tast1_gen_print_forward_decl(Tast1_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast1_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_tast1_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(tast1) str_view_print(");
    extend_tast1_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(tast1, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_tast1_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* tast1, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void tast1_gen_new_internal(Tast1_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast1_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_tast1_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_tast1_name_lower(&function, type.name);
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
        extend_parent_tast1_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_tast1 = ");
        extend_parent_tast1_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_tast1->type = ");
        extend_tast1_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    tast1_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_tast1)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(&type.members, idx);

            string_extend_cstr(&gen_a, &function, "    tast1_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_tast1)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return tast1_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap(base_tast1);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void tast1_gen_internal_get_pos(Tast1_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        tast1_gen_internal_get_pos(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    tast1_get_pos(const Tast1* tast1)");
    } else {
        string_extend_cstr(&gen_a, &function, "    tast1_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_get_pos(const ");
        extend_tast1_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* tast1)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return tast1->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (tast1->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Tast1_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_tast1_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return tast1_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_get_pos(tast1_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const_unwrap(tast1));\n");

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

static void gen_tast1_new_forward_decl(Tast1_type tast1) {
    tast1_gen_new_internal(tast1, false);
}

static void gen_tast1_new_define(Tast1_type tast1) {
    tast1_gen_new_internal(tast1, true);
}

static void gen_tast1_forward_decl_get_pos(Tast1_type tast1) {
    tast1_gen_internal_get_pos(tast1, false);
}

static void gen_tast1_define_get_pos(Tast1_type tast1) {
    tast1_gen_internal_get_pos(tast1, true);
}

static void gen_tast1_vecs(Tast1_type tast1) {
    for (size_t idx = 0; idx < tast1.sub_types.info.count; idx++) {
        gen_tast1_vecs(vec_at(&tast1.sub_types, idx));
    }

    String vec_name = {0};
    extend_tast1_name_first_upper(&vec_name, tast1.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_tast1_name_first_upper(&item_name, tast1.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_all_tast1s(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Tast1_type tast1 = tast1_gen_tast1();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef TAST1_H\n");
        gen_gen("#define TAST1_H\n");

        gen_gen("#include <tast1_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
    } else {
        gen_gen("#ifndef TAST1_FORWARD_DECL_H\n");
        gen_gen("#define TAST1_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    tast1_gen_tast1_forward_decl(tast1);

    if (!implementation) {
        gen_tast1_vecs(tast1);
    }

    if (implementation) {
        tast1_gen_tast1_struct(tast1);

        tast1_gen_tast1_unwrap(tast1);
        tast1_gen_tast1_wrap(tast1);

        gen_gen("%s\n", "static inline Tast1* tast1_new(void) {");
        gen_gen("%s\n", "    Tast1* new_tast1 = arena_alloc(&a_main, sizeof(*new_tast1));");
        gen_gen("%s\n", "    return new_tast1;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_tast1_new_forward_decl(tast1);
    }
    tast1_gen_print_forward_decl(tast1);
    if (implementation) {
        gen_tast1_new_define(tast1);
    }

    gen_tast1_forward_decl_get_pos(tast1);
    if (implementation) {
        gen_tast1_define_get_pos(tast1);
    }

    if (implementation) {
        gen_gen("#endif // TAST1_H\n");
    } else {
        gen_gen("#endif // TAST1_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_TAST1_H
