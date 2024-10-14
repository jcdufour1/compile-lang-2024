#ifndef NODE_PTR_VEC_H
#define NODE_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

#define NODE_PTR_VEC_DEFAULT_CAPACITY 1

struct Node_;

typedef struct Node_ Node;

typedef struct {
    Vec_base info;
    Node** buf;
} Node_ptr_vec;

#endif // NODE_PTR_VEC_H
