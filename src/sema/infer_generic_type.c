
#include <infer_generic_type.h>
#include <lang_type_print.h>
#include <lang_type_from_ulang_type.h>
#include <str_and_num_utils.h>

bool bit_width_calculation(uint32_t* new_width, uint32_t old_width, Pos pos_arg) {
    if (old_width <= 32) {
        *new_width = 32;
        return true;
    }

    if (old_width <= 64) {
        *new_width = 64;
        return true;
    }

    // TODO
    msg(DIAG_GEN_INFER_MORE_THAN_64_WIDE, pos_arg, "bit widths larger than 64 for type inference in generics is not yet implemented\n");
    return false;
}

bool infer_generic_type(
    Ulang_type* infered,
    Lang_type arg_to_infer_from,
    bool arg_to_infer_is_lit,
    Uast_variable_def* param_corres_to_arg,
    Name name_to_infer,
    Pos pos_arg
) {
    // TODO: make helper function to do this lang_type conversion (to normalize integer bit widths)
    if (arg_to_infer_is_lit && arg_to_infer_from.type == LANG_TYPE_PRIMITIVE) {
        Lang_type_primitive primitive = lang_type_primitive_const_unwrap(arg_to_infer_from);
        switch (primitive.type) {
            case LANG_TYPE_SIGNED_INT: {
                Lang_type_signed_int sign = lang_type_signed_int_const_unwrap(primitive);
                if (!bit_width_calculation(&sign.bit_width, sign.bit_width, pos_arg)) {
                    return false;
                }
                arg_to_infer_from = lang_type_primitive_const_wrap(
                    lang_type_signed_int_const_wrap(sign)
                );
            } break;
            case LANG_TYPE_UNSIGNED_INT: {
                Lang_type_unsigned_int unsign = lang_type_unsigned_int_const_unwrap(primitive);
                Lang_type_signed_int sign = lang_type_signed_int_new(
                    pos_arg,
                    unsign.bit_width,
                    unsign.pointer_depth
                );
                if (!bit_width_calculation(&sign.bit_width, sign.bit_width, pos_arg)) {
                    return false;
                }
                arg_to_infer_from = lang_type_primitive_const_wrap(
                    lang_type_signed_int_const_wrap(sign)
                );
            } break;
            case LANG_TYPE_FLOAT: {
                Lang_type_float lang_float = lang_type_float_const_unwrap(primitive);
                if (!bit_width_calculation(&lang_float.bit_width, lang_float.bit_width, pos_arg)) {
                    return false;
                }
                arg_to_infer_from = lang_type_primitive_const_wrap(
                    lang_type_float_const_wrap(lang_float)
                );
            } break;
            case LANG_TYPE_OPAQUE:
                break;
            default:
                unreachable("");
        }
    }

    //log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, arg_to_infer_from));
    //log(LOG_DEBUG, FMT"\n", uast_variable_def_print(param_corres_to_arg));
    //log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name_to_infer));

    switch (param_corres_to_arg->lang_type.type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular reg = ulang_type_regular_const_unwrap(param_corres_to_arg->lang_type);
            if (strv_is_equal(reg.atom.str.base, name_to_infer.base)) {
                //log(LOG_DEBUG, FMT"\n", strv_print(reg.atom.str.base));
                if (reg.atom.str.gen_args.info.count > 0 || name_to_infer.gen_args.info.count > 0) {
                    // TODO
                    return false;
                }
                *infered = lang_type_to_ulang_type(arg_to_infer_from);
                int16_t new_ptr_depth = ulang_type_get_pointer_depth(*infered) - reg.atom.pointer_depth;
                if (new_ptr_depth < 0) {
                    // TODO
                    msg_todo("error message for this situation", pos_arg);
                    return false;
                }
                ulang_type_set_pointer_depth(infered, new_ptr_depth);
                return true;
            }

            for (size_t idx = 0; idx < min(reg.atom.str.gen_args.info.count, lang_type_get_str(LANG_TYPE_MODE_LOG, arg_to_infer_from).gen_args.info.count); idx++) {
                if (infer_generic_type(
                    infered,
                    lang_type_from_ulang_type(vec_at(lang_type_get_str(LANG_TYPE_MODE_LOG, arg_to_infer_from).gen_args, idx)),
                    false,
                    uast_variable_def_new(pos_arg /* TODO */, vec_at(reg.atom.str.gen_args, idx), util_literal_name_new()),
                    name_to_infer,
                    pos_arg
                )) {
                    return true;
                }
            }

            return false;
        }
        case ULANG_TYPE_TUPLE:
            // TODO
            return false;
        case ULANG_TYPE_FN:
            // TODO
            return false;
        case ULANG_TYPE_GEN_PARAM:
            // TODO
            return false;
        case ULANG_TYPE_ARRAY:
            // TODO
            return false;
        case ULANG_TYPE_EXPR:
            todo();
        case ULANG_TYPE_INT:
            todo();
    }
    unreachable("");
}
