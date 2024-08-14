#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"

typedef enum {
    PARSE_NORMAL,
    PARSE_FUN_PARAMS,
    PARSE_FUN_BODY,
    PARSE_FUN_RETURN_TYPES,
} PARSE_STATE;

static Node* parse_rec(PARSE_STATE state, const Token* tokens, size_t count) {
    if (state == PARSE_FUN_PARAMS) {
        if (tokens[0].type != TOKEN_OPEN_PAR) {
            log(LOG_ERROR, "error: '(' expected\n");
            exit(1);
        }
        if (tokens[1].type != TOKEN_CLOSE_PAR) {
            // function parameters not implemented
            todo();
            exit(1);
        }
        Node* parameters = Node_new();
        parameters->type = NODE_FUNCTION_PARAMETERS;
        return parameters;
    }

    if (state == PARSE_FUN_RETURN_TYPES) {
        if (count < 1) {
            unreachable();
        }
        if (count > 1) {
            todo();
        }
        Node* return_types = Node_new();
        return_types->type = NODE_FUNCTION_RETURN_TYPES;
        return_types->lang_type = tokens[0].text;
        return return_types;
    }

    if (state == PARSE_FUN_BODY) {
        if (count < 2) {
            unreachable();
        }
        if (count > 2) {
            todo();
        }
        Node* body = Node_new();
        body->type = NODE_FUNCTION_BODY;
        return body;
    }

    if (tokens[0].type == TOKEN_SYMBOL && 0 == Strv_cmp_cstr(tokens[0].text, "fn")) {
        Node* function = Node_new();
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
        if (tokens[return_types_start].type == TOKEN_SYMBOL && 0 == Strv_cmp_cstr(tokens[return_types_start].text, "void")) {
            return_types_len = 1;
        } else {
            todo();
        }
        function->return_types = parse_rec(PARSE_FUN_RETURN_TYPES, &tokens[return_types_start], return_types_len);

        size_t body_len;
        size_t body_start = return_types_start + return_types_len;
        assert(count > body_start);
        if (tokens[body_start].type == TOKEN_OPEN_CURLY_BRACE && (tokens[body_start + 1].type == TOKEN_CLOSE_CURLY_BRACE)) {
            body_len = 2;
        } else {
            todo();
        }
        function->body = parse_rec(PARSE_FUN_BODY, &tokens[body_start], body_len);

        return function;
    } 

    log(LOG_TRACE, "parse_rec other: "TOKEN_FMT"\n", Token_print(tokens[0]));
    log(LOG_TRACE, "cmp: %d\n", Strv_cmp_cstr(tokens[0].text, "fn"));
    unreachable();
}

void parse(const Tokens tokens) {
    Node* root = parse_rec(PARSE_NORMAL, &tokens.buf[0], tokens.count);
    todo();
}
