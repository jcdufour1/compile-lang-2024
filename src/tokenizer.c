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

static bool local_isdigit(char prev, char curr) {
    (void) prev;
    return isdigit(curr);
}

static bool is_not_quote(char prev, char curr) {
    return curr != '"' || prev == '\\';
}

static bool get_next_token(size_t* line_num, Token* token, Str_view* file_text) {
    memset(token, 0, sizeof(*token));

    while (file_text->count > 0 && (isspace(str_view_front(*file_text)) || iscntrl(str_view_front(*file_text)))) {
        Str_view ch = str_view_chop_front(file_text);
        if (str_view_front(ch) == '\n') {
            (*line_num)++;
        }
    }

    if (file_text->count < 1) {
        return false;
    }

    token->line_num = *line_num;

    if (isalpha(str_view_front(*file_text))) {
        token->text = str_view_chop_on_cond(file_text, local_isalnum);
        token->type = TOKEN_SYMBOL;
        return true;
    } else if (isdigit(str_view_front(*file_text))) {
        token->text = str_view_chop_on_cond(file_text, local_isdigit);
        token->type = TOKEN_NUM_LITERAL;
        return true;
    } else if (str_view_front(*file_text) == '(') {
        str_view_chop_front(file_text);
        token->type = TOKEN_OPEN_PAR;
        return true;
    } else if (str_view_front(*file_text) == ')') {
        str_view_chop_front(file_text);
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (str_view_front(*file_text) == '{') {
        str_view_chop_front(file_text);
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (str_view_front(*file_text) == '}') {
        str_view_chop_front(file_text);
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (str_view_front(*file_text) == '"') {
        token->type = TOKEN_STRING_LITERAL;
        str_view_chop_front(file_text);
        token->text = str_view_chop_on_cond(file_text, is_not_quote);
        str_view_chop_front(file_text);
        return true;
    } else if (str_view_front(*file_text) == ';') {
        str_view_chop_front(file_text);
        token->type = TOKEN_SEMICOLON;
        return true;
    } else if (str_view_front(*file_text) == ',') {
        str_view_chop_front(file_text);
        token->type = TOKEN_COMMA;
        return true;
    } else if (str_view_front(*file_text) == '+') {
        str_view_chop_front(file_text);
        token->type = TOKEN_PLUS_SIGN;
        return true;
    } else if (str_view_front(*file_text) == '-') {
        str_view_chop_front(file_text);
        token->type = TOKEN_MINUS_SIGN;
        return true;
    } else {
        log(LOG_FETAL, "unknown symbol: %c (%x)\n", str_view_front(*file_text), str_view_front(*file_text));
        todo();
    }
}

Tokens tokenize(const String file_text) {
    Tokens tokens = {0};

    Str_view curr_file_text = {.str = file_text.buf, .count = file_text.count};

    size_t line_num = 0;
    Token curr_token;
    while (get_next_token(&line_num, &curr_token, &curr_file_text)) {
        log(LOG_TRACE, "token received: "TOKEN_FMT"\n", token_print(curr_token));
        tokens_append(&tokens, &curr_token);
    }

    return tokens;
}

