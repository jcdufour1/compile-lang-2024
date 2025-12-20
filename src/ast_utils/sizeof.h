#ifndef NEWSIZEOF_H
#define NEWSIZEOF_H

#include <tast.h>
#include <ir.h>

uint64_t sizeof_lang_type(Lang_type lang_type);

uint64_t sizeof_ir_lang_type(Ir_lang_type lang_type);

uint64_t sizeof_item(const Tast* item);

uint64_t sizeof_struct(const Tast* struct_literal);

uint64_t sizeof_struct_def_base(const Struct_def_base* base, bool is_sum_type);

uint64_t alignof_struct_def_base(const Struct_def_base* base);

uint64_t sizeof_struct_literal(const Tast_struct_literal* struct_literal);

uint64_t ir_sizeof_item(const Ir* item);

uint64_t sizeof_def(const Tast_def* def);

uint64_t sizeof_ir_lang_type(Ir_lang_type lang_type);

uint64_t ir_sizeof_struct_def_base(const Struct_def_base* base);

uint64_t sizeof_def(const Tast_def* def);

uint64_t alignof_def(const Tast_def* def);

uint64_t sizeof_array_def(const Tast_array_def* def);

uint64_t alignof_array_def(const Tast_array_def* def);

#endif // NEWSIZEOF_H
