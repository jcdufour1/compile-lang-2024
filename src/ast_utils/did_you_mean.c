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

// idea for this algorithm from:
//   https://www.codeproject.com/articles/Fast-memory-efficient-Levenshtein-algorithm-2
uint32_t levenshtein_distance(Strv s, Strv t) {
    size_t n = s.count;
    size_t m = t.count;
    if (n < 1) {
        return m;
    }
    if (m < 1) {
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

    for (size_t s_idx = 1; s_idx < s.count + 1; s_idx++) {
        *vec_at_ref(&curr, 0) = s_idx;

        for (size_t t_idx = 1; t_idx < t.count + 1; t_idx++) {
            uint32_t difference_cost = 0;
            if (strv_at(s, s_idx - 1) != strv_at(t, t_idx - 1)) {
                difference_cost = 1;
            }

            *vec_at_ref(&curr, t_idx) = levenshtein_min(
                vec_at(curr, t_idx - 1) + 1,
                vec_at(prev, t_idx) + 1,
                vec_at(prev, t_idx - 1) + difference_cost
            );
        }

        Uint32_t_vec temp = prev;
        prev = curr;
        curr = temp;
    }

    return vec_at(prev, m);
}

Strv did_you_mean_symbol_print_internal(Name sym_name) {
    assert(levenshtein_distance(sv("na"), sv("a")) == 1);
    assert(levenshtein_distance(sv("na"), sv("n")) == 1);
    assert(levenshtein_distance(sv("a"), sv("na")) == 1);
    assert(levenshtein_distance(sv("n"), sv("na")) == 1);
    assert(levenshtein_distance(sv("na"), sv("na")) == 0);

    Name_vec candidates = {0};

    Scope_id curr_scope = sym_name.scope_id;
    while (1) {
        Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            switch (curr->type) {
                case UAST_LABEL:
                    continue;
                case UAST_VOID_DEF:
                    continue;
                case UAST_POISON_DEF:
                    continue;
                case UAST_IMPORT_PATH:
                    continue;
                case UAST_MOD_ALIAS:
                    continue;
                case UAST_GENERIC_PARAM:
                    continue;
                case UAST_FUNCTION_DEF:
                    continue;
                case UAST_VARIABLE_DEF:
                    continue;
                case UAST_STRUCT_DEF:
                    break;
                case UAST_RAW_UNION_DEF:
                    break;
                case UAST_ENUM_DEF:
                    break;
                case UAST_LANG_DEF:
                    continue;
                case UAST_PRIMITIVE_DEF:
                    continue;
                case UAST_FUNCTION_DECL:
                    continue;
                case UAST_BUILTIN_DEF:
                    continue;
                default:
                    unreachable("");
            }

            uint32_t max_difference = strv_is_equal(curr_name.mod_path, sym_name.mod_path) ? 3 : 1;
            if (levenshtein_distance(curr_name.base, sym_name.base) <= max_difference) {
                vec_append(&a_temp, &candidates, curr_name);
            }
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
