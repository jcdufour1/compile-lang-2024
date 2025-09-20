#ifndef LLVM_LANG_TYPE_HAND_WRITTEN
#define LLVM_LANG_TYPE_HAND_WRITTEN

#include <strv.h>
#include <vector.h>
#include <ulang_type.h>

typedef struct {
    Name str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Llvm_lang_type_atom;

struct Llvm_lang_type_;
typedef struct Llvm_lang_type_ Llvm_lang_type;

typedef struct {
    Vec_base info;
    Llvm_lang_type* buf;
} Llvm_lang_type_vec;

static inline Llvm_lang_type_atom llvm_lang_type_atom_new(Name str, int16_t pointer_depth) {
    return (Llvm_lang_type_atom) {.str = str, .pointer_depth = pointer_depth};
}

static inline Llvm_lang_type_atom llvm_lang_type_atom_new_from_cstr(const char* cstr, int16_t pointer_depth, Scope_id scope_id) {
    return llvm_lang_type_atom_new(
        name_new((Strv) {0}, sv(cstr), (Ulang_type_vec) {0}, scope_id),
        pointer_depth
    );
}

#endif // LLVM_LANG_TYPE_HAND_WRITTEN
