#ifndef EXTEND_NAME_H
#define EXTEND_NAME_H

#include <util.h>
#include <newstring.h>

// TODO: move this function elsewhere
static inline void extend_name(bool is_llvm, String* buf, Name name) {
    if (!is_llvm) {
        string_extend_cstr(&print_arena, buf, "(");
    }
    string_extend_strv(&print_arena, buf, name.mod_path);
    if (!is_llvm) {
        string_extend_cstr(&print_arena, buf, "::");
    }
    string_extend_strv(&print_arena, buf, name.base);
    if (!is_llvm) {
        string_extend_cstr(&print_arena, buf, ")");
    }
}

#endif // EXTEND_NAME_H
