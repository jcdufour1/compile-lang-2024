#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "token.h"
#include "tokens.h"
#include "newstring.h"
#include "parameters.h"

Tokens tokenize(const String file_text, Parameters params);

#endif // TOKENIZER_H
