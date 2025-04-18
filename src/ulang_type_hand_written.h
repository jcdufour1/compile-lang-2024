#ifndef ULANG_TYPE_HAND_WRITTEN
#define ULANG_TYPE_HAND_WRITTEN

#include <str_view.h>
#include <vector.h>
#include <uast_forward_decl.h>

struct Ulang_type_;
typedef struct Ulang_type_ Ulang_type;

struct Ulang_type_atom_;
typedef struct Ulang_type_atom_ Ulang_type_atom;

typedef struct {
    Vec_base info;
    Ulang_type* buf;
} Ulang_type_vec;

typedef struct {
    Vec_base info;
    Ulang_type_atom* buf;
} Ulang_type_atom_vec;

// TODO: move this struct
typedef struct {
    Str_view mod_path;
    Str_view base;
    Ulang_type_vec gen_args;
} Name;

typedef struct {
    bool is_alias;
    Uast_symbol* mod_alias;

    Str_view base;
    Ulang_type_vec gen_args;
} Uname;

typedef struct Ulang_type_atom_ {
    Uname str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Ulang_type_atom;

static inline bool ulang_type_is_equal(Ulang_type a, Ulang_type b);

static inline Ulang_type_atom ulang_type_atom_new(Uname str, int16_t pointer_depth) {
    return (Ulang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Ulang_type_atom ulang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return ulang_type_atom_new(
        (Uname) {.is_alias = false, .mod_alias = NULL, str_view_from_cstr(cstr)},
        pointer_depth
    );
}

#endif // ULANG_TYPE_HAND_WRITTEN
