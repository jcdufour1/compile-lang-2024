#include <did_you_mean.h>
#include <symbol_iter.h>
#include <uast.h>
#include <uast_utils.h>

uint32_t levenshtein_dist(Strv lhs, Strv rhs) {
    if (rhs.count < 1) {
        return lhs.count;
    }
    if (lhs.count < 1) {
        return rhs.count;
    }

    Strv tail_lhs = strv_slice(lhs, 1, lhs.count - 1);
    Strv tail_rhs = strv_slice(rhs, 1, rhs.count - 1);

    if (strv_at(lhs, 0) == strv_at(rhs, 0)) {
        return levenshtein_dist(tail_lhs, tail_rhs);
    }

    return 1 + min(levenshtein_dist(tail_lhs, rhs), min(levenshtein_dist(lhs, tail_rhs), levenshtein_dist(tail_lhs, tail_rhs)));
}

Strv did_you_mean_symbol_print_internal(Name sym_name) {
    Name_vec candidates = {0};

    Scope_id curr_scope = sym_name.scope_id;
    while (1) {
        Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            if (!strv_is_equal(curr_name.mod_path, sv("main")) || !strv_is_equal(curr_name.base, sv("UndefinedType1"))) {
                continue;
            }
            if (levenshtein_dist(curr_name.base, sym_name.base) < 2) {
                vec_append(&a_temp, &candidates, curr_name);
            }
            log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, curr_name));
        }

        if (curr_scope == SCOPE_TOP_LEVEL) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }

    if (candidates.info.count < 1) {
        return sv("");
    }

    String buf = {0};
    string_extend_cstr(&a_temp, &buf, "; did you mean:");
    vec_foreach(idx, Name, candidate, candidates) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, &buf, ",");
        }
        string_extend_f(&a_temp, &buf, " "FMT, name_print(NAME_MSG, candidate));
    }
    return string_to_strv(buf);
}
