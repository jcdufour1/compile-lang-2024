#ifndef LANG_TYPE_SERIALIZE_H
#define LANG_TYPE_SERIALIZE_H

#include <lang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Str_view serialize_struct_def_base(Struct_def_base base);

Str_view serialize_lang_type_get_prefix(Lang_type lang_type);

Str_view serialize_lang_type_struct_thing(Lang_type lang_type);

Str_view serialize_lang_type(Lang_type lang_type);

Str_view serialize_lang_type_tuple(Lang_type_tuple lang_type);

Lang_type deserialize_lang_type(Str_view* serialized);

#endif // LANG_TYPE_SERIALIZE_H
