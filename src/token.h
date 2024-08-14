#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include "str_view.h"

typedef struct {
    Str_view text;
} Token;

static inline void Token_init(Token* token) {
    memset(token, 0, sizeof(*token));
}

#endif // TOKEN_H

