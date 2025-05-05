#ifndef COMMON_H
#define COMMON_H

#include <util.h>
#include <llvm.h>

bool is_extern_c(const Llvm* llvm);

void llvm_extend_name(String* output, Name name);

#endif // COMMON_H
