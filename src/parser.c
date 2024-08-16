#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"

static inline void log_tokens(LOG_LEVEL log_level, Tk_view tokens) {
    for (size_t idx = 0; idx < tokens.count; idx++) {
        log(log_level, "    "TOKEN_FMT"\n", token_print(tokens.tokens[idx]));
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
static bool get_idx_matching_token(size_t* idx_matching, Tk_view tokens, TOKEN_TYPE type_to_match) {
    for (size_t idx = 0; idx < tokens.count; idx++) {
        if (tokens.tokens[idx].type == type_to_match) {
            *idx_matching = idx;;
            return true;
        }
    }
    return false;
}

// this function will not consider nested ()
static bool get_idx_token(size_t* idx_matching, Tk_view tokens, size_t start, TOKEN_TYPE type_to_match) {
    for (size_t idx = start; idx < tokens.count; idx++) {
        if (tokens.tokens[idx].type == type_to_match) {
            if (idx_matching) {
                *idx_matching = idx;
            }
            return true;
        }
    }
    return false;
}

static bool tokens_start_with_function_call(size_t* idx_semicolon, Tk_view tokens) {
    if (tokens.count < 2) {
        return false;
    }

    if (tokens.tokens[0].type != TOKEN_SYMBOL) {
        return false;
    }

    if (tokens.tokens[1].type != TOKEN_OPEN_PAR) {
        return false;
    }

    size_t semicolon_pos;
    get_idx_matching_token(&semicolon_pos, tokens, TOKEN_SEMICOLON);
    if (tokens.tokens[semicolon_pos - 1].type != TOKEN_CLOSE_PAR) {
        return false;
    }

    *idx_semicolon = semicolon_pos;
    return true;
}

static Node_idx parse_rec(PARSE_STATE state, Tk_view tokens) {
    if (state == PARSE_FUN_PARAMS) {
        if (tk_view_front(tokens).type == TOKEN_OPEN_PAR) {
            log(LOG_ERROR, "error: '(' not expected\n");
            abort();
        }
        if (tokens.count > 0) {
            todo();
        }
        Node_idx parameters = node_new();
        nodes_at(parameters)->type = NODE_FUNCTION_PARAMETERS;
        return parameters;
    }

    if (state == PARSE_FUN_ARGUMENTS) {
        size_t idx_semicolon;
        if (get_idx_token(&idx_semicolon, tokens, 0, TOKEN_SEMICOLON)) {
            todo();
        }
        bool end_after_this = false;
        Node_idx prev_node = NODE_IDX_NULL;
        while (1) {
            Tk_view curr_arg_tokens;
            if (get_idx_token(NULL, tokens, 0, TOKEN_COMMA)) {
                curr_arg_tokens = Tk_view_chop_on_type_delim(&tokens, TOKEN_COMMA);
                tk_view_chop_front(&tokens); // trim comma off tokens
            } else {
                curr_arg_tokens = tokens;
                end_after_this = true;
            }

            Node_idx curr_node = parse_rec(PARSE_NORMAL, curr_arg_tokens);
            if (prev_node != NODE_IDX_NULL) {
                nodes_set_next(prev_node, curr_node);
            }

            prev_node = curr_node;
            if (end_after_this) {
                return nodes_get_local_leftmost(curr_node);
            }
        }
    }

    if (state == PARSE_FUN_RETURN_TYPES) {
        if (tokens.count < 1) {
            unreachable();
        }
        if (tokens.count > 1) {
            todo();
        }
        Node_idx return_types = node_new();
        nodes_at(return_types)->type = NODE_FUNCTION_RETURN_TYPES;
        nodes_at(return_types)->lang_type = tokens.tokens[0].text;
        return return_types;
    }

    if (state == PARSE_FUN_BODY) {
        if (tokens.count < 2) {
            unreachable();
        }
        Node_idx body = node_new();
        nodes_at(body)->type = NODE_FUNCTION_BODY;
        assert(tokens.tokens[0].type == TOKEN_SYMBOL);
        assert(tokens.tokens[1].type == TOKEN_OPEN_PAR);
        size_t dummy;
        assert(tokens_start_with_function_call(&dummy, tokens));
        nodes_set_right_child(body, parse_rec(PARSE_NORMAL, tokens));
        return body;
    }

    if (tk_view_front(tokens).type == TOKEN_SYMBOL && 0 == str_view_cmp_cstr(tk_view_front(tokens).text, "fn")) {
        tk_view_chop_front(&tokens);
        Node_idx function = node_new();
        nodes_at(function)->type = NODE_FUNCTION_DEFINITION;
        if (tk_view_front(tokens).type == TOKEN_SYMBOL) {
            nodes_at(function)->name = tk_view_chop_front(&tokens).tokens[0].text;
        } else {
            todo();
        }
        Tk_view open_par = tk_view_chop_front(&tokens);
        assert(open_par.tokens[0].type == TOKEN_OPEN_PAR);

        size_t parameters_len;
        if (tk_view_front(tk_view_chop_front(&tokens)).type == TOKEN_CLOSE_PAR) {
            parameters_len = 2;
        } else {
            todo();
        }
        Tk_view parameters_tokens = tk_view_chop_count(&tokens, parameters_len - 2); // exclude ()
        Node_idx parameters = parse_rec(PARSE_FUN_PARAMS, parameters_tokens);
        nodes_set_right_child(function, parameters);

        size_t return_types_len;
        if (tk_view_front(tokens).type == TOKEN_SYMBOL && 0 == str_view_cmp_cstr(tk_view_front(tokens).text, "void")) {
            return_types_len = 1;
        } else {
            todo();
        }
        Tk_view return_type_tokens = tk_view_chop_count(&tokens, return_types_len);
        Node_idx return_types = parse_rec(PARSE_FUN_RETURN_TYPES, return_type_tokens);
        nodes_set_next(parameters, return_types);

        size_t body_len;
        if (tk_view_front(tokens).type != TOKEN_OPEN_CURLY_BRACE) {
            unreachable();
        }
        if (!get_idx_matching_token(&body_len, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
            todo();
        }
        tk_view_chop_front(&tokens);
        Tk_view body_tokens = {.tokens = tokens.tokens, .count = body_len};
        Node_idx body = parse_rec(PARSE_FUN_BODY, body_tokens);
        nodes_set_next(return_types, body);

        return function;
    }

    size_t semicolon_pos;
    if (tokens_start_with_function_call(&semicolon_pos, tokens)) {
        Node_idx function_call = node_new();
        nodes_at(function_call)->type = NODE_FUNCTION_CALL;
        nodes_at(function_call)->name = tokens.tokens[0].text;
        log_tokens(LOG_TRACE, tokens);
        tk_view_chop_count(&tokens, 2); // exclude outer ()
        size_t parameters_end;
        if (!get_idx_matching_token(&parameters_end, tokens, TOKEN_CLOSE_PAR)) {
            todo();
        }
        Tk_view parameters_tokens = tk_view_chop_count(&tokens, parameters_end); // exclude outer ()
        nodes_set_right_child(function_call, parse_rec(PARSE_FUN_ARGUMENTS, parameters_tokens));
        return function_call;
    }

    if (tokens.count == 1 && Token_is_literal(tokens.tokens[0])) {
        Node_idx new_node = node_new();
        nodes_at(new_node)->type = NODE_LITERAL;
        nodes_at(new_node)->name = tokens.tokens[0].text;
        nodes_at(new_node)->literal_type = tokens.tokens[0].type;
        return new_node;
    }

    if (tokens.count < 1) {
        return NODE_IDX_NULL;
    }

    log(LOG_TRACE, "parse_rec other: "TOKEN_FMT"\n", token_print(tokens.tokens[0]));
    log(LOG_TRACE, "parse_rec other: %zu\n", tokens.count);
    log(LOG_TRACE, "cmp: %d\n", str_view_cmp_cstr(tokens.tokens[0].text, "fn"));
    unreachable();
}

void parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.count};
    Node_idx root = parse_rec(PARSE_NORMAL, token_view);
    log_tree(LOG_VERBOSE, root);
    todo();
}
