// TODO: rename this file to local_string.c
#include <local_string.h>

#ifdef IN_AUTOGEN
    static Arena fake_a_leak = {0};
#   define A_LEAK_THIS_FILE fake_a_leak
#else
#   define A_LEAK_THIS_FILE a_leak
#endif // IN_AUTOGEN


static void string_extend_f_va(Arena* arena, String* string, const char* format, va_list args1) {
    va_list args2;
    va_copy(args2, args1);

    static String temp_buf = {0};

    size_t count_needed = (size_t)vsnprintf(NULL, 0, format, args1);
    count_needed++;
    if (count_needed > temp_buf.info.count) {
        while (temp_buf.info.count < count_needed) {
            darr_append(&A_LEAK_THIS_FILE, &temp_buf, '\0');
        }
    }
    vsnprintf(temp_buf.buf, count_needed, format, args2);

    string_extend_strv(arena, string, (Strv) {temp_buf.buf, count_needed - 1});

    va_end(args2);
}

__attribute__((format (printf, 3, 4)))
void string_extend_f(Arena* arena, String* string, const char* format, ...) {
    va_list args1;
    va_start(args1, format);

    string_extend_f_va(arena, string, format, args1);

    va_end(args1);
}

__attribute__((format (printf, 2, 3)))
Strv strv_from_f(Arena* arena, const char* format, ...) {
    va_list args1;
    va_start(args1, format);

    String buf = {0};
    string_extend_f_va(arena, &buf, format, args1);

    va_end(args1);

    return string_to_strv(buf);
}

#define char_repr_local_thing(arena, ch) \
    do { \
        String buf = {0}; \
        darr_append(arena, &buf, ch); \
        return string_to_strv(buf); \
    } while(0);

Strv char_repr(Arena* arena, char ch) {
    String buf = {0}; \

    if (isalnum(ch)) {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '_') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '/') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '\\') {
        darr_append(arena, &buf, '\\');
        darr_append(arena, &buf, '\\');
        return string_to_strv(buf);
    }
    todo();
}
