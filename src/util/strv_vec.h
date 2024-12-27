#ifndef STRV_VEC_H
#define STRV_VEC_H

typedef struct {
    Vec_base info;
    Str_view* buf;
} Strv_vec;

#endif // STRV_VEC_H
