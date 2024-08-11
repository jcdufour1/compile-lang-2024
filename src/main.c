#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define STRING_DEFAULT_CAPACITY 4048

#define todo() \
    do { \
        fprintf(stderr, "%s:%d:error: not implemented", __FILE__, __LINE__); \
        abort(); \
    } while (0);

typedef struct {
    char* buf;
    size_t count;
    size_t capacity;
} String;

typedef struct {
    String file_name;
} Parameters;

void String_init(String* str) {
    memset(str, 0, sizeof(*str));
}

void* safe_realloc(void* old_ptr, size_t new_capacity) {
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        todo();
    }
    return new_ptr;
}

void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        todo();
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#define STRING_FMT "%.*s"

#define String_print(string) (int)((string).count), (string).buf

void String_reserve(String* str, size_t minimum_count_empty_slots) {
    // str->capacity must be at least one greater than str->count for null termination
    while (str->count + minimum_count_empty_slots + 1 > str->capacity) {
        if (str->capacity < 1) {
            str->capacity = STRING_DEFAULT_CAPACITY;
            str->buf = safe_malloc(str->capacity);
        } else {
            str->capacity *= 2;
            str->buf = safe_realloc(str->buf, str->capacity);
        }
    }
}

// string->buf is always null terminated
void String_append(String* str, char ch) {
    String_reserve(str, 1);
    str->buf[str->count++] = ch;
}

String String_from_cstr(const char* cstr) {
    String string;
    String_init(&string);
    for (int idx = 0; cstr[idx]; idx++) {
        String_append(&string, cstr[idx]);
    }
    return string;
}

Parameters parse_args(int argc, char** argv) {
    if (argc != 2) {
        todo();
    }

    Parameters params = {.file_name = String_from_cstr(argv[1])};
    return params; 
}

int read_file(String* file_text, const String* input_file_name) {
    assert(!input_file_name->buf[input_file_name->count]);
    FILE* file = fopen(input_file_name->buf, "rb");
    if (!file) {
        return 0;
    }

    size_t buf_size = 2024;
    size_t amount_read;
    // TODO: check for errors?
    do {
        String_reserve(file_text, buf_size);
        amount_read = fread(file_text->buf, 1, buf_size, file);
        file_text->count += amount_read;
    } while (amount_read > 0);

    return 1;
}

int main(int argc, char** argv) {
    Parameters params = parse_args(argc, argv);

    String file_text;
    String_init(&file_text);
    if (!read_file(&file_text, &params.file_name)) {
        fprintf(stderr, "fetal error: could not open file "STRING_FMT": errno %d (%s)", String_print(params.file_name), errno, strerror(errno));
        return 1;
    }
    printf("file:"STRING_FMT"\n", String_print(file_text));
    //
    // tokenize
    //
    // parse
    //
    // llvm thing

    return 0;
}
