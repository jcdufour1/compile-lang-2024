#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "tokens.h"

typedef enum {
    PARSE_NORMAL,
    PARSE_FUN_PARAMS,
    PARSE_FUN_BODY,
    PARSE_FUN_RETURN_TYPES,
    PARSE_FUN_SINGLE_RETURN_TYPE,
    PARSE_FUN_ARGUMENTS,
    PARSE_FUN_SINGLE_ARGUMENT,
} PARSE_STATE;

void parse(const Tokens tokens);

#endif // PARSER_H
