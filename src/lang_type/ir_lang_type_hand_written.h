#ifndef IR_LANG_TYPE_HAND_WRITTEN
#define IR_LANG_TYPE_HAND_WRITTEN

#include <strv.h>
#include <vector.h>
#include <ulang_type.h>

typedef struct {
    Name str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Ir_lang_type_atom;

struct Ir_lang_type_;
typedef struct Ir_lang_type_ Ir_lang_type;

typedef struct {
    Vec_base info;
    Ir_lang_type* buf;
} Ir_lang_type_vec;

static inline Ir_lang_type_atom ir_lang_type_atom_new(Name str, int16_t pointer_depth) {
    return (Ir_lang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Ir_lang_type_atom ir_lang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth, Scope_id scope_id) {
    return ir_lang_type_atom_new(
        name_new(MOD_PATH_BUILTIN, sv(cstr), (Ulang_type_vec) {0}, scope_id),
        pointer_depth
    );
}

#endif // IR_LANG_TYPE_HAND_WRITTEN
