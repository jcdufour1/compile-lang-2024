#ifndef ULANG_TYPE_HAND_WRITTEN
#define ULANG_TYPE_HAND_WRITTEN

#include <name.h>

typedef enum {
    LANG_TYPE_MODE_LOG,
    LANG_TYPE_MODE_MSG,
    LANG_TYPE_MODE_EMIT_LLVM,
    LANG_TYPE_MODE_EMIT_C,
} LANG_TYPE_MODE;

struct Uast_expr_;
typedef struct Uast_expr_ Uast_expr;

struct Uast_struct_literal_;
typedef struct Uast_struct_literal_ Uast_struct_literal;

#define ulang_type_print(mode, lang_type) strv_print(ulang_type_print_internal((mode), (lang_type)))

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type);

#endif // ULANG_TYPE_HAND_WRITTEN
