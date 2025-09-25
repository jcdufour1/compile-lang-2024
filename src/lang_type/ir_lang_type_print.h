#ifndef IR_LANG_TYPE_PRINT_H
#define IR_LANG_TYPE_PRINT_H

#include <env.h>
#include <newstring.h>
#include <strv.h>
#include <lang_type_mode.h>

#define ir_lang_type_print(mode, ir_lang_type) strv_print(ir_lang_type_print_internal((mode), (ir_lang_type)))

#define ir_lang_type_atom_print(mode, atom) strv_print(ir_lang_type_atom_print_internal(atom, mode))

Strv ir_lang_type_print_internal(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type);

Strv ir_lang_type_atom_print_internal(Ir_lang_type_atom atom, LANG_TYPE_MODE mode);

void extend_serialize_ir_lang_type_to_string(String* string, Ir_lang_type ir_lang_type, bool do_tag);

void extend_ir_lang_type_atom(String* string, LANG_TYPE_MODE mode, Ir_lang_type_atom atom);

void extend_ir_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type);

#endif // IR_LANG_TYPE_PRINT_H
