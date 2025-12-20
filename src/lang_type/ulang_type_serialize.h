#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <local_string.h>
#include <tast.h>
#include <tast_utils.h>

Name serialize_ulang_type_fn(Strv mod_path, Ulang_type_fn ulang_type, bool include_scope);

Name serialize_ulang_type_tuple(Strv mod_path, Ulang_type_tuple ulang_type, bool include_scope);

Name serialize_ulang_type_regular(Strv mod_path, Ulang_type_regular ulang_type, bool include_scope);

Name serialize_ulang_type(Strv mod_path, Ulang_type ulang_type, bool include_scope);

Name serialize_ulang_type_int_lit(Ulang_type_int_lit ulang_type);

Name serialize_ulang_type_float_lit(Ulang_type_float_lit ulang_type);

Name serialize_ulang_type_string_lit(Ulang_type_string_lit ulang_type);

Name serialize_ulang_type_struct_lit(Strv mod_path, Ulang_type_struct_lit ulang_type);

Name serialize_ulang_type_lit(Strv mod_path, Ulang_type_lit ulang_type);

Name serialize_ulang_type_fn_lit(Ulang_type_fn_lit ulang_type);

Name serialize_ulang_type_array(Strv mod_path, Ulang_type_array ulang_type, bool include_scope);

#endif // ULANG_TYPE_SERIALIZE_H
