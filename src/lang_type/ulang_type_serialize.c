#include <ulang_type_serialize.h>
#include <ulang_type.h>
#include <name.h>
#include <uast_expr_to_ulang_type.h>
#include <type_checking.h>

#define poison name_new(MOD_PATH_ARRAYS, sv(""), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0})

static Name serialize_ulang_type_expr_lit(Strv mod_path, const Uast_expr* expr);

Strv serialize_ulang_type_atom(Ulang_type_atom atom, bool include_scope, Pos pos) {
    Name temp = {0};
    unwrap(name_from_uname(&temp, atom.str, pos));
    Strv serialized = {0};
    if (include_scope) {
        serialized = serialize_name(temp);
    } else {
        serialized = serialize_name_symbol_table(&a_main, temp);
    }

    String name = {0};
    string_extend_int16_t(&a_main, &name, atom.pointer_depth);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_size_t(&a_main, &name, serialized.count);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, serialized);
    string_extend_cstr(&a_main, &name, "__");
    return string_to_strv(name);
}

Name serialize_ulang_type_fn(Strv mod_path, Ulang_type_fn ulang_type, bool include_scope) {
    String name = {0};
    extend_name(NAME_LOG, &name, serialize_ulang_type_tuple(mod_path, ulang_type.params, include_scope));
    string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(
        mod_path,
        *ulang_type.return_type,
        include_scope
    )));
    return name_new(mod_path, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_array(Strv mod_path, Ulang_type_array ulang_type, bool include_scope) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(
        mod_path,
        *ulang_type.item_type,
        include_scope
    )));

    Ulang_type count = {0};
    if (!uast_expr_to_ulang_type(&count, ulang_type.count)) {
        return util_literal_name_new_poison();
    }
    string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(
        mod_path,
        count,
        include_scope
    )));

    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_const_expr(Strv mod_path, Ulang_type_const_expr ulang_type) {
    switch (ulang_type.type) {
        case ULANG_TYPE_INT:
            return serialize_ulang_type_int(ulang_type_int_const_unwrap(ulang_type));
        case ULANG_TYPE_FLOAT_LIT:
            return serialize_ulang_type_float_lit(ulang_type_float_lit_const_unwrap(ulang_type));
        case ULANG_TYPE_STRING_LIT:
            return serialize_ulang_type_string_lit(ulang_type_string_lit_const_unwrap(ulang_type));
        case ULANG_TYPE_STRUCT_LIT:
            return serialize_ulang_type_struct_lit(
                mod_path,
                ulang_type_struct_lit_const_unwrap(ulang_type)
            );
        case ULANG_TYPE_FN_LIT:
            return serialize_ulang_type_fn_lit(ulang_type_fn_lit_const_unwrap(ulang_type));
    }
    unreachable("");
}

static Name serialize_ulang_type_expr_lit_struct_literal(Strv mod_path, const Uast_struct_literal* lit) {
    String name = {0};

    string_extend_cstr(&a_main, &name, "_struct");
    string_extend_size_t(&a_main, &name, lit->members.info.count);

    vec_foreach(idx, Uast_expr*, memb, lit->members) {
        string_extend_strv(
            &a_main,
            &name,
            serialize_name(serialize_ulang_type_expr_lit(mod_path, memb))
        );
    }

    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

static Name serialize_ulang_type_expr_lit_literal(Strv mod_path, const Uast_literal* lit) {
    (void) mod_path;
    String name = {0};
    string_extend_cstr(&a_main, &name, "_");

    switch (lit->type) {
        case UAST_INT:
            string_extend_int64_t(&a_main, &name, uast_int_const_unwrap(lit)->data);
            return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
        case UAST_FLOAT:
            string_extend_strv(&a_main, &name, serialize_double(uast_float_const_unwrap(lit)->data));
            return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
        case UAST_STRING:
            serialize_strv_actual(&name, uast_string_const_unwrap(lit)->data);
            return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
        case UAST_VOID:
            msg_todo("void literal as generic argument", uast_literal_get_pos(lit));
            return poison;
    }
    unreachable("");
}

static Name serialize_ulang_type_expr_lit(Strv mod_path, const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_LITERAL:
            return serialize_ulang_type_expr_lit_literal(mod_path, uast_literal_const_unwrap(expr));
        case UAST_SYMBOL:
            // TODO: make helper function for name_new or return Strv from functions instead of Name
            return name_new(
                MOD_PATH_ARRAYS,
                serialize_name(uast_symbol_const_unwrap(expr)->name),
                (Ulang_type_vec) {0},
                SCOPE_TOP_LEVEL,
                (Attrs) {0}
            );
        case UAST_STRUCT_LITERAL:
            return serialize_ulang_type_expr_lit_struct_literal(mod_path, uast_struct_literal_const_unwrap(expr));
        case UAST_ARRAY_LITERAL:
            msg_todo("array literal used as literal", uast_expr_get_pos(expr));
            return poison;
        case UAST_IF_ELSE_CHAIN:
            fallthrough;
        case UAST_BLOCK:
            fallthrough;
        case UAST_SWITCH:
            fallthrough;
        case UAST_UNKNOWN:
            fallthrough;
        case UAST_OPERATOR:
            fallthrough;
        case UAST_MEMBER_ACCESS:
            fallthrough;
        case UAST_INDEX:
            fallthrough;
        case UAST_FUNCTION_CALL:
            fallthrough;
        case UAST_TUPLE:
            fallthrough;
        case UAST_MACRO:
            fallthrough;
        case UAST_ENUM_ACCESS:
            fallthrough;
        case UAST_ENUM_GET_TAG:
            fallthrough;
        case UAST_ORELSE:
            fallthrough;
        case UAST_FN:
            fallthrough;
        case UAST_QUESTION_MARK:
            fallthrough;
        case UAST_UNDERSCORE:
            fallthrough;
        case UAST_EXPR_REMOVED:
            msg_todo(
                "actual error message for non-literal found where literal was expected",
                uast_expr_get_pos(expr)
            );
            return name_new(MOD_PATH_ARRAYS, sv(""), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
    }
    unreachable("");
}

Name serialize_ulang_type_struct_lit(Strv mod_path, Ulang_type_struct_lit ulang_type) {
    if (ulang_type.expr->type == UAST_STRUCT_LITERAL) {
        return serialize_ulang_type_expr_lit_struct_literal(mod_path, uast_struct_literal_const_unwrap(ulang_type.expr));
    }

    if (ulang_type.expr->type != UAST_OPERATOR) {
        msg_todo("", ulang_type.pos);
        return poison;
    }
    const Uast_operator* oper = uast_operator_const_unwrap(ulang_type.expr);
    if (oper->type != UAST_UNARY) {
        msg_todo("", ulang_type.pos);
        return poison;
    }
    const Uast_expr* child = uast_unary_const_unwrap(oper)->child;
    if (child->type != UAST_STRUCT_LITERAL) {
        msg_todo("", ulang_type.pos);
        return poison;
    }
    return serialize_ulang_type_expr_lit_struct_literal(mod_path, uast_struct_literal_const_unwrap(child));
}


Name serialize_ulang_type_fn_lit(Ulang_type_fn_lit ulang_type) {
    String name = {0};
    string_extend_cstr(&a_main, &name, "_fn");
    string_extend_strv(&a_main, &name, serialize_name(ulang_type.name));

    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_int(Ulang_type_int ulang_type) {
    String name = {0};
    string_extend_cstr(&a_main, &name, "_");
    string_extend_int64_t(&a_main, &name, ulang_type.data);
    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_float_lit(Ulang_type_float_lit ulang_type) {
    String name = {0};
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, serialize_double(ulang_type.data));
    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_string_lit(Ulang_type_string_lit ulang_type) {
    String name = {0};
    string_extend_cstr(&a_main, &name, "_");
    serialize_strv_actual(&name, ulang_type.data);
    return name_new(MOD_PATH_ARRAYS, string_to_strv(name), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_tuple(Strv mod_path, Ulang_type_tuple ulang_type, bool include_scope) {
    String name = {0};
    for (size_t idx = 0; idx < ulang_type.ulang_types.info.count; idx++) {
        Ulang_type curr = vec_at(ulang_type.ulang_types, idx);
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(
            mod_path,
            curr,
            include_scope
        )));
    }
    return name_new(mod_path, string_to_strv(name), (Ulang_type_vec) {0}, 0, (Attrs) {0});
}

Name serialize_ulang_type_regular(Strv mod_path, Ulang_type_regular ulang_type, bool include_scope) {
    return name_new(mod_path, serialize_ulang_type_atom(ulang_type.atom, include_scope, ulang_type.pos), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

Name serialize_ulang_type_gen_param(Strv mod_path) {
    return name_new(mod_path, sv("gen_param"), (Ulang_type_vec) {0}, 0, (Attrs) {0});
}

Name serialize_ulang_type_removed(Strv mod_path) {
    return name_new(mod_path, sv("removed"), (Ulang_type_vec) {0}, 0, (Attrs) {0});
}

Strv serialize_ulang_type_vec(Strv mod_path, Ulang_type_vec vec, bool include_scope) {
    String name = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(
            mod_path,
            vec_at(vec, idx),
            include_scope
        )));
    }
    return string_to_strv(name);
}

Name serialize_ulang_type(Strv mod_path, Ulang_type ulang_type, bool include_scope) {
    switch (ulang_type.type) {
        case ULANG_TYPE_REGULAR:
            return serialize_ulang_type_regular(mod_path, ulang_type_regular_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_FN:
            return serialize_ulang_type_fn(mod_path, ulang_type_fn_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_TUPLE:
            return serialize_ulang_type_tuple(mod_path, ulang_type_tuple_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_REMOVED:
            return serialize_ulang_type_removed(mod_path);
        case ULANG_TYPE_ARRAY:
            return serialize_ulang_type_array(mod_path, ulang_type_array_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_EXPR: {
            // TODO: consider if all Ulang_type_exprs should be removed before doing actual type checking?
            Ulang_type inner = {0};
            if (!uast_expr_to_ulang_type(&inner, ulang_type_expr_const_unwrap(ulang_type).expr)) {
                return util_literal_name_new_poison();
            }
            return serialize_ulang_type(mod_path, inner, include_scope);
        }
        case ULANG_TYPE_CONST_EXPR:
            return serialize_ulang_type_const_expr(
                mod_path,
                ulang_type_const_expr_const_unwrap(ulang_type)
            );
    }
    unreachable("");
}

