#ifndef ULANG_TYPE_H
#define ULANG_TYPE_H

// TODO: merge ulang_type_hand_written.h header into this one
#include <strv.h>
#include <vector.h>
#include <name.h>
#include <lang_type_mode.h>
#include <msg_todo.h>
#include <parameters.h>

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

// TODO: move this struct
typedef struct {
    Vec_base info;
    Name* buf;
} Name_vec;

// TODO: move this struct
typedef struct {
    Vec_base info;
    Ir_name* buf;
} Ir_name_vec;


struct Ulang_type_tuple_;
typedef struct Ulang_type_tuple_ Ulang_type_tuple;

struct Ulang_type_fn_;
typedef struct Ulang_type_fn_ Ulang_type_fn;

struct Ulang_type_regular_;
typedef struct Ulang_type_regular_ Ulang_type_regular;

struct Ulang_type_array_;
typedef struct Ulang_type_array_ Ulang_type_array;

struct Ulang_type_expr_;
typedef struct Ulang_type_expr_ Ulang_type_expr;

struct Ulang_type_int_;
typedef struct Ulang_type_int_ Ulang_type_int;

struct Ulang_type_removed_;
typedef struct Ulang_type_removed_ Ulang_type_removed;

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
    int16_t pointer_depth;
}Ulang_type_fn;

// TODO: refactor so that Ulang_type_atom struct in inlined into Ulang_type_regular?
typedef struct Ulang_type_regular_ {
    Ulang_type_atom atom;
    Pos pos;
}Ulang_type_regular;

struct Uast_expr;
typedef struct Uast_expr_ Uast_expr;

typedef struct Ulang_type_array_ {
    Ulang_type* item_type;
    Uast_expr* count; 
    Pos pos;
    int16_t pointer_depth;
}Ulang_type_array;

struct Uast_expr_;
typedef struct Uast_expr_ Uast_expr;

typedef struct Ulang_type_expr_ {
    Uast_expr* expr;
    int16_t pointer_depth;
    Pos pos;
}Ulang_type_expr;

typedef struct Ulang_type_int_ {
    int64_t data;
    int16_t pointer_depth;
    Pos pos;
}Ulang_type_int;

typedef struct Ulang_type_removed_ {
    Pos pos;
    int16_t pointer_depth;
}Ulang_type_removed;

typedef union Ulang_type_as_ {
    Ulang_type_tuple ulang_type_tuple;
    Ulang_type_fn ulang_type_fn;
    Ulang_type_regular ulang_type_regular;
    Ulang_type_array ulang_type_array;
    Ulang_type_expr ulang_type_expr;
    Ulang_type_int ulang_type_int;
    Ulang_type_removed ulang_type_removed;
}Ulang_type_as;
typedef enum ULANG_TYPE_TYPE_ {
    ULANG_TYPE_TUPLE,
    ULANG_TYPE_FN,
    ULANG_TYPE_REGULAR,
    ULANG_TYPE_ARRAY,
    ULANG_TYPE_EXPR,
    ULANG_TYPE_INT,
    ULANG_TYPE_REMOVED,
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
static inline Ulang_type_removed ulang_type_removed_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_REMOVED);
    return ulang_type.as.ulang_type_removed;
}
static inline Ulang_type_regular* ulang_type_regular_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_REGULAR);
    return &ulang_type->as.ulang_type_regular;
}
static inline Ulang_type_fn* ulang_type_fn_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_FN);
    return &ulang_type->as.ulang_type_fn;
}
static inline Ulang_type_removed* ulang_type_removed_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_REMOVED);
    return &ulang_type->as.ulang_type_removed;
}
static inline Ulang_type_array ulang_type_array_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_ARRAY);
    return ulang_type.as.ulang_type_array;
}
static inline Ulang_type_expr ulang_type_expr_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_EXPR);
    return ulang_type.as.ulang_type_expr;
}
static inline Ulang_type_int ulang_type_int_const_unwrap(const Ulang_type ulang_type) {
    unwrap(ulang_type.type == ULANG_TYPE_INT);
    return ulang_type.as.ulang_type_int;
}
static inline Ulang_type_array* ulang_type_array_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_ARRAY);
    return &ulang_type->as.ulang_type_array;
}
static inline Ulang_type_tuple* ulang_type_tuple_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_TUPLE);
    return &ulang_type->as.ulang_type_tuple;
}
static inline Ulang_type_expr* ulang_type_expr_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_EXPR);
    return &ulang_type->as.ulang_type_expr;
}
static inline Ulang_type_int* ulang_type_int_unwrap(Ulang_type* ulang_type) {
    unwrap(ulang_type->type == ULANG_TYPE_INT);
    return &ulang_type->as.ulang_type_int;
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
static inline Ulang_type ulang_type_array_const_wrap(Ulang_type_array ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_ARRAY;
    new_ulang_type.as.ulang_type_array = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_expr_const_wrap(Ulang_type_expr ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_EXPR;
    new_ulang_type.as.ulang_type_expr = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_int_const_wrap(Ulang_type_int ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_INT;
    new_ulang_type.as.ulang_type_int = ulang_type;
    return new_ulang_type;
}
static inline Ulang_type ulang_type_removed_const_wrap(Ulang_type_removed ulang_type) {
    Ulang_type new_ulang_type = {0};
    new_ulang_type.type = ULANG_TYPE_REMOVED;
    new_ulang_type.as.ulang_type_removed = ulang_type;
    return new_ulang_type;
}
#define ulang_type_gen_param_print(ulang_type) strv_print(ulang_type_gen_param_print_internal(ulang_type, 0))
#define ulang_type_tuple_print(ulang_type) strv_print(ulang_type_tuple_print_internal(ulang_type, 0))
Strv ulang_type_tuple_print_internal(const Ulang_type_tuple* ulang_type, Indent indent);
#define ulang_type_fn_print(ulang_type) strv_print(ulang_type_fn_print_internal(ulang_type, 0))
Strv ulang_type_fn_print_internal(const Ulang_type_fn* ulang_type, Indent indent);
#define ulang_type_regular_print(ulang_type) strv_print(ulang_type_regular_print_internal(ulang_type, 0))
Strv ulang_type_regular_print_internal(const Ulang_type_regular* ulang_type, Indent indent);
#define ulang_type_array_print(ulang_type) strv_print(ulang_type_array_print_internal(ulang_type, 0))
Strv ulang_type_array_print_internal(const Ulang_type_array* ulang_type, Indent indent);
#define ulang_type_expr_print(ulang_type) strv_print(ulang_type_expr_print_internal(ulang_type, 0))
Strv ulang_type_expr_print_internal(const Ulang_type_expr* ulang_type, Indent indent);
#define ulang_type_int_print(ulang_type) strv_print(ulang_type_int_print_internal(ulang_type, 0))
Strv ulang_type_int_print_internal(const Ulang_type_int* ulang_type, Indent indent);
#define ulang_type_removed_print(ulang_type) strv_print(ulang_type_removed_print_internal(ulang_type, 0))
Strv ulang_type_removed_print_internal(const Ulang_type_removed* ulang_type, Indent indent);
static inline Ulang_type_tuple ulang_type_tuple_new(Ulang_type_vec ulang_types, Pos pos){
    return (Ulang_type_tuple) { .ulang_types = ulang_types, .pos = pos};
}
static inline Ulang_type_fn ulang_type_fn_new(Ulang_type_tuple params, Ulang_type* return_type, Pos pos){
    return (Ulang_type_fn) { .params = params,  .return_type = return_type, .pos = pos};
}
static inline Ulang_type_regular ulang_type_regular_new(Ulang_type_atom atom, Pos pos){
    return (Ulang_type_regular) { .atom = atom, .pos = pos};
}
static inline Ulang_type_array ulang_type_array_new(Ulang_type* item_type, Uast_expr* count, Pos pos){
    return (Ulang_type_array) { .item_type = item_type, .count = count, .pos = pos};
}
static inline Ulang_type_expr ulang_type_expr_new(Uast_expr* expr, int16_t pointer_depth, Pos pos){
    return (Ulang_type_expr) { .expr = expr, .pointer_depth = pointer_depth, .pos = pos};
}
static inline Ulang_type_int ulang_type_int_new(int64_t data, int16_t pointer_depth, Pos pos){
    return (Ulang_type_int) { .data = data, .pointer_depth = pointer_depth, .pos = pos};
}
static inline Ulang_type_removed ulang_type_removed_new(int16_t pointer_depth, Pos pos){
    return (Ulang_type_removed) { .pointer_depth = pointer_depth, .pos = pos};
}

static inline int16_t ulang_type_get_pointer_depth(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REMOVED:
            return ulang_type_removed_const_unwrap(lang_type).pointer_depth;
        case ULANG_TYPE_TUPLE:
            // TODO
            return 0;
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_unwrap(lang_type).pointer_depth;
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).atom.pointer_depth;
        case ULANG_TYPE_ARRAY:
            return ulang_type_array_const_unwrap(lang_type).pointer_depth;
        case ULANG_TYPE_EXPR:
            return ulang_type_expr_const_unwrap(lang_type).pointer_depth;
        case ULANG_TYPE_INT:
            return ulang_type_int_const_unwrap(lang_type).pointer_depth;
    }
    unreachable("");
}

// TODO: remove this function?
static inline void ulang_type_set_pointer_depth(Ulang_type* lang_type, int16_t pointer_depth) {
    switch (lang_type->type) {
        case ULANG_TYPE_REMOVED:
            ulang_type_removed_unwrap(lang_type)->pointer_depth = pointer_depth;
            return;
        case ULANG_TYPE_TUPLE:
            unwrap(pointer_depth == 0);
            return;
        case ULANG_TYPE_FN:
            ulang_type_fn_unwrap(lang_type)->pointer_depth = pointer_depth;
            return;
        case ULANG_TYPE_REGULAR:
            ulang_type_regular_unwrap(lang_type)->atom.pointer_depth = pointer_depth;
            return;
        case ULANG_TYPE_ARRAY:
            ulang_type_array_unwrap(lang_type)->pointer_depth = pointer_depth;
            return;
        case ULANG_TYPE_EXPR:
            ulang_type_expr_unwrap(lang_type)->pointer_depth = pointer_depth;
            return;
        case ULANG_TYPE_INT:
            ulang_type_int_unwrap(lang_type)->pointer_depth = pointer_depth;
            return;
    }
    unreachable("");
}

static inline void ulang_type_add_pointer_depth(Ulang_type* lang_type, int16_t pointer_depth) {
    int16_t prev = ulang_type_get_pointer_depth(*lang_type);
    ulang_type_set_pointer_depth(lang_type, prev + pointer_depth);
}

#define ulang_type_print(mode, lang_type) strv_print(ulang_type_print_internal((mode), (lang_type)))

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type);

#endif // ULANG_TYPE_H

