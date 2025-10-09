#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char* buf;
    size_t count;
} Str;

typedef struct {
    Str* buf;
    size_t count;
} Str_slice;

typedef struct {
    FILE* file;
} File;

#define BUF_SIZE 4096

int own_actual_main(void);

// TODO: deduplicate own_printf and own_fprintf
int own_printf(Str format, ...) {
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

int own_fprintf(File file, Str format, ...) {
    va_list args;
    va_start(args, format);  

    static char format_buf[BUF_SIZE];
    int status = sprintf(format_buf, "%.*s", (int)format.count, format.buf);
    if (status < 0) {
        goto end;
    }

    status = vfprintf(file.file, format_buf, args);

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

static char** actual_argv = NULL;
static int actual_argv_count = 0;
static Str_slice own_argv_cache = (Str_slice) {0};

Str_slice own_argv(void) {
    if (actual_argv_count < 1) {
        return (Str_slice) {0};
    }
    if (own_argv_cache.count > 0) {
        return own_argv_cache;
    }
    
    own_argv_cache.buf = malloc(sizeof(*own_argv_cache.buf)*actual_argv_count);
    if (!own_argv_cache.buf) {
        fprintf(stderr, "fetal error: out of memory (when allocating space for argv)\n");
        abort();
    }
    for (size_t idx = 0; idx < actual_argv_count; idx++) {
        own_argv_cache.buf[idx] = (Str) {.buf = actual_argv[idx], .count = strlen(actual_argv[idx])};
    }
    own_argv_cache.count = actual_argv_count;

    return own_argv_cache;
}

// TODO: some of these funtions and globals are part of the runtime. put them in runtime directory, etc.?
int main(int argc, char** argv) {
    actual_argv = argv;
    actual_argv_count = argc;
    return own_actual_main();
}

