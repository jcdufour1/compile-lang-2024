#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "tokens.h"
#include "node.h"

Node* parse(const Tokens tokens);

#endif // PARSER_H
