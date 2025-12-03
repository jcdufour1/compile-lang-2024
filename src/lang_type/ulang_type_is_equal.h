#ifndef ULANG_TYPE_IS_EQUAL_H
#define ULANG_TYPE_IS_EQUAL_H

#include <ulang_type.h>
#include <util.h>

bool ulang_type_atom_is_equal(Ulang_type_atom a, Ulang_type_atom b);

bool ulang_type_regular_is_equal(Ulang_type_regular a, Ulang_type_regular b);

bool ulang_type_is_equal(Ulang_type a, Ulang_type b);

bool ulang_type_vec_is_equal(Ulang_type_vec a, Ulang_type_vec b);

bool ulang_type_const_expr_is_equal(Ulang_type_const_expr a, Ulang_type_const_expr b);

#endif // ULANG_TYPE_IS_EQUAL_H
