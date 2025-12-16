#ifndef ULANG_TYPE_IS_EQUAL_H
#define ULANG_TYPE_IS_EQUAL_H

#include <ulang_type.h>
#include <util.h>

bool ulang_type_regular_is_equal(Ulang_type_regular a, Ulang_type_regular b);

bool ulang_type_is_equal(Ulang_type a, Ulang_type b);

bool ulang_type_vec_is_equal(Ulang_type_vec a, Ulang_type_vec b);

bool ulang_type_lit_is_equal(Ulang_type_lit a, Ulang_type_lit b);

// TODO: move elsewhere?
bool uast_expr_is_equal(const Uast_expr* a, const Uast_expr* b);

#endif // ULANG_TYPE_IS_EQUAL_H
