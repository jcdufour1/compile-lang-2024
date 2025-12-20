#ifndef LANG_TYPE_HAND_WRITTEN
#define LANG_TYPE_HAND_WRITTEN

#include <strv.h>
#include <darr.h>
#include <ulang_type.h>
#include <name.h>

struct Lang_type_;
typedef struct Lang_type_ Lang_type;

typedef struct {
    Vec_base info;
    Lang_type* buf;
} Lang_type_darr;

struct Tast_struct_literal_;
typedef struct Tast_struct_literal_ Tast_struct_literal;

#endif // LANG_TYPE_HAND_WRITTEN
