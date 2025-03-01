#ifndef ULANG_TYPE_SERIALIZE_H
#define ULANG_TYPE_SERIALIZE_H

#include <ulang_type.h>
#include <tast_hand_written.h>
#include <newstring.h>
#include <tast.h>
#include <tast_utils.h>

static inline Str_view serialize_ulang_type_struct_thing_get_prefix(Ulang_type ulang_type) {
    (void) ulang_type;
    todo();
}

static inline Str_view serialize_ulang_type_struct_thing(Ulang_type ulang_type) {
    (void) ulang_type;
    todo();
}

// TODO: make separate function for Tast_ulang_type and Ulang_type
static inline Str_view serialize_ulang_type(Ulang_type ulang_type) {
    Str_view name = ulang_type_print_internal(LANG_TYPE_MODE_LOG, ulang_type);
    assert(name.count > 1);
    return str_view_slice(name, 0, name.count - 1);
}

#endif // ULANG_TYPE_SERIALIZE_H
