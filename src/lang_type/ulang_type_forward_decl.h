#ifndef ULANG_TYPE_FORWARD_DECL_H
#define ULANG_TYPE_FORWARD_DECL_H

#include <vector.h>

struct Ulang_type_;
typedef struct Ulang_type_ Ulang_type;

typedef struct {
    Vec_base info;
    Ulang_type* buf;
} Ulang_type_vec;

#endif // ULANG_TYPE_FORWARD_DECL_H
