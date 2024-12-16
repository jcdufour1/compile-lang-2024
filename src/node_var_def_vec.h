#ifndef NODE_VAR_DEF_VEC_H
#define NODE_VAR_DEF_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Node_variable_def_;

typedef struct Node_variable_def_ Node_variable_def;

typedef struct {
    Vec_base info;
    Node_variable_def** buf;
} Node_var_def_vec;


#endif // NODE_VAR_DEF_VEC_H
