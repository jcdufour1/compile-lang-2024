#include <check_gen_constraints.h>

bool check_gen_constraints(Uast_generic_param_vec gen_params, Ulang_type_vec a_genrgs) {
    return true;
    (void) gen_params;
    (void) a_genrgs;
    vec_foreach(idx, Ulang_type, a_genrg, a_genrgs) {
        (void) a_genrg;
        Uast_generic_param* gen_param = vec_at(gen_params, idx);

        if (!gen_param->is_expr) {
            continue;
        }

        //log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, a_genrg));
        //log(LOG_DEBUG, "%u\n", a_genrg.type);

        //todo();
    }

    return true;
}
