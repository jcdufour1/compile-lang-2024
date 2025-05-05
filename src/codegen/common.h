#ifndef COMMON_H
#define COMMON_H

#include <util.h>
#include <llvm.h>

static inline const Llvm_expr* llvm_expr_lookup_from_name(Name name) {
    Llvm* child = NULL;
    unwrap(alloca_lookup(&child, name));
    return llvm_expr_const_unwrap(child);
}

bool is_extern_c(const Llvm* llvm);

void llvm_extend_name(String* output, Name name);

#endif // COMMON_H
