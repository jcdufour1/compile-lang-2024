#include <did_you_mean.h>
#include <symbol_iter.h>
#include <uast.h>
#include <uast_utils.h>
#include <misc_print_functions.h>

typedef struct {
    Vec_base info;
    uint32_t* buf;
} Uint32_t_darr;

static Strv sym_mod_path = {0};

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

    static Uint32_t_darr prev = {0};
    static Uint32_t_darr curr = {0};
    darr_reserve(&a_leak, &prev, m + 1);
    darr_reserve(&a_leak, &curr, m + 1);
    {
        darr_foreach_ref(idx, uint32_t, item, prev) {
            *item = 0;
        }
    }
    {
        darr_foreach_ref(idx, uint32_t, item, curr) {
            *item = 0;
        }
    }
    while (prev.info.count < m + 1) {
        darr_append(&a_leak, &prev, 0);
    }
    while (curr.info.count < m + 1) {
        darr_append(&a_leak, &curr, 0);
    }

    {
        darr_foreach_ref(idx, uint32_t, item, prev) {
            *item = idx;
        }
    }

    for (size_t s_idx = 1; s_idx < s.count + 1; s_idx++) {
        *darr_at_ref(&curr, 0) = s_idx;

        for (size_t t_idx = 1; t_idx < t.count + 1; t_idx++) {
            uint32_t difference_cost = 0;
            if (strv_at(s, s_idx - 1) != strv_at(t, t_idx - 1)) {
                difference_cost = 1;
            }

            *darr_at_ref(&curr, t_idx) = levenshtein_min(
                darr_at(curr, t_idx - 1) + 1,
                darr_at(prev, t_idx) + 1,
                darr_at(prev, t_idx - 1) + difference_cost
            );
        }

        Uint32_t_darr temp = prev;
        prev = curr;
        curr = temp;
    }

    uint32_t result = darr_at(prev, m);
    darr_reset(&prev);
    darr_reset(&curr);
    return result;
}

typedef struct {
    Name name;
    uint32_t difference;
} Candidate;

typedef struct {
    Vec_base info;
    Candidate* buf;
} Candidate_darr;

static Candidate candidate_new(Name name, uint32_t difference) {
    return (Candidate) {.name = name, .difference = difference};
}

static int candidate_compare_gen_args(Ulang_type_darr lhs_gen_args, Ulang_type_darr rhs_gen_args);

static int candidate_compare_gen_arg(Ulang_type lhs_gen_arg, Ulang_type rhs_gen_arg) {
    if (lhs_gen_arg.type != rhs_gen_arg.type) {
        return lhs_gen_arg.type > rhs_gen_arg.type ? QSORT_LESS_THAN : QSORT_MORE_THAN;
    }

    if (lhs_gen_arg.type != ULANG_TYPE_REGULAR) {
        return QSORT_EQUAL; // TODO
    }
    Ulang_type_regular lhs_reg = ulang_type_regular_const_unwrap(lhs_gen_arg);
    Ulang_type_regular rhs_reg = ulang_type_regular_const_unwrap(rhs_gen_arg);

    if (!strv_is_equal(lhs_reg.name.base, rhs_reg.name.base)) {
        return strv_cmp(lhs_reg.name.base, rhs_reg.name.base) < 0 ? QSORT_LESS_THAN : QSORT_MORE_THAN;
    }
    return candidate_compare_gen_args(lhs_reg.name.gen_args, rhs_reg.name.gen_args);
}

static int candidate_compare_gen_args(Ulang_type_darr lhs_gen_args, Ulang_type_darr rhs_gen_args) {
    if (lhs_gen_args.info.count != rhs_gen_args.info.count) {
        return lhs_gen_args.info.count < rhs_gen_args.info.count ? QSORT_LESS_THAN : QSORT_MORE_THAN;
    }

    darr_foreach(idx, Ulang_type, lhs_gen_arg, lhs_gen_args) {
        int result = candidate_compare_gen_arg(lhs_gen_arg, darr_at(rhs_gen_args, idx));
        if (result != QSORT_EQUAL) {
            return result;
        }
    }
    return QSORT_EQUAL; // TODO
}

static int candidate_compare(const void* lhs_, const void* rhs_) {
    const Candidate* lhs = lhs_;
    const Candidate* rhs = rhs_;

    log(LOG_NOTE, FMT"\n", name_print(NAME_LOG, lhs->name));
    log(LOG_NOTE, FMT"\n", name_print(NAME_LOG, rhs->name));
    if (lhs->difference < rhs->difference) {
        return QSORT_LESS_THAN;
    }
    if (lhs->difference > rhs->difference) {
        return QSORT_MORE_THAN;
    }

    bool lhs_is_local = strv_is_equal(lhs->name.mod_path, sym_mod_path);
    bool rhs_is_local = strv_is_equal(rhs->name.mod_path, sym_mod_path);
    if (lhs_is_local == rhs_is_local) {
        if (!strv_is_equal(lhs->name.base, rhs->name.base)) {
            return strv_cmp(lhs->name.base, rhs->name.base) < 0 ? QSORT_LESS_THAN : QSORT_MORE_THAN;
        }
        if (!strv_is_equal(lhs->name.mod_path, rhs->name.mod_path)) {
            // TODO: only call strv_replace in ci?
            return strv_cmp(
                strv_replace(&a_temp, lhs->name.mod_path, sv("\\"), sv("/")),
                strv_replace(&a_temp, rhs->name.mod_path, sv("\\"), sv("/"))
            ) < 0 ? QSORT_LESS_THAN : QSORT_MORE_THAN;
        }
        return candidate_compare_gen_args(lhs->name.gen_args, rhs->name.gen_args);
    }
    return lhs_is_local ? QSORT_LESS_THAN : QSORT_MORE_THAN;
}

typedef bool(*Is_correct_sym_type)(UAST_DEF_TYPE);

static Strv did_you_mean_print_common_finish(Candidate_darr candidates) {
    if (candidates.info.count < 1) {
        return sv("");
    }

    assert(candidates.info.count > 0 && candidates.buf && "qsort expected non-null pointer");
    qsort(candidates.buf, candidates.info.count, sizeof(candidates.buf[0]), candidate_compare);

    String buf = {0};
    string_extend_cstr(&a_temp, &buf, "; did you mean:");
    darr_foreach(idx, Candidate, candidate, candidates) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, &buf, ",");
        }
        string_extend_f(&a_temp, &buf, " "FMT, name_print(NAME_MSG, candidate.name));
    }
    return string_to_strv(buf);
}

static Strv did_you_mean_print_common(Name sym_name, Is_correct_sym_type is_correct_sym_type_fn) {
    sym_mod_path = sym_name.mod_path;

    assert(levenshtein_distance(sv("na"), sv("a")) == 1);
    assert(levenshtein_distance(sv("na"), sv("n")) == 1);
    assert(levenshtein_distance(sv("a"), sv("na")) == 1);
    assert(levenshtein_distance(sv("n"), sv("na")) == 1);
    assert(levenshtein_distance(sv("na"), sv("na")) == 0);

    Candidate_darr candidates = {0};

    Scope_id curr_scope = sym_name.scope_id;
    while (1) {
        // TODO: consider if levenshtein_distance calculations should be cached 
        //   in case multiple did you mean errors are printed
        Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            if (!is_correct_sym_type_fn(curr->type)) {
                continue;
            }
            if (curr_name.gen_args.info.count > 0 && sym_name.gen_args.info.count == 0) {
                continue;
            }

            uint32_t max_difference = strv_is_equal(curr_name.mod_path, sym_name.mod_path) ? 3 : 1;
            uint32_t result = levenshtein_distance(curr_name.base, sym_name.base);
            if (result <= max_difference) {
                darr_append(&a_temp, &candidates, candidate_new(curr_name, result));
            }
        }

        if (curr_scope == SCOPE_TOP_LEVEL) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }

    return did_you_mean_print_common_finish(candidates);

}

static bool local_is_struct_like(UAST_DEF_TYPE type) {
    switch (type) {
        case UAST_LABEL:
            return false;
        case UAST_VOID_DEF:
            return false;
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            return false;
        case UAST_MOD_ALIAS:
            return false;
        case UAST_GENERIC_PARAM:
            return false;
        case UAST_FUNCTION_DEF:
            return false;
        case UAST_VARIABLE_DEF:
            return false;
        case UAST_STRUCT_DEF:
            return true;
        case UAST_RAW_UNION_DEF:
            return true;
        case UAST_ENUM_DEF:
            return true;
        case UAST_LANG_DEF:
            // TODO: return true if def expands to struct like?
            return false;
        case UAST_PRIMITIVE_DEF:
            return false;
        case UAST_FUNCTION_DECL:
            return false;
        case UAST_BUILTIN_DEF:
            return false;
        default:
            unreachable("");
    }
    unreachable("");
}

Strv did_you_mean_type_print_internal(Name sym_name) {
    return did_you_mean_print_common(sym_name, local_is_struct_like);
}

static bool is_symbol(UAST_DEF_TYPE type) {
    switch (type) {
        case UAST_LABEL:
            return true;
        case UAST_VOID_DEF:
            return false;
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            return false;
        case UAST_MOD_ALIAS:
            return true;
        case UAST_GENERIC_PARAM:
            return true;
        case UAST_FUNCTION_DEF:
            return true;
        case UAST_VARIABLE_DEF:
            return true;
        case UAST_STRUCT_DEF:
            return false;
        case UAST_RAW_UNION_DEF:
            return false;
        case UAST_ENUM_DEF:
            return false;
        case UAST_LANG_DEF:
            // TODO: return true if def expands to symbol?
            return false;
        case UAST_PRIMITIVE_DEF:
            return false;
        case UAST_FUNCTION_DECL:
            return true;
        case UAST_BUILTIN_DEF:
            return false;
        default:
            unreachable("");
    }
    unreachable("");
}

Strv did_you_mean_symbol_print_internal(Name sym_name) {
    return did_you_mean_print_common(sym_name, is_symbol);
}

Strv did_you_mean_strv_choice_print_internal(Strv sym_strv, Strv_darr candidates_strv) {
    Candidate_darr candidates = {0};
    darr_foreach(idx, Strv, curr_strv, candidates_strv) {
        uint32_t difference = levenshtein_distance(sym_strv, curr_strv);
        if (difference <= 5) {
            darr_append(&a_temp, &candidates, candidate_new(
                name_new(
                    MOD_PATH_BUILTIN,
                    curr_strv,
                    (Ulang_type_darr) {0},
                    SCOPE_TOP_LEVEL
                ),
                difference
            ));
        }
    }

    return did_you_mean_print_common_finish(candidates);
}
