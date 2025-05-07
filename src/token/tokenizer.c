#include <stdbool.h>
#include <util.h>
#include <token.h>
#include <token_view.h>
#include <str_view_col.h>
#include <parameters.h>
#include <env.h>
#include <do_passes.h>
#include <ctype.h>
#include <parser_utils.h>
#include <pos_vec.h>

static Arena tk_arena = {0};

static void msg_tokenizer_invalid_token(Str_view_col token_text, Pos pos) {
    msg(LOG_ERROR, EXPECT_FAIL_INVALID_TOKEN, pos, "invalid token `"STR_VIEW_COL_FMT"`\n", str_view_col_print(token_text));
}

static bool local_isalnum_or_underscore(char prev, char curr) {
    (void) prev;
    return isalnum(curr) || curr == '_';
}

static bool local_ishex(char prev, char curr) {
    (void) prev;
    return ishex(curr) || curr == '_' || curr == 'x';
}

static Str_view consume_num(Pos* pos, Str_view_col* file_text) {
    Str_view_col num = {0};
    unwrap(str_view_col_try_consume_while(&num, pos, file_text, local_isalnum_or_underscore));
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

static bool is_xor(char prev, char curr) {
    (void) prev;
    return curr == '^';
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

static bool get_next_token(
    Pos* pos,
    Token* token,
    Str_view_col* file_text_rem,
    Str_view file_path
) {
    memset(token, 0, sizeof(*token));
    arena_reset(&tk_arena);

    trim_non_newline_whitespace(file_text_rem, pos);

    if (file_text_rem->base.count < 1) {
        return false;
    }

    pos->file_path = file_path;
    token->pos.column = pos->column + 1;
    token->pos.line = pos->line;
    token->pos.file_path = pos->file_path;

    if (isalpha(str_view_col_front(*file_text_rem))) {
        Str_view text = str_view_col_consume_while(pos, file_text_rem, local_isalnum_or_underscore).base;
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
        } else if (str_view_cstr_is_equal(text, "switch")) {
            token->type = TOKEN_SWITCH;
        } else if (str_view_cstr_is_equal(text, "case")) {
            token->type = TOKEN_CASE;
        } else if (str_view_cstr_is_equal(text, "default")) {
            token->type = TOKEN_DEFAULT;
        } else if (str_view_cstr_is_equal(text, "else")) {
            token->type = TOKEN_ELSE;
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
        } else if (str_view_cstr_is_equal(text, "raw_union")) {
            token->type = TOKEN_RAW_UNION;
        } else if (str_view_cstr_is_equal(text, "enum")) {
            token->type = TOKEN_ENUM;
        } else if (str_view_cstr_is_equal(text, "sum")) {
            token->type = TOKEN_SUM;
        } else if (str_view_cstr_is_equal(text, "continue")) {
            token->type = TOKEN_CONTINUE;
        } else if (str_view_cstr_is_equal(text, "type")) {
            token->type = TOKEN_TYPE_DEF;
        } else if (str_view_cstr_is_equal(text, "import")) {
            token->type = TOKEN_IMPORT;
        } else if (str_view_cstr_is_equal(text, "def")) {
            token->type = TOKEN_DEF;
        } else {
            token->text = text;
            token->type = TOKEN_SYMBOL;
        }
        return true;
    } else if (isdigit(str_view_col_front(*file_text_rem))) {
        token->text = consume_num(pos, file_text_rem);
        token->type = TOKEN_INT_LITERAL;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '(')) {
        if (str_view_col_try_consume(pos, file_text_rem, '<')) {
            token->type = TOKEN_OPEN_GENERIC;
        } else {
            token->type = TOKEN_OPEN_PAR;
        }
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, ')')) {
        token->type = TOKEN_CLOSE_PAR;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '{')) {
        token->type = TOKEN_OPEN_CURLY_BRACE;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '}')) {
        token->type = TOKEN_CLOSE_CURLY_BRACE;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '"')) {
        token->type = TOKEN_STRING_LITERAL;
        Str_view_col quote_str = {0};
        if (!str_view_col_try_consume_while(&quote_str, pos, file_text_rem, is_not_quote)) {
            msg(LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE, token->pos, "unmatched `\"`\n");
            token->type = TOKEN_NONTYPE;
            return false;
        }
        unwrap(str_view_col_try_consume(pos, file_text_rem, '"'));
        token->text = quote_str.base;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, ';')) {
        token->type = TOKEN_SEMICOLON;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, ',')) {
        token->type = TOKEN_COMMA;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '+')) {
        unwrap((file_text_rem->base.count < 1 || str_view_col_front(*file_text_rem) != '+') && "double + not implemented");
        token->type = TOKEN_SINGLE_PLUS;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '-')) {
        token->type = TOKEN_SINGLE_MINUS;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '*')) {
        if (str_view_col_try_consume(pos, file_text_rem, '/')) {
            msg(
                LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_MULTILINE, 
                *pos, "unmatched closing `/*`\n"
            );
            return false;
        }
        token->type = TOKEN_ASTERISK;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '%')) {
        token->type = TOKEN_MODULO;
        return true;
    } else if (file_text_rem->base.count > 1 && str_view_cstr_is_equal(str_view_slice(file_text_rem->base, 0, 2), "//")) {
        str_view_col_consume_until(pos, file_text_rem, '\n');
        trim_non_newline_whitespace(file_text_rem, pos);
        token->type = TOKEN_COMMENT;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '/')) {
        if (str_view_col_try_consume(pos, file_text_rem, '*')) {
            Pos_vec pos_stack = {0};
            vec_append(&tk_arena, &pos_stack, *pos);
            while (pos_stack.info.count > 0) {
                Str_view temp_text = file_text_rem->base;
                if (file_text_rem->base.count < 2) {
                    msg(
                        LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_MULTILINE, 
                        vec_top(&pos_stack), "unmatched opening `/*`\n"
                    );
                    return false;
                }

                if (str_view_try_consume(&temp_text, '/') && str_view_try_consume(&temp_text, '*')) {
                    vec_append(&tk_arena, &pos_stack, *pos);
                    str_view_col_consume_count(pos, file_text_rem, 2);
                } else if (str_view_try_consume(&temp_text, '*') && str_view_try_consume(&temp_text, '/')) {
                    vec_rem_last(&pos_stack);
                    str_view_col_consume_count(pos, file_text_rem, 2);
                } else {
                    str_view_col_consume(pos, file_text_rem);
                }
            }

            token->type = TOKEN_COMMENT;
        } else {
            token->type = TOKEN_SLASH;
        }
        return true;
    } else if (str_view_col_front(*file_text_rem) == '&') {
        Str_view_col equals = str_view_col_consume_while(pos, file_text_rem, is_and);
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
    } else if (str_view_col_front(*file_text_rem) == '^') {
        Str_view_col equals = str_view_col_consume_while(pos, file_text_rem, is_xor);
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
    } else if (str_view_col_front(*file_text_rem) == '|') {
        Str_view_col equals = str_view_col_consume_while(pos, file_text_rem, is_or);
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
    } else if (str_view_col_try_consume(pos, file_text_rem, ':')) {
        token->type = TOKEN_COLON;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '!')) {
        if (str_view_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_NOT_EQUAL;
            return true;
        }
        token->type = TOKEN_NOT;
        return true;
    } else if (str_view_col_front(*file_text_rem) == '=') {
        Str_view_col equals = str_view_col_consume_while(pos, file_text_rem, is_equal);
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
    } else if (str_view_col_try_consume(pos, file_text_rem, '>')) {
        if (str_view_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_GREATER_OR_EQUAL;
            return true;
        } else if (str_view_col_try_consume(pos, file_text_rem, '>')) {
            token->type = TOKEN_SHIFT_RIGHT;
            return true;
        } else if (str_view_col_try_consume(pos, file_text_rem, ')')) {
            token->type = TOKEN_CLOSE_GENERIC;
            return true;
        } else {
            token->type = TOKEN_GREATER_THAN;
            return true;
        }
    } else if (str_view_col_try_consume(pos, file_text_rem, '<')) {
        if (str_view_col_try_consume(pos, file_text_rem, '=')) {
            token->type = TOKEN_LESS_OR_EQUAL;
            return true;
        } else if (str_view_col_try_consume(pos, file_text_rem, '<')) {
            token->type = TOKEN_SHIFT_LEFT;
            return true;
        } else {
            token->type = TOKEN_LESS_THAN;
            return true;
        }
    } else if (str_view_col_try_consume(pos, file_text_rem, '[')) {
        token->type = TOKEN_OPEN_SQ_BRACKET;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, ']')) {
        token->type = TOKEN_CLOSE_SQ_BRACKET;
        return true;
    } else if (str_view_col_try_consume(pos, file_text_rem, '\'')) {
        token->type = TOKEN_CHAR_LITERAL;

        token->text = (Str_view){.str = file_text_rem->base.str, .count = 1};
        str_view_col_advance_pos(pos, str_view_col_front(*file_text_rem));
        str_view_consume(&file_text_rem->base);

        // TODO: expected failure case for char literal being too large
        // TODO: expected success case for char literal having '\n'
        unwrap(str_view_col_try_consume(pos, file_text_rem, '\''));
        return true;
    } else if (str_view_col_front(*file_text_rem) == '.') {
        Str_view_col dots = str_view_col_consume_while(pos, file_text_rem, is_dot);
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
    } else if (str_view_col_try_consume(pos, file_text_rem, '\n')) {
        token->type = TOKEN_NEW_LINE;
        return true;
    } else {
        unreachable("unknown symbol: %c (%x)\n", str_view_col_front(*file_text_rem), str_view_col_front(*file_text_rem));
    }
}

static Token token_new(const char* text, TOKEN_TYPE token_type) {
    Token token = {.text = str_view_from_cstr(text), .type = token_type};
    return token;
}

static void test(const char* file_text, Tk_view expected) {
    (void) file_text;
    (void) expected;
    todo();
    //Env env = {.file_text = str_view_from_cstr(file_text)};

    //Token_vec tokens = {0};
    //if (!tokenize(&tokens, & (Str_view){0})) {
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
    // TODO: consider reenabling these?
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    //test6();
    //test7();
    //test8();
}

bool tokenize(Token_vec* result, Str_view file_path) {
    size_t prev_err_count = error_count;
    Token_vec tokens = {0};

    Str_view* file_con = NULL;
    unwrap(file_path_to_text_tbl_lookup(&file_con, file_path));
    Str_view_col curr_file_text = {.base = *file_con};

    Pos pos = {.line = 1, .column = 0};
    Token curr_token = {0};
    while (get_next_token( &pos, &curr_token, &curr_file_text, file_path)) {
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

        vec_append(&a_main, &tokens, curr_token);
    }

    *result = tokens;
    return error_count == prev_err_count;
}

