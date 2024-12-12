#ifndef NODE_DEF_VEC_H
#define NODE_DEF_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Node_def_;

typedef struct Node_def_ Node_def;

typedef struct {
    Vec_base info;
    Node_def** buf;
} Node_def_vec;


#endif // NODE_DEF_VEC_H
