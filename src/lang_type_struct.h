#ifndef LANG_TYPE_STRUCT
#define LANG_TYPE_STRUCT

#include <str_view_struct.h>
#include <stdint.h>

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type;

#endif // LANG_TYPE_STRUCT
