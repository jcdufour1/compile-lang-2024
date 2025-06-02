#ifndef LANG_TYPE_SERIALIZE_H
#define LANG_TYPE_SERIALIZE_H

#include <lang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

Strv serialize_struct_def_base(Struct_def_base base);

Strv serialize_lang_type_get_prefix(Lang_type lang_type);

Strv serialize_lang_type_struct_thing(Lang_type lang_type);

Strv serialize_lang_type(Lang_type lang_type);

Strv serialize_lang_type_tuple(Lang_type_tuple lang_type);

Lang_type deserialize_lang_type(Strv* serialized);

#endif // LANG_TYPE_SERIALIZE_H
