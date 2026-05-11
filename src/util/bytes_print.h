#ifndef BYTES_PRINT_H
#define BYTES_PRINT_H

#include <local_string.h>

static Strv bytes_print_internal(Bytes bytes) {
    return strv_from_f(&a_temp, "%"PRIu64, bytes.value);
}

#define bytes_print(bytes) strv_print(bytes_print_internal(bytes))

#endif // BYTES_PRINT_H
