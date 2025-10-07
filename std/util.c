#include <stdio.h>
#include <stdarg.h>

typedef struct {
    char* buf;
    size_t count;
} Slice;

#define BUF_SIZE 4096

int own_printf(Slice format, ...) {
    va_list args;
    va_start(args, format);  

    static char format_buf[BUF_SIZE];
    int status = sprintf(format_buf, "%.*s", (int)format.count, format.buf);
    if (status < 0) {
        goto end;
    }

    status = vprintf(format_buf, args);

end:
    va_end(args);
    return status;
}

FILE* own_stdout(void) {
    return stdout;
}

FILE* own_stderr(void) {
    return stderr;
}

FILE* own_stdin(void) {
    return stdin;
}
