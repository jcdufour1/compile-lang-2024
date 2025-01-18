
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
    Str_view parent;
    Str_view base;
} Uast_name;

typedef struct Uast_type_ {
    Uast_name name;
    Members members;
    Uast_type_vec sub_types;
} Uast_type;

static void extend_uast_name_upper(String* output, Uast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
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
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
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
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
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
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
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
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
        string_extend_cstr(&gen_a, output, "uast");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_uast_name_first_upper(String* output, Uast_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "uast")) {
        string_extend_cstr(&gen_a, output, "Uast");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Uast_name uast_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Uast_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Uast_type uast_gen_block(const char* prefix) {
    Uast_type block = {.name = uast_name_new(prefix, "block", false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Uast_stmt_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Uast_type uast_gen_unary(const char* prefix) {
    Uast_type unary = {.name = uast_name_new(prefix, "unary", false)};

    append_member(&unary.members, "Uast_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");

    return unary;
}

static Uast_type uast_gen_binary(const char* prefix) {
    Uast_type binary = {.name = uast_name_new(prefix, "binary", false)};

    append_member(&binary.members, "Uast_expr*", "lhs");
    append_member(&binary.members, "Uast_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");

    return binary;
}

static Uast_type uast_gen_operator(const char* prefix) {
    const char* base_name = "operator";
    Uast_type operator = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &operator.sub_types, uast_gen_unary(base_name));
    vec_append(&gen_a, &operator.sub_types, uast_gen_binary(base_name));

    return operator;
}

static Uast_type uast_gen_symbol_untyped(const char* prefix) {
    Uast_type sym = {.name = uast_name_new(prefix, "symbol_untyped", false)};

    append_member(&sym.members, "Str_view", "name");

    return sym;
}

static Uast_type uast_gen_member_access_untyped(const char* prefix) {
    Uast_type access = {.name = uast_name_new(prefix, "member_access_untyped", false)};

    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Uast_expr*", "callee");

    return access;
}

static Uast_type uast_gen_index_untyped(const char* prefix) {
    Uast_type index = {.name = uast_name_new(prefix, "index_untyped", false)};

    append_member(&index.members, "Uast_expr*", "index");
    append_member(&index.members, "Uast_expr*", "callee");

    return index;
}

static Uast_type uast_gen_number(const char* prefix) {
    Uast_type number = {.name = uast_name_new(prefix, "number", false)};

    append_member(&number.members, "int64_t", "data");

    return number;
}

static Uast_type uast_gen_string(const char* prefix) {
    Uast_type string = {.name = uast_name_new(prefix, "string", false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Uast_type uast_gen_char(const char* prefix) {
    Uast_type lang_char = {.name = uast_name_new(prefix, "char", false)};

    append_member(&lang_char.members, "char", "data");

    return lang_char;
}

static Uast_type uast_gen_void(const char* prefix) {
    Uast_type lang_void = {.name = uast_name_new(prefix, "void", false)};

    return lang_void;
}

static Uast_type uast_gen_enum_lit(const char* prefix) {
    Uast_type enum_lit = {.name = uast_name_new(prefix, "enum_lit", false)};

    append_member(&enum_lit.members, "int64_t", "data");

    return enum_lit;
}

static Uast_type uast_gen_literal(const char* prefix) {
    const char* base_name = "literal";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lit.sub_types, uast_gen_number(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_string(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_void(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_enum_lit(base_name));
    vec_append(&gen_a, &lit.sub_types, uast_gen_char(base_name));

    return lit;
}

static Uast_type uast_gen_function_call(const char* prefix) {
    Uast_type call = {.name = uast_name_new(prefix, "function_call", false)};

    append_member(&call.members, "Uast_expr_vec", "args");
    append_member(&call.members, "Uast_expr*", "callee");

    return call;
}

static Uast_type uast_gen_struct_literal(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "struct_literal", false)};

    append_member(&lit.members, "Uast_stmt_vec", "members");
    append_member(&lit.members, "Str_view", "name");

    return lit;
}

static Uast_type uast_gen_tuple(const char* prefix) {
    Uast_type lit = {.name = uast_name_new(prefix, "tuple", false)};

    append_member(&lit.members, "Uast_expr_vec", "members");

    return lit;
}

static Uast_type uast_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Uast_type expr = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &expr.sub_types, uast_gen_operator(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_symbol_untyped(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_member_access_untyped(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_index_untyped(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_function_call(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_struct_literal(base_name));
    vec_append(&gen_a, &expr.sub_types, uast_gen_tuple(base_name));

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

    append_member(&def.members, "Uast_function_params*", "params");
    append_member(&def.members, "Uast_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

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

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Uast_type uast_gen_enum_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "enum_def", false)};

    append_member(&def.members, "Ustruct_def_base", "base");

    return def;
}

static Uast_type uast_gen_sum_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "sum_def", false)};

    append_member(&def.members, "Ustruct_def_base", "base");

    return def;
}

static Uast_type uast_gen_primitive_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "primitive_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Uast_type uast_gen_string_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "string_def", false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Uast_type uast_gen_struct_lit_def(const char* prefix) {
    Uast_type def = {.name = uast_name_new(prefix, "struct_lit_def", false)};

    append_member(&def.members, "Uast_vec", "members");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Uast_type uast_gen_literal_def(const char* prefix) {
    const char* base_name = "literal_def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, uast_gen_string_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_struct_lit_def(base_name));

    return def;
}

static Uast_type uast_gen_def(const char* prefix) {
    const char* base_name = "def";
    Uast_type def = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &def.sub_types, uast_gen_function_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_variable_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_struct_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_raw_union_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_enum_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_sum_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_primitive_def(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_function_decl(base_name));
    vec_append(&gen_a, &def.sub_types, uast_gen_literal_def(base_name));

    return def;
}

static Uast_type uast_gen_function_params(const char* prefix) {
    Uast_type params = {.name = uast_name_new(prefix, "function_params", false)};

    append_member(&params.members, "Uast_variable_def_vec", "params");

    return params;
}

static Uast_type uast_gen_lang_type(const char* prefix) {
    Uast_type lang_type = {.name = uast_name_new(prefix, "lang_type", false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
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

static Uast_type uast_gen_for_range(const char* prefix) {
    Uast_type range = {.name = uast_name_new(prefix, "for_range", false)};

    append_member(&range.members, "Uast_variable_def*", "var_def");
    append_member(&range.members, "Uast_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Uast_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Uast_block*", "body");

    return range;
}

static Uast_type uast_gen_for_with_cond(const char* prefix) {
    Uast_type for_cond = {.name = uast_name_new(prefix, "for_with_cond", false)};

    append_member(&for_cond.members, "Uast_condition*", "condition");
    append_member(&for_cond.members, "Uast_block*", "body");

    return for_cond;
}

static Uast_type uast_gen_break(const char* prefix) {
    Uast_type lang_break = {.name = uast_name_new(prefix, "break", false)};

    return lang_break;
}

static Uast_type uast_gen_continue(const char* prefix) {
    Uast_type lang_cont = {.name = uast_name_new(prefix, "continue", false)};

    return lang_cont;
}

static Uast_type uast_gen_assignment(const char* prefix) {
    Uast_type assign = {.name = uast_name_new(prefix, "assignment", false)};

    append_member(&assign.members, "Uast_stmt*", "lhs");
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
    Uast_type lang_if = {.name = uast_name_new(prefix, "case", false)};

    append_member(&lang_if.members, "bool", "is_default");
    append_member(&lang_if.members, "Uast_expr*", "expr");
    append_member(&lang_if.members, "Uast_stmt*", "if_true");

    return lang_if;
}

static Uast_type uast_gen_switch(const char* prefix) {
    Uast_type lang_if = {.name = uast_name_new(prefix, "switch", false)};

    append_member(&lang_if.members, "Uast_expr*", "operand");
    append_member(&lang_if.members, "Uast_case_vec", "cases");

    return lang_if;
}

static Uast_type uast_gen_return(const char* prefix) {
    Uast_type rtn = {.name = uast_name_new(prefix, "return", false)};

    append_member(&rtn.members, "Uast_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Uast_type uast_gen_if_else_chain(const char* prefix) {
    Uast_type chain = {.name = uast_name_new(prefix, "if_else_chain", false)};

    append_member(&chain.members, "Uast_if_vec", "uasts");

    return chain;
}

static Uast_type uast_gen_stmt(const char* prefix) {
    const char* base_name = "stmt";
    Uast_type stmt = {.name = uast_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &stmt.sub_types, uast_gen_block(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_expr(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_def(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_for_range(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_for_with_cond(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_break(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_continue(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_assignment(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_return(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_if_else_chain(base_name));
    vec_append(&gen_a, &stmt.sub_types, uast_gen_switch(base_name));

    return stmt;
}

static Uast_type uast_gen_uast(void) {
    const char* base_name = "uast";
    Uast_type uast = {.name = uast_name_new("uast", "", true)};

    vec_append(&gen_a, &uast.sub_types, uast_gen_stmt(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_function_params(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_lang_type(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_for_lower_bound(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_for_upper_bound(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_condition(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_if(base_name));
    vec_append(&gen_a, &uast.sub_types, uast_gen_case(base_name));

    return uast;
}

static void uast_gen_uast_forward_decl(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_forward_decl(vec_at(&uast.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void uast_gen_uast_struct_as(String* output, Uast_type uast) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(&uast.sub_types, idx);
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
        Uast_type curr = vec_at(&uast.sub_types, idx);
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
        uast_gen_uast_struct(vec_at(&uast.sub_types, idx));
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
        extend_struct_member(&output, vec_at(&uast.members, idx));
    }

    if (uast.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void uast_gen_unwrap_internal(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast_unwrap_##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_unwrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast) {\n");

    //    try(uast->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(uast->type == ");
    extend_uast_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &uast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &uast->as.");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void uast_gen_wrap_internal(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast_unwrap_##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_wrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
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

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void uast_gen_uast_unwrap(Uast_type uast) {
    uast_gen_unwrap_internal(uast, false);
    uast_gen_unwrap_internal(uast, true);
}

void uast_gen_uast_wrap(Uast_type uast) {
    uast_gen_wrap_internal(uast, false);
    uast_gen_wrap_internal(uast, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Uast)
static void uast_gen_print_forward_decl(Uast_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(uast) str_view_print(");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(uast, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void uast_gen_new_internal(Uast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
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

        Member curr = vec_at(&type.members, idx);

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
            string_extend_cstr(&gen_a, &function, "    uast_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_uast)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(&type.members, idx);

            string_extend_cstr(&gen_a, &function, "    uast_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_uast)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return uast_unwrap_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(base_uast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void uast_gen_get_pos_internal(Uast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_get_pos_internal(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    uast_get_pos(const Uast* uast)");
    } else {
        string_extend_cstr(&gen_a, &function, "    uast_get_pos_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(const ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* uast)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return uast->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (uast->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Uast_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_uast_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return uast_get_pos_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "(uast_unwrap_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const(uast));\n");

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

static void gen_uast_new_forward_decl(Uast_type uast) {
    uast_gen_new_internal(uast, false);
}

static void gen_uast_new_define(Uast_type uast) {
    uast_gen_new_internal(uast, true);
}

static void gen_uast_get_pos_forward_decl(Uast_type uast) {
    uast_gen_get_pos_internal(uast, false);
}

static void gen_uast_get_pos_define(Uast_type uast) {
    uast_gen_get_pos_internal(uast, true);
}

static void gen_uast_vecs(Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        gen_uast_vecs(vec_at(&uast.sub_types, idx));
    }

    String vec_name = {0};
    extend_uast_name_first_upper(&vec_name, uast.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_uast_name_first_upper(&item_name, uast.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_all_uasts(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Uast_type uast = uast_gen_uast();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef UAST_H\n");
        gen_gen("#define UAST_H\n");

        gen_gen("#include <uast_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
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
    if (implementation) {
        gen_uast_new_define(uast);
    }

    gen_uast_get_pos_forward_decl(uast);
    if (implementation) {
        gen_uast_get_pos_define(uast);
    }

    if (implementation) {
        gen_gen("#endif // UAST_H\n");
    } else {
        gen_gen("#endif // UAST_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_UAST_H
