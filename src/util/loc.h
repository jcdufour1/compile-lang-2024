#ifndef LOC_H
#define LOC_H

#include <strv_struct.h>
#include <local_string.h>

typedef struct {
    const char* file;
    int line;
} Loc;

#define loc_new() ((Loc) {.file = __FILE__, .line = __LINE__})

static inline Strv loc_print_internal(Loc loc) {
    String buf = {0};

    string_extend_cstr(&a_temp, &buf, " /* loc ");
    string_extend_cstr(&a_temp, &buf, loc.file);
    string_extend_cstr(&a_temp, &buf, ":");
    string_extend_int(&a_temp, &buf, loc.line);
    string_extend_cstr(&a_temp, &buf, " */");

    return string_to_strv(buf);
}

#define loc_print(loc) \
    strv_print(loc_print_internal(loc))

#ifdef NDEBUG
#   define loc_get(item) (Loc) {0}
#else
#   define loc_get(item) (item)->loc
#endif // NDEBUG

#endif // LOC_H
