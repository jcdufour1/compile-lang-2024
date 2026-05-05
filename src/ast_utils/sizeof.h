#ifndef NEWSIZEOF_H
#define NEWSIZEOF_H

#include <tast.h>
#include <ir.h>

static inline Bytes bits_to_bytes(Bits bit_width) {
    return bytes_new((bit_width.value + 7)/8);
}

Bytes sizeof_lang_type(Lang_type lang_type);

Bytes sizeof_ir_lang_type(Ir_lang_type lang_type);

Bytes sizeof_item(const Tast* item);

Bytes sizeof_struct(const Tast* struct_literal);

Bytes sizeof_struct_def_base(const Struct_def_base* base, bool is_sum_type);

Bytes alignof_struct_def_base(const Struct_def_base* base);

Bytes sizeof_struct_literal(const Tast_struct_literal* struct_literal);

Bytes ir_sizeof_item(const Ir* item);

Bytes sizeof_def(const Tast_def* def);

Bytes sizeof_ir_lang_type(Ir_lang_type lang_type);

Bytes ir_sizeof_struct_def_base(const Struct_def_base* base);

Bytes sizeof_def(const Tast_def* def);

Bytes alignof_def(const Tast_def* def);

Bytes sizeof_array_def(const Tast_array_def* def);

Bytes alignof_array_def(const Tast_array_def* def);

#endif // NEWSIZEOF_H
