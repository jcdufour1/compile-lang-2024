#include <did_you_mean.h>
#include <symbol_iter.h>
#include <uast.h>
#include <uast_utils.h>

typedef struct {
    Vec_base info;
    uint32_t* buf;
} Uint32_t_vec;

static Arena a_did_you_mean = {0};

uint32_t levenshtein_min(uint32_t a, uint32_t b, uint32_t c) {
    return min(a, min(b, c));
}

uint32_t levenshtein_dist(Strv s, Strv t) {
    log(LOG_DEBUG, "entering levenshtein_dist\n");
    //if (t.count < s.count) {
    //    Strv temp_strv = s;
    //    s = t;
    //    t = temp_strv;
    //}

    size_t n = s.count;
    size_t m = t.count;
    if (n == 0) {
        return m;
    }
    if (m == 0) {
        return n;
    }

    Uint32_t_vec prev = {0};
    Uint32_t_vec curr = {0};
    vec_reserve(&a_did_you_mean, &prev, m + 1);
    vec_reserve(&a_did_you_mean, &curr, m + 1);
    for (size_t idx = 0; idx < m + 1; idx++) {
        vec_append(&a_did_you_mean, &prev, 0);
        vec_append(&a_did_you_mean, &curr, 0);
    }

    {
        vec_foreach_ref(idx, uint32_t, item, prev) {
            *item = idx;
        }
    }

    //for (size_t idx = 0; idx < n; idx++) {
    //    *vec_at_ref(&prev, idx) = idx;
    //}

    for (size_t s_idx = 1; s_idx < s.count + 1; s_idx++) {
        *vec_at_ref(&curr, 0) = s_idx;

        log(LOG_DEBUG, "s_idx = %zu\n", s_idx);
        for (size_t t_idx = 1; t_idx < t.count + 1; t_idx++) {
            {
                String vecs = {0};
                string_extend_f(&a_temp, &vecs, "s_idx = %zu, t_idx = %zu: prev = [", s_idx, t_idx);
                {
                    vec_foreach(idx_thing, uint32_t, item, prev) {
                        if (idx_thing > 0) {
                            string_extend_cstr(&a_temp, &vecs, ", ");
                        }
                        string_extend_f(&a_temp, &vecs, "%"PRIu32, item);
                    }
                }
                string_extend_cstr(&a_temp, &vecs, "]; curr = [");
                {
                    vec_foreach(idx_thing, uint32_t, item, curr) {
                        if (idx_thing > 0) {
                            string_extend_cstr(&a_temp, &vecs, ", ");
                        }
                        string_extend_f(&a_temp, &vecs, "%"PRIu32, item);
                    }
                }
                string_extend_cstr(&a_temp, &vecs, "]");
                log(LOG_DEBUG, FMT"\n", string_print(vecs));
            }

            uint32_t difference_cost = 0;
            if (strv_at(s, s_idx - 1) != strv_at(t, t_idx - 1)) {
                difference_cost = 1;
            }

            //log(LOG_DEBUG, "t_idx = %zu\n", t_idx);
            //log(LOG_DEBUG, "vec_at(prev, t_idx - 1) = %"PRIu32"\n", vec_at(prev, t_idx - 1));
            //log(LOG_DEBUG, "curr[%zu] will == %"PRIu32"\n", t_idx, levenshtein_min(
            //    vec_at(curr, t_idx - 1) + 1,
            //    vec_at(prev, t_idx) + 1,
            //    vec_at(prev, t_idx - 1) + difference_cost
            //));
            //log(LOG_DEBUG, "curr[t_idx] will == %"PRIu32"\n", levenshtein_min(
            //    vec_at(curr, t_idx - 1) + 1,
            //    vec_at(prev, t_idx) + 1,
            //    vec_at(prev, t_idx - 1) + difference_cost
            //));
            //
            log(LOG_DEBUG, "thing 1: %"PRIu32"; thing 2: %"PRIu32"; thing 3: %"PRIu32"\n", vec_at(curr, t_idx - 1) + 1, vec_at(prev, t_idx) + 1, vec_at(prev, t_idx - 1) + difference_cost);
            log(LOG_DEBUG, "thing 3.5: %"PRIu32"\n", levenshtein_min(
                vec_at(curr, t_idx - 1) + 1,
                vec_at(prev, t_idx) + 1,
                vec_at(prev, t_idx - 1) + difference_cost
            ));

            *vec_at_ref(&curr, t_idx) = levenshtein_min(
                vec_at(curr, t_idx - 1) + 1,
                vec_at(prev, t_idx) + 1,
                vec_at(prev, t_idx - 1) + difference_cost
            );

            {
                String vecs = {0};
                string_extend_f(&a_temp, &vecs, "s_idx = %zu, t_idx = %zu: prev = [", s_idx, t_idx);
                {
                    vec_foreach(idx_thing, uint32_t, item, prev) {
                        if (idx_thing > 0) {
                            string_extend_cstr(&a_temp, &vecs, ", ");
                        }
                        string_extend_f(&a_temp, &vecs, "%"PRIu32, item);
                    }
                }
                string_extend_cstr(&a_temp, &vecs, "]; curr = [");
                {
                    vec_foreach(idx_thing, uint32_t, item, curr) {
                        if (idx_thing > 0) {
                            string_extend_cstr(&a_temp, &vecs, ", ");
                        }
                        string_extend_f(&a_temp, &vecs, "%"PRIu32, item);
                    }
                }
                string_extend_cstr(&a_temp, &vecs, "]");
                log(LOG_DEBUG, FMT"\n", string_print(vecs));
            }
            log(LOG_DEBUG, "thing 4: %"PRIu32"\n", vec_at(curr, t_idx));
        }

        Uint32_t_vec temp = prev;
        prev = curr;
        curr = temp;
    }



    log(LOG_DEBUG, FMT" "FMT"\n", strv_print(s), strv_print(t));
    log(LOG_DEBUG, "m = %zu\n", m);
    assert(vec_at(prev, m) > 0);
    return vec_at(prev, m);
}

///*uint32_t levenshtein_dist(char s[1..m], char t[1..n]) {*/
//uint32_t levenshtein_dist(Strv s, Strv t) {
//    // for all i and j, d[i,j] will hold the Levenshtein distance between
//    // the first i characters of s and the first j characters of t
//    Uint32_t_vec d = {0};
//
//    vec_reserve(&a_did_you_mean, &d, (s.count + 1)*(t.count + 1));
//    for (size_t idx = 0; idx < (s.count + 1)*(t.count + 1); idx++) {
//        vec_append(&a_did_you_mean, &d, 0);
//    }
// 
//    // set each element in d to zero
// 
//    // source prefixes can be transformed into empty string by
//    // dropping all characters
//    for (size_t idx = 1; idx < s.count + 1; idx++) {
//        *vec_at_ref(&d, idx) = idx;
//        //d[i, 0] := i
//    }
// 
//    // target prefixes can be reached from empty source prefix
//    // by inserting every character
//    for (size_t j = 1; j < t.count + 1; j++) {
//        *vec_at_ref(&d, (s.count + 1)*j) = j;
//    }
// 
//    for (size_t j = 1; j < t.count/* + 1*/; j++) {
//        for (size_t i = 1; i < s.count/* + 1*/; i++) {
//            uint32_t substitution_cost = 1;
//            if (strv_at(s, i - 1) == strv_at(t, j - 1)) {
//                substitution_cost = 0;
//            }
//
//            log(LOG_DEBUG, "thing: %zu len: %zu\n", (s.count + 1)*(i) + (j - 1), d.info.count);
//            *vec_at_ref(&d, (s.count + 1)*i + j) = levenshtein_min(
//                vec_at(d, (s.count + 1)*(i - 1) + j) + 1, // insertion
//                vec_at(d, (s.count + 1)*(i) + (j - 1)) + 1,                   // deletion
//                vec_at(d, (s.count + 1)*(i - 1) + (j - 1)) + substitution_cost  // substitution
//            );                  // insertion
//            //d[i, j] := minimum(d[i-1, j] + 1,                   // deletion
//            //                   d[i, j-1] + 1,                   // insertion
//            //                   d[i-1, j-1] + substitutionCost)  // substitution
//        }
//    }
// 
//
//    arena_reset(&a_did_you_mean);
//    return vec_last(d);
//    //return d[m, n]
//}

uint32_t levenshtein_dist_old(Strv lhs, Strv rhs) {
    if (rhs.count < 1) {
        return lhs.count;
    }
    if (lhs.count < 1) {
        return rhs.count;
    }

    Strv tail_lhs = strv_slice(lhs, 1, lhs.count - 1);
    Strv tail_rhs = strv_slice(rhs, 1, rhs.count - 1);

    if (strv_at(lhs, 0) == strv_at(rhs, 0)) {
        return levenshtein_dist_old(tail_lhs, tail_rhs);
    }

    return 1 + min(levenshtein_dist_old(tail_lhs, rhs), min(levenshtein_dist_old(lhs, tail_rhs), levenshtein_dist_old(tail_lhs, tail_rhs)));
}

Strv did_you_mean_symbol_print_internal(Name sym_name) {
    assert(levenshtein_dist(sv("na"), sv("a")) == 1);
    assert(levenshtein_dist(sv("na"), sv("n")) == 1);

    Name_vec candidates = {0};

    Scope_id curr_scope = sym_name.scope_id;
    while (1) {
        Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            if (!strv_is_equal(curr_name.mod_path, sv("main")) || !strv_is_equal(curr_name.base, sv("UndefinedType1"))) {
                //continue;
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
