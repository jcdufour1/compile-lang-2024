#ifndef ULANG_TYPE_H
#define ULANG_TYPE_H

// TODO: merge ulang_type_hand_written.h header into this one
#include <ulang_type_hand_written.h>

struct Ulang_type_tuple_;
typedef struct Ulang_type_tuple_ Ulang_type_tuple;

struct Ulang_type_fn_;
typedef struct Ulang_type_fn_ Ulang_type_fn;

struct Ulang_type_regular_;
typedef struct Ulang_type_regular_ Ulang_type_regular;

struct Ulang_type_reg_generic_;
typedef struct Ulang_type_reg_generic_ Ulang_type_reg_generic;

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

typedef struct Ulang_type_regular_ {
    Ulang_type_atom atom;
    Pos pos;
}Ulang_type_regular;

typedef struct Ulang_type_reg_generic_ {
    Ulang_type_atom atom;
    Ulang_type_vec generic_args;
    Pos pos;
}Ulang_type_reg_generic;

typedef union Ulang_type_as_ {
    Ulang_type_tuple ulang_type_tuple;
    Ulang_type_fn ulang_type_fn;
    Ulang_type_regular ulang_type_regular;
    Ulang_type_reg_generic ulang_type_reg_generic;
}Ulang_type_as;
typedef enum ULANG_TYPE_TYPE_ {
    ULANG_TYPE_TUPLE,
    ULANG_TYPE_FN,
    ULANG_TYPE_REGULAR,
    ULANG_TYPE_REG_GENERIC,
}ULANG_TYPE_TYPE;
typedef struct Ulang_type_ {
    Ulang_type_as as;
    ULANG_TYPE_TYPE type;
}Ulang_type;

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
static inline Ulang_type_reg_generic ulang_type_reg_generic_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_REG_GENERIC);
    return ulang_type.as.ulang_type_reg_generic;
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
static inline Ulang_type ulang_type_reg_generic_const_wrap(Ulang_type_reg_generic ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_REG_GENERIC;
    new_ulang_type.as.ulang_type_reg_generic = ulang_type;
    return new_ulang_type;
}
#define ulang_type_tuple_print(ulang_type) str_view_print(ulang_type_tuple_print_internal(ulang_type, 0))
Str_view ulang_type_tuple_print_internal(const Ulang_type_tuple* ulang_type, int recursion_depth);
#define ulang_type_fn_print(ulang_type) str_view_print(ulang_type_fn_print_internal(ulang_type, 0))
Str_view ulang_type_fn_print_internal(const Ulang_type_fn* ulang_type, int recursion_depth);
#define ulang_type_regular_print(ulang_type) str_view_print(ulang_type_regular_print_internal(ulang_type, 0))
Str_view ulang_type_regular_print_internal(const Ulang_type_regular* ulang_type, int recursion_depth);
#define ulang_type_reg_generic_print(ulang_type) str_view_print(ulang_type_reg_generic_print_internal(ulang_type, 0))
Str_view ulang_type_reg_generic_print_internal(const Ulang_type_reg_generic* ulang_type, int recursion_depth);
static inline Ulang_type_tuple ulang_type_tuple_new(Ulang_type_vec ulang_types, Pos pos){
    return (Ulang_type_tuple) { .ulang_types = ulang_types, .pos = pos};
}
static inline Ulang_type_fn ulang_type_fn_new(Ulang_type_tuple params, Ulang_type* return_type, Pos pos){
    return (Ulang_type_fn) { .params = params,  .return_type = return_type, .pos = pos};
}
static inline Ulang_type_regular ulang_type_regular_new(Ulang_type_atom atom, Pos pos){
    return (Ulang_type_regular) { .atom = atom, .pos = pos};
}
static inline Ulang_type_reg_generic ulang_type_reg_generic_new(Ulang_type_atom atom, Ulang_type_vec generic_args, Pos pos){
    return (Ulang_type_reg_generic) { .atom = atom,  .generic_args = generic_args, .pos = pos};
}
#endif // ULANG_TYPE_H

