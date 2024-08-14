#include <stdbool.h>
#include "util.h"
#include "token.h"
#include "tokenizer.h"
#include "str_view.h"
#include <ctype.h>

static bool local_isalnum(char ch) {
    return isalnum(ch);
}

static bool get_next_token(Token* token, Str_view* file_text) {
    if (file_text->count < 1) {
        return false;
    }

    Token_init(token);

    while (strv_front(*file_text) == '\n') {
        strv_chop_front(file_text);
    }

    if (isalpha(strv_front(*file_text))) {
        token->text = strv_chop_on_cond(file_text, local_isalnum);
        token->type = TOKEN_SYMBOL;
        log(LOG_TRACE, TOKEN_FMT"\n", Token_print(*token));
        return true;
    } else {
        todo();
    }
}

Tokens tokenize(const String file_text) {
    Tokens tokens;
    Tokens_init(&tokens);

    Str_view curr_file_text = {.str = file_text.buf, .count = file_text.count};

    Token curr_token;
    while (get_next_token(&curr_token, &curr_file_text)) {
        Tokens_append(&tokens, &curr_token);
    }

    return tokens;
}

