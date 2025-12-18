#ifndef misc_print_functions_h
#define misc_print_functions_h

#include <strv.h>

static inline Strv uint32_t_print_internal(uint32_t num) {
    return strv_from_f(&a_temp, "%"PRIu32, num);
}

#define uint32_t_print(num) \
    strv_print(uint32_t_print_internal(num))

#endif // misc_print_functions_h
