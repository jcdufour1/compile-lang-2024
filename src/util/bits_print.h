#ifndef BITS_PRINT_H
#define BITS_PRINT_H

#include <local_string.h>

static Strv bits_print_internal(Bits bits) {
    return strv_from_f(&a_temp, "%"PRIu64, bits.value);
}

#define bits_print(bits) strv_print(bits_print_internal(bits))

#endif // BITS_PRINT_H
