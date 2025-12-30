#ifndef STRING_VEC_H
#define STRING_VEC_H

#include <darr.h>
#include <local_string.h>

typedef struct {
    Vec_base info;
    String* buf;
} String_darr;

#endif // STRING_VEC_H
