#include <stdbool.h>
#include "util.h"
#include "token.h"
#include "tokenizer.h"
#include "str_view.h"
#include "parameters.h"
#include <ctype.h>

static bool local_isalnum_or_underscore(char prev, char curr) {
    (void) prev;
    return isalnum(curr) || curr == '_';
}

static bool local_isdigit(char prev, char curr) {
    (void) prev;
    return isdigit(curr);
}

static bool is_not_quote(char prev, char curr) {
    return curr != '"' || prev == '\\';
}

static bool is_equal(char prev, char curr) {
    (void) prev;
    return curr == '=';
}

static bool is_dot(char prev, char curr) {
    (void) prev;
    return curr == '.';
}

static void trim_whitespace(Str_view* file_text, size_t* line_num) {
    while (file_text->count > 0 && (isspace(str_view_front(*file_text)) || iscntrl(str_view_front(*file_text)))) {
        char ch = str_view_consume(file_text);
        if (ch == '\n') {
            (*line_num)++;
        }
    }
}

static bool get_next_token(size_t* line_num, Token* token, Str_view* file_text, Parameters params) {
    memset(token, 0, sizeof(*token));

    trim_whitespace(file_text, line_num);

    if (file_text->count < 1) {
        return false;
    }

    token->pos.line = *line_num;
    token->pos.file_path = params.input_file_name;

    if (isalpha(str_view_front(*file_text))) {
        token->text = str_view_consume_while(file_text, local_isalnum_or_underscore);
        token->type = TOKEN_SYMBOL;
        return true;
    } else if (isdigit(str_view_front(*file_text))) {
        token->text = str_view_consume_while(file_text, local_isdigit);
        token->type = TOKEN_NUM_LITERAL;
        return true;
    } else if (str_view_try_consume(file_text, '(')) {
        token->type = TOKEN_OPEN_PAR;
        return true;
    } else if (str_view_try_consume(file_text, ')')) {
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (str_view_try_consume(file_text, '{')) {
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (str_view_try_consume(file_text, '}')) {
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (str_view_try_consume(file_text, '"')) {
        token->type = TOKEN_STRING_LITERAL;
        token->text = str_view_consume_while(file_text, is_not_quote);
        try(str_view_try_consume(file_text, '"'));
        return true;
    } else if (str_view_try_consume(file_text, ';')) {
        token->type = TOKEN_SEMICOLON;
        return true;
    } else if (str_view_try_consume(file_text, ',')) {
        token->type = TOKEN_COMMA;
        return true;
    } else if (str_view_try_consume(file_text, '+')) {
        assert((file_text->count < 1 || str_view_front(*file_text) != '+') && "double + not implemented");
        token->type = TOKEN_SINGLE_PLUS;
        return true;
    } else if (str_view_try_consume(file_text, '-')) {
        token->type = TOKEN_SINGLE_MINUS;
        return true;
    } else if (str_view_try_consume(file_text, '*')) {
        // TODO: * may not always be multiplication
        token->type = TOKEN_ASTERISK;
        return true;
    } else if (file_text->count > 1 && str_view_cstr_is_equal(str_view_slice(*file_text, 0, 2), "//")) {
        str_view_consume_until(file_text, '\n');
        trim_whitespace(file_text, line_num);
        token->type = TOKEN_COMMENT;
        return true;
    } else if (str_view_try_consume(file_text, '/')) {
        token->type = TOKEN_SLASH;
        return true;
    } else if (str_view_try_consume(file_text, ':')) {
        token->type = TOKEN_COLON;
        return true;
    } else if (str_view_try_consume(file_text, '!')) {
        if (str_view_try_consume(file_text, '=')) {
            token->type = TOKEN_NOT_EQUAL;
            return true;
        }
        token->type = TOKEN_NOT;
        return true;
    } else if (str_view_front(*file_text) == '=') {
        Str_view equals = str_view_consume_while(file_text, is_equal);
        if (equals.count == 1) {
            token->type = TOKEN_SINGLE_EQUAL;
            return true;
        } else if (equals.count == 2) {
            token->type = TOKEN_DOUBLE_EQUAL;
            return true;
        } else {
            todo();
        }
    } else if (str_view_try_consume(file_text, '>')) {
        assert((file_text->count < 1 || str_view_front(*file_text) != '=') && ">= not implemented");
        token->type = TOKEN_GREATER_THAN;
        return true;
    } else if (str_view_try_consume(file_text, '<')) {
        assert((file_text->count < 1 || str_view_front(*file_text) != '=') && ">= not implemented");
        token->type = TOKEN_LESS_THAN;
        return true;
    } else if (str_view_front(*file_text) == '.') {
        Str_view dots = str_view_consume_while(file_text, is_dot);
        if (dots.count == 1) {
            token->type = TOKEN_SINGLE_DOT;
            return true;
        } else if (dots.count == 2) {
            token->type = TOKEN_DOUBLE_DOT;
            return true;
        } else if (dots.count == 3) {
            token->type = TOKEN_TRIPLE_DOT;
            return true;
        } else {
            todo();
        }
    } else {
        log(LOG_FETAL, "unknown symbol: %c (%x)\n", str_view_front(*file_text), str_view_front(*file_text));
        todo();
    }
}

Tokens tokenize(const String file_text, Parameters params) {
    Tokens tokens = {0};

    Str_view curr_file_text = {.str = file_text.buf, .count = file_text.info.count};

    size_t line_num = 1;
    Token curr_token;
    while (get_next_token(&line_num, &curr_token, &curr_file_text, params)) {
        //log(LOG_TRACE, "token received: "TOKEN_FMT"\n", token_print(curr_token));
        if (curr_token.type != TOKEN_COMMENT) {
            tokens_append(&tokens, &curr_token);
        }
    }

    return tokens;
}

