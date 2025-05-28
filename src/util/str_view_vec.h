#ifndef STR_VIEW_VEC_H
#define STR_VIEW_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    Str_view* buf;
} Str_view_vec;

#endif // STR_VIEW_VEC_H
