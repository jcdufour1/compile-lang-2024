#ifndef LANG_TYPE_IS
#define LANG_TYPE_IS

#include <ulang_type.h>
#include <lang_type.h>
#include <lang_type_after.h>

static inline bool lang_type_is_slice(Ulang_type* a_genrg, Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_STRUCT) {
        return false;
    }
    Lang_type_struct lang_type_struct = lang_type_struct_const_unwrap(lang_type);
    if (!strv_is_equal(lang_type_struct.atom.str.mod_path, MOD_PATH_RUNTIME)) {
        return false;
    }
    unwrap(lang_type_struct.atom.str.a_genrgs.info.count == 1);
    *a_genrg = vec_at(lang_type_struct.atom.str.a_genrgs, 0);
    return true;
}

#endif // LANG_TYPE_IS
