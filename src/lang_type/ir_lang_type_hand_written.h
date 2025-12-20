#ifndef IR_LANG_TYPE_HAND_WRITTEN
#define IR_LANG_TYPE_HAND_WRITTEN

#include <strv.h>
#include <darr.h>
#include <ulang_type.h>
#include <strv.h>
#include <darr.h>
#include <name.h>
#include <parameters.h>

struct Ir_lang_type_;
typedef struct Ir_lang_type_ Ir_lang_type;

typedef struct {
    Vec_base info;
    Ir_lang_type* buf;
} Ir_lang_type_darr;

#endif // IR_LANG_TYPE_HAND_WRITTEN
