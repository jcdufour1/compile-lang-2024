#ifndef LANG_TYPE_PRINT_H
#define LANG_TYPE_PRINT_H

#include <env.h>
#include <local_string.h>
#include <strv.h>
#include <lang_type_mode.h>
#include <lang_type.h>

#define lang_type_print(mode, lang_type) strv_print(lang_type_print_internal((mode), (lang_type)))

#define lang_type_atom_print(mode, atom) strv_print(lang_type_atom_print_internal(atom, mode))

Strv lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type);

Strv lang_type_atom_print_internal(Lang_type_atom atom, LANG_TYPE_MODE mode);

void extend_serialize_lang_type_to_string(String* string, Lang_type lang_type, bool do_tag);

void extend_lang_type_atom(String* string, LANG_TYPE_MODE mode, Lang_type_atom atom);

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type);

#endif // LANG_TYPE_PRINT_H
