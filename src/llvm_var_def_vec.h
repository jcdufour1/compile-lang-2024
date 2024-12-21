#ifndef LLVM_VAR_DEF_VEC_H
#define LLVM_VAR_DEF_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Llvm_variable_def_;
typedef struct Llvm_variable_def_ Llvm_variable_def;

typedef struct {
    Vec_base info;
    Llvm_variable_def** buf;
} Llvm_var_def_vec;


#endif // LLVM_VAR_DEF_VEC_H
