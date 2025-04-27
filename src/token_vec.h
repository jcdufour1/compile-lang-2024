#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>
#include "string.h"
#include "token.h"
#include "util.h"
#include "vector.h"

typedef struct {
    Vec_base info;
    Token* buf;
} Token_vec;

#endif // TOKENS_H
