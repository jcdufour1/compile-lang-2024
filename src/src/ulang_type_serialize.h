#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Str_view serialize_ulang_type_atom(Ulang_type_atom atom, bool include_scope);

Name serialize_ulang_type_fn(Str_view mod_path, Ulang_type_fn ulang_type, bool include_scope);

Name serialize_ulang_type_tuple(Str_view mod_path, Ulang_type_tuple ulang_type, bool include_scope);

Name serialize_ulang_type_regular(Str_view mod_path, Ulang_type_regular ulang_type, bool include_scope);

Name serialize_ulang_type(Str_view mod_path, Ulang_type ulang_type, bool include_scope);

#endif // ULANG_TYPE_SERIALIZE_H
