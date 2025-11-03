#ifndef SIZE_T_VEC_H
#define SIZE_T_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    size_t* buf;
} Size_t_vec;

#endif // SIZE_T_VEC_H
