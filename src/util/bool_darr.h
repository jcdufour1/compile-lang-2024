#ifndef BOOL_VEC_H
#define BOOL_VEC_H

#include <darr.h>

typedef struct {
    Vec_base info;
    bool* buf;
} Bool_darr;

#endif // BOOL_VEC_H
