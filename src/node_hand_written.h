#ifndef NODE_HAND_WRITTEN_H
#define NODE_HAND_WRITTEN_H

#include <node_ptr_vec.h>
#include <symbol_table_struct.h>

typedef size_t Llvm_id;

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type;

typedef struct {
    Lang_type lang_type;
    struct Node_* node;
} Llvm_register_sym;

#endif // NODE_HAND_WRITTEN_H
