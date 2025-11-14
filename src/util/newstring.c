#include <newstring.h>

__attribute__((format (printf, 3, 4)))
void string_extend_f(Arena* arena, String* string, const char* format, ...) {
    va_list args1;
    va_start(args1, format);
    va_list args2;
    va_copy(args2, args1);

    static String temp_buf = {0};

    size_t count_needed = (size_t)vsnprintf(NULL, 0, format, args1);
    count_needed++;
    if (count_needed > temp_buf.info.count) {
        while (temp_buf.info.count < count_needed) {
            vec_append(&a_leak, &temp_buf, '\0');
        }
    }
    vsnprintf(temp_buf.buf, count_needed, format, args2);

    string_extend_strv(arena, string, (Strv) {temp_buf.buf, count_needed - 1});

    va_end(args1);
    va_end(args1);
}

