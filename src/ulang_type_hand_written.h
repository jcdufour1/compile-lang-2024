#ifndef ULANG_TYPE_HAND_WRITTEN
#define ULANG_TYPE_HAND_WRITTEN

#include <str_view.h>

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Ulang_type_atom;

struct Ulang_type_;
typedef struct Ulang_type_ Ulang_type;

typedef struct {
    Vec_base info;
    Ulang_type* buf;
} Ulang_type_vec;

static inline Ulang_type_atom ulang_type_atom_new(Str_view str, int16_t pointer_depth) {
    return (Ulang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Ulang_type_atom ulang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return ulang_type_atom_new(str_view_from_cstr(cstr), pointer_depth);
}

#endif // ULANG_TYPE_HAND_WRITTEN
