#ifndef ULANG_TYPE_HAND_WRITTEN
#define ULANG_TYPE_HAND_WRITTEN

#include <name.h>

typedef struct Ulang_type_atom_ {
    Uname str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Ulang_type_atom;

static inline Ulang_type_atom ulang_type_atom_new(Uname str, int16_t pointer_depth) {
    return (Ulang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Ulang_type_atom ulang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return ulang_type_atom_new(
        uname_new(MOD_ALIAS_BUILTIN, sv(cstr), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
        pointer_depth
    );
}

struct Uast_expr_;
typedef struct Uast_expr_ Uast_expr;

#endif // ULANG_TYPE_HAND_WRITTEN
