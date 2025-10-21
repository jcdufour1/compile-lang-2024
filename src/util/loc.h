#ifndef LOC_H
#define LOC_H

#include <strv_struct.h>
#include <newstring.h>

typedef struct {
    const char* file;
    int line;
} Loc;

#define loc_new() ((Loc) {.file = __FILE__, .line = __LINE__})

static inline Strv loc_print_internal(Loc loc) {
    String buf = {0};

    string_extend_cstr(&a_print, &buf, "loc(");
    string_extend_cstr(&a_print, &buf, loc.file);
    string_extend_cstr(&a_print, &buf, ":");
    string_extend_size_t(&a_print, &buf, loc.line);
    string_extend_cstr(&a_print, &buf, ")");

    return string_to_strv(buf);
}

#define loc_print(loc) \
    strv_print(loc_print_internal(loc))

#endif // LOC_H
