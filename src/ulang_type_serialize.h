#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Str_view serialize_ulang_type_atom(Ulang_type_atom atom);

Name serialize_ulang_type_fn(Env* env, Str_view mod_path, Ulang_type_fn ulang_type);

Name serialize_ulang_type_tuple(Env* env, Str_view mod_path, Ulang_type_tuple ulang_type);

Name serialize_ulang_type_regular(Str_view mod_path, Ulang_type_regular ulang_type);

Name serialize_ulang_type(Env* env, Str_view mod_path, Ulang_type ulang_type);

Ulang_type_atom deserialize_ulang_type_atom(Name* serialized);

Ulang_type deserialize_ulang_type(Name* serialized, int16_t pointer_depth);

#endif // ULANG_TYPE_SERIALIZE_H
