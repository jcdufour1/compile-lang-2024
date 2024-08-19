#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "tokens.h"
#include "node.h"

Node_id parse(const Tokens tokens);

#endif // PARSER_H
