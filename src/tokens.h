#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>
#include "string.h"
#include "token.h"
#include "util.h"

#define TOKENS_DEFAULT_CAPACITY 512

typedef struct {
    Token* buf;
    size_t count;
    size_t capacity;
} Tokens;

static inline void Tokens_init(Tokens* token) {
    memset(token, 0, sizeof(*token));
}

static inline void Tokens_reserve(Tokens* tokens, size_t minimum_count_empty_slots) {
    // tokens->capacity must be at least one greater than tokens->count for null termination
    while (tokens->count + minimum_count_empty_slots + 1 > tokens->capacity) {
        if (tokens->capacity < 1) {
            tokens->capacity = TOKENS_DEFAULT_CAPACITY;
            tokens->buf = safe_malloc(tokens->capacity);
        } else {
            tokens->capacity *= 2;
            tokens->buf = safe_realloc(tokens->buf, tokens->capacity);
        }
    }
}

// string->buf is always null terminated
static inline void Tokens_append(Tokens* tokens, const Token* token) {
    Tokens_reserve(tokens, 1);
    tokens->buf[tokens->count++] = *token;
}

#endif // TOKENS_H
