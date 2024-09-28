#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>
#include "string.h"
#include "token.h"
#include "util.h"
#include "vector.h"

#define TOKENS_DEFAULT_CAPACITY 512

typedef struct {
    Vec_base info;
    Token* buf;
} Tokens;

static inline void tokens_reserve(Tokens* tokens, size_t minimum_count_empty_slots) {
    vector_reserve(&arena, tokens, sizeof(tokens->buf[0]), minimum_count_empty_slots, TOKENS_DEFAULT_CAPACITY);
}

static inline void tokens_append(Tokens* tokens, const Token* token) {
    vector_append(&arena, tokens, sizeof(tokens->buf[0]), token, TOKENS_DEFAULT_CAPACITY);
}

#endif // TOKENS_H
