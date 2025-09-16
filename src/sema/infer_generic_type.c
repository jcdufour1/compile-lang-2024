
#include <infer_generic_type.h>
#include <lang_type_print.h>
#include <lang_type_from_ulang_type.h>

bool infer_generic_type(
    Ulang_type* infered,
    Lang_type arg_to_infer_from,
    Uast_variable_def* param_corres_to_arg,
    Name name_to_infer
) {
    log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, arg_to_infer_from));
    log(LOG_DEBUG, FMT"\n", uast_variable_def_print(param_corres_to_arg));
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name_to_infer));

    switch (param_corres_to_arg->lang_type.type) {
        case ULANG_TYPE_REGULAR:
            todo();
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_GEN_PARAM:
            if (!strv_is_equal(param_corres_to_arg->name.base, name_to_infer.base)) {
                return false;
            }
            *infered = lang_type_to_ulang_type(arg_to_infer_from);
            log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, *infered));
            return true;
    }
    unreachable("");
}
