#ifndef INFER_GENERIC_TYPE_H
#define INFER_GENERIC_TYPE_H

#include <util.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <uast.h>
#include <name.h>

bool infer_generic_type(
    Ulang_type* infered,
    Ulang_type arg_to_infer_from, // must have been set already (not a generic arg still)
    bool arg_to_infer_is_lit,
    Ulang_type param_corres_to_arg, // may still have generic stuff (must sometimes or always 
                                    // have generic stuff for infer_generic_type to be useful)
    Name name_to_infer,
    Pos pos_arg
);

#endif // INFER_GENERIC_TYPE_H
