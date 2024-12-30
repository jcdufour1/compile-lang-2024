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
