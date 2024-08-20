#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"

static Node_id parse_rec(Tk_view tokens);
static Node_id parse_function_single_return_type(Tk_view tokens);
static Node_id parse_block(Tk_view tokens);

#define log_tokens(log_level, token_view, indent) \
    do { \
        log(log_level, "tokens:\n"); \
        for (size_t idx = 0; idx < (token_view).count; idx++) { \
            log_indent(log_level, indent, " "TOKEN_FMT"\n", token_print((token_view).tokens[idx])); \
        } \
    } while(0);

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

static bool tokens_start_with_function_call(Tk_view tokens) {
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

static bool extract_function_parameter(Node_id* child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }
    assert(tokens->count > 1 && "type or parameter name not specified");

    size_t idx_token;
    Tk_view param_tokens;
    if (get_idx_matching_token(&idx_token, *tokens, TOKEN_COMMA)) {
        param_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_COMMA);
        tk_view_chop_front(tokens); // remove ,
    } else {
        param_tokens = tk_view_chop_count(tokens, tokens->count);
    }

    if (param_tokens.count != 2) {
        todo();
    }

    Node_id param = node_new();
    nodes_at(param)->type = NODE_LANG_TYPE;
    nodes_at(param)->lang_type = tk_view_front(tk_view_chop_front(&param_tokens)).text;
    nodes_at(param)->name = tk_view_front(tk_view_chop_front(&param_tokens)).text;

    *child = param;
    return true;
}

static Node_id parse_function_parameters(Tk_view tokens) {
    //log(LOG_TRACE, "entering parse_function_parameters\n");
    log(LOG_DEBUG, "entering parse_function_parameters\n");
    log_tokens(LOG_DEBUG, tokens, 0);

    Node_id fun_params = node_new();
    nodes_at(fun_params)->type = NODE_FUNCTION_PARAMETERS;

    Node_id param;
    while (extract_function_parameter(&param, &tokens)) {
        nodes_append_child(fun_params, param);
    }

    assert(tokens.count == 0);
    return fun_params;
}

static Node_id parse_function_single_return_type(Tk_view tokens) {
    assert(tokens.count == 1);
    assert(tokens.tokens[0].type == TOKEN_SYMBOL);

    Node_id return_type = node_new();
    nodes_at(return_type)->type = NODE_LANG_TYPE;
    nodes_at(return_type)->lang_type = tk_view_front(tokens).text;
    return return_type;
}

static bool extract_function_return_type(Node_id* child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }

    // a return type is only one token, at least for now
    Tk_view return_type_token = tk_view_chop_front(tokens);
    if (get_idx_token(NULL, *tokens, 0, TOKEN_COMMA)) {
        tk_view_chop_front(tokens); // remove comma
    }

    *child = parse_function_single_return_type(return_type_token);
    return true;
}

static Node_id parse_function_return_types(Tk_view tokens) {
    Node_id return_types = node_new();
    nodes_at(return_types)->type = NODE_FUNCTION_RETURN_TYPES;

    Node_id child;
    while (extract_function_return_type(&child, &tokens)) {
        nodes_append_child(return_types, child);
    }
    return return_types;
}

static bool tokens_start_with_function_definition(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }

    if (tk_view_front(tokens).type != TOKEN_SYMBOL) {
        return false;
    }

    if (0 != str_view_cmp_cstr(tk_view_front(tokens).text, "fn")) {
        return false;
    }

    return true;
}

static Node_id parse_function_definition(Tk_view tokens) {
    tk_view_chop_front(&tokens);
    Node_id function = node_new();
    nodes_at(function)->type = NODE_FUNCTION_DEFINITION;
    if (tk_view_front(tokens).type == TOKEN_SYMBOL) {
        nodes_at(function)->name = tk_view_chop_front(&tokens).tokens[0].text;
    } else {
        todo();
    }
    Tk_view open_par = tk_view_chop_front(&tokens); // remove (
    assert(open_par.tokens[0].type == TOKEN_OPEN_PAR);

    assert(tk_view_front(open_par).type == TOKEN_OPEN_PAR);
    size_t parameters_len;
    get_idx_matching_token(&parameters_len, tokens, TOKEN_CLOSE_PAR);
    Tk_view parameters_tokens = tk_view_chop_count(&tokens, parameters_len); // exclude ()
    Node_id parameters = parse_function_parameters(parameters_tokens);
    nodes_append_child(function, parameters);
    tk_view_chop_front(&tokens); // remove )

    Tk_view return_type_tokens = tk_view_chop_on_type_delim(&tokens, TOKEN_OPEN_CURLY_BRACE);
    Tk_view open_curly_brace_token = tk_view_chop_front(&tokens); // remove open curly brace
    assert(tk_view_front(open_curly_brace_token).type == TOKEN_OPEN_CURLY_BRACE);
    Node_id return_types = parse_function_return_types(return_type_tokens);
    nodes_append_child(function, return_types);

    log_tokens(LOG_TRACE, tokens, 0);
    Tk_view body_tokens = tk_view_chop_count(&tokens, tokens.count);
    Node_id body = parse_block(body_tokens);
    nodes_append_child(function, body);

    assert(tokens.count == 0);
    
    return function;
}

static bool extract_function_definition(Node_id* child, Tk_view* tokens) {
    if (!tokens_start_with_function_definition(*tokens)) {
        return false;
    }

    Tk_view function_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_CLOSE_CURLY_BRACE);
    tk_view_chop_front(tokens); // remove }

    *child = parse_function_definition(function_tokens);
    return true;
}

static Node_id parse_literal(Tk_view tokens) {
    Node_id new_node = node_new();
    nodes_at(new_node)->type = NODE_LITERAL;
    nodes_at(new_node)->name = tk_view_front(tokens).text;
    nodes_at(new_node)->token_type = tk_view_front(tokens).type;

    sym_tbl_add(new_node);
    return new_node;
}

static Node_id parse_symbol(Tk_view tokens) {
    assert(tokens.count == 1);

    Node_id sym_node = node_new();
    nodes_at(sym_node)->type = NODE_SYMBOL;
    nodes_at(sym_node)->name = tk_view_front(tokens).text;
    return sym_node;
}

static Node_id parse_operation(Tk_view tokens) {
    log_tokens(LOG_TRACE, tokens, 0);

    size_t idx_operator = get_idx_lowest_precedence_operator(tokens);
    Tk_view left_tokens = tk_view_chop_count(&tokens, idx_operator);
    Tk_view operator_token = tk_view_chop_front(&tokens);
    Tk_view right_tokens = tokens;

    assert(left_tokens.count > 0);
    assert(operator_token.count == 1);
    assert(right_tokens.count > 0);

    Node_id operator_node = node_new();
    nodes_at(operator_node)->type = NODE_OPERATOR;
    nodes_at(operator_node)->token_type = tk_view_front(operator_token).type;

    Node_id left_node = parse_rec(left_tokens);
    Node_id right_node = parse_rec(right_tokens);
    nodes_append_child(operator_node, left_node);
    nodes_append_child(operator_node, right_node);
    return operator_node;
}

// returns true if the parse was successful
static bool extract_function_argument(Node_id* child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }

    size_t idx_delim;
    Tk_view curr_arg_tokens;

    bool remove_comma = false;
    if (get_idx_token(&idx_delim, *tokens, 0, TOKEN_COMMA)) {
        remove_comma = true;
    } else {
        idx_delim = tokens->count;
    }
    curr_arg_tokens = tk_view_chop_count(tokens, idx_delim);

    if (remove_comma) {
        tk_view_chop_front(tokens); // remove comma
    }

    log(LOG_DEBUG, "curr_arg_tokens tokens: "TK_VIEW_FMT"\n", tk_view_print(curr_arg_tokens));
    *child = parse_rec(curr_arg_tokens);
    return true;
}

static bool extract_function_call(Node_id* child, Tk_view* tokens) {
    log_tokens(LOG_TRACE, *tokens, 0);
    if (!tokens_start_with_function_call(*tokens)) {
        return false;
    }

    Node_id function_call = node_new();
    nodes_at(function_call)->type = NODE_FUNCTION_CALL;
    nodes_at(function_call)->name = tokens->tokens[0].text;
    assert(tk_view_at(*tokens, 0).type == TOKEN_SYMBOL);
    assert(tk_view_at(*tokens, 1).type == TOKEN_OPEN_PAR);
    tk_view_chop_count(tokens, 2); // exclude outer ()

    size_t parameters_end;
    if (!get_idx_matching_token(&parameters_end, *tokens, TOKEN_CLOSE_PAR)) {
        todo();
    }
    Tk_view arguments_tokens = tk_view_chop_count(tokens, parameters_end);
    log_tokens(LOG_DEBUG, arguments_tokens, 0);

    Node_id argument;
    while (extract_function_argument(&argument, &arguments_tokens)) {
        nodes_append_child(function_call, argument);
    }

    assert(tk_view_front(*tokens).type == TOKEN_CLOSE_PAR);
    assert(tk_view_at(*tokens, 1).type == TOKEN_SEMICOLON);
    tk_view_chop_count(tokens, 2); // remove );

    *child = function_call;
    return true;
}

static Node_id parse_block(Tk_view tokens) {
    Node_id block = node_new();
    nodes_at(block)->type = NODE_BLOCK;
    while (tokens.count > 0) {
        Node_id child;
        if (extract_function_definition(&child, &tokens)) {
            nodes_append_child(block, child);
        } else if (extract_function_call(&child, &tokens)) {
            nodes_append_child(block, child);
        } else {
            todo();
        }
    }
    return block;
}

static Node_id parse_rec(Tk_view tokens) {
    unsigned int indent_amt = 0;
    log_tokens(LOG_TRACE, tokens, indent_amt);

    if (tokens.count < 1) {
        //log_tree(LOG_VERBOSE, root);
        unreachable();
    }

    if (tokens.count == 1 && token_is_literal(tokens.tokens[0])) {
        return parse_literal(tokens);
    }

    if (tokens.count == 1 && tk_view_front(tokens).type == TOKEN_SYMBOL) {
        return parse_symbol(tokens);
    }

    if (tokens_start_with_function_call(tokens) || tokens_start_with_function_definition(tokens)) {
        return parse_block(tokens);
    }

    if (count_operators(tokens) > 0) {
        return parse_operation(tokens);
    }

    log_tokens(LOG_DEBUG, tokens, 0);
    log_tree(LOG_VERBOSE, 0);
    unreachable();
}

Node_id parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.count};
    Node_id root = parse_rec(token_view);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    return root;
}
