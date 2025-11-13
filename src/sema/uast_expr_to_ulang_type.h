#ifndef UAST_EXPR_TO_ULANG_TYPE_H
#define UAST_EXPR_TO_ULANG_TYPE_H

#include <util.h>
#include <ulang_type.h>
#include <uast.h>

// TODO: move to .c file
typedef enum {
    EXPR_TO_ULANG_TYPE_NORMAL,
    EXPR_TO_ULANG_TYPE_PTR_DEPTH,
    EXPR_TO_ULANG_TYPE_ERROR,

    // for static asserts
    EXPR_TO_ULANG_TYPE_COUNT,
} EXPR_TO_ULANG_TYPE;

bool uast_expr_to_ulang_type_concise(Ulang_type* result, const Uast_expr* expr);

#endif // UAST_EXPR_TO_ULANG_TYPE_H
