#ifndef TASTS_H
#define TASTS_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include <tast.h>
#include <tast_utils.h>
#include "assert.h"
#include "vector.h"

#define log_tree(log_level, root) \
    do { \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"TAST_FMT, tast_print(root)); \
    } while(0)

static inline Tast* get_left_child_expr(Tast_expr* expr) {
    (void) expr;
    unreachable("");
}

static inline Tast* get_left_child(Tast* tast) {
    switch (tast->type) {
        case TAST_EXPR:
            return get_left_child_expr(tast_unwrap_expr(tast));
        case TAST_LANG_TYPE:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            return tast_wrap_expr(tast_unwrap_for_lower_bound(tast)->child);
        case TAST_FOR_UPPER_BOUND:
            return tast_wrap_expr(tast_unwrap_for_upper_bound(tast)->child);
        case TAST_BREAK:
            unreachable("");
        case TAST_RETURN:
            return tast_wrap_expr(tast_unwrap_return(tast)->child);
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
            unreachable("");
        case TAST_BLOCK:
            unreachable("");
        default:
            unreachable(TAST_FMT, tast_print(tast));
    }
}

static inline const Tast* get_left_child_const(const Tast* tast) {
    return get_left_child((Tast*)tast);
}

//static inline Tast_expr* tast_expr_new(Pos pos, TAST_EXPR_TYPE expr_type) {
//    Tast_expr* expr = tast_unwrap_expr(tast_new(pos, TAST_EXPR));
//    expr->type = expr_type;
//    return expr;
//}

static inline Tast* tast_clone(const Tast* tast_to_clone) {
    (void) tast_to_clone;
    todo();
#if 0
    Tast* new_tast = tast_new(tast_to_clone->pos, tast_to_clone->type);
    *new_tast = *tast_to_clone;
    tasts_reset_links_of_self_only(new_tast, false);
    return new_tast;
#endif // 0
}

#endif // TASTS_H
