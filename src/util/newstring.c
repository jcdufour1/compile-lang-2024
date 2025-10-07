#include <newstring.h>
#include <env.h>

void string_extend_f(Arena* arena, String* string, const char* format, ...) {
    // TODO: this function does not work
    todo();

    va_list args1;
    va_start(args1, format);
    va_list args2;
    va_copy(args2, args1);

    size_t count_needed = vsnprintf(env.sprintf_buf.buf, env.sprintf_buf.info.count, format, args1);
    if (count_needed > env.sprintf_buf.info.count) {
        while (env.sprintf_buf.info.count < count_needed) {
            vec_append(&a_main, &env.sprintf_buf, '\0');
        }
        vsnprintf(env.sprintf_buf.buf, env.sprintf_buf.info.count, format, args2);
    }

    log(LOG_DEBUG, FMT"\n", string_print(env.sprintf_buf));
    log(LOG_DEBUG, "%zu\n", count_needed);
    log(LOG_DEBUG, FMT"\n", strv_print(((Strv) {env.sprintf_buf.buf, count_needed - 1})));
    string_extend_strv(arena, string, (Strv) {env.sprintf_buf.buf, count_needed - 1});

    va_end(args1);
    va_end(args1);
}

