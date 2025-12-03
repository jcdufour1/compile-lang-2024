#ifndef ULANG_TYPE_HAND_WRITTEN
#define ULANG_TYPE_HAND_WRITTEN

#include <name.h>

typedef enum {
    LANG_TYPE_MODE_LOG,
    LANG_TYPE_MODE_MSG,
    LANG_TYPE_MODE_EMIT_LLVM,
    LANG_TYPE_MODE_EMIT_C,
} LANG_TYPE_MODE;

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

#define ulang_type_print(mode, lang_type) strv_print(ulang_type_print_internal((mode), (lang_type)))

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type);

//static inline void ulang_type_add_pointer_depth(Ulang_type* lang_type, int16_t pointer_depth) {
//    int16_t prev = ulang_type_get_pointer_depth(*lang_type);
//    ulang_type_set_pointer_depth(lang_type, prev + pointer_depth);
//}

#endif // ULANG_TYPE_HAND_WRITTEN
