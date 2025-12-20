#include <check_gen_constraints.h>

bool check_gen_constraints(Uast_generic_param_darr gen_params, Ulang_type_darr gen_args) {
    return true;
    (void) gen_params;
    (void) gen_args;
    darr_foreach(idx, Ulang_type, gen_arg, gen_args) {
        (void) gen_arg;
        Uast_generic_param* gen_param = darr_at(gen_params, idx);

        if (!gen_param->is_expr) {
            continue;
        }

        //log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, gen_arg));
        //log(LOG_DEBUG, "%u\n", gen_arg.type);

        //todo();
    }

    return true;
}
