#ifndef ULANG_TYPE_H
#define ULANG_TYPE_H

// TODO: merge ulang_type_hand_written.h header into this one
#include <strv.h>
#include <vector.h>
#include <name.h>
#include <lang_type_mode.h>

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
        uname_new((Name) {0}/* TODO */, sv(cstr), (Ulang_type_vec) {0}, SCOPE_BUILTIN),
        pointer_depth
    );
}

// TODO: move this struct
typedef struct {
    Vec_base info;
    Name* buf;
} Name_vec;


struct Ulang_type_tuple_;
typedef struct Ulang_type_tuple_ Ulang_type_tuple;

struct Ulang_type_fn_;
typedef struct Ulang_type_fn_ Ulang_type_fn;

struct Ulang_type_regular_;
typedef struct Ulang_type_regular_ Ulang_type_regular;

struct Ulang_type_;
typedef struct Ulang_type_ Ulang_type;

typedef struct Ulang_type_tuple_ {
    Ulang_type_vec ulang_types;
    Pos pos;
}Ulang_type_tuple;

typedef struct Ulang_type_fn_ {
    Ulang_type_tuple params;
    Ulang_type* return_type;
    Pos pos;
}Ulang_type_fn;

// TODO: refactor so that Ulang_type_atom struct in inlined into Ulang_type_regular?
typedef struct Ulang_type_regular_ {
    Ulang_type_atom atom;
    Pos pos;
}Ulang_type_regular;

typedef struct Ulang_type_gen_param_ {
    Pos pos;
}Ulang_type_gen_param;

typedef union Ulang_type_as_ {
    Ulang_type_tuple ulang_type_tuple;
    Ulang_type_fn ulang_type_fn;
    Ulang_type_regular ulang_type_regular;
    Ulang_type_gen_param ulang_type_gen_param;
}Ulang_type_as;
typedef enum ULANG_TYPE_TYPE_ {
    ULANG_TYPE_TUPLE,
    ULANG_TYPE_FN,
    ULANG_TYPE_REGULAR,
    ULANG_TYPE_GEN_PARAM,
}ULANG_TYPE_TYPE;
typedef struct Ulang_type_ {
    Ulang_type_as as;
    ULANG_TYPE_TYPE type;
}Ulang_type;

static inline Ulang_type_gen_param ulang_type_gen_param_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_GEN_PARAM);
    return ulang_type.as.ulang_type_gen_param;
}
static inline Ulang_type_tuple ulang_type_tuple_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_TUPLE);
    return ulang_type.as.ulang_type_tuple;
}
static inline Ulang_type_fn ulang_type_fn_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_FN);
    return ulang_type.as.ulang_type_fn;
}
static inline Ulang_type_regular ulang_type_regular_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_REGULAR);
    return ulang_type.as.ulang_type_regular;
}
static inline Ulang_type_regular* ulang_type_regular_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_REGULAR);
    return &ulang_type->as.ulang_type_regular;
}
static inline Ulang_type ulang_type_gen_param_const_wrap(Ulang_type_gen_param ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_GEN_PARAM;
    new_ulang_type.as.ulang_type_gen_param = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_tuple_const_wrap(Ulang_type_tuple ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_TUPLE;
    new_ulang_type.as.ulang_type_tuple = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_fn_const_wrap(Ulang_type_fn ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_FN;
    new_ulang_type.as.ulang_type_fn = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_regular_const_wrap(Ulang_type_regular ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_REGULAR;
    new_ulang_type.as.ulang_type_regular = ulang_type;
    return new_ulang_type;
}
#define ulang_type_gen_param_print(ulang_type) strv_print(ulang_type_gen_param_print_internal(ulang_type, 0))
Strv ulang_type_gen_param_print_internal(const Ulang_type_gen_param* ulang_type, int recursion_depth);
#define ulang_type_tuple_print(ulang_type) strv_print(ulang_type_tuple_print_internal(ulang_type, 0))
Strv ulang_type_tuple_print_internal(const Ulang_type_tuple* ulang_type, int recursion_depth);
#define ulang_type_fn_print(ulang_type) strv_print(ulang_type_fn_print_internal(ulang_type, 0))
Strv ulang_type_fn_print_internal(const Ulang_type_fn* ulang_type, int recursion_depth);
#define ulang_type_regular_print(ulang_type) strv_print(ulang_type_regular_print_internal(ulang_type, 0))
Strv ulang_type_regular_print_internal(const Ulang_type_regular* ulang_type, int recursion_depth);
static inline Ulang_type_gen_param ulang_type_gen_param_new(Pos pos){
    return (Ulang_type_gen_param) { .pos = pos};
}
static inline Ulang_type_tuple ulang_type_tuple_new(Ulang_type_vec ulang_types, Pos pos){
    return (Ulang_type_tuple) { .ulang_types = ulang_types, .pos = pos};
}
static inline Ulang_type_fn ulang_type_fn_new(Ulang_type_tuple params, Ulang_type* return_type, Pos pos){
    return (Ulang_type_fn) { .params = params,  .return_type = return_type, .pos = pos};
}
static inline Ulang_type_regular ulang_type_regular_new(Ulang_type_atom atom, Pos pos){
    return (Ulang_type_regular) { .atom = atom, .pos = pos};
}

static inline int16_t ulang_type_get_pointer_depth(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_GEN_PARAM:
            unreachable("");
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).atom.pointer_depth;
    }
    unreachable("");
}

static inline void ulang_type_set_pointer_depth(Ulang_type* lang_type, int16_t pointer_depth) {
    switch (lang_type->type) {
        case ULANG_TYPE_GEN_PARAM:
            unreachable("");
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_REGULAR:
            ulang_type_regular_unwrap(lang_type)->atom.pointer_depth = pointer_depth;
            return;
    }
    unreachable("");
}

#define ulang_type_print(mode, lang_type) strv_print(ulang_type_print_internal((mode), (lang_type)))

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type);

#endif // ULANG_TYPE_H

