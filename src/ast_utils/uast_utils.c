#include <uast_utils.h>
#include <str_and_num_utils.h>
#include <lang_type_new_convenience.h>

bool uast_def_get_lang_type(Lang_type* result, const Uast_def* def, Ulang_type_vec generics, Pos dest_pos) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            if (generics.info.count > 0) {
                msg(
                    DIAG_INVALID_COUNT_GENERIC_ARGS,
                    dest_pos,
                    "generic arguments were not expected here\n"
                );
                return false;
            }
            return try_lang_type_from_ulang_type(result,  uast_variable_def_const_unwrap(def)->lang_type);
        case UAST_FUNCTION_DECL:
            return try_lang_type_from_ulang_type(result, uast_function_decl_const_unwrap(def)->return_type);
        case UAST_PRIMITIVE_DEF:
            *result = uast_primitive_def_const_unwrap(def)->lang_type;
            return true;
        case UAST_STRUCT_DEF:
            fallthrough;
        case UAST_RAW_UNION_DEF:
            fallthrough;
        case UAST_ENUM_DEF: {
            Ulang_type ulang_type = {0};
            if (!ustruct_def_base_get_lang_type_(&ulang_type, uast_def_get_struct_def_base(def), generics, uast_def_get_pos(def))) {
                return false;
            }
            return try_lang_type_from_ulang_type(result, ulang_type);
        }
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("");
        case UAST_BUILTIN_DEF:
            unreachable("");
        case UAST_VOID_DEF:
            *result = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN, 0));
            return true;
        case UAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec a_genrgs, Pos pos) {
    Uname base_name = name_to_uname(base.name);
    base_name.a_genrgs = a_genrgs;
    LANG_TYPE_TYPE type = {0};
    return resolve_generics_ulang_type_regular(
        &type,
        result,
        ulang_type_regular_new(pos, ulang_type_atom_new(base_name, 0))
    );
}

Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl) {
    Ulang_type_vec params = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        vec_append(&a_main, &params, vec_at(decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* return_type = arena_alloc(&a_main, sizeof(*return_type));
    *return_type = decl->return_type;
    Ulang_type_fn fn = ulang_type_fn_new(decl->pos, ulang_type_tuple_new(decl->pos, params, 0), return_type, 1/*TODO*/);
    return ulang_type_fn_const_wrap(fn);
}

#pragma GCC diagnostic ignored "-Wswitch-enum"

// will print error on failure
bool util_try_uast_literal_new_from_strv(Uast_expr** new_lit, const Strv value, TOKEN_TYPE token_type, Pos pos) {
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            int64_t raw = 0;
            if (!try_strv_to_int64_t(&raw,  pos, value)) {
                return false;
            }
            Uast_int* literal = uast_int_new(pos, raw);
            *new_lit = uast_literal_wrap(uast_int_wrap(literal));
            break;
        }
        case TOKEN_FLOAT_LITERAL: {
            double raw = 0;
            if (!try_strv_to_double(&raw,  pos, value)) {
                return false;
            }
            Uast_float* literal = uast_float_new(pos, raw);
            *new_lit = uast_literal_wrap(uast_float_wrap(literal));
            break;
        }
        case TOKEN_STRING_LITERAL: {
            Uast_string* string = uast_string_new(pos, value);
            *new_lit = uast_literal_wrap(uast_string_wrap(string));
            break;
        }
        case TOKEN_VOID: {
            Uast_void* lang_void = uast_void_new(pos);
            *new_lit = uast_literal_wrap(uast_void_wrap(lang_void));
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            char raw = '\0';
            if (!try_strv_to_char(&raw, pos, value)) {
                return false;
            }
            *new_lit = uast_literal_wrap(uast_int_wrap(uast_int_new(pos, raw)));
            break;
        }
        default:
            msg_todo("", pos);
    }

    unwrap(*new_lit);
    return true;
}

#pragma GCC diagnostic warning "-Wswitch-enum"

Uast_expr* util_uast_literal_new_from_strv(const Strv value, TOKEN_TYPE token_type, Pos pos) {
    Uast_expr* lit = NULL;
    unwrap(util_try_uast_literal_new_from_strv(&lit,  value, token_type, pos));
    return lit;
}

#pragma GCC diagnostic ignored "-Wswitch-enum"

Uast_expr* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Uast_expr* new_lit = NULL;

    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Uast_int* literal = uast_int_new(pos, value);
            literal->data = value;
            new_lit = uast_literal_wrap(uast_int_wrap(literal));
            break;
        }
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_VOID: {
            Uast_void* literal = uast_void_new(pos);
            new_lit = uast_literal_wrap(uast_void_wrap(literal));
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            assert(value < INT8_MAX);
            new_lit = uast_literal_wrap(uast_int_wrap(uast_int_new(pos, value)));
            break;
        }
        default:
            msg_todo("", pos);
    }

    return new_lit;
}

#pragma GCC diagnostic warning "-Wswitch-enum"

Uast_literal* util_uast_literal_new_from_double(double value, Pos pos) {
    Uast_literal* lit = uast_float_wrap(uast_float_new(pos, value));
    return lit;
}

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child) {
    Uast_binary* binary = uast_binary_new(
        uast_expr_get_pos(if_cond_child),
        util_uast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, uast_expr_get_pos(if_cond_child)),
        if_cond_child,
        BINARY_NOT_EQUAL
    );

    return uast_binary_wrap(binary);
}

Strv print_enum_def_member_internal(Lang_type enum_def_lang_type, size_t memb_idx) {
    String buf = {0};

    Uast_def* enum_def_ = NULL;
    unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, enum_def_lang_type)));
    Ustruct_def_base enum_def = uast_enum_def_unwrap(enum_def_)->base;

    string_extend_f(
        &a_temp,
        &buf,
        FMT"."FMT,
        strv_print(lang_type_print_internal(LANG_TYPE_MODE_MSG, enum_def_lang_type)),
        strv_print(name_print_internal(NAME_MSG, false, vec_at(enum_def.members, memb_idx)->name))
    );

    return string_to_strv(buf);
}

