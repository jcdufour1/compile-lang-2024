#ifndef EXTEND_NAME_H
#define EXTEND_NAME_H

#include <util.h>
#include <newstring.h>
#include <ulang_type_print.h>

// TODO: move this function elsewhere
static inline void extend_name(bool is_llvm, String* buf, Name name) {
    string_extend_strv(&print_arena, buf, name.mod_path);
    if (!is_llvm && name.mod_path.count > 0) {
        string_extend_cstr(&print_arena, buf, "::");
    }
    string_extend_strv(&print_arena, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&print_arena, buf, ", ");
        }
        string_extend_strv(&print_arena, buf, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, ">)");
    }
}

#define name_print(name) str_view_print(name_print_internal(false, name))

#endif // EXTEND_NAME_H
