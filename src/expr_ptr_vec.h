#ifndef EXPR_PTR_VEC_H
#define EXPR_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Node_Expr_;

typedef struct {
    Vec_base info;
    struct Node_expr_** buf;
} Expr_ptr_vec;

#endif // EXPR_PTR_VEC_H
