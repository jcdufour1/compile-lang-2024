#include <uast_utils.h>
#include <str_and_num_utils.h>
#include <lang_type_new_convenience.h>

bool uast_def_get_lang_type(Lang_type* result, const Uast_def* def, Ulang_type_darr generics, Pos dest_pos) {
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

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_darr gen_args, Pos pos) {
    Uname base_name = name_to_uname(base.name);
    base_name.gen_args = gen_args;
    LANG_TYPE_TYPE type = {0};
    return resolve_generics_ulang_type_regular(
        &type,
        result,
        ulang_type_regular_new(pos, base_name, 0)
    );
}

Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl) {
    Ulang_type_darr params = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        darr_append(&a_main, &params, darr_at(decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* return_type = arena_alloc(&a_main, sizeof(*return_type));
    *return_type = decl->return_type;
    Ulang_type_fn fn = ulang_type_fn_new(decl->pos, ulang_type_tuple_new(decl->pos, params, 0), return_type, 1/*TODO*/);
    return ulang_type_fn_const_wrap(fn);
}

// will print error on failure
bool util_try_uast_literal_new_from_strv(Uast_expr** new_lit, const Strv value, TOKEN_TYPE token_type, Pos pos) {
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            int64_t raw = 0;
            if (!try_strv_to_int64_t(&raw, pos, value)) {
                return false;
            }
            Uast_int* literal = uast_int_new(pos, raw);
            *new_lit = uast_literal_wrap(uast_int_wrap(literal));
            break;
        }
        case TOKEN_FLOAT_LITERAL: {
            double raw = 0;
            if (!try_strv_to_double(&raw, pos, value)) {
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
        case TOKEN_NONTYPE:
            unreachable("");
        case TOKEN_SINGLE_PLUS:
            unreachable("");
        case TOKEN_SINGLE_MINUS:
            unreachable("");
        case TOKEN_ASTERISK:
            unreachable("");
        case TOKEN_MODULO:
            unreachable("");
        case TOKEN_SLASH:
            unreachable("");
        case TOKEN_LESS_THAN:
            unreachable("");
        case TOKEN_LESS_OR_EQUAL:
            unreachable("");
        case TOKEN_GREATER_THAN:
            unreachable("");
        case TOKEN_GREATER_OR_EQUAL:
            unreachable("");
        case TOKEN_DOUBLE_EQUAL:
            unreachable("");
        case TOKEN_LOGICAL_NOT_EQUAL:
            unreachable("");
        case TOKEN_BITWISE_AND:
            unreachable("");
        case TOKEN_BITWISE_OR:
            unreachable("");
        case TOKEN_BITWISE_XOR:
            unreachable("");
        case TOKEN_LOGICAL_AND:
            unreachable("");
        case TOKEN_LOGICAL_OR:
            unreachable("");
        case TOKEN_SHIFT_LEFT:
            unreachable("");
        case TOKEN_SHIFT_RIGHT:
            unreachable("");
        case TOKEN_LOGICAL_NOT:
            unreachable("");
        case TOKEN_BITWISE_NOT:
            unreachable("");
        case TOKEN_UNSAFE_CAST:
            unreachable("");
        case TOKEN_ORELSE:
            unreachable("");
        case TOKEN_QUESTION_MARK:
            unreachable("");
        case TOKEN_OPEN_PAR:
            unreachable("");
        case TOKEN_OPEN_CURLY_BRACE:
            unreachable("");
        case TOKEN_OPEN_SQ_BRACKET:
            unreachable("");
        case TOKEN_OPEN_GENERIC:
            unreachable("");
        case TOKEN_CLOSE_PAR:
            unreachable("");
        case TOKEN_CLOSE_CURLY_BRACE:
            unreachable("");
        case TOKEN_CLOSE_SQ_BRACKET:
            unreachable("");
        case TOKEN_CLOSE_GENERIC:
            unreachable("");
        case TOKEN_ENUM:
            unreachable("");
        case TOKEN_SYMBOL:
            unreachable("");
        case TOKEN_DOUBLE_QUOTE:
            unreachable("");
        case TOKEN_NEW_LINE:
            unreachable("");
        case TOKEN_COMMA:
            unreachable("");
        case TOKEN_COLON:
            unreachable("");
        case TOKEN_SINGLE_EQUAL:
            unreachable("");
        case TOKEN_SINGLE_DOT:
            unreachable("");
        case TOKEN_DOUBLE_DOT:
            unreachable("");
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_EOF:
            unreachable("");
        case TOKEN_ASSIGN_BY_BIN:
            unreachable("");
        case TOKEN_MACRO:
            unreachable("");
        case TOKEN_DEFER:
            unreachable("");
        case TOKEN_DOUBLE_TICK:
            unreachable("");
        case TOKEN_ONE_LINE_BLOCK_START:
            unreachable("");
        case TOKEN_UNDERSCORE:
            unreachable("");
        case TOKEN_FN:
            unreachable("");
        case TOKEN_FOR:
            unreachable("");
        case TOKEN_IF:
            unreachable("");
        case TOKEN_SWITCH:
            unreachable("");
        case TOKEN_CASE:
            unreachable("");
        case TOKEN_DEFAULT:
            unreachable("");
        case TOKEN_ELSE:
            unreachable("");
        case TOKEN_RETURN:
            unreachable("");
        case TOKEN_EXTERN:
            unreachable("");
        case TOKEN_STRUCT:
            unreachable("");
        case TOKEN_LET:
            unreachable("");
        case TOKEN_IN:
            unreachable("");
        case TOKEN_BREAK:
            unreachable("");
        case TOKEN_YIELD:
            unreachable("");
        case TOKEN_CONTINUE:
            unreachable("");
        case TOKEN_RAW_UNION:
            unreachable("");
        case TOKEN_TYPE_DEF:
            unreachable("");
        case TOKEN_GENERIC_TYPE:
            unreachable("");
        case TOKEN_IMPORT:
            unreachable("");
        case TOKEN_DEF:
            unreachable("");
        case TOKEN_SIZEOF:
            unreachable("");
        case TOKEN_COUNTOF:
            unreachable("");
        case TOKEN_USING:
            unreachable("");
        case TOKEN_COMMENT:
            unreachable("");
        case TOKEN_COUNT:
            unreachable("");
        case TOKEN_AT_SIGN:
            unreachable("");
        default:
            msg_todo("", pos);
    }

    unwrap(*new_lit);
    return true;
}

Uast_expr* util_uast_literal_new_from_strv(const Strv value, TOKEN_TYPE token_type, Pos pos) {
    Uast_expr* lit = NULL;
    unwrap(util_try_uast_literal_new_from_strv(&lit,  value, token_type, pos));
    return lit;
}

Uast_expr* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Uast_expr* new_lit = NULL;

    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Uast_int* literal = uast_int_new(pos, value);
            literal->data = value;
            new_lit = uast_literal_wrap(uast_int_wrap(literal));
            break;
        }
        case TOKEN_FLOAT_LITERAL: {
            Uast_float* literal = uast_float_new(pos, value);
            literal->data = value;
            new_lit = uast_literal_wrap(uast_float_wrap(literal));
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
        case TOKEN_NONTYPE:
            unreachable("");
        case TOKEN_SINGLE_PLUS:
            unreachable("");
        case TOKEN_SINGLE_MINUS:
            unreachable("");
        case TOKEN_ASTERISK:
            unreachable("");
        case TOKEN_MODULO:
            unreachable("");
        case TOKEN_SLASH:
            unreachable("");
        case TOKEN_LESS_THAN:
            unreachable("");
        case TOKEN_LESS_OR_EQUAL:
            unreachable("");
        case TOKEN_GREATER_THAN:
            unreachable("");
        case TOKEN_GREATER_OR_EQUAL:
            unreachable("");
        case TOKEN_DOUBLE_EQUAL:
            unreachable("");
        case TOKEN_LOGICAL_NOT_EQUAL:
            unreachable("");
        case TOKEN_BITWISE_AND:
            unreachable("");
        case TOKEN_BITWISE_OR:
            unreachable("");
        case TOKEN_BITWISE_XOR:
            unreachable("");
        case TOKEN_LOGICAL_AND:
            unreachable("");
        case TOKEN_LOGICAL_OR:
            unreachable("");
        case TOKEN_SHIFT_LEFT:
            unreachable("");
        case TOKEN_SHIFT_RIGHT:
            unreachable("");
        case TOKEN_LOGICAL_NOT:
            unreachable("");
        case TOKEN_BITWISE_NOT:
            unreachable("");
        case TOKEN_UNSAFE_CAST:
            unreachable("");
        case TOKEN_ORELSE:
            unreachable("");
        case TOKEN_QUESTION_MARK:
            unreachable("");
        case TOKEN_OPEN_PAR:
            unreachable("");
        case TOKEN_OPEN_CURLY_BRACE:
            unreachable("");
        case TOKEN_OPEN_SQ_BRACKET:
            unreachable("");
        case TOKEN_OPEN_GENERIC:
            unreachable("");
        case TOKEN_CLOSE_PAR:
            unreachable("");
        case TOKEN_CLOSE_CURLY_BRACE:
            unreachable("");
        case TOKEN_CLOSE_SQ_BRACKET:
            unreachable("");
        case TOKEN_CLOSE_GENERIC:
            unreachable("");
        case TOKEN_ENUM:
            unreachable("");
        case TOKEN_SYMBOL:
            unreachable("");
        case TOKEN_DOUBLE_QUOTE:
            unreachable("");
        case TOKEN_NEW_LINE:
            unreachable("");
        case TOKEN_COMMA:
            unreachable("");
        case TOKEN_COLON:
            unreachable("");
        case TOKEN_SINGLE_EQUAL:
            unreachable("");
        case TOKEN_SINGLE_DOT:
            unreachable("");
        case TOKEN_DOUBLE_DOT:
            unreachable("");
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_EOF:
            unreachable("");
        case TOKEN_ASSIGN_BY_BIN:
            unreachable("");
        case TOKEN_MACRO:
            unreachable("");
        case TOKEN_DEFER:
            unreachable("");
        case TOKEN_DOUBLE_TICK:
            unreachable("");
        case TOKEN_ONE_LINE_BLOCK_START:
            unreachable("");
        case TOKEN_UNDERSCORE:
            unreachable("");
        case TOKEN_FN:
            unreachable("");
        case TOKEN_FOR:
            unreachable("");
        case TOKEN_IF:
            unreachable("");
        case TOKEN_SWITCH:
            unreachable("");
        case TOKEN_CASE:
            unreachable("");
        case TOKEN_DEFAULT:
            unreachable("");
        case TOKEN_ELSE:
            unreachable("");
        case TOKEN_RETURN:
            unreachable("");
        case TOKEN_EXTERN:
            unreachable("");
        case TOKEN_STRUCT:
            unreachable("");
        case TOKEN_LET:
            unreachable("");
        case TOKEN_IN:
            unreachable("");
        case TOKEN_BREAK:
            unreachable("");
        case TOKEN_YIELD:
            unreachable("");
        case TOKEN_CONTINUE:
            unreachable("");
        case TOKEN_RAW_UNION:
            unreachable("");
        case TOKEN_TYPE_DEF:
            unreachable("");
        case TOKEN_GENERIC_TYPE:
            unreachable("");
        case TOKEN_IMPORT:
            unreachable("");
        case TOKEN_DEF:
            unreachable("");
        case TOKEN_SIZEOF:
            unreachable("");
        case TOKEN_COUNTOF:
            unreachable("");
        case TOKEN_USING:
            unreachable("");
        case TOKEN_COMMENT:
            unreachable("");
        case TOKEN_COUNT:
            unreachable("");
        case TOKEN_AT_SIGN:
            unreachable("");
        default:
            msg_todo("", pos);
    }

    return new_lit;
}

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

UAST_GET_MEMB_DEF uast_try_get_member_def(
    Uast_expr** new_expr,
    Uast_variable_def** member_def,
    const Ustruct_def_base* base,
    Strv member_name,
    Pos dest_pos
) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_variable_def* curr = darr_at(base->members, idx);
        if (strv_is_equal(curr->name.base, member_name)) {
            *member_def = curr;
            return UAST_GET_MEMB_DEF_NORMAL;
        }
    }

    darr_foreach(idx, Uast_generic_param*, gen_param, base->generics) {
        if (gen_param->is_expr && strv_is_equal(member_name, gen_param->name.base)) {
            if (darr_at(base->name.gen_args, idx).type != ULANG_TYPE_LIT) {
                msg_todo("non const expression here", dest_pos);
                return UAST_GET_MEMB_DEF_NONE;
            }

            Ulang_type_lit const_expr = ulang_type_lit_const_unwrap(darr_at(base->name.gen_args, idx));
            switch (const_expr.type) {
                case ULANG_TYPE_INT_LIT: {
                    Ulang_type_int_lit lit = ulang_type_int_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_int_wrap(uast_int_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_FLOAT_LIT: {
                    Ulang_type_float_lit lit = ulang_type_float_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_float_wrap(uast_float_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_STRING_LIT: {
                    Ulang_type_string_lit lit = ulang_type_string_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_string_wrap(uast_string_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_STRUCT_LIT: {
                    Ulang_type_struct_lit lit = ulang_type_struct_lit_const_unwrap(const_expr);
                    Uast_expr* inner = uast_expr_clone(lit.expr, false, 0, lit.pos); // clone
                    unwrap(gen_param->is_expr);
                    *new_expr = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                        lit.pos,
                        inner,
                        UNARY_UNSAFE_CAST,
                        gen_param->expr_lang_type
                    )));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_FN_LIT: {
                    Ulang_type_fn_lit lit = ulang_type_fn_lit_const_unwrap(const_expr);
                    *new_expr = uast_symbol_wrap(uast_symbol_new(lit.pos, lit.name));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                default:
                    unreachable("");
            }
            unreachable("");
        }
    }

    return UAST_GET_MEMB_DEF_NONE;
}

Strv print_enum_def_member_internal(Lang_type enum_def_lang_type, size_t memb_idx) {
    String buf = {0};

    Uast_def* enum_def_ = NULL;
    Name name = {0};
    if (!lang_type_get_name(&name, enum_def_lang_type)) {
        // TODO
        return sv("<none>");
    }
    unwrap(usymbol_lookup(&enum_def_, name));
    Ustruct_def_base enum_def = uast_enum_def_unwrap(enum_def_)->base;

    string_extend_f(
        &a_temp,
        &buf,
        FMT"."FMT,
        strv_print(lang_type_print_internal(LANG_TYPE_MODE_MSG, enum_def_lang_type)),
        strv_print(name_print_internal(NAME_MSG, false, darr_at(enum_def.members, memb_idx)->name))
    );

    return string_to_strv(buf);
}

