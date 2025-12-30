#ifndef STR_VIEW_VEC_H
#define STR_VIEW_VEC_H

#include <darr.h>

typedef struct {
    Vec_base info;
    Strv* buf;
} Strv_darr;

#endif // STR_VIEW_VEC_H
