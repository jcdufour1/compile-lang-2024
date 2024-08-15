#include <stdbool.h>
#include "util.h"
#include "token.h"
#include "tokenizer.h"
#include "str_view.h"
#include <ctype.h>

static bool local_isalnum(char prev, char curr) {
    (void) prev;
    return isalnum(curr);
}

static bool is_not_quote(char prev, char curr) {
    return curr != '"' || prev == '\\';
}

static bool get_next_token(Token* token, Str_view* file_text) {
    if (file_text->count < 1) {
        return false;
    }

    Token_init(token);

    while (file_text->count > 0 && (isspace(strv_front(*file_text)) || iscntrl(strv_front(*file_text)))) {
        strv_chop_front(file_text);
    }

    if (file_text->count < 1) {
        return false;
    }

    if (isalpha(strv_front(*file_text))) {
        token->text = strv_chop_on_cond(file_text, local_isalnum);
        token->type = TOKEN_SYMBOL;
        return true;
    } else if (strv_front(*file_text) == '(') {
        strv_chop_front(file_text);
        token->type = TOKEN_OPEN_PAR;
        return true;
    } else if (strv_front(*file_text) == ')') {
        strv_chop_front(file_text);
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (strv_front(*file_text) == '{') {
        strv_chop_front(file_text);
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (strv_front(*file_text) == '}') {
        strv_chop_front(file_text);
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (strv_front(*file_text) == '"') {
        token->type = TOKEN_STRING_LITERAL;
        strv_chop_front(file_text);
        token->text = strv_chop_on_cond(file_text, is_not_quote);
        strv_chop_front(file_text);
        return true;
    } else if (strv_front(*file_text) == ';') {
        strv_chop_front(file_text);
        token->type = TOKEN_SEMICOLON;
        return true;
    } else {
        log(LOG_FETAL, "unknown symbol: %c (%x)\n", strv_front(*file_text), strv_front(*file_text));
        todo();
    }
}

Tokens tokenize(const String file_text) {
    Tokens tokens;
    Tokens_init(&tokens);

    Str_view curr_file_text = {.str = file_text.buf, .count = file_text.count};

    Token curr_token;
    while (get_next_token(&curr_token, &curr_file_text)) {
        log(LOG_TRACE, "token received: "TOKEN_FMT"\n", Token_print(curr_token));
        Tokens_append(&tokens, &curr_token);
    }

    return tokens;
}

