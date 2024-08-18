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

#define PARSE_STATE_FMT "%s"

static inline const char* parse_state_print(PARSE_STATE state) {
    switch (state) {
        case PARSE_NORMAL:
            return "normal";
        case PARSE_FUN_PARAMS:
            return "fun_params";
        case PARSE_FUN_BODY:
            return "fun_body";
        case PARSE_FUN_RETURN_TYPES:
            return "fun_return_types";
        case PARSE_FUN_SINGLE_RETURN_TYPE:
            return "fun_single_return_type";
        case PARSE_FUN_ARGUMENTS:
            return "fun_args";
        case PARSE_FUN_SINGLE_ARGUMENT:
            return "fun_single_arg";
        default:
            unreachable();
    }
}

void parse(const Tokens tokens);

#endif // PARSER_H
