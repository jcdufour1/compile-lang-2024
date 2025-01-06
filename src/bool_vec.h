#ifndef BOOL_VEC_H
#define BOOL_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    bool* buf;
} Bool_vec;

#endif // BOOL_VEC_H
