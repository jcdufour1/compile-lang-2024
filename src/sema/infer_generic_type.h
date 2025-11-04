#ifndef INFER_GENERIC_TYPE_H
#define INFER_GENERIC_TYPE_H

#include <util.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <uast.h>
#include <name.h>

bool infer_generic_type(
    Ulang_type* infered,
    Lang_type arg_to_infer_from,
    bool arg_to_infer_is_lit,
    Ulang_type param_corres_to_arg,
    Name name_to_infer,
    Pos pos_arg
);

#endif // INFER_GENERIC_TYPE_H
