#ifndef SYM_TBL_VEC
#define SYM_TBL_VEC

typedef struct {
    Vec_base info;
    Symbol_table* buf;
} Sym_tbl_vec;

#endif // SYM_TBL_VEC
