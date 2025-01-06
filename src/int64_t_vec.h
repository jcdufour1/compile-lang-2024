#ifndef INT64_T_VEC_H
#define INT64_T_VEC_H

#include <vector.h>

typedef struct {
    Vec_base info;
    int64_t* buf;
} Int64_t_vec;

#endif // INT64_T_VEC_H
