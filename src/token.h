#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include "str_view.h"

typedef enum {
    TOKEN_SYMBOL
} TOKEN_TYPE;

typedef struct {
    Str_view text;
    TOKEN_TYPE type;
} Token;

static inline void Token_init(Token* token) {
    memset(token, 0, sizeof(*token));
}

static inline char* Token_type_print(TOKEN_TYPE type) {
    switch (type) {
        case TOKEN_SYMBOL:
            return "sym";
            break;
        default:
            unreachable();
    }
}

#define TOKEN_FMT "%s("STRV_FMT")"

#define Token_print(token) Token_type_print((token).type), Strv_print((token).text)

#endif // TOKEN_H

