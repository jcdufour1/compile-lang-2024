#ifndef UAST_HAND_WRITTEN_H
#define UAST_HAND_WRITTEN_H

#include <darrs.h>
#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <lang_type.h>

#define ir_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), tast_print((reg_sym).tast)

#define ustruct_def_base_print(base) strv_print(ustruct_def_base_print_internal(base, 0))

#endif // UAST_HAND_WRITTEN_H
