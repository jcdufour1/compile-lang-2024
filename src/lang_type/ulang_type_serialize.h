#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Strv serialize_ulang_type_atom(Ulang_type_atom atom, bool include_scope, Pos pos);

Name serialize_ulang_type_fn(Strv mod_path, Ulang_type_fn ulang_type, bool include_scope);

Name serialize_ulang_type_tuple(Strv mod_path, Ulang_type_tuple ulang_type, bool include_scope);

Name serialize_ulang_type_regular(Strv mod_path, Ulang_type_regular ulang_type, bool include_scope);

Name serialize_ulang_type(
    Strv mod_path,
    Ulang_type ulang_type,
    bool include_scope,
    bool is_parent_name,
    Name parent_name,
    size_t parent_idx
);

Name serialize_ulang_type_int(Strv mod_path, Ulang_type_int ulang_type, bool include_scope);

Name serialize_ulang_type_float_lit(Strv mod_path, Ulang_type_float_lit ulang_type, bool include_scope);

Name serialize_ulang_type_struct_lit(
    Strv mod_path,
    Ulang_type_struct_lit ulang_type,
    bool include_scope,
    bool is_parent_name,
    Name parent_name,
    size_t parent_idx
);

Name serialize_ulang_type_const_expr(
    Strv mod_path,
    Ulang_type_const_expr ulang_type,
    bool include_scope,
    bool is_parent_name,
    Name parent_name,
    size_t parent_idx
);

Name serialize_ulang_type_fn_lit(
    Strv mod_path,
    Ulang_type_fn_lit ulang_type,
    bool include_scope,
    bool is_parent_info,
    Name parent_name,
    size_t parent_idx
);

#endif // ULANG_TYPE_SERIALIZE_H
