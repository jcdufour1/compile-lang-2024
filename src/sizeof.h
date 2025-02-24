#ifndef NEWSIZEOF_H
#define NEWSIZEOF_H

#include <tast.h>
#include <llvm.h>

uint64_t sizeof_lang_type(Env* env, Lang_type lang_type);

uint64_t sizeof_item(Env* env, const Tast* item);

uint64_t sizeof_struct(Env* env, const Tast* struct_literal);

uint64_t sizeof_struct_def_base(Env* env, const Struct_def_base* base);

uint64_t sizeof_struct_literal(Env* env, const Tast_struct_literal* struct_literal);

uint64_t llvm_sizeof_item(Env* env, const Llvm* item);

uint64_t sizeof_expr(Env* env, const Tast_expr* expr);

uint64_t sizeof_def(Env* env, const Tast_def* def);

uint64_t llvm_sizeof_struct_def_base(Env* env, const Struct_def_base* base);

uint64_t llvm_sizeof_struct_expr(Env* env, const Llvm_expr* struct_literal_or_def);

#endif // NEWSIZEOF_H
