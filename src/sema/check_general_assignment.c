#include <check_general_assignment.h>
#include <type_checking.h>
#include <lang_type_after.h>
#include <tast_utils.h>
#include <msg.h>

static bool can_be_implicitly_converted_lang_type_primitive(Lang_type_primitive dest, Lang_type_primitive src, bool src_is_zero, bool implicit_pointer_depth) {
    if (!implicit_pointer_depth) {
        if (lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, src) != lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, dest)) {
            return false;
        }
    }

    if (!lang_type_primitive_is_number(dest) || !lang_type_primitive_is_number(src)) {
        return false;
    }

    int32_t dest_bit_width = lang_type_primitive_get_bit_width(dest);
    int32_t src_bit_width = lang_type_primitive_get_bit_width(src);

    // both or none of types must be float
    if ((dest.type == LANG_TYPE_FLOAT) != (src.type == LANG_TYPE_FLOAT)) {
        return false;
    }

    if (dest.type == LANG_TYPE_UNSIGNED_INT && src.type != LANG_TYPE_UNSIGNED_INT) {
        return false;
    }

    if (src_is_zero) {
        return true;
    }

    if (dest.type == LANG_TYPE_SIGNED_INT) {
        unwrap(dest_bit_width > 0);
        dest_bit_width--;
    }
    if (src.type == LANG_TYPE_SIGNED_INT) {
        unwrap(src_bit_width > 0);
        src_bit_width--;
    }

    return dest_bit_width >= src_bit_width;
}

static bool can_be_implicitly_converted_tuple(Lang_type_tuple dest, Lang_type_tuple src, bool implicit_pointer_depth) {
    (void) implicit_pointer_depth;
    if (dest.lang_types.info.count != src.lang_types.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < dest.lang_types.info.count; idx++) {
        if (!can_be_implicitly_converted(
             vec_at(dest.lang_types, idx),
             vec_at(src.lang_types, idx),
             false,
             implicit_pointer_depth
        )) {
            return false;
        }
    }

    return true;
}

static bool can_be_implicitly_converted_fn(Lang_type_fn dest, Lang_type_fn src, bool implicit_pointer_depth) {
    if (!can_be_implicitly_converted_tuple(dest.params, src.params, implicit_pointer_depth)) {
        return false;
    }
    return can_be_implicitly_converted(*dest.return_type, *src.return_type, false, implicit_pointer_depth);
}

// TODO: this function should also actually do the implicit conversion I think
bool can_be_implicitly_converted(Lang_type dest, Lang_type src, bool src_is_zero, bool implicit_pointer_depth) {
    if (dest.type != LANG_TYPE_PRIMITIVE) {
        goto next;
    }
    if (lang_type_primitive_const_unwrap(dest).type != LANG_TYPE_UNSIGNED_INT) {
        goto next;
    }
    if (lang_type_get_pointer_depth(dest) != 1) {
        goto next;
    }
    if (src.type != LANG_TYPE_STRUCT) {
        goto next;
    }
    if (!name_is_equal(lang_type_struct_const_unwrap(src).atom.str, name_new(MOD_PATH_RUNTIME, sv("Slice"), ulang_type_gen_args_char_new(), SCOPE_TOP_LEVEL))) {
        goto next;
    }
    return true;

next:
    if (dest.type != src.type) {
        return false;
    }

    switch (src.type) {
        case LANG_TYPE_FN:
            return can_be_implicitly_converted_fn(lang_type_fn_const_unwrap(dest), lang_type_fn_const_unwrap(src), implicit_pointer_depth);
        case LANG_TYPE_TUPLE:
            return can_be_implicitly_converted_tuple(lang_type_tuple_const_unwrap(dest), lang_type_tuple_const_unwrap(src), implicit_pointer_depth);
        case LANG_TYPE_PRIMITIVE:
            return can_be_implicitly_converted_lang_type_primitive(
                lang_type_primitive_const_unwrap(dest),
                lang_type_primitive_const_unwrap(src),
                src_is_zero,
                implicit_pointer_depth
            );
        case LANG_TYPE_ENUM:
            return false; // TODO
        case LANG_TYPE_STRUCT:
            return false; // TODO
        case LANG_TYPE_RAW_UNION:
            return false; // TODO
        case LANG_TYPE_VOID:
            return true;
        case LANG_TYPE_ARRAY:
            return false; // TODO
    }
    unreachable("");
}

typedef enum {
    IMPLICIT_CONV_INVALID_TYPES,
    IMPLICIT_CONV_CONVERTED,
    IMPLICIT_CONV_OK,
} IMPLICIT_CONV_STATUS;

static CHECK_ASSIGN_STATUS check_general_assignment_finish(
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    bool src_is_zero,
    Tast_expr* src
) {
    if (lang_type_is_equal(dest_lang_type, tast_expr_get_lang_type(src))) {
        if (src->type == TAST_ENUM_CALLEE) {
            Tast_enum_callee* callee = tast_enum_callee_unwrap(src);
            Uast_def* enum_def_ = NULL;
            unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, callee->enum_lang_type)));
            Ustruct_def_base enum_def = uast_enum_def_unwrap(enum_def_)->base;

            // TODO: make helper function to print member more easily
            msg(
                DIAG_ENUM_NON_VOID_CASE_NO_PAR_ON_ASSIGN, tast_expr_get_pos(src),
                "enum case with non-void inner type cannot be assigned without using (); "
                "use `"FMT"."FMT"()` instead of `"FMT"."FMT"`\n",
                lang_type_print(LANG_TYPE_MODE_MSG, callee->enum_lang_type),
                name_print(NAME_MSG, vec_at(enum_def.members, (size_t)callee->tag->data)->name),
                lang_type_print(LANG_TYPE_MODE_MSG, callee->enum_lang_type),
                name_print(NAME_MSG, vec_at(enum_def.members, (size_t)callee->tag->data)->name)
            );
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(dest_lang_type, tast_expr_get_lang_type(src), src_is_zero, false)) {
        if (src->type == TAST_LITERAL) {
            *new_src = src;
            tast_expr_set_lang_type(*new_src, dest_lang_type);
            return CHECK_ASSIGN_OK;
        }
        msg_todo("non literal implicit conversion", tast_expr_get_pos(src));
        return CHECK_ASSIGN_ERROR;
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

// TODO: make src/sema/check_general_assignment.c, and also put can_be_implicitly_converted in there?
CHECK_ASSIGN_STATUS check_general_assignment(
    Type_checking_env* check_env,
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    Uast_expr* src,
    Pos pos
) {
    if (src->type == UAST_STRUCT_LITERAL) {
        Tast_struct_literal* new_src_ = NULL;
        if (!try_set_struct_literal_types(
             &new_src_,
             dest_lang_type,
             uast_struct_literal_unwrap(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_struct_literal_wrap(new_src_);
    } else if (src->type == UAST_ARRAY_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        if (!try_set_array_literal_types(
             &new_src_,
             dest_lang_type,
             uast_array_literal_unwrap(src),
             pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_expr_unwrap(new_src_);
    } else if (src->type == UAST_TUPLE) {
        Tast_tuple* new_src_ = NULL;
        if (!try_set_tuple_assignment_types(
             &new_src_,
             dest_lang_type,
             uast_tuple_unwrap(src)
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_tuple_wrap(new_src_);
    } else {
        Lang_type old_lhs_lang_type = check_env->lhs_lang_type;
        Lang_type old_break_type = check_env->break_type;
        PARENT_OF old_parent_of = check_env->parent_of;
        check_env->parent_of = PARENT_OF_ASSIGN_RHS;
        check_env->lhs_lang_type = dest_lang_type;
        if (check_env->parent_of == PARENT_OF_ASSIGN_RHS) {
            if (check_env->lhs_lang_type.type == LANG_TYPE_PRIMITIVE && strv_is_equal(lang_type_get_str(LANG_TYPE_MODE_LOG, check_env->lhs_lang_type).base, sv("u8"))) {
                //todo();
            }
            check_env->break_type = check_env->lhs_lang_type;
        } else {
            check_env->break_type = lang_type_void_const_wrap(lang_type_void_new(pos));
        }

        if (!try_set_expr_types(new_src, src)) {
            check_env->lhs_lang_type = old_lhs_lang_type;
            check_env->break_type = old_break_type;
            check_env->parent_of = old_parent_of;
            return CHECK_ASSIGN_ERROR;
        }
        check_env->lhs_lang_type = old_lhs_lang_type;
        check_env->break_type = old_break_type;
        check_env->parent_of = old_parent_of;
    }

    bool src_is_zero = false;
    if (src->type == UAST_LITERAL && uast_literal_unwrap(src)->type == UAST_INT && uast_int_unwrap(uast_literal_unwrap(src))->data == 0) {
        src_is_zero = true;
    }

    return check_general_assignment_finish(new_src, dest_lang_type, src_is_zero, *new_src);
}

