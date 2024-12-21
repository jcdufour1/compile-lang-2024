#ifndef LLVM_DEF_VEC_H
#define LLVM_DEF_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Llvm_def_;

typedef struct Llvm_def_ Llvm_def;

typedef struct {
    Vec_base info;
    Llvm_def** buf;
} Llvm_def_vec;


#endif // LLVM_DEF_VEC_H
