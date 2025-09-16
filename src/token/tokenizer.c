#include <stdbool.h>
#include <util.h>
#include <token.h>
#include <token_view.h>
#include <strv_col.h>
#include <parameters.h>
#include <env.h>
#include <do_passes.h>
#include <ctype.h>
#include <parser_utils.h>
#include <pos_vec.h>

static Arena a_token = {0};

#define msg_tokenizer_invalid_token(token_text, pos) msg_tokenizer_invalid_token_internal(__FILE__, __LINE__, token_text, pos)

static void msg_tokenizer_invalid_token_internal(const char* file, int line, Strv_col token_text, Pos pos) {
    msg_internal(file, line, DIAG_INVALID_TOKEN, pos, "invalid token `"FMT"`\n", strv_col_print(token_text));
}

static bool local_isalnum_or_underscore(char prev, char curr) {
    (void) prev;
    return isalnum(curr) || curr == '_';
}

static Strv consume_int(Pos* pos, Strv_col* file_text) {
    Strv_col num = {0};
    unwrap(strv_col_try_consume_while(&num, pos, file_text, local_isalnum_or_underscore));
    return num.base;
}

static bool is_not_quote(char prev, char curr) {
    return curr != '"' || prev == '\\';
}

static bool is_equal(char prev, char curr) {
    (void) prev;
    return curr == '=';
}

static bool is_and(char prev, char curr) {
    (void) prev;
    return curr == '&';
}

static bool is_or(char prev, char curr) {
    (void) prev;
    return curr == '|';
}

static bool is_tick(char prev, char curr) {
    (void) prev;
    return curr == '\'';
}

static bool is_xor(char prev, char curr) {
    (void) prev;
    return curr == '^';
}

static bool is_dot(char prev, char curr) {
    (void) prev;
    return curr == '.';
}

static bool not_single_quote(char prev, char curr) {
    (void) prev;
    return curr != '\'';
}

// returns count of characters trimmed
static size_t trim_non_newline_whitespace(Strv_col* file_text, Pos* pos) {
    size_t count = 0;
    if (file_text->base.count < 1) {
        return count;
    }
    char curr = strv_col_front(*file_text);
    while (file_text->base.count > 0 && (isspace(curr) || iscntrl(curr)) && (curr != '\n')) {
        count++;
        strv_col_consume(pos, file_text);
        curr = strv_col_front(*file_text);
    }
    return count;
}

static bool get_next_token(
    Pos* pos,
    bool* is_preced_space,
    Token* token,
    Strv_col* file_text_rem,
    Strv file_path
) {
    memset(token, 0, sizeof(*token));
    arena_reset(&a_token);

    *is_preced_space = trim_non_newline_whitespace(file_text_rem, pos) > 0;

    if (file_text_rem->base.count < 1) {
        return false;
    }

    pos->file_path = file_path;
    token->pos.column = pos->column + 1;
    token->pos.line = pos->line;
    token->pos.file_path = pos->file_path;

    static_assert(TOKEN_COUNT == 73, "exhausive handling of token types (only keywords are explicitly handled)");
    if (isalpha(strv_col_front(*file_text_rem))) {
        Strv text = strv_col_consume_while(pos, file_text_rem, local_isalnum_or_underscore).base;
        if (strv_is_equal(text, sv("unsafe_cast"))) {
            token->type = TOKEN_UNSAFE_CAST;
        } else if (strv_is_equal(text, sv("defer"))) {
            token->type = TOKEN_DEFER;
        } else if (strv_is_equal(text, sv("fn"))) {
            token->type = TOKEN_FN;
        } else if (strv_is_equal(text, sv("for"))) {
            token->type = TOKEN_FOR;
        } else if (strv_is_equal(text, sv("if"))) {
            token->type = TOKEN_IF;
        } else if (strv_is_equal(text, sv("switch"))) {
            token->type = TOKEN_SWITCH;
        } else if (strv_is_equal(text, sv("case"))) {
            token->type = TOKEN_CASE;
        } else if (strv_is_equal(text, sv("default"))) {
            token->type = TOKEN_DEFAULT;
        } else if (strv_is_equal(text, sv("else"))) {
            token->type = TOKEN_ELSE;
        } else if (strv_is_equal(text, sv("return"))) {
            token->type = TOKEN_RETURN;
        } else if (strv_is_equal(text, sv("extern"))) {
            token->type = TOKEN_EXTERN;
        } else if (strv_is_equal(text, sv("struct"))) {
            token->type = TOKEN_STRUCT;
        } else if (strv_is_equal(text, sv("let"))) {
            token->type = TOKEN_LET;
        } else if (strv_is_equal(text, sv("in"))) {
            token->type = TOKEN_IN;
        } else if (strv_is_equal(text, sv("break"))) {
            token->type = TOKEN_BREAK;
        } else if (strv_is_equal(text, sv("raw_union"))) {
            token->type = TOKEN_RAW_UNION;
        } else if (strv_is_equal(text, sv("enum"))) {
            token->type = TOKEN_ENUM;
        } else if (strv_is_equal(text, sv("continue"))) {
            token->type = TOKEN_CONTINUE;
        } else if (strv_is_equal(text, sv("type"))) {
            token->type = TOKEN_TYPE_DEF;
        } else if (strv_is_equal(text, sv("import"))) {
            token->type = TOKEN_IMPORT;
        } else if (strv_is_equal(text, sv("def"))) {
            token->type = TOKEN_DEF;
        } else if (strv_is_equal(text, sv("sizeof"))) {
            token->type = TOKEN_SIZEOF;
        } else if (strv_is_equal(text, sv("yield"))) {
            token->type = TOKEN_YIELD;
        } else if (strv_is_equal(text, sv("countof"))) {
            token->type = TOKEN_COUNTOF;
        } else if (strv_is_equal(text, sv("Type"))) {
            token->type = TOKEN_GENERIC_TYPE;
        } else {
            token->text = text;
            token->type = TOKEN_SYMBOL;
        }
        return true;
    } else if (isdigit(strv_col_front(*file_text_rem))) {
        Pos temp_pos = {0};
        Strv_col temp_text = *file_text_rem;
        Strv before_dec = consume_int(&temp_pos, &temp_text);
        // right hand side of || is to account for .. in for loop, variadic args, etc.
        if (!strv_col_try_consume(&temp_pos, &temp_text, '.') || strv_col_try_consume(&temp_pos, &temp_text, '.')) {
            token->text = strv_col_consume_count(pos, file_text_rem, before_dec.count).base;
            token->type = TOKEN_INT_LITERAL;
            return true;
        }
        Strv_col after_dec = {0};
        unwrap(strv_col_try_consume_while(&after_dec, &temp_pos, &temp_text, local_isalnum_or_underscore));
        token->text = strv_col_consume_count(pos, file_text_rem, before_dec.count + 1 + after_dec.base.count).base;
        token->type = TOKEN_FLOAT_LITERAL;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '(')) {
        if (strv_col_try_consume(pos, file_text_rem, '<')) {
            token->type = TOKEN_OPEN_GENERIC;
        } else {
            token->type = TOKEN_OPEN_PAR;
        }
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, ')')) {
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '{')) {
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '}')) {
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '#')) {
        token->type = TOKEN_MACRO;
        token->text = strv_col_consume_while(pos, file_text_rem, local_isalnum_or_underscore).base;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '"')) {
        token->type = TOKEN_STRING_LITERAL;
        Strv_col quote_str = {0};
        if (!strv_col_try_consume_while(&quote_str, pos, file_text_rem, is_not_quote)) {
            msg(DIAG_MISSING_CLOSE_DOUBLE_QUOTE, token->pos, "unmatched `\"`\n");
            token->type = TOKEN_NONTYPE;
            return false;
        }
        unwrap(strv_col_try_consume(pos, file_text_rem, '"'));
        token->text = quote_str.base;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, ';')) {
        token->type = TOKEN_SEMICOLON;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, ',')) {
        token->type = TOKEN_COMMA;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '+')) {
        unwrap((file_text_rem->base.count < 1 || strv_col_front(*file_text_rem) != '+') && "double + not implemented");
        token->type = TOKEN_SINGLE_PLUS;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '-')) {
        token->type = TOKEN_SINGLE_MINUS;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '*')) {
        if (strv_col_try_consume(pos, file_text_rem, '/')) {
            msg(
                DIAG_MISSING_CLOSE_MULTILINE, 
                *pos, "unmatched closing `/*`\n"
            );
            return false;
        }
        token->type = TOKEN_ASTERISK;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '%')) {
        token->type = TOKEN_MODULO;
        return true;
    } else if (file_text_rem->base.count > 1 && strv_is_equal(strv_slice(file_text_rem->base, 0, 2), sv("//"))) {
        strv_col_consume_until(pos, file_text_rem, '\n');
        trim_non_newline_whitespace(file_text_rem, pos);
        token->type = TOKEN_COMMENT;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '/')) {
        if (strv_col_try_consume(pos, file_text_rem, '*')) {
            Pos_vec pos_stack = {0};
            vec_append(&a_token, &pos_stack, *pos);
            while (pos_stack.info.count > 0) {
                Strv temp_text = file_text_rem->base;
                if (file_text_rem->base.count < 2) {
                    msg(
                        DIAG_MISSING_CLOSE_MULTILINE, 
                        vec_top(&pos_stack), "unmatched opening `/*`\n"
                    );
                    return false;
                }

                if (strv_try_consume(&temp_text, '/') && strv_try_consume(&temp_text, '*')) {
                    vec_append(&a_token, &pos_stack, *pos);
                    strv_col_consume_count(pos, file_text_rem, 2);
                } else if (strv_try_consume(&temp_text, '*') && strv_try_consume(&temp_text, '/')) {
                    vec_rem_last(&pos_stack);
                    strv_col_consume_count(pos, file_text_rem, 2);
                } else {
                    strv_col_consume(pos, file_text_rem);
                }
            }

            token->type = TOKEN_COMMENT;
        } else {
            token->type = TOKEN_SLASH;
        }
        return true;
    } else if (strv_col_front(*file_text_rem) == '&') {
        Strv_col equals = strv_col_consume_while(pos, file_text_rem, is_and);
        if (equals.base.count == 1) {
            token->type = TOKEN_BITWISE_AND;
            return true;
        } else if (equals.base.count == 2) {
            token->type = TOKEN_LOGICAL_AND;
            return true;
        } else {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (strv_col_front(*file_text_rem) == '^') {
        Strv_col equals = strv_col_consume_while(pos, file_text_rem, is_xor);
        if (equals.base.count == 1) {
            token->type = TOKEN_BITWISE_XOR;
            return true;
        } else if (equals.base.count == 2) {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        } else {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (strv_col_front(*file_text_rem) == '|') {
        // TODO: rename equals
        Strv_col equals = strv_col_consume_while(pos, file_text_rem, is_or);
        if (equals.base.count == 1) {
            token->type = TOKEN_BITWISE_OR;
            return true;
        } else if (equals.base.count == 2) {
            token->type = TOKEN_LOGICAL_OR;
            return true;
        } else {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (strv_col_front(*file_text_rem) == '\'') {
        Strv_col equals = strv_col_consume_while(pos, file_text_rem, is_tick);
        if (equals.base.count == 1) {
            token->type = TOKEN_CHAR_LITERAL;

            Strv_col result = {0};
            if (!strv_col_try_consume_while(&result, pos, file_text_rem, not_single_quote)) {
                msg(DIAG_MISSING_CLOSE_SINGLE_QUOTE, token->pos, "unmatched opening `'`\n");
                return false;
            }

            unwrap(strv_col_consume(pos, file_text_rem));
            token->text = result.base;
            return true;
        } else if (equals.base.count == 2) {
            token->type = TOKEN_DOUBLE_TICK;
            return true;
        } else {
            msg_tokenizer_invalid_token(equals, *pos);
            token->type = TOKEN_NONTYPE;
            return true;
        }
    } else if (strv_col_try_consume(pos, file_text_rem, ':')) {
        token->type = TOKEN_COLON;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, '!')) {
        if (strv_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_NOT_EQUAL;
            return true;
        }
        token->type = TOKEN_NOT;
        return true;
    } else if (strv_col_front(*file_text_rem) == '=') {
        Strv_col equals = strv_col_consume_while(pos, file_text_rem, is_equal);
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
    } else if (strv_col_try_consume(pos, file_text_rem, '>')) {
        if (strv_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_GREATER_OR_EQUAL;
            return true;
        } else if (strv_col_try_consume(pos, file_text_rem, '>')) {
            token->type = TOKEN_SHIFT_RIGHT;
            return true;
        } else if (strv_col_try_consume(pos, file_text_rem, ')')) {
            token->type = TOKEN_CLOSE_GENERIC;
            return true;
        } else {
            token->type = TOKEN_GREATER_THAN;
            return true;
        }
    } else if (strv_col_try_consume(pos, file_text_rem, '<')) {
        if (strv_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_LESS_OR_EQUAL;
            return true;
        } else if (strv_col_try_consume(pos, file_text_rem, '<')) {
            token->type = TOKEN_SHIFT_LEFT;
            return true;
        } else {
            token->type = TOKEN_LESS_THAN;
            return true;
        }
    } else if (strv_col_try_consume(pos, file_text_rem, '[')) {
        token->type = TOKEN_OPEN_SQ_BRACKET;
        return true;
    } else if (strv_col_try_consume(pos, file_text_rem, ']')) {
        token->type = TOKEN_CLOSE_SQ_BRACKET;
        return true;
    } else if (strv_col_front(*file_text_rem) == '.') {
        Strv_col dots = strv_col_consume_while(pos, file_text_rem, is_dot);
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
    } else if (strv_col_try_consume(pos, file_text_rem, '\n')) {
        token->type = TOKEN_NEW_LINE;
        return true;
    } else {
        String buf = {0};
        string_extend_strv(&a_token, &buf, sv("unknown symbol: "));
        vec_append(&a_token, &buf, strv_col_front(*file_text_rem));
        msg_todo_strv(string_to_strv(buf), *pos);
        return false;
    }
}

static Token token_new(const char* text, TOKEN_TYPE token_type) {
    Token token = {.text = sv(text), .type = token_type};
    return token;
}

static void test(const char* file_text, Tk_view expected) {
    (void) file_text;
    (void) expected;
    todo();
    //Env env = {.file_text = sv(file_text)};

    //Token_vec tokens = {0};
    //if (!tokenize(&tokens, & (Strv){0})) {
    //    unreachable("");
    //}
    //Tk_view tk_view = {.tokens = tokens.buf, .count = tokens.info.count};

    //unwrap(tk_view_is_equal_log(LOG_TRACE, tk_view, expected));
}

static inline Tk_view tokens_to_tk_view(Token_vec tokens) {
    Tk_view tk_view = {.tokens = tokens.buf, .count = tokens.info.count};
    return tk_view;
}

static void test1(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("hello", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    test("hello\n", tokens_to_tk_view(expected));
}

static void test2(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    test("// hello\n", tokens_to_tk_view(expected));
}

static void test3(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("", TOKEN_IF));
    vec_append(&a_main, &expected, token_new("1", TOKEN_INT_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("hello", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

static Token_vec get_expected(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("", TOKEN_IF));
    vec_append(&a_main, &expected, token_new("1", TOKEN_INT_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("hello", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    vec_append(&a_main, &expected, token_new("", TOKEN_ELSE));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("unreachable", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    return expected;
}

static void test4(void) {
    Token_vec expected = get_expected();

    test(
        "if 1 {\n    puts(\"hello\")\n}\n else {\n    puts(\"unreachable\")\n}\n",
        tokens_to_tk_view(expected)
    );

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "}\n"
        "else {\n"
        "    puts(\"unreachable\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

static void test5(void) {
    Token_vec expected = get_expected();

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "}\n"
        "// thing thing \n"
        "else {\n"
        "    puts(\"unreachable\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

static void test6(void) {
    Token_vec expected = get_expected();

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "// thing thing \n"
        "}\n"
        "else {\n"
        "    puts(\"unreachable\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

static void test7(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("", TOKEN_IF));
    vec_append(&a_main, &expected, token_new("1", TOKEN_INT_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("hello", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    vec_append(&a_main, &expected, token_new("", TOKEN_ELSE));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("unreachable", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "// thing thing \n"
        "\n"
        "}\n"
        "else {\n"
        "    puts(\"unreachable\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

static void test8(void) {
    Token_vec expected = {0};
    vec_append(&a_main, &expected, token_new("", TOKEN_IF));
    vec_append(&a_main, &expected, token_new("1", TOKEN_INT_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("hello", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    vec_append(&a_main, &expected, token_new("", TOKEN_ELSE));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("puts", TOKEN_SYMBOL));
    vec_append(&a_main, &expected, token_new("", TOKEN_OPEN_PAR));
    vec_append(&a_main, &expected, token_new("unreachable", TOKEN_STRING_LITERAL));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_PAR));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));
    vec_append(&a_main, &expected, token_new("", TOKEN_CLOSE_CURLY_BRACE));
    vec_append(&a_main, &expected, token_new("", TOKEN_NEW_LINE));

    const char* text = 
        "if 1 {\n"
        "    puts(\"hello\")\n"
        "\n"
        "\n"
        "}\n"
        "else {\n"
        "    puts(\"unreachable\")\n"
        "}\n";

    test(text, tokens_to_tk_view(expected));
}

void tokenize_do_test(void) {
    // note: this function is not currently being called
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
}

bool tokenize(Token_vec* result, Strv file_path) {
    size_t prev_err_count = error_count;
    Token_vec tokens = {0};

    Strv* file_con = NULL;
    unwrap(file_path_to_text_tbl_lookup(&file_con, file_path));
    Strv_col curr_file_text = {.base = *file_con};

    Pos pos = {.line = 1, .column = 0};
    Token curr_token = {0};
    bool is_preced_space = false;
    while (get_next_token(&pos, &is_preced_space, &curr_token, &curr_file_text, file_path)) {
        if (curr_token.type == TOKEN_COMMENT) {
            continue;
        }

        // avoid consecutive newline tokens
        if (
            tokens.info.count > 1 && 
            vec_top(&tokens).type == TOKEN_NEW_LINE &&
            curr_token.type == TOKEN_NEW_LINE
        ) {
            continue;
        }

        if (token_is_binary(curr_token.type)) {
            Token next_token = {0};
            Pos temp_pos = pos;
            Strv_col temp_file_text = curr_file_text;
            if (get_next_token(&pos, &is_preced_space, &next_token, &curr_file_text, file_path)) {
                if (!is_preced_space && next_token.type == TOKEN_SINGLE_EQUAL) {
                    // +=, *=, etc.
                    // append TOKEN_ASSIGN_BY_BIN, which will be followed by the binary operator
                    vec_append(&a_main, &tokens, ((Token) {.text = sv(""), .type = TOKEN_ASSIGN_BY_BIN, .pos = pos}));
                } else {
                    // put back the next token
                    pos = temp_pos;
                    curr_file_text = temp_file_text;
                }
            }
        }

        vec_append(&a_main, &tokens, curr_token);
    }

    vec_append(&a_main, &tokens, ((Token) {.text = sv(""), TOKEN_EOF, pos}));

    *result = tokens;
    return error_count == prev_err_count;
}

