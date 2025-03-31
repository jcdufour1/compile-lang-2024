#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>
#include "string.h"
#include "token.h"
#include "util.h"
#include "vector.h"

// TODO: change to Token_vec for consistancy
typedef struct {
    Vec_base info;
    Token* buf;
} Tokens;

#endif // TOKENS_H
