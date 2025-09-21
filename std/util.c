#include <stdio.h>

typedef struct {
    char* buf;
    size_t count;
} Slice;

int own_printf(Slice format, ...) {
    va_list args;
    va_start(args, n);  

    vnprintf(buf, "%.*", format.count, format.buf, args);

    va_end(args);
}
