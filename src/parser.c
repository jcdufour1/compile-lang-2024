#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"

#define log_tokens(log_level, token_view, indent) \
    do { \
        for (size_t idx = 0; idx < tokens.count; idx++) { \
            log_indent(log_level, indent, " "TOKEN_FMT"\n", token_print(tokens.tokens[idx])); \
        } \
    } while(0);

typedef enum {
    PARSE_NORMAL,
    PARSE_FUN_PARAMS,
    PARSE_FUN_BODY,
    PARSE_FUN_RETURN_TYPES,
    PARSE_FUN_SINGLE_RETURN_TYPE,
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

static size_t count_operators(Tk_view tokens) {
    size_t count_op = 0;
    for (size_t idx = 0; idx < tokens.count; idx++) {
        if (token_is_operator(tk_view_at(tokens, idx))) {
            count_op++;
        }
    }
    return count_op;
}

static size_t get_idx_lowest_precedence_operator(Tk_view tokens) {
    size_t idx_lowest = SIZE_MAX;
    uint32_t lowest_pre = UINT32_MAX; // higher numbers have higher precedence

    for (size_t idx_ = tokens.count; idx_ > 0; idx_--) {
        size_t actual_idx = idx_ - 1;

        if (!token_is_operator(tk_view_at(tokens, actual_idx))) {
            continue;
        }

        uint32_t curr_precedence = token_get_precedence_operator(tk_view_at(tokens, actual_idx));
        if (curr_precedence < lowest_pre) {
            lowest_pre = curr_precedence;
            idx_lowest = actual_idx;
        }
    }

    if (idx_lowest == SIZE_MAX) {
        unreachable();
    }
    return idx_lowest;
}

static Node_idx parse_rec(PARSE_STATE state, Tk_view tokens, int rec_depth) {
    unsigned int indent_amt = 2*rec_depth;
    log_tokens(LOG_TRACE, tokens, indent_amt);
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
                curr_arg_tokens = tk_view_chop_on_type_delim(&tokens, TOKEN_COMMA);
                tk_view_chop_front(&tokens); // trim comma off tokens
            } else {
                curr_arg_tokens = tokens;
                end_after_this = true;
            }

            Node_idx curr_node = parse_rec(PARSE_NORMAL, curr_arg_tokens, rec_depth + 1);
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
        log_tokens(LOG_TRACE, lang_type_tokens, indent_amt);
        if (tokens.count < 1) {
            unreachable();
        }
        Node_idx return_types = node_new();
        nodes_at(return_types)->type = NODE_FUNCTION_RETURN_TYPES;
        Node_idx prev_node = NODE_IDX_NULL;
        bool end_after_this = false;
        while (1) {
            Tk_view lang_type_tokens;
            if (get_idx_token(NULL, tokens, 0, TOKEN_COMMA)) {
                lang_type_tokens = tk_view_chop_on_type_delim(&tokens, TOKEN_COMMA);
                if (tokens.tokens[0].type == TOKEN_OPEN_CURLY_BRACE) {
                    // we are parsing last return type
                    end_after_this = true;
                }
                tk_view_chop_front(&tokens); // trim comma
                log_tokens(LOG_TRACE, lang_type_tokens, indent_amt);
            } else {
                lang_type_tokens = tokens;
                end_after_this = true;
            }

            Node_idx curr_node = parse_rec(PARSE_FUN_SINGLE_RETURN_TYPE, lang_type_tokens, rec_depth + 1);
            if (prev_node != NODE_IDX_NULL) {
                nodes_set_next(prev_node, curr_node);
            }

            prev_node = curr_node;
            if (end_after_this) {
                nodes_set_right_child(return_types, nodes_get_local_leftmost(curr_node));
                return return_types;
            }
        }
    }

    if (state == PARSE_FUN_SINGLE_RETURN_TYPE) {
        if (tokens.count > 1) {
            log(LOG_FETAL, "tokens.count: %zu\n", tokens.count);
            log_tokens(LOG_FETAL, tokens, indent_amt);
            unreachable();
        }
        assert(tokens.tokens[0].type == TOKEN_SYMBOL);
        Node_idx return_type = node_new();
        nodes_at(return_type)->type = NODE_LANG_TYPE;
        nodes_at(return_type)->lang_type = tokens.tokens[0].text;
        return return_type;
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
        nodes_set_right_child(body, parse_rec(PARSE_NORMAL, tokens, rec_depth + 1));
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
        Node_idx parameters = parse_rec(PARSE_FUN_PARAMS, parameters_tokens, rec_depth + 1);
        nodes_set_right_child(function, parameters);

        if (tk_view_front(tokens).type != TOKEN_SYMBOL) {
            todo();
        }
        Tk_view return_type_tokens = tk_view_chop_on_type_delim(&tokens, TOKEN_OPEN_CURLY_BRACE);
        tk_view_chop_front(&tokens); // remove open curly brace
        Node_idx return_types = parse_rec(PARSE_FUN_RETURN_TYPES, return_type_tokens, rec_depth + 1);
        nodes_set_next(parameters, return_types);

        log_tokens(LOG_TRACE, tokens, indent_amt);
        assert(tokens.tokens[tokens.count - 1].type == TOKEN_CLOSE_CURLY_BRACE);
        Tk_view body_tokens = {.tokens = tokens.tokens, .count = tokens.count - 1 /* exclude closing curly brace */};
        Node_idx body = parse_rec(PARSE_FUN_BODY, body_tokens, rec_depth + 1);
        nodes_set_next(return_types, body);

        return function;
    }

    size_t semicolon_pos;
    if (tokens_start_with_function_call(&semicolon_pos, tokens)) {
        Node_idx function_call = node_new();
        nodes_at(function_call)->type = NODE_FUNCTION_CALL;
        nodes_at(function_call)->name = tokens.tokens[0].text;
        log_tokens(LOG_TRACE, tokens, indent_amt);
        tk_view_chop_count(&tokens, 2); // exclude outer ()
        size_t parameters_end;
        if (!get_idx_matching_token(&parameters_end, tokens, TOKEN_CLOSE_PAR)) {
            todo();
        }
        Tk_view parameters_tokens = tk_view_chop_count(&tokens, parameters_end); // exclude outer ()
        nodes_set_right_child(function_call, parse_rec(PARSE_FUN_ARGUMENTS, parameters_tokens, rec_depth + 1));
        return function_call;
    }

    log(LOG_TRACE, "token type: %d\n", tokens.tokens[0].type);
    log(LOG_TRACE, "token type: "TOKEN_TYPE_FMT"\n", token_type_print(tokens.tokens[0].type));
    log(LOG_TRACE, "token_is_literal: %s\n", bool_print(token_is_literal(tokens.tokens[0])));
    log(LOG_TRACE, "tokens.count: %zu\n", tokens.count);
    if (tokens.count == 1 && token_is_literal(tokens.tokens[0])) {
        Node_idx new_node = node_new();
        nodes_at(new_node)->type = NODE_LITERAL;
        nodes_at(new_node)->name = tokens.tokens[0].text;
        nodes_at(new_node)->token_type = tokens.tokens[0].type;
        return new_node;
    }

    if (count_operators(tokens) > 0) {
        log_tokens(LOG_TRACE, tokens, 0);
        size_t idx_operator = get_idx_lowest_precedence_operator(tokens);
        Tk_view left_tokens = tk_view_chop_count(&tokens, idx_operator);
        Tk_view operator_token = tk_view_chop_front(&tokens);
        Tk_view right_tokens = tokens;

        Node_idx operator_node = node_new();
        nodes_at(operator_node)->type = NODE_OPERATOR;
        nodes_at(operator_node)->left_child = parse_rec(PARSE_NORMAL, left_tokens, rec_depth + 1);
        nodes_at(operator_node)->right_child = parse_rec(PARSE_NORMAL, right_tokens, rec_depth + 1);
        nodes_at(operator_node)->token_type = tk_view_front(operator_token).type;
        return operator_node;
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
    Node_idx root = parse_rec(PARSE_NORMAL, token_view, 0);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    todo();
}
