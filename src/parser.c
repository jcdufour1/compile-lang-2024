#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"

static inline void log_tokens(LOG_LEVEL log_level, const Token* tokens, size_t count) {
    for (size_t idx = 0; idx < count; idx++) {
        log(log_level, "    "TOKEN_FMT"\n", token_print(tokens[idx]));
    }
}

typedef enum {
    PARSE_NORMAL,
    PARSE_FUN_PARAMS,
    PARSE_FUN_BODY,
    PARSE_FUN_RETURN_TYPES,
    PARSE_FUN_ARGUMENTS,
} PARSE_STATE;

// TODO: make this work when there are eg. nested (())
static bool get_idx_matching_token(size_t* idx_matching, const Token* tokens, size_t count, TOKEN_TYPE type_to_match) {
    for (size_t idx = 0; idx < count; idx++) {
        if (tokens[idx].type == type_to_match) {
            *idx_matching = idx;;
            return true;
        }
    }
    return false;
}

// this function will not consider nested ()
static bool get_idx_token(size_t* idx_matching, const Token* tokens, size_t count, TOKEN_TYPE type_to_match) {
    for (size_t idx = 0; idx < count; idx++) {
        if (tokens[idx].type == type_to_match) {
            *idx_matching = idx;;
            return true;
        }
    }
    return false;
}

static bool tokens_start_with_function_call(size_t* idx_semicolon, const Token* tokens, size_t count) {
    if (count < 2) {
        return false;
    }

    if (tokens[0].type != TOKEN_SYMBOL) {
        return false;
    }

    if (tokens[1].type != TOKEN_OPEN_PAR) {
        return false;
    }

    size_t semicolon_pos;
    get_idx_matching_token(&semicolon_pos, &tokens[0], count, TOKEN_SEMICOLON);
    if (tokens[semicolon_pos - 1].type != TOKEN_CLOSE_PAR) {
        return false;
    }

    *idx_semicolon = semicolon_pos;
    return true;
}

static Node* parse_rec(PARSE_STATE state, const Token* tokens, size_t count) {
    if (state == PARSE_FUN_PARAMS) {
        if (tokens[0].type != TOKEN_OPEN_PAR) {
            log(LOG_ERROR, "error: '(' expected\n");
            exit(1);
        }
        if (tokens[1].type != TOKEN_CLOSE_PAR) {
            todo();
        }
        Node* parameters = node_new();
        parameters->type = NODE_FUNCTION_PARAMETERS;
        return parameters;
    }

    if (state == PARSE_FUN_ARGUMENTS) {
        Node* argument = node_new();
        size_t idx_semicolon;
        if (get_idx_token(&idx_semicolon, tokens, count, TOKEN_SEMICOLON)) {
            todo();
        }
        if (get_idx_token(&idx_semicolon, tokens, count, TOKEN_COMMA)) {
            todo();
        }
        switch (tokens[0].type) {
            case TOKEN_STRING_LITERAL:
                argument->type = NODE_LITERAL;
                argument->name = tokens[0].text;
                argument->literal_type = TOKEN_STRING_LITERAL;
                assert(count == 1);
                return argument;
            default:
                todo();
        }
    }

    if (state == PARSE_FUN_RETURN_TYPES) {
        if (count < 1) {
            unreachable();
        }
        if (count > 1) {
            todo();
        }
        Node* return_types = node_new();
        return_types->type = NODE_FUNCTION_RETURN_TYPES;
        return_types->lang_type = tokens[0].text;
        return return_types;
    }

    if (state == PARSE_FUN_BODY) {
        if (count < 2) {
            unreachable();
        }
        Node* body = node_new();
        body->type = NODE_FUNCTION_BODY;
        body->right = parse_rec(PARSE_NORMAL, &tokens[1], count - 1);
        return body;
    }

    if (tokens[0].type == TOKEN_SYMBOL && 0 == str_view_cmp_cstr(tokens[0].text, "fn")) {
        Node* function = node_new();
        function->type = NODE_FUNCTION_DEFINITION;
        if (tokens[1].type == TOKEN_SYMBOL) {
            function->name = tokens[1].text;
        } else {
            todo();
        }

        size_t parameters_len;
        size_t parameters_start = 2;
        if (tokens[3].type == TOKEN_CLOSE_PAR) {
            parameters_len = 2;
        } else {
            todo();
        }
        function->parameters = parse_rec(PARSE_FUN_PARAMS, &tokens[parameters_start], parameters_len);

        size_t return_types_len;
        size_t return_types_start = parameters_start + parameters_len;
        if (tokens[return_types_start].type == TOKEN_SYMBOL && 0 == str_view_cmp_cstr(tokens[return_types_start].text, "void")) {
            return_types_len = 1;
        } else {
            todo();
        }
        function->return_types = parse_rec(PARSE_FUN_RETURN_TYPES, &tokens[return_types_start], return_types_len);

        size_t body_len;
        size_t body_start = return_types_start + return_types_len;
        assert(count > body_start);
        if (tokens[body_start].type != TOKEN_OPEN_CURLY_BRACE) {
            unreachable();
        }
        if (!get_idx_matching_token(&body_len, &tokens[body_start], count - body_start, TOKEN_CLOSE_CURLY_BRACE)) {
            todo();
        }
        function->body = parse_rec(PARSE_FUN_BODY, &tokens[body_start], body_len);

        return function;
    } 

    size_t semicolon_pos;
    if (tokens_start_with_function_call(&semicolon_pos, tokens, count)) {
        Node* function_call = node_new();
        function_call->type = NODE_FUNCTION_CALL;
        function_call->name = tokens[0].text;
        log_tokens(LOG_TRACE, tokens, count);
        size_t parameters_start = 2; // exclude outer ()
        size_t parameters_end;
        if (!get_idx_matching_token(&parameters_end, tokens, count, TOKEN_CLOSE_PAR)) {
            todo();
        }
        parameters_end--; // exclude outer ()
        function_call->parameters = parse_rec(PARSE_FUN_ARGUMENTS, &tokens[parameters_start], parameters_start - parameters_end + 1);
        return function_call;
    }

    if (count < 1) {
        return NULL;
    }

    log(LOG_TRACE, "parse_rec other: "TOKEN_FMT"\n", token_print(tokens[0]));
    log(LOG_TRACE, "cmp: %d\n", str_view_cmp_cstr(tokens[0].text, "fn"));
    unreachable();
}

void parse(const Tokens tokens) {
    Node* root = parse_rec(PARSE_NORMAL, &tokens.buf[0], tokens.count);
    log_tree(LOG_VERBOSE, root);
    todo();
}
