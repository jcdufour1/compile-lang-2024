#ifndef LANG_TYPE_PRINT_H
#define LANG_TYPE_PRINT_H

#include <env.h>
#include <newstring.h>
#include <str_view.h>

#define lang_type_print(mode, lang_type) str_view_print(lang_type_print_internal((mode), (lang_type)))

#define lang_type_atom_print(mode, atom) str_view_print(lang_type_atom_print_internal(atom))

Str_view lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type);

Str_view lang_type_atom_print_internal(Lang_type_atom atom);

void extend_serialize_lang_type_to_string(Env* env, String* string, Lang_type lang_type, bool do_tag);

void extend_lang_type_atom(String* string, Lang_type_atom atom);

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type);

#endif // LANG_TYPE_PRINT_H
