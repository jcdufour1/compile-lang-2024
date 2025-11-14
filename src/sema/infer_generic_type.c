
#include <infer_generic_type.h>
#include <lang_type_print.h>
#include <lang_type_from_ulang_type.h>
#include <str_and_num_utils.h>
#include <uast_expr_to_ulang_type.h>
#include <ulang_type_remove_expr.h>

bool infer_generic_type(
    Ulang_type* infered,
    Ulang_type arg_to_infer_from,
    bool arg_to_infer_is_lit,
    Ulang_type param_corres_to_arg,
    Name name_to_infer,
    Pos pos_arg
) {
    // TODO: make helper function to do this lang_type conversion (to normalize integer bit widths)

    Lang_type temp_arg = {0};
    if (try_lang_type_from_ulang_type(&temp_arg, arg_to_infer_from)) {
        arg_to_infer_from = lang_type_to_ulang_type(lang_type_standardize(temp_arg, arg_to_infer_is_lit, pos_arg));
    }
    if (try_lang_type_from_ulang_type(&temp_arg, arg_to_infer_from)) {
    }

    //log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, arg_to_infer_from));
    //log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, param_corres_to_arg));
    //log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name_to_infer));

    switch (param_corres_to_arg.type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular reg = ulang_type_regular_const_unwrap(param_corres_to_arg);
            if (strv_is_equal(reg.atom.str.base, name_to_infer.base)) {
                if (reg.atom.str.gen_args.info.count > 0 || name_to_infer.gen_args.info.count > 0) {
                    // TODO
                    return false;
                }
                *infered = arg_to_infer_from;
                int16_t new_ptr_depth = ulang_type_get_pointer_depth(*infered) - reg.atom.pointer_depth;
                if (new_ptr_depth < 0) {
                    // TODO
                    msg_todo("error message for this situation", pos_arg);
                    return false;
                }
                ulang_type_set_pointer_depth(infered, new_ptr_depth);
                return true;
            }

            if (arg_to_infer_from.type == ULANG_TYPE_REGULAR) {
                for (
                    size_t idx = 0;
                    idx < min(
                        reg.atom.str.gen_args.info.count,
                        ulang_type_regular_const_unwrap(arg_to_infer_from).atom.str.gen_args.info.count
                    );
                    idx++
                ) {
                    if (infer_generic_type(
                        infered,
                        vec_at(ulang_type_regular_const_unwrap(arg_to_infer_from).atom.str.gen_args, idx),
                        false,
                        vec_at(reg.atom.str.gen_args, idx),
                        name_to_infer,
                        pos_arg
                    )) {
                        return true;
                    }
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
        case ULANG_TYPE_REMOVED:
            // TODO
            return false;
        case ULANG_TYPE_EXPR: {
            // TODO: this may be inefficient, because infer_generic_type could be called
            //   multiple times with the same param_corres_to_arg for the same function call
            //   (ulang_type_remove_expr could be called multiple times)

            Ulang_type inner = {0};
            if (!ulang_type_remove_expr(&inner, param_corres_to_arg)) {
                return false;
            }

            return infer_generic_type(
                infered,
                arg_to_infer_from,
                arg_to_infer_is_lit,
                inner,
                name_to_infer,
                pos_arg
            );
        }
        case ULANG_TYPE_INT:
            // TODO
            return false;
    }
    unreachable("");
}
