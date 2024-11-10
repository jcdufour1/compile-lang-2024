#include <stdbool.h>
#include "../util.h"
#include "../token.h"
#include "../tokens.h"
#include "../token_view.h"
#include "../str_view_col.h"
#include "../parameters.h"
#include <ctype.h>

static void msg_tokenizer_invalid_token(Str_view_col text, Pos pos) {
    msg(LOG_ERROR, EXPECT_FAIL_INVALID_TOKEN, pos, "invalid token `"STR_VIEW_FMT"`\n", str_view_col_print(text));
}

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

static void trim_non_newline_whitespace(Str_view_col* file_text, Pos* pos) {
    if (file_text->base.count < 1) {
        return;
    }
    char curr = str_view_col_front(*file_text);
    while (file_text->base.count > 0 && (isspace(curr) || iscntrl(curr)) && (curr != '\n')) {
        if (file_text->base.count < 1) {
            return;
        }
        str_view_col_consume(pos, file_text);
        curr = str_view_col_front(*file_text);
    }
}

static bool get_next_token(Pos* pos, Token* token, Str_view_col* file_text, Parameters params) {
    memset(token, 0, sizeof(*token));

    trim_non_newline_whitespace(file_text, pos);

    if (file_text->base.count < 1) {
        return false;
    }

    pos->file_path = params.input_file_name;

    token->pos.column = pos->column + 1;
    token->pos.line = pos->line;
    token->pos.file_path = pos->file_path;

    if (isalpha(str_view_col_front(*file_text))) {
        Str_view text = str_view_col_consume_while(pos, file_text, local_isalnum_or_underscore).base;
        if (str_view_cstr_is_equal(text, "deref")) {
            token->type = TOKEN_DEREF;
        } else if (str_view_cstr_is_equal(text, "refer")) {
            token->type = TOKEN_REFER;
        } else if (str_view_cstr_is_equal(text, "unsafe_cast")) {
            token->type = TOKEN_UNSAFE_CAST;
        } else if (str_view_cstr_is_equal(text, "fn")) {
            token->type = TOKEN_FN;
        } else if (str_view_cstr_is_equal(text, "for")) {
            token->type = TOKEN_FOR;
        } else if (str_view_cstr_is_equal(text, "if")) {
            token->type = TOKEN_IF;
        } else if (str_view_cstr_is_equal(text, "return")) {
            token->type = TOKEN_RETURN;
        } else if (str_view_cstr_is_equal(text, "extern")) {
            token->type = TOKEN_EXTERN;
        } else if (str_view_cstr_is_equal(text, "struct")) {
            token->type = TOKEN_STRUCT;
        } else if (str_view_cstr_is_equal(text, "let")) {
            token->type = TOKEN_LET;
        } else if (str_view_cstr_is_equal(text, "in")) {
            token->type = TOKEN_IN;
        } else if (str_view_cstr_is_equal(text, "break")) {
            token->type = TOKEN_BREAK;
        } else {
            token->text = text;
            token->type = TOKEN_SYMBOL;
        }
        return true;
    } else if (isdigit(str_view_col_front(*file_text))) {
        token->text = str_view_col_consume_while(pos, file_text, local_isdigit).base;
        token->type = TOKEN_INT_LITERAL;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '(')) {
        token->type = TOKEN_OPEN_PAR;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, ')')) {
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '{')) {
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '}')) {
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '"')) {
        token->type = TOKEN_STRING_LITERAL;
        Str_view_col quote_str = {0};
        if (!str_view_col_try_consume_while(&quote_str, pos, file_text, is_not_quote)) {
            msg(LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE, token->pos, "unmatched `\"`\n");
            token->type = TOKEN_NONTYPE;
            return false;
        }
        try(str_view_col_try_consume(pos, file_text, '"'));
        token->text = quote_str.base;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, ';')) {
        token->type = TOKEN_SEMICOLON;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, ',')) {
        token->type = TOKEN_COMMA;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '+')) {
        assert((file_text->base.count < 1 || str_view_col_front(*file_text) != '+') && "double + not implemented");
        token->type = TOKEN_SINGLE_PLUS;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '-')) {
        token->type = TOKEN_SINGLE_MINUS;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '*')) {
        token->type = TOKEN_ASTERISK;
        return true;
    } else if (file_text->base.count > 1 && str_view_cstr_is_equal(str_view_slice(file_text->base, 0, 2), "//")) {
        str_view_col_consume_until(pos, file_text, '\n');
        trim_non_newline_whitespace(file_text, pos);
        token->type = TOKEN_COMMENT;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '/')) {
        token->type = TOKEN_SLASH;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, ':')) {
        token->type = TOKEN_COLON;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '!')) {
        if (str_view_col_try_consume(pos, file_text, '=')) {
            token->type = TOKEN_NOT_EQUAL;
            return true;
        }
        token->type = TOKEN_NOT;
        return true;
    } else if (str_view_col_front(*file_text) == '=') {
        Str_view_col equals = str_view_col_consume_while(pos, file_text, is_equal);
        if (equals.base.count == 1) {
            token->type = TOKEN_SINGLE_EQUAL;
            return true;
        } else if (equals.base.count == 2) {
            token->type = TOKEN_DOUBLE_EQUAL;
            return true;
        } else {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (str_view_col_try_consume(pos, file_text, '>')) {
        assert((file_text->base.count < 1 || str_view_col_front(*file_text) != '=') && ">= not implemented");
        token->type = TOKEN_GREATER_THAN;
        return true;
    } else if (str_view_col_try_consume(pos, file_text, '<')) {
        assert((file_text->base.count < 1 || str_view_col_front(*file_text) != '=') && ">= not implemented");
        token->type = TOKEN_LESS_THAN;
        return true;
    } else if (str_view_col_front(*file_text) == '.') {
        Str_view_col dots = str_view_col_consume_while(pos, file_text, is_dot);
        if (dots.base.count == 1) {
            token->type = TOKEN_SINGLE_DOT;
            return true;
        } else if (dots.base.count == 2) {
            token->type = TOKEN_DOUBLE_DOT;
            return true;
        } else if (dots.base.count == 3) {
            token->type = TOKEN_TRIPLE_DOT;
            return true;
        } else {
            msg_tokenizer_invalid_token(dots, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (str_view_col_try_consume(pos, file_text, '\n')) {
        while (str_view_col_try_consume(pos, file_text, '\n'));
        token->type = TOKEN_NEW_LINE;
        return true;
    } else {
        unreachable("unknown symbol: %c (%x)\n", str_view_col_front(*file_text), str_view_col_front(*file_text));
    }
}

Tokens tokenize(const String file_text, const Parameters params) {
    Tokens tokens = {0};

    Str_view_col curr_file_text = {.base = {.str = file_text.buf, .count = file_text.info.count}};

    Pos pos = {.line = 1, .column = 0};
    Token curr_token;
    while (get_next_token(&pos, &curr_token, &curr_file_text, params)) {
        //log(LOG_TRACE, "token received: "TOKEN_FMT"\n", token_print(curr_token));
        if (curr_token.type != TOKEN_COMMENT) {
            vec_append(&a_main, &tokens, curr_token);
        }
    }

    return tokens;
}

