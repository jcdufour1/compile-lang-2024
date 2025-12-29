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

Strv char_repr(Arena* arena, char ch) {
    String buf = {0};

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
    if (ch == '{') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '}') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '(') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == ')') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '?') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '.') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == ':') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '=') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == ' ') {
        darr_append(arena, &buf, ch);
        return string_to_strv(buf);
    }
    if (ch == '\n') {
        darr_append(arena, &buf, '\\');
        darr_append(arena, &buf, 'n');
        return string_to_strv(buf);
    }
    if (ch == '\\') {
        // TODO: make string_append function
        darr_append(arena, &buf, '\\');
        darr_append(arena, &buf, '\\');
        return string_to_strv(buf);
    }

    string_extend_f(arena, &buf, "\\x%02x", ch);
    return string_to_strv(buf);
}

Strv strv_repr(Arena* arena, Strv strv) {
    String buf = {0};
    for (size_t idx = 0; idx < strv.count; idx++) {
        string_extend_strv(arena, &buf, char_repr(arena, strv_at(strv, idx)));
    }
    return string_to_strv(buf);
}

Strv strv_replace(Arena* arena, Strv strv, Strv find, Strv replace_with) {
    String buf = {0};
    size_t idx = 0;
    while (idx < strv.count) {
        Strv potential_find = strv_slice(strv, idx, strv.count - idx);
        if (strv_starts_with(potential_find, find)) {
            string_extend_strv(arena, &buf, replace_with);
            idx += find.count;
        } else {
            darr_append(arena, &buf, strv_at(strv, idx));
            idx++;
        }
    }

    return string_to_strv(buf);
}
