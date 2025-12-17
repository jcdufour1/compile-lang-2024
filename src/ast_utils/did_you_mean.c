#include <did_you_mean.h>
#include <symbol_iter.h>
#include <uast.h>
#include <uast_utils.h>

typedef struct {
    Vec_base info;
    uint32_t* buf;
} Uint32_t_vec;

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

    static Uint32_t_vec prev = {0};
    static Uint32_t_vec curr = {0};
    vec_reserve(&a_leak, &prev, m + 1);
    vec_reserve(&a_leak, &curr, m + 1);
    {
        vec_foreach_ref(idx, uint32_t, item, prev) {
            *item = 0;
        }
    }
    {
        vec_foreach_ref(idx, uint32_t, item, curr) {
            *item = 0;
        }
    }
    while (prev.info.count < m + 1) {
        vec_append(&a_leak, &prev, 0);
    }
    while (curr.info.count < m + 1) {
        vec_append(&a_leak, &curr, 0);
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

    uint32_t result = vec_at(prev, m);
    vec_reset(&prev);
    vec_reset(&curr);
    return result;
}

typedef struct {
    Name name;
    uint32_t difference;
} Candidate;

typedef struct {
    Vec_base info;
    Candidate* buf;
} Candidate_vec;

static Candidate candidate_new(Name name, uint32_t difference) {
    return (Candidate) {.name = name, .difference = difference};
}

int candidate_compare(const void* lhs_, const void* rhs_) {
    const Candidate* lhs = lhs_;
    const Candidate* rhs = rhs_;

    if (lhs->difference < rhs->difference) {
        return QSORT_LESS_THAN;
    }
    if (lhs->difference > rhs->difference) {
        return QSORT_MORE_THAN;
    }

    bool lhs_is_local = strv_is_equal(lhs->name.mod_path, sym_mod_path);
    bool rhs_is_local = strv_is_equal(rhs->name.mod_path, sym_mod_path);
    if (lhs_is_local == rhs_is_local) {
        assert(!strv_is_equal(lhs->name.base, rhs->name.base));
        return strv_cmp(lhs->name.base, rhs->name.base) < 0 ? QSORT_LESS_THAN : QSORT_MORE_THAN;
    }
    return lhs_is_local ? QSORT_LESS_THAN : QSORT_MORE_THAN;
}

typedef bool(*Is_correct_sym_type)(UAST_DEF_TYPE);

static Strv did_you_mean_print_common(Name sym_name, Is_correct_sym_type is_correct_sym_type_fn) {
    sym_mod_path = sym_name.mod_path;

    assert(levenshtein_distance(sv("na"), sv("a")) == 1);
    assert(levenshtein_distance(sv("na"), sv("n")) == 1);
    assert(levenshtein_distance(sv("a"), sv("na")) == 1);
    assert(levenshtein_distance(sv("n"), sv("na")) == 1);
    assert(levenshtein_distance(sv("na"), sv("na")) == 0);

    Candidate_vec candidates = {0};

    Scope_id curr_scope = sym_name.scope_id;
    while (1) {
        Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            if (!is_correct_sym_type_fn(curr->type)) {
                continue;
            }

            uint32_t max_difference = strv_is_equal(curr_name.mod_path, sym_name.mod_path) ? 3 : 1;
            uint32_t result = levenshtein_distance(curr_name.base, sym_name.base);
            if (result <= max_difference) {
                vec_append(&a_temp, &candidates, candidate_new(curr_name, result));
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

    qsort(candidates.buf, candidates.info.count, sizeof(candidates.buf[0]), candidate_compare);

    String buf = {0};
    string_extend_cstr(&a_temp, &buf, "; did you mean:");
    vec_foreach(idx, Candidate, candidate, candidates) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, &buf, ",");
        }
        string_extend_f(&a_temp, &buf, " "FMT, name_print(NAME_MSG, candidate.name));
    }
    return string_to_strv(buf);
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
