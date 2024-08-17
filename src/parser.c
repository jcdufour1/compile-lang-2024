#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "parse_items.h"

#define log_tokens(log_level, token_view, indent) \
    do { \
        for (size_t idx = 0; idx < token_view.count; idx++) { \
            log_indent(log_level, indent, " "TOKEN_FMT"\n", token_print(token_view.tokens[idx])); \
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

static Node_idx parse_later(PARSE_STATE parse_state, Tk_view tokens) {
    assert(tokens.count > 0);
    // create a node that will later be parsed fully (but node next, prev, and left_child may be set eariler), return idx of that node
    Node_idx new_node = node_new();
    Parse_item new_item = {.parse_state = parse_state, .tokens = tokens, .root_node = new_node};
    parse_items_append(new_item);
    return new_node;
}

static void parse_function_parameters(Parse_item item) {
    if (tk_view_front(item.tokens).type == TOKEN_OPEN_PAR) {
        log(LOG_ERROR, "error: '(' not expected\n");
        abort();
    }
    if (item.tokens.count > 0) {
        todo();
    }
    nodes_at(item.root_node)->type = NODE_FUNCTION_PARAMETERS;
}

static void parse_function_return_types(Parse_item item) {
    log_tokens(LOG_TRACE, item.tokens, 0);
    if (item.tokens.count < 1) {
        unreachable();
    }
    Node_idx return_types = item.root_node;
    nodes_at(return_types)->type = NODE_FUNCTION_RETURN_TYPES;
    Node_idx prev_node = NODE_IDX_NULL;
    bool end_after_this = false;
    while (1) {
        Tk_view lang_type_tokens;
        if (get_idx_token(NULL, item.tokens, 0, TOKEN_COMMA)) {
            lang_type_tokens = tk_view_chop_on_type_delim(&item.tokens, TOKEN_COMMA);
            if (item.tokens.tokens[0].type == TOKEN_OPEN_CURLY_BRACE) {
                // we are parsing last return type
                end_after_this = true;
            }
            tk_view_chop_front(&item.tokens); // trim comma
            log_tokens(LOG_TRACE, lang_type_tokens, 0);
        } else {
            lang_type_tokens = item.tokens;
            end_after_this = true;
        }

        Node_idx curr_node = parse_later(PARSE_FUN_SINGLE_RETURN_TYPE, lang_type_tokens);
        if (prev_node != NODE_IDX_NULL) {
            nodes_set_next(prev_node, curr_node);
        }

        prev_node = curr_node;
        if (end_after_this) {
            nodes_set_left_child(return_types, nodes_get_local_leftmost(curr_node));
            //return return_types;
            return;
        }
    }
}

static void parse_function_single_return_type(Parse_item item) {
    if (item.tokens.count > 1) {
        log(LOG_FETAL, "tokens.count: %zu\n", item.tokens.count);
        log_tokens(LOG_FETAL, item.tokens, 0);
        unreachable();
    }
    assert(item.tokens.tokens[0].type == TOKEN_SYMBOL);
    Node_idx return_type = item.root_node;
    nodes_at(return_type)->type = NODE_LANG_TYPE;
    nodes_at(return_type)->lang_type = item.tokens.tokens[0].text;
    //return return_type;
    return;
}

static void parse_function_body(Parse_item item) {
    if (item.tokens.count < 2) {
        unreachable();
    }
    Node_idx body = item.root_node;
    nodes_at(body)->type = NODE_FUNCTION_BODY;
    assert(item.tokens.tokens[0].type == TOKEN_SYMBOL);
    assert(item.tokens.tokens[1].type == TOKEN_OPEN_PAR);
    size_t dummy;
    assert(tokens_start_with_function_call(&dummy, item.tokens));
    Tk_view body_tokens = item.tokens;
    nodes_set_left_child(body, parse_later(PARSE_NORMAL, body_tokens));
    //return body;
    return;
}

static void parse_function_definition(Parse_item item) {
    tk_view_chop_front(&item.tokens);
    Node_idx function = item.root_node;
    nodes_at(function)->type = NODE_FUNCTION_DEFINITION;
    if (tk_view_front(item.tokens).type == TOKEN_SYMBOL) {
        nodes_at(function)->name = tk_view_chop_front(&item.tokens).tokens[0].text;
    } else {
        todo();
    }
    Tk_view open_par = tk_view_chop_front(&item.tokens);
    assert(open_par.tokens[0].type == TOKEN_OPEN_PAR);

    size_t parameters_len;
    if (tk_view_front(tk_view_chop_front(&item.tokens)).type == TOKEN_CLOSE_PAR) {
        parameters_len = 2;
    } else {
        todo();
    }
    Tk_view parameters_tokens = tk_view_chop_count(&item.tokens, parameters_len - 2); // exclude ()
    Node_idx parameters = parse_later(PARSE_FUN_PARAMS, parameters_tokens);
    nodes_set_left_child(function, parameters);

    if (tk_view_front(item.tokens).type != TOKEN_SYMBOL) {
        todo();
    }
    Tk_view return_type_tokens = tk_view_chop_on_type_delim(&item.tokens, TOKEN_OPEN_CURLY_BRACE);
    tk_view_chop_front(&item.tokens); // remove open curly brace
    Node_idx return_types = parse_later(PARSE_FUN_RETURN_TYPES, return_type_tokens);
    nodes_set_next(parameters, return_types);

    log_tokens(LOG_TRACE, item.tokens, 0);
    Tk_view body_tokens = tk_view_chop_on_type_delim(&item.tokens, TOKEN_CLOSE_CURLY_BRACE);
    tk_view_chop_front(&item.tokens); // remove closing brace
    Node_idx body = parse_later(PARSE_FUN_BODY, body_tokens);
    nodes_set_next(return_types, body);
    if (item.tokens.count > 0) {
        todo();
    }
    //return function;
    return;
}

static void parse_literal(Parse_item item) {
    Node_idx new_node = item.root_node;
    nodes_at(new_node)->type = NODE_LITERAL;
    nodes_at(new_node)->name = item.tokens.tokens[0].text;
    nodes_at(new_node)->token_type = item.tokens.tokens[0].type;
    //return new_node;
    return;
}

static void parse_operation(Parse_item item) {
    log_tokens(LOG_TRACE, item.tokens, 0);
    size_t idx_operator = get_idx_lowest_precedence_operator(item.tokens);
    Tk_view left_tokens = tk_view_chop_count(&item.tokens, idx_operator);
    Tk_view operator_token = tk_view_chop_front(&item.tokens);
    Tk_view right_tokens = item.tokens;

    Node_idx operator_node = item.root_node;
    nodes_at(operator_node)->type = NODE_OPERATOR;
    nodes_at(operator_node)->token_type = tk_view_front(operator_token).type;
    Node_idx left_node = parse_later(PARSE_NORMAL, left_tokens);
    Node_idx right_node = parse_later(PARSE_NORMAL, right_tokens);
    nodes_set_left_child(operator_node, left_node);
    nodes_set_next(left_node, right_node); 
    //return operator_node;
    return;
}

// returns true if the parse was successful
static bool extract_function_argument(Node_idx* child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }

    size_t idx_delim;
    Tk_view curr_arg_tokens;
    if (!get_idx_token(&idx_delim, *tokens, 0, TOKEN_COMMA)) {
        idx_delim = tokens->count - 1;
    }
    curr_arg_tokens = tk_view_chop_count(tokens, idx_delim);
    assert(curr_arg_tokens.count > 0);
    tk_view_chop_front(tokens); // remove comma or closing parenthesis

    *child = parse_later(PARSE_NORMAL, curr_arg_tokens);
    return true;
}

static void parse_function_call(Parse_item item) {
    log_tokens(LOG_TRACE, item.tokens, 0);
    Node_idx function_call = item.root_node;
    nodes_at(function_call)->type = NODE_FUNCTION_CALL;
    nodes_at(function_call)->name = item.tokens.tokens[0].text;
    tk_view_chop_count(&item.tokens, 2); // exclude outer ()

    size_t parameters_end;
    if (!get_idx_matching_token(&parameters_end, item.tokens, TOKEN_CLOSE_PAR)) {
        todo();
    }
    Tk_view arguments_tokens = tk_view_chop_count(&item.tokens, parameters_end);
    tk_view_chop_front(&item.tokens); // remove closing )
    tk_view_chop_front(&item.tokens); // remove ;

    Node_idx child;
    while (extract_function_argument(&child, &arguments_tokens)) {
        nodes_append_child(item.root_node, child);
    }
    log_tokens(LOG_TRACE, item.tokens, 0);
    assert(item.tokens.count == 0);
}

static void parse_parse_item(Parse_item item) {
    unsigned int indent_amt = 0;
    log_tokens(LOG_TRACE, item.tokens, indent_amt);

    switch (item.parse_state) {
        case PARSE_NORMAL:
            break;
        case PARSE_FUN_PARAMS:
            parse_function_parameters(item);
            return;
        case PARSE_FUN_RETURN_TYPES:
            parse_function_return_types(item);
            return;
        case PARSE_FUN_SINGLE_RETURN_TYPE:
            parse_function_single_return_type(item);
            return;
        case PARSE_FUN_BODY:
            parse_function_body(item);
            return;
        case PARSE_FUN_SINGLE_ARGUMENT:
            todo();
            return;
        default:
            unreachable();
    }

    if (tk_view_front(item.tokens).type == TOKEN_SYMBOL && 0 == str_view_cmp_cstr(tk_view_front(item.tokens).text, "fn")) {
        parse_function_definition(item);
        return;
    }

    size_t semicolon_pos;
    if (tokens_start_with_function_call(&semicolon_pos, item.tokens)) {
        parse_function_call(item);
        return;
    }

    if (item.tokens.count == 1 && token_is_literal(item.tokens.tokens[0])) {
        parse_literal(item);
        return;
    }

    if (count_operators(item.tokens) > 0) {
        parse_operation(item);
        return;
    }

    if (item.tokens.count < 1) {
        todo();
    }

    log_tokens(LOG_TRACE, item.tokens, 0);
    log(LOG_TRACE, "parse_rec other: %zu\n", item.tokens.count);
    log(LOG_TRACE, "cmp: %d\n", str_view_cmp_cstr(item.tokens.tokens[0].text, "fn"));
    unreachable();
}

static Node_idx new_parse(Tk_view tokens) {
    parse_items_set_to_zero_len();

    // push initial item
    Parse_item parse_to_root = {.root_node = node_new(), .tokens = tokens, .parse_state = PARSE_NORMAL};
    parse_items_append(parse_to_root);

    while (parse_items.count > 0) {
        Parse_item item = Parse_items_pop();
        parse_parse_item(item);
    }

    return 0; // assume root is 0 for now
}


void parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.count};
    Node_idx root = new_parse(token_view);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    todo();
}
