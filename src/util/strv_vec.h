#ifndef STR_VIEW_VEC_H
#define STR_VIEW_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    Strv* buf;
} Strv_vec;

#endif // STR_VIEW_VEC_H
