#ifndef EMIT_LLVM_H
#define EMIT_LLVM_H

#include "node.h"
#include "env.h"

void emit_llvm_from_tree(Env* env, const Node_block* root);

#endif // EMIT_LLVM_H
