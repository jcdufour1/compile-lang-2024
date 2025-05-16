#ifndef NEWSIZEOF_H
#define NEWSIZEOF_H

#include <tast.h>
#include <llvm.h>

uint64_t sizeof_lang_type(Lang_type lang_type);

uint64_t sizeof_item(const Tast* item);

uint64_t sizeof_struct(const Tast* struct_literal);

uint64_t sizeof_struct_def_base(const Struct_def_base* base);

uint64_t sizeof_struct_literal(const Tast_struct_literal* struct_literal);

uint64_t llvm_sizeof_item(const Llvm* item);

uint64_t sizeof_expr(const Tast_expr* expr);

uint64_t sizeof_def(const Tast_def* def);

uint64_t llvm_sizeof_struct_def_base(const Struct_def_base* base);

uint64_t llvm_sizeof_struct_expr(const Llvm_expr* struct_literal_or_def);

#endif // NEWSIZEOF_H
