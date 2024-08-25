#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"
#include "string_vec.h"

static Node_id parse_single_item(Tk_view tokens);
static Node_id parse_function_single_return_type(Token tokens);
static Node_id parse_block(Tk_view tokens);
INLINE bool extract_block_element(Node_id* child, Tk_view* tokens);
static Node_id parse_single_item_or_block(Tk_view tokens);

#define log_tokens(log_level, token_view, indent) \
    do { \
        log(log_level, "tokens:\n"); \
        for (size_t idx = 0; idx < (token_view).count; idx++) { \
            log_indent(log_level, indent, " "TOKEN_FMT"\n", token_print((token_view).tokens[idx])); \
        } \
        log(log_level, "\n"); \
    } while(0);

static Str_view literal_name_new() {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    char var_name[20];
    sprintf(var_name, "str%zu", count);

    String symbol_name = {0};
    string_cpy_cstr_inplace(&symbol_name, var_name);
    string_vec_append(&literal_strings, symbol_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

// TODO: this function will consider ([])
static bool get_idx_matching_token(
    size_t* idx_matching,
    Tk_view tokens,
    TOKEN_TYPE type_to_match
) {
    int par_level = 0; // return when type_to_match found only if this is 0
    for (size_t idx = 0; idx < tokens.count; idx++) {
        Token curr_token = tokens.tokens[idx];
        if (par_level == 0 && curr_token.type == type_to_match) {
            *idx_matching = idx;
            return true;
        }

        if (token_is_closing(curr_token)) {
            par_level--;
        } else if (token_is_opening(curr_token)) {
            par_level++;
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

    if (0 == str_view_cmp_cstr(tk_view_front(tokens).text, "extern")) {
        return false;
    }

    if (tokens.tokens[0].type != TOKEN_SYMBOL) {
        return false;
    }

    if (tokens.tokens[1].type != TOKEN_OPEN_PAR) {
        return false;
    }

    size_t semicolon_pos;
    if (!get_idx_token(&semicolon_pos, tokens, 0, TOKEN_SEMICOLON)) {
        semicolon_pos = tokens.count;
    }
    if (semicolon_pos < 1) {
        todo();
    }

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

static bool is_assignment(Tk_view tokens) {
    if (!get_idx_token(NULL, tokens, 0, TOKEN_SEMICOLON)) {
        return false;
    }

    Tk_view statement_tokens = tk_view_chop_on_type_delim(&tokens, TOKEN_SEMICOLON);
    return get_idx_token(NULL, statement_tokens, 0, TOKEN_SINGLE_EQUAL);
}

static bool extract_function_parameter(Node_id* child, Tk_view* tokens) {
    if (tokens->count < 2) {
        return false;
    }

    size_t idx_token;
    Tk_view param_tokens;
    if (get_idx_matching_token(&idx_token, *tokens, TOKEN_COMMA)) {
        param_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_COMMA);
        tk_view_chop_front(tokens); // remove ,
    } else {
        param_tokens = tk_view_chop_count(tokens, tokens->count);
    }

    if (param_tokens.count < 2 || param_tokens.count > 3) {
        todo();
    }

    Node_id param = node_new();
    nodes_at(param)->type = NODE_LANG_TYPE;

    nodes_at(param)->lang_type = tk_view_chop_front(&param_tokens).text;

    if (tk_view_front(param_tokens).type == TOKEN_TRIPLE_DOT) {
        tk_view_chop_front(&param_tokens);
        nodes_at(param)->is_variadic = true;
    }

    nodes_at(param)->name = tk_view_chop_front(&param_tokens).text;
    sym_tbl_add(param);

    *child = param;
    return true;
}

static Node_id parse_function_parameters(Tk_view tokens) {
    log(LOG_TRACE, "entering parse_function_parameters\n");

    Node_id fun_params = node_new();
    nodes_at(fun_params)->type = NODE_FUNCTION_PARAMETERS;

    Node_id param;
    while (extract_function_parameter(&param, &tokens)) {
        nodes_append_child(fun_params, param);
    }

    assert(tokens.count == 0);
    return fun_params;
}

static Node_id parse_function_single_return_type(Token token) {
    assert(token.type == TOKEN_SYMBOL);

    Node_id return_type = node_new();
    nodes_at(return_type)->type = NODE_LANG_TYPE;
    nodes_at(return_type)->lang_type = token.text;
    assert(nodes_at(return_type)->lang_type.count > 0);
    return return_type;
}

static bool extract_function_return_type(Node_id* child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }

    // a return type is only one token, at least for now
    Token return_type_token = tk_view_chop_front(tokens);
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

static bool extract_function_parameters(Node_id* result, Tk_view* tokens) {
    size_t parameters_len;
    if (!get_idx_matching_token(&parameters_len, *tokens, TOKEN_CLOSE_PAR)) {
        todo();
    }

    Tk_view parameters_tokens = tk_view_chop_count(tokens, parameters_len);
    tk_view_chop_front(tokens); // remove )

    *result = parse_function_parameters(parameters_tokens);

    return true;
}

static bool extract_function_return_types(Node_id* result, Tk_view* tokens) {
    Tk_view return_type_tokens = tk_view_chop_on_type_delim_or_all(tokens, TOKEN_OPEN_CURLY_BRACE);

    *result = parse_function_return_types(return_type_tokens);
    return true;
}

static Node_id extract_function_declaration_common(Tk_view* tokens) {
    Node_id fun_declaration = node_new();

    nodes_at(fun_declaration)->name = tk_view_chop_front(tokens).text;
    assert(nodes_at(fun_declaration)->name.count > 0);

    sym_tbl_add(fun_declaration);

    Token open_par_token = tk_view_chop_front(tokens); // remove "("
    assert(open_par_token.type == TOKEN_OPEN_PAR);

    Node_id parameters;
    if (!extract_function_parameters(&parameters, tokens)) {
        todo();
    }
    nodes_append_child(fun_declaration, parameters);
    
    Node_id return_types;
    if (!extract_function_return_types(&return_types, tokens)) {
        todo();
    }
    nodes_append_child(fun_declaration, return_types);

    return fun_declaration;
}

static Node_id parse_function_definition(Tk_view tokens) {
    tk_view_chop_front(&tokens); // remove fn

    Node_id function = extract_function_declaration_common(&tokens);

    Token open_curly_brace_token = tk_view_chop_front(&tokens); // remove open curly brace
    assert(open_curly_brace_token.type == TOKEN_OPEN_CURLY_BRACE);

    nodes_at(function)->type = NODE_FUNCTION_DEFINITION;

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

static bool tokens_start_with_variable_declaration(Tk_view tokens) {
    if (is_assignment(tokens)) {
        return false;
    }

    if (tk_view_front(tokens).type != TOKEN_SYMBOL) {
        return false;
    }
    if (0 != str_view_cmp_cstr(tk_view_front(tokens).text, "let")) {
        return false;
    }

    return true;
}

static Node_id parse_variable_definition(Tk_view variable_tokens) {
    Token return_keyword_token = tk_view_chop_front(&variable_tokens); // remove "return"
    assert(0 == token_is_equal(return_keyword_token, "return", TOKEN_SYMBOL));

    Node_id variable_def = node_new();
    nodes_at(variable_def)->type = NODE_VARIABLE_DEFINITION;

    Token variable_name_token = tk_view_chop_front(&variable_tokens);
    nodes_at(variable_def)->name = variable_name_token.text;

    tk_view_chop_front(&variable_tokens); // remove ":"

    nodes_at(variable_def)->lang_type = tk_view_chop_front(&variable_tokens).text;

    /*
    tk_view_chop_front(&variable_tokens); // remove "="

    nodes_at(variable_def)->str_data = tk_view_chop_front(&variable_tokens).text;
    */

    sym_tbl_add(variable_def);

    assert(variable_tokens.count == 0);

    return variable_def;
}

static bool extract_variable_declaration(Node_id* child, Tk_view* tokens) {
    if (!tokens_start_with_variable_declaration(*tokens)) {
        return false;
    }

    Tk_view var_decl_tokens;
    if (get_idx_matching_token(NULL, *tokens, TOKEN_SEMICOLON)) {
        var_decl_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
        tk_view_chop_front(tokens); // remove ;
    } else {
        var_decl_tokens = tk_view_chop_count(tokens, tokens->count);
    }

    *child = parse_variable_definition(var_decl_tokens);
    return true;
}

static Node_id parse_function_declaration(Tk_view tokens) {
    tk_view_chop_count(&tokens, 2); // remove "extern" and "("
    Token extern_type_token = tk_view_chop_front(&tokens); // remove "c"
    tk_view_chop_count(&tokens, 2); // remove ")" and "fn"

    Node_id fun_declaration = extract_function_declaration_common(&tokens);
    nodes_at(fun_declaration)->type = NODE_FUNCTION_DECLARATION;

    if (0 != str_view_cmp_cstr(extern_type_token.text, "c")) {
        todo();
    }

    assert(tokens.count == 0);
    return fun_declaration;
}

static bool tokens_start_with_function_declaration(Tk_view tokens) {
    if (tokens.count < 2) {
        return false;
    }

    return 0 == str_view_cmp_cstr(tk_view_front(tokens).text, "extern");
}

static bool extract_function_declaration(Node_id* child, Tk_view* tokens) {
    if (!tokens_start_with_function_declaration(*tokens)) {
        return false;
    }

    Tk_view declaration_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
    tk_view_chop_front(tokens); // remove ;

    *child = parse_function_declaration(declaration_tokens);
    return true;
}

static Str_view get_literal_lang_type_from_token_type(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            return str_view_from_cstr("String");
        case TOKEN_NUM_LITERAL:
            return str_view_from_cstr("i32");
        default:
            todo();
    }
}

static Node_id parse_literal(Tk_view tokens) {
    Node_id new_node = node_new();
    nodes_at(new_node)->type = NODE_LITERAL;
    nodes_at(new_node)->name = literal_name_new();
    assert(nodes_at(new_node)->str_data.count < 1);
    nodes_at(new_node)->str_data = tk_view_front(tokens).text;
    nodes_at(new_node)->token_type = tk_view_front(tokens).type;
    nodes_at(new_node)->line_num = tk_view_front(tokens).line_num;
    nodes_at(new_node)->lang_type = get_literal_lang_type_from_token_type(nodes_at(new_node)->token_type);

    sym_tbl_add(new_node);
    return new_node;
}

static Node_id parse_symbol(Tk_view tokens) {
    assert(tokens.count == 1);

    Node_id sym_node = node_new();
    nodes_at(sym_node)->type = NODE_SYMBOL;
    nodes_at(sym_node)->name = tk_view_front(tokens).text;
    nodes_at(sym_node)->line_num = tk_view_front(tokens).line_num;
    assert(nodes_at(sym_node)->line_num > 0);
    return sym_node;
}

static Node_id parse_operation(Tk_view tokens) {
    log_tokens(LOG_TRACE, tokens, 0);

    size_t idx_operator = get_idx_lowest_precedence_operator(tokens);
    Tk_view left_tokens = tk_view_chop_count(&tokens, idx_operator);
    Token operator_token = tk_view_chop_front(&tokens);
    Tk_view right_tokens = tokens;

    assert(left_tokens.count > 0);
    assert(right_tokens.count > 0);

    Node_id operator_node = node_new();
    nodes_at(operator_node)->type = NODE_OPERATOR;
    nodes_at(operator_node)->token_type = operator_token.type;
    nodes_at(operator_node)->line_num = operator_token.line_num;

    Node_id left_node = parse_single_item(left_tokens);
    Node_id right_node = parse_single_item(right_tokens);
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

    *child = parse_single_item(curr_arg_tokens);
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

    Node_id argument;
    log_tokens(LOG_DEBUG, arguments_tokens, 0);
    while (extract_function_argument(&argument, &arguments_tokens)) {
        nodes_append_child(function_call, argument);
    }

    assert(tk_view_front(*tokens).type == TOKEN_CLOSE_PAR);
    assert(tokens->count == 1 || tk_view_at(*tokens, 1).type == TOKEN_SEMICOLON);
    tk_view_chop_count_at_most(tokens, 2); // remove );

    *child = function_call;
    return true;
}

static Node_id parse_function_return_statement(Tk_view tokens) {
    tk_view_chop_front(&tokens); // remove "return"

    Node_id new_node = node_new();
    nodes_at(new_node)->type = NODE_RETURN_STATEMENT;
    nodes_append_child(new_node, parse_single_item(tokens));
    return new_node;
}

static bool extract_function_return_statement(Node_id* result, Tk_view* tokens) {
    if (0 != str_view_cmp_cstr(tk_view_front(*tokens).text, "return")) {
        return false;
    }

    Tk_view return_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
    tk_view_chop_front(tokens); // remove ;

    *result = parse_function_return_statement(return_tokens);
    return true;
}

static bool is_block(Tk_view tokens) {
    return \
        tokens_start_with_function_call(tokens) || \
        tokens_start_with_function_definition(tokens) || \
        tokens_start_with_function_declaration(tokens);
}

static bool extract_assignment(Node_id* result, Tk_view* tokens) {
    if (!is_assignment(*tokens)) {
        return false;
    }

    Node_id assignment = node_new();
    nodes_at(assignment)->type = NODE_ASSIGNMENT;
    
    Tk_view lhs_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SINGLE_EQUAL);
    nodes_append_child(assignment, parse_single_item(lhs_tokens));

    tk_view_chop_front(tokens); // remove =

    Tk_view rhs_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
    nodes_append_child(assignment, parse_single_item(rhs_tokens));

    tk_view_chop_front(tokens); // remove ;

    *result = assignment;
    return true;
}

INLINE bool extract_block_element(Node_id* child, Tk_view* tokens) {
    return \
        extract_function_definition(child, tokens) || \
        extract_function_call(child, tokens) || \
        extract_function_return_statement(child, tokens) || \
        extract_assignment(child, tokens) || \
        extract_variable_declaration(child, tokens) || \
        extract_function_declaration(child, tokens);
}

static Node_id parse_block(Tk_view tokens) {
    Node_id block = node_new();
    nodes_at(block)->type = NODE_BLOCK;
    while (tokens.count > 0) {
        Node_id child;
        if (!extract_block_element(&child, &tokens)) {
            todo();
        }
        nodes_append_child(block, child);
    }
    return block;
}

// give block only if there is a need
static Node_id parse_single_item_or_block(Tk_view tokens) {
    Node_id block = parse_block(tokens);
    if (nodes_count_children(block) < 1) {
        unreachable();
    }
    if (nodes_count_children(block) == 1) {
        return nodes_at(block)->left_child;
    }
    return block;
}

static Node_id parse_single_item(Tk_view tokens) {
    unsigned int indent_amt = 0;
    log_tokens(LOG_TRACE, tokens, indent_amt);

    if (tokens.count < 1) {
        unreachable();
    }

    Node_id var_declaration;
    if (extract_variable_declaration(&var_declaration, &tokens)) {
        assert(tokens.count < 1);
        return var_declaration;
    }

    if (tokens.count == 1 && token_is_literal(tokens.tokens[0])) {
        return parse_literal(tokens);
    }

    if (tokens.count == 1 && tk_view_front(tokens).type == TOKEN_SYMBOL) {
        return parse_symbol(tokens);
    }

    Node_id block_element;
    if (extract_block_element(&block_element, &tokens)) {
        assert(tokens.count < 1);
        return block_element;
    }

    if (count_operators(tokens) > 0) {
        return parse_operation(tokens);
    }

    log_tree(LOG_VERBOSE, 0);
    unreachable();
}

Node_id parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Node_id root = parse_block(token_view);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    return root;
}
