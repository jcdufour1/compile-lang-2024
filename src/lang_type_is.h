#ifndef LANG_TYPE_IS
#define LANG_TYPE_IS

#include <ulang_type.h>
#include <lang_type.h>
#include <lang_type_after.h>

static inline bool lang_type_is_slice(Env* env, Lang_type* gen_arg, Lang_type lang_type) {
    (void) env;
    (void) gen_arg;
    if (lang_type.type != LANG_TYPE_STRUCT) {
        return false;
    }
    
    Lang_type_struct lang_struct = lang_type_struct_const_unwrap(lang_type);
    (void) lang_struct;
    //Ulang_type_generic deserialized = {0};
    todo();
    //if (deserialize_generic(&deserialized, lang_struct.atom.pointer_depth, &lang_struct.atom.str)) {
    //    assert(deserialized.generic_args.info.count == 1 && "TODO\n");
    //    *gen_arg = lang_type_from_ulang_type(env, vec_at(&deserialized.generic_args, 0));
    //    return str_view_cstr_is_equal(deserialized.atom.str.base, "Slice");
    //}
    //return str_view_cstr_is_equal(lang_struct.atom.str.base, "Slice");
}

#endif // LANG_TYPE_IS
