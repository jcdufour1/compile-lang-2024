#ifndef LLVM_PTR_VEC_H
#define LLVM_PTR_VEC_H

#include <stddef.h>
#include "vector.h"
#include <assert.h>

#define LLVM_PTR_VEC_DEFAULT_CAPACITY 1

struct Llvm_;
typedef struct Llvm_ Llvm;

typedef struct {
    Vec_base info;
    Llvm** buf;
} Llvm_ptr_vec;

#endif // LLVM_PTR_VEC_H
