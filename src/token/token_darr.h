#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>
#include "string.h"
#include "token.h"
#include "util.h"
#include "darr.h"

typedef struct {
    Vec_base info;
    Token* buf;
} Token_darr;

#endif // TOKENS_H
