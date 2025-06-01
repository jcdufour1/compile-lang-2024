#ifndef LANG_TYPE_HAND_WRITTEN
#define LANG_TYPE_HAND_WRITTEN

#include <strv.h>
#include <vector.h>
#include <ulang_type_hand_written.h>

typedef struct {
    Name str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type_atom;

struct Lang_type_;
typedef struct Lang_type_ Lang_type;

typedef struct {
    Vec_base info;
    Lang_type* buf;
} Lang_type_vec;

static inline Lang_type_atom lang_type_atom_new(Name str, int16_t pointer_depth) {
    return (Lang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Lang_type_atom lang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth, Scope_id scope_id) {
    return lang_type_atom_new(
        name_new((Strv) {0}, sv(cstr), (Ulang_type_vec) {0}, scope_id),
        pointer_depth
    );
}

#endif // LANG_TYPE_HAND_WRITTEN
