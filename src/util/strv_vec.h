#ifndef STRV_VEC_H
#define STRV_VEC_H

typedef struct {
    Vec_base info;
    Str_view* buf;
} Strv_vec;

// TODO: move this struct
typedef struct {
    Vec_base info;
    Name* buf;
} Name_vec;

#endif // STRV_VEC_H
