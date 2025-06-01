#ifndef LANG_TYPE_IS
#define LANG_TYPE_IS

#include <ulang_type.h>
#include <lang_type.h>
#include <lang_type_after.h>

static inline bool lang_type_is_slice(Ulang_type* gen_arg, Lang_type lang_type) {
    (void) env;
    if (lang_type.type != LANG_TYPE_STRUCT) {
        return false;
    }
    unwrap(lang_type_struct_const_unwrap(lang_type).atom.str.gen_args.info.count == 1);
    *gen_arg = vec_at_const(lang_type_struct_const_unwrap(lang_type).atom.str.gen_args, 0);
    return strv_cstr_is_equal(lang_type_struct_const_unwrap(lang_type).atom.str.base, "Slice");
}

#endif // LANG_TYPE_IS
