#ifndef ULANG_TYPE_FORWARD_DECL_H
#define ULANG_TYPE_FORWARD_DECL_H

#include <vector.h>

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

#endif // ULANG_TYPE_FORWARD_DECL_H
