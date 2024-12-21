#ifndef LLVM_IF_PTR_VEC_H
#define LLVM_IF_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

struct Llvm_if_;

typedef struct Llvm_if_ Llvm_if;

typedef struct {
    Vec_base info;
    Llvm_if** buf;
} Llvm_if_ptr_vec;

#endif // LLVM_IF_PTR_VEC_H
