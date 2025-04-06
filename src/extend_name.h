#ifndef EXTEND_NAME_H
#define EXTEND_NAME_H

#include <util.h>
#include <newstring.h>

// TODO: move this function elsewhere
static inline void extend_name(bool is_llvm, String* buf, Name name) {
    string_extend_strv(&print_arena, buf, name.mod_path);
    if (!is_llvm && name.mod_path.count > 0) {
        string_extend_cstr(&print_arena, buf, "::");
    }
    string_extend_strv(&print_arena, buf, name.base);
}

#endif // EXTEND_NAME_H
