#ifndef IR_LANG_TYPE_PRINT_H
#define IR_LANG_TYPE_PRINT_H

#include <env.h>
#include <local_string.h>
#include <strv.h>
#include <ir_lang_type.h>

#define ir_lang_type_print(mode, ir_lang_type) strv_print(ir_lang_type_print_internal((mode), (ir_lang_type)))

Strv ir_lang_type_print_internal(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type);

void extend_serialize_ir_lang_type_to_string(String* string, Ir_lang_type ir_lang_type, bool do_tag);

void extend_ir_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type);

#endif // IR_LANG_TYPE_PRINT_H
