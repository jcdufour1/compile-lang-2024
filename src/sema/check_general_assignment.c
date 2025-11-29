#include <check_general_assignment.h>
#include <type_checking.h>
#include <lang_type_after.h>
#include <tast_utils.h>
#include <msg.h>

// TODO: rename this function?
static bool do_implicit_convertions_primitive(
    Lang_type_primitive dest,
    Tast_expr** new_src,
    Tast_expr* src,
    bool src_is_zero,
    bool implicit_pointer_depth
) {
    *new_src = src;
    Lang_type src_type_ = tast_expr_get_lang_type(*new_src);
    Lang_type_primitive src_type = lang_type_primitive_const_unwrap(src_type_);

    if (!implicit_pointer_depth) {
        if (lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, src_type) != lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, dest)) {
            return false;
        }
    }

    if (!lang_type_primitive_is_number(dest) || !lang_type_primitive_is_number(src_type)) {
        return false;
    }

    uint32_t dest_bit_width = lang_type_primitive_get_bit_width(dest);
    uint32_t src_bit_width = lang_type_primitive_get_bit_width(src_type);

    // both or none of types must be float
    if ((dest.type == LANG_TYPE_FLOAT) != (src_type.type == LANG_TYPE_FLOAT)) {
        return false;
    }

    if (dest.type == LANG_TYPE_UNSIGNED_INT && src_type.type != LANG_TYPE_UNSIGNED_INT) {
        return false;
    }

    if (src_is_zero) {
        return true;
    }

    if (dest.type == LANG_TYPE_SIGNED_INT) {
        unwrap(dest_bit_width > 0);
        dest_bit_width--;
    }
    if (src_type.type == LANG_TYPE_SIGNED_INT) {
        unwrap(src_bit_width > 0);
        src_bit_width--;
    }

    if (dest_bit_width < src_bit_width) {
        return false;
    }

    if ((*new_src)->type == TAST_LITERAL) {
        Tast_literal* lit = tast_literal_unwrap(*new_src);
        switch (lit->type) {
            case TAST_INT:
                tast_int_unwrap(lit)->lang_type = lang_type_primitive_const_wrap(dest);
                return true;
            case TAST_FLOAT:
                tast_float_unwrap(lit)->lang_type = lang_type_primitive_const_wrap(dest);
                return true;
            case TAST_ENUM_LIT:
                msg_todo("", tast_expr_get_pos(*new_src));
                return false;
            case TAST_FUNCTION_LIT:
                goto after;
            case TAST_STRING:
                goto after;
            case TAST_VOID:
                goto after;
            case TAST_ENUM_TAG_LIT:
                goto after;
            case TAST_RAW_UNION_LIT:
                goto after;
        }
        unreachable("");
    }

after:
    *new_src = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
        tast_expr_get_pos(*new_src),
        *new_src,
        UNARY_UNSAFE_CAST,
        lang_type_primitive_const_wrap(dest)
    )));
    return true;
}

static bool should_convert_src_to_cstr(Lang_type dest, Tast_expr** new_src) {
    Lang_type src_type = tast_expr_get_lang_type(*new_src);
    if (dest.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    if (lang_type_primitive_const_unwrap(dest).type != LANG_TYPE_UNSIGNED_INT) {
        return false;
    }
    if (lang_type_get_pointer_depth(dest) != 1) {
        return false;
    }
    if (src_type.type != LANG_TYPE_STRUCT) {
        return false;
    }
    return name_is_equal(lang_type_struct_const_unwrap(src_type).atom.str, name_new(
        MOD_PATH_RUNTIME,
        sv("Slice"),
        ulang_type_gen_args_char_new(),
        SCOPE_TOP_LEVEL,
        (Attrs) {0}
    ));
}

static bool should_convert_src_to_print_format(Lang_type dest, Tast_expr** new_src) {
    if (dest.type != LANG_TYPE_STRUCT) {
        return false;
    }
    Name dest_name = lang_type_struct_const_unwrap(dest).atom.str;
    dest_name.gen_args.info.count = 0;

    if (!strv_is_equal(dest_name.mod_path, MOD_PATH_RUNTIME)) {
        return false;
    }
    if (!strv_is_equal(dest_name.base, sv("PrintFormat"))) {
        return false;
    }

    Lang_type src_type = tast_expr_get_lang_type(*new_src);
    return name_is_equal(lang_type_struct_const_unwrap(src_type).atom.str, name_new(
        MOD_PATH_RUNTIME,
        sv("Slice"),
        ulang_type_gen_args_char_new(),
        SCOPE_TOP_LEVEL,
        (Attrs) {0}
    ));
}

static bool do_src_to_print_format_conversions(Tast_expr** new_src, Tast_expr* src, Lang_type dest) {
    Pos pos = tast_expr_get_pos(src);
    Strv str = tast_string_unwrap(tast_literal_unwrap(src))->data;
    String new_buf = {0};

    for (size_t idx = 0; idx < str.count; idx++) {
        char curr = strv_at(str, idx);
        if (curr == '%') {
            string_extend_cstr(&a_main, &new_buf, "%%");
        } else if (curr == '{') {
            todo();
        } else if (curr == '}') {
            todo();
        } else {
            vec_append(&a_main, &new_buf, curr);
        }
    }

    Tast_expr_vec new_inner_membs = {0};
    //for (size_t idx = 0; idx < lit->members.info.count; idx++) {
    //    todo();
    //    Uast_expr* rhs = vec_at(lit->members, idx);
    //    Tast_expr* new_rhs = NULL;
    //    switch (check_general_assignment(
    //         &check_env,
    //         &new_rhs,
    //         gen_arg,
    //         rhs,
    //         uast_expr_get_pos(rhs)
    //    )) {
    //        case CHECK_ASSIGN_OK:
    //            break;
    //        case CHECK_ASSIGN_INVALID:
    //            msg(
    //                DIAG_ASSIGNMENT_MISMATCHED_TYPES,
    //                uast_expr_get_pos(rhs),
    //                "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
    //                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
    //                lang_type_print(LANG_TYPE_MODE_MSG, gen_arg)
    //            );
    //            return false;
    //        case CHECK_ASSIGN_ERROR:
    //            return false;
    //        default:
    //            unreachable("");
    //    }

    //    vec_append(&a_main, &new_membs, new_rhs);
    //}

    Tast_variable_def_vec inner_def_membs = {0};
    //for (size_t idx = 0; idx < new_membs.info.count; idx++) {
    //    vec_append(&a_main, &inner_def_membs, tast_variable_def_new(
    //        lit->pos,
    //        gen_arg,
    //        false,
    //        util_literal_name_new()
    //    ));
    //}
    Tast_struct_def* inner_def = tast_struct_def_new(
        pos,
        (Struct_def_base) {.members = inner_def_membs, .name = util_literal_name_new()}
    );
    sym_tbl_add(tast_struct_def_wrap(inner_def));

    Tast_struct_literal* new_inner_lit = tast_struct_literal_new(
        pos,
        new_inner_membs,
        util_literal_name_new(),
        lang_type_struct_const_wrap(lang_type_struct_new(pos, lang_type_atom_new(inner_def->base.name, 0)))
    );

    log(LOG_DEBUG, FMT"\n", tast_struct_literal_print(new_inner_lit));
    //todo();

    Ulang_type gen_arg = vec_at(lang_type_struct_const_unwrap(dest).atom.str.gen_args, 0);
    log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, dest));
    log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, gen_arg));
    Lang_type unary_lang_type = lang_type_new_print_format_arg(gen_arg);
    lang_type_set_pointer_depth(&unary_lang_type, lang_type_get_pointer_depth(unary_lang_type) + 1);


    Tast_expr* ptr = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
        pos,
        tast_struct_literal_wrap(new_inner_lit),
        UNARY_REFER,
        unary_lang_type
    )));

    Tast_expr_vec new_slice_membs = {0};
    vec_append(&a_main, &new_slice_membs, ptr);
    vec_append(&a_main, &new_slice_membs, tast_literal_wrap(tast_int_wrap(tast_int_new(pos, (int64_t)new_inner_membs.info.count, lang_type_new_usize()))));

    Lang_type outer_type = lang_type_new_slice(pos, lang_type_to_ulang_type(lang_type_new_print_format_arg(gen_arg)), 0/*TODO*/);
    Tast_expr_vec new_membs = {0};
    vec_append(&a_main, &new_membs, tast_literal_wrap(tast_string_wrap(tast_string_new(pos, string_to_strv(new_buf), false))));
    vec_append(&a_main, &new_membs, tast_struct_literal_wrap(tast_struct_literal_new(
        pos,
        new_slice_membs,
        util_literal_name_new(),
        outer_type
    )));
    Tast_struct_def* new_src_def = tast_struct_def_new(
        pos,
        (Struct_def_base) {.members = (Tast_variable_def_vec) {0}, .name = util_literal_name_new()}
    );
    sym_tbl_add(tast_struct_def_wrap(new_src_def));

    // TODO: move this to lang_type_new_slice function?
    LANG_TYPE_TYPE dummy = {0};
    Ulang_type result = {0};
    if (!resolve_generics_ulang_type_regular(&dummy, &result, ulang_type_regular_const_unwrap(lang_type_to_ulang_type(outer_type)))) {
        todo();
    }

    *new_src = tast_struct_literal_wrap(tast_struct_literal_new(
        pos,
        new_membs,
        new_src_def->base.name,
        lang_type_new_print_format(gen_arg)
    ));

    msg_todo("formatting", tast_expr_get_pos(*new_src))
    return false;
}

bool do_implicit_convertions(
    Lang_type dest,
    Tast_expr** new_src,
    Tast_expr* src,
    bool src_is_zero,
    bool implicit_pointer_depth
) {
    log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, dest));

    Lang_type src_type = tast_expr_get_lang_type(*new_src);
    *new_src = src;
    if (should_convert_src_to_cstr(dest, new_src)) {
        tast_string_unwrap(tast_literal_unwrap(*new_src))->is_cstr = true;
        return true;
    }

    if (should_convert_src_to_print_format(dest, new_src)) {
        return do_src_to_print_format_conversions(new_src, src, dest);
    }

    log(LOG_DEBUG, FMT" "FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, dest), lang_type_print(LANG_TYPE_MODE_LOG, src_type));
    if (dest.type != src_type.type) {
        return false;
    }

    switch (src_type.type) {
        case LANG_TYPE_FN:
            return false;
        case LANG_TYPE_TUPLE:
            return false;
        case LANG_TYPE_PRIMITIVE:
            return do_implicit_convertions_primitive(
                lang_type_primitive_const_unwrap(dest),
                new_src,
                *new_src,
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
        case LANG_TYPE_REMOVED:
            return true; // TODO
        case LANG_TYPE_INT:
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

            msg(
                DIAG_ENUM_NON_VOID_CASE_NO_PAR_ON_ASSIGN, tast_expr_get_pos(src),
                "enum case with non-void inner type cannot be assigned without using (); "
                "use `"FMT"()` instead of `"FMT"`\n",
                print_enum_def_member(callee->enum_lang_type, (size_t)callee->tag->data),
                print_enum_def_member(callee->enum_lang_type, (size_t)callee->tag->data)
            );
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (do_implicit_convertions(dest_lang_type, new_src, src, src_is_zero, false)) {
        return CHECK_ASSIGN_OK;
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

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
            check_env->break_type = check_env->lhs_lang_type;
        } else {
            check_env->break_type = lang_type_void_const_wrap(lang_type_void_new(pos));
        }

        if (!try_set_expr_types_internal(new_src, src, false, (Lang_type) {0}, true)) {
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

