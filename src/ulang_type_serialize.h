#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Str_view serialize_ulang_type(Ulang_type ulang_type);

Str_view serialize_ulang_type_atom(Ulang_type_atom atom);

Str_view serialize_ulang_type_fn(Ulang_type_fn ulang_type);

Str_view serialize_ulang_type_tuple(Ulang_type_tuple ulang_type);

Str_view serialize_ulang_type_regular(Ulang_type_regular ulang_type);

Str_view serialize_ulang_type(Ulang_type ulang_type);

Ulang_type deserialize_ulang_type(Str_view serialized);

#endif // ULANG_TYPE_SERIALIZE_H
