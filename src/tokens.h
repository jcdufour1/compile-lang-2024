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

#endif // TOKENS_H
