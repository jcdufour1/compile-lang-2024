#ifndef STRING_VEC_H
#define STRING_VEC_H

#include "vector.h"
#include "newstring.h"

typedef struct {
    Vec_base info;
    String* buf;
} String_vec;

#endif // STRING_VEC_H
