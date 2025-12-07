#ifndef TOKEN_VIEW_H
#define TOKEN_VIEW_H

#include <stddef.h>
#include "token.h"
#include "assert.h"
#include <local_math.h>

typedef struct {
    const Token* tokens;
    size_t count;
} Tk_view;

static inline Token tk_view_at(Tk_view tk_view, size_t idx) {
    unwrap(tk_view.count > idx);
    return tk_view.tokens[idx];
}

static inline Token tk_view_front(Tk_view tk_view) {
    return tk_view_at(tk_view, 0);
}

static inline void log_tokens_internal(const char* file, int line, LOG_LEVEL log_level, Tk_view tk_view) {
    log_internal(log_level, file, line, 0, "tokens:\n");
    for (size_t idx = 0; idx < tk_view.count; idx++) {
        log_internal(log_level, file, line, 0, FMT"\n", token_print(TOKEN_MODE_LOG, tk_view_at(tk_view, idx)));
    }
    log_internal(log_level, file, line, 0, "\n");
}

#define log_tokens(log_level, tk_view) \
    log_tokens_internal(__FILE__, __LINE__, log_level, tk_view)

static inline Tk_view tk_view_consume_count(Tk_view* tk_view, size_t count) {
    if (tk_view->count < count) {
        unreachable("out of bounds");
    }
    Tk_view result = {.tokens = tk_view->tokens, .count = count};
    tk_view->tokens += count;
    tk_view->count -= count;
    return result;
}

static inline Token tk_view_consume(Tk_view* tk_view) {
    return tk_view_front(tk_view_consume_count(tk_view, 1));
}

static inline bool tk_view_try_consume(Token* result, Tk_view* tokens, TOKEN_TYPE expected) {
    if (tokens->count < 1 || tk_view_front(*tokens).type != expected) {
        return false;
    }
    
    Token token = tk_view_consume(tokens);
    if (result) {
        *result = token;
    }
    unwrap(result);
    return true;
}

static inline bool tk_view_try_consume_symbol(Token* result, Tk_view* tokens, const char* cstr) {
    if (!strv_is_equal(tk_view_front(*tokens).text, sv(cstr))) {
        return false;
    }
    return tk_view_try_consume(result, tokens, TOKEN_SYMBOL);
}

static inline Strv tk_view_print_internal(Arena* arena, Tk_view tk_view) {
    String buf = {0};
    vec_reset(&buf);

    for (size_t idx = 0; idx < tk_view.count; idx++) {
        string_extend_strv(&a_temp, &buf, token_print_internal(arena, TOKEN_MODE_LOG, tk_view_at(tk_view, idx)));
        string_extend_cstr(&a_temp, &buf, ";    ");
    }

    Strv strv = {.str = buf.buf, .count = buf.info.count};
    return strv;
}

static inline bool tk_view_is_equal_internal(LOG_LEVEL log_level, Tk_view a, Tk_view b, bool do_log) {
    for (size_t idx = 0; idx < min(a.count, b.count); idx++) {
        if (!token_is_equal(tk_view_at(a, idx), tk_view_at(b, idx))) {
            if (do_log) {
                log(log_level, "TOKENS actual:\n");
                log_tokens(log_level, a);
                log(log_level, "TOKENS expected:\n");
                log_tokens(log_level, b);
                log(
                    log_level, "idx %zu: "FMT" is not equal to "FMT"\n",
                    idx, token_print(TOKEN_MODE_LOG, tk_view_at(a, idx)), token_print(TOKEN_MODE_LOG, tk_view_at(b, idx))
                );
            }
            return false;
        }
    }

    if (a.count != b.count) {
        if (do_log) {
            log(log_level, "count is not equal\n");
            log(log_level, "TOKENS actual:\n");
            log_tokens(log_level, a);
            log(log_level, "TOKENS expected:\n");
            log_tokens(log_level, b);
        }
        return false;
    }

    return true;
}

static inline bool tk_view_is_equal(Tk_view a, Tk_view b) {
    return tk_view_is_equal_internal(LOG_TRACE, a, b, false);
}

static inline bool tk_view_is_equal_log(LOG_LEVEL log_level, Tk_view a, Tk_view b) {
    return tk_view_is_equal_internal(log_level, a, b, true);
}

#define tk_view_print(tk_view) strv_print(tk_view_print_internal(tk_view))

#endif // TOKEN_VIEW_H
