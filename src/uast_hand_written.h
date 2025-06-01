#ifndef UAST_HAND_WRITTEN_H
#define UAST_HAND_WRITTEN_H

#include <vecs.h>
#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <lang_type.h>

#define ir_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), tast_print((reg_sym).tast)

typedef struct {
    Uast_generic_param_vec generics;
    Uast_variable_def_vec members;
    Name name;
} Ustruct_def_base;

#define ustruct_def_base_print(base) strv_print(ustruct_def_base_print_internal(base, 0))

#endif // UAST_HAND_WRITTEN_H
