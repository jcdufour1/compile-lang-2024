#ifndef ULANG_TYPE_PRINT_H
#define ULANG_TYPE_PRINT_H

#include <util.h>
#include <str_view.h>
#include <newstring.h>
#include <lang_type.h>
#include <ulang_type.h>
#include <extend_name.h>

#define ulang_type_print(mode, lang_type) str_view_print(ulang_type_print_internal((mode), (lang_type)))

Str_view ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type);

#endif // ULANG_TYPE_PRINT_H
