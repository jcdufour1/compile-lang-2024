#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"
#include "parser_utils.h"

static Node* parse_function_single_return_type(Token tokens);
static Node* parse_block(Tk_view tokens);
INLINE bool extract_statement(Node** child, Tk_view* tokens);
static Node* parse_single_item_or_block(Tk_view tokens);
static Node* parse_statement(Tk_view tokens);
static Node* parse_expression(Tk_view tokens);

#define log_tokens(log_level, token_view, indent) \
    do { \
        log(log_level, "tokens:\n"); \
        for (size_t idx = 0; idx < (token_view).count; idx++) { \
            log_indent((log_level), (indent), " "TOKEN_FMT"\n", token_print((token_view).tokens[idx])); \
        } \
        log(log_level, "\n"); \
    } while(0);

// TODO: this function will consider ([])
static bool get_idx_matching_token(
    size_t* idx_matching,
    Tk_view tokens, 
    bool include_opposite_type_to_match, // true if opening bracket that matches closing bracket 
                                         // type_to_match is included from tokens, 
                                         // false otherwise
    TOKEN_TYPE type_to_match
) {
    int par_level = include_opposite_type_to_match ? (-1) : (0);
    for (size_t idx = 0; idx < tokens.count; idx++) {
        Token curr_token = tokens.tokens[idx];
        if (par_level == 0 && curr_token.type == type_to_match) {
            if (idx_matching) {
                *idx_matching = idx;
            }
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

// consume tokens from { to } (inclusive) and discard outer {}
static Tk_view extract_items_inside_brackets(Tk_view* tokens, TOKEN_TYPE closing_bracket_type) {
    Token opening_bracket = tk_view_chop_front(tokens);
    assert(opening_bracket.type == TOKEN_OPEN_CURLY_BRACE);
    // the opening_bracket type should be the opening bracket type that corresponds to closing_brace_type
    switch (closing_bracket_type) {
        case TOKEN_CLOSE_CURLY_BRACE:
            assert(opening_bracket.type == TOKEN_OPEN_CURLY_BRACE);
            break;
        case TOKEN_CLOSE_PAR:
            assert(opening_bracket.type == TOKEN_OPEN_PAR);
            break;
        default:
            unreachable("invalid or unimplemented bracket type");
    }

    size_t idx_closing_bracket;
    if (!get_idx_matching_token(&idx_closing_bracket, *tokens, false, closing_bracket_type)) {
        unreachable("closing bracket not found");
    }
    Tk_view inside_brackets = tk_view_chop_count(tokens, idx_closing_bracket);

    Token closing_bracket = tk_view_chop_front(tokens);
    assert(closing_bracket.type == closing_bracket_type);
    return inside_brackets;
}

static bool tokens_start_with_function_call(Tk_view tokens) {
    if (tokens.count < 2) {
        return false;
    }

    if (str_view_cstr_is_equal(tk_view_front(tokens).text, "extern")) {
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
    uint16_t par_level = 1;

    for (size_t idx_ = tokens.count; idx_ > 0; idx_--) {
        size_t actual_idx = idx_ - 1;
        Token curr_token = tk_view_at(tokens, actual_idx);

        if (curr_token.type == TOKEN_OPEN_PAR) {
            log(LOG_DEBUG, "open\n");
            assert(par_level > 1);
            par_level--;
            continue;
        }
        if (curr_token.type == TOKEN_CLOSE_PAR) {
            log(LOG_DEBUG, "close\n");
            par_level++;
            continue;
        }

        if (!token_is_operator(curr_token)) {
            continue;
        }

        assert(par_level > 0);
        uint32_t curr_precedence = (par_level*TOKEN_MAX_PRECEDENCE)*token_get_precedence_operator(
            tk_view_at(tokens, actual_idx)
        );
        if (curr_precedence < lowest_pre) {
            lowest_pre = curr_precedence;
            idx_lowest = actual_idx;
        }
    }

    if (idx_lowest == SIZE_MAX) {
        unreachable("");
    }
    return idx_lowest;
}

static bool is_assignment(Tk_view tokens) {
    Tk_view statement_tokens = tk_view_chop_on_type_delim_or_all(&tokens, TOKEN_SEMICOLON);
    return get_idx_token(NULL, statement_tokens, 0, TOKEN_SINGLE_EQUAL);
}

static bool extract_function_parameter(Node** child, Tk_view* tokens) {
    if (tokens->count < 2) {
        return false;
    }

    size_t idx_token;
    Tk_view param_tokens;
    if (get_idx_matching_token(&idx_token, *tokens, false, TOKEN_COMMA)) {
        param_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_COMMA);
        tk_view_chop_front(tokens); // remove ,
    } else {
        param_tokens = tk_view_chop_count(tokens, tokens->count);
    }

    Node* param = node_new();
    param->type = NODE_VARIABLE_DEFINITION;

    param->name = tk_view_chop_front(&param_tokens).text;

    tk_view_chop_front(&param_tokens); // remove :

    param->lang_type = tk_view_chop_front(&param_tokens).text;

    if (param_tokens.count > 0 && tk_view_front(param_tokens).type == TOKEN_TRIPLE_DOT) {
        tk_view_chop_front(&param_tokens);
        param->is_variadic = true;
    }

    sym_tbl_add(param);

    *child = param;
    assert(param_tokens.count == 0);
    return true;
}

static Node* parse_function_parameters(Tk_view tokens) {
    //log(LOG_TRACE, "entering parse_function_parameters\n");

    Node* fun_params = node_new();
    fun_params->type = NODE_FUNCTION_PARAMETERS;

    Node* param;
    while (extract_function_parameter(&param, &tokens)) {
        nodes_append_child(fun_params, param);
    }

    assert(tokens.count == 0);
    return fun_params;
}

static Node* parse_function_single_return_type(Token token) {
    assert(token.type == TOKEN_SYMBOL);

    Node* return_type = node_new();
    return_type->type = NODE_LANG_TYPE;
    return_type->lang_type = token.text;
    assert(return_type->lang_type.count > 0);
    return return_type;
}

static bool extract_function_return_type(Node** child, Tk_view* tokens) {
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

static Node* parse_function_return_types(Tk_view tokens) {
    Node* return_types = node_new();
    return_types->type = NODE_FUNCTION_RETURN_TYPES;

    Node* child;
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

    if (!str_view_cstr_is_equal(tk_view_front(tokens).text, "fn")) {
        return false;
    }

    return true;
}

static bool extract_function_parameters(Node** result, Tk_view* tokens) {
    size_t parameters_len;
    if (!get_idx_matching_token(&parameters_len, *tokens, false, TOKEN_CLOSE_PAR)) {
        todo();
    }

    Tk_view parameters_tokens = tk_view_chop_count(tokens, parameters_len);
    tk_view_chop_front(tokens); // remove )

    *result = parse_function_parameters(parameters_tokens);

    return true;
}

static bool extract_function_return_types(Node** result, Tk_view* tokens) {
    Tk_view return_type_tokens = tk_view_chop_on_type_delim_or_all(tokens, TOKEN_OPEN_CURLY_BRACE);

    *result = parse_function_return_types(return_type_tokens);
    return true;
}

static Node* extract_function_declaration_common(Tk_view* tokens) {
    Node* fun_declaration = node_new();

    fun_declaration->name = tk_view_chop_front(tokens).text;
    assert(fun_declaration->name.count > 0);

    sym_tbl_add(fun_declaration);

    Token open_par_token = tk_view_chop_front(tokens); // remove "("
    assert(open_par_token.type == TOKEN_OPEN_PAR);

    Node* parameters;
    if (!extract_function_parameters(&parameters, tokens)) {
        todo();
    }
    nodes_append_child(fun_declaration, parameters);
    
    Node* return_types;
    if (!extract_function_return_types(&return_types, tokens)) {
        todo();
    }
    nodes_append_child(fun_declaration, return_types);

    return fun_declaration;
}

static Node* parse_function_definition(Tk_view tokens) {
    tk_view_chop_front(&tokens); // remove fn

    Node* function = extract_function_declaration_common(&tokens);

    Token open_curly_brace_token = tk_view_chop_front(&tokens); // remove open curly brace
    assert(open_curly_brace_token.type == TOKEN_OPEN_CURLY_BRACE);

    function->type = NODE_FUNCTION_DEFINITION;

    Tk_view body_tokens = tk_view_chop_count(&tokens, tokens.count);
    Node* body = parse_block(body_tokens);
    nodes_append_child(function, body);

    assert(tokens.count == 0);
    
    return function;
}

static Node* extract_function_definition(Tk_view* tokens) {
    size_t close_curly_brace_idx;
    if (!get_idx_matching_token(&close_curly_brace_idx, *tokens, true, TOKEN_CLOSE_CURLY_BRACE)) {
        todo();
    }
    Tk_view function_tokens = tk_view_chop_count(tokens, close_curly_brace_idx);
    tk_view_chop_front(tokens); // remove }

    return parse_function_definition(function_tokens);
}

static bool tokens_start_with_variable_declaration(Tk_view tokens) {
    if (tk_view_front(tokens).type != TOKEN_SYMBOL) {
        return false;
    }
    if (!str_view_cstr_is_equal(tk_view_front(tokens).text, "let")) {
        return false;
    }

    return true;
}

static Node* parse_variable_definition(Tk_view variable_tokens, bool require_let) {
     // remove "let"
    if (token_is_equal(tk_view_front(variable_tokens), "let", TOKEN_SYMBOL)) {
        tk_view_chop_front(&variable_tokens);
    } else if (require_let) {
        unreachable("");
    }

    Node* variable_def = node_new();
    variable_def->type = NODE_VARIABLE_DEFINITION;

    Token variable_name_token = tk_view_chop_front(&variable_tokens);
    variable_def->name = variable_name_token.text;

    tk_view_chop_front(&variable_tokens); // remove ":"

    variable_def->lang_type = tk_view_chop_front(&variable_tokens).text;

    sym_tbl_add(variable_def);

    if (variable_tokens.count > 0) {
        nodes_append_child(variable_def, parse_expression(variable_tokens));
    }

    return variable_def;
}

static bool extract_variable_declaration(Node** child, Tk_view* tokens) {
    Tk_view var_decl_tokens;
    if (get_idx_matching_token(NULL, *tokens, false, TOKEN_SEMICOLON)) {
        var_decl_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
        tk_view_chop_front(tokens); // remove ;
    } else {
        var_decl_tokens = tk_view_chop_count(tokens, tokens->count);
    }

    *child = parse_variable_definition(var_decl_tokens, true);
    return true;
}

static bool tokens_start_with_for_loop(Tk_view tokens) {
    if (tk_view_front(tokens).type != TOKEN_SYMBOL) {
        return false;
    }
    return str_view_cstr_is_equal(tk_view_front(tokens).text, "for");
}

static bool is_not_symbol_in(const Token* prev, const Token* curr) {
    (void) prev;
    if (curr->type != TOKEN_SYMBOL) {
        return true;
    }
    return !str_view_cstr_is_equal(curr->text, "in");
}

static Node* extract_for_loop(Tk_view* tokens) {
    Node* for_loop = node_new();
    for_loop->type = NODE_FOR_LOOP;
    tk_view_chop_front(tokens); // remove for

    Tk_view for_var_decl_tokens = tk_view_chop_on_cond(tokens, is_not_symbol_in);
    Node* var_def = node_new();
    var_def->type = NODE_FOR_VARIABLE_DEF;
    nodes_append_child(var_def, parse_variable_definition(for_var_decl_tokens, false));
    nodes_append_child(for_loop, var_def);
    tk_view_chop_front(tokens); // remove in

    Token next_token = tk_view_chop_front(tokens); // remove {
    if (next_token.type != TOKEN_OPEN_CURLY_BRACE) {
        todo();
    }
    Tk_view lower_bound_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_DOUBLE_DOT);
    Node* lower_bound = node_new();
    lower_bound->type = NODE_FOR_LOWER_BOUND;
    nodes_append_child(lower_bound, parse_expression(lower_bound_tokens));
    nodes_append_child(for_loop, lower_bound);

    tk_view_chop_front(tokens); // remove ..

    Tk_view upper_bound_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_CLOSE_CURLY_BRACE);
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    Node* upper_bound = node_new();
    upper_bound->type = NODE_FOR_UPPER_BOUND;
    nodes_append_child(upper_bound, parse_expression(upper_bound_tokens));
    nodes_append_child(for_loop, upper_bound);

    Tk_view body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE);
    nodes_append_child(for_loop, parse_block(body_tokens));

    return for_loop;
}

static Node* parse_function_declaration(Tk_view tokens) {
    try(tk_view_try_consume_symbol(NULL, &tokens, "extern"));
    try(tk_view_try_consume(NULL, &tokens, TOKEN_OPEN_PAR));
    Token extern_type_token = tk_view_chop_front(&tokens); // remove "c"
    try(tk_view_try_consume(NULL, &tokens, TOKEN_CLOSE_PAR));
    try(tk_view_try_consume_symbol(NULL, &tokens, "fn"));

    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        todo();
    }

    Node* fun_declaration = extract_function_declaration_common(&tokens);
    fun_declaration->type = NODE_FUNCTION_DECLARATION;

    assert(tokens.count == 0);
    return fun_declaration;
}

static bool tokens_start_with_function_declaration(Tk_view tokens) {
    if (tokens.count < 2) {
        return false;
    }

    return str_view_cstr_is_equal(tk_view_front(tokens).text, "extern");
}

static Node* extract_function_declaration(Tk_view* tokens) {
    Tk_view declaration_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
    try(tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON));
    return parse_function_declaration(declaration_tokens);
}

static Str_view get_literal_lang_type_from_token_type(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            return str_view_from_cstr("ptr");
        case TOKEN_NUM_LITERAL:
            return str_view_from_cstr("i32");
        default:
            todo();
    }
}

static Node* parse_literal(Tk_view tokens) {
    Node* new_node = node_new();
    new_node->type = NODE_LITERAL;
    new_node->name = literal_name_new();
    assert(new_node->str_data.count < 1);
    new_node->str_data = tk_view_front(tokens).text;
    new_node->token_type = tk_view_front(tokens).type;
    new_node->line_num = tk_view_front(tokens).line_num;
    new_node->lang_type = get_literal_lang_type_from_token_type(new_node->token_type);

    sym_tbl_add(new_node);
    return new_node;
}

static Node* parse_symbol(Tk_view tokens) {
    assert(tokens.count == 1);

    Node* sym_node = node_new();
    sym_node->type = NODE_SYMBOL;
    sym_node->name = tk_view_front(tokens).text;
    sym_node->line_num = tk_view_front(tokens).line_num;
    assert(sym_node->line_num > 0);
    return sym_node;
}

static Node* parse_operation(Tk_view tokens) {
    //log_tokens(LOG_TRACE, tokens, 0);
    size_t idx_operator = get_idx_lowest_precedence_operator(tokens);
    Tk_view left_tokens = tk_view_chop_count(&tokens, idx_operator);
    Token operator_token = tk_view_chop_front(&tokens);
    Tk_view right_tokens = tokens;

    assert(left_tokens.count > 0);
    assert(right_tokens.count > 0);

    Node* operator_node = node_new();
    operator_node->type = NODE_OPERATOR;
    operator_node->token_type = operator_token.type;
    operator_node->line_num = operator_token.line_num;

    Node* left_node = parse_expression(left_tokens);
    Node* right_node = parse_expression(right_tokens);
    nodes_append_child(operator_node, left_node);
    nodes_append_child(operator_node, right_node);
    return operator_node;
}

// returns true if the parse was successful
static bool extract_function_argument(Node** child, Tk_view* tokens) {
    if (tokens->count < 1) {
        return false;
    }

    assert(tk_view_front(*tokens).type != TOKEN_COMMA);
    Tk_view curr_arg_tokens = tk_view_chop_on_type_delim_or_all(tokens, TOKEN_COMMA);
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    *child = parse_expression(curr_arg_tokens);
    return true;
}

static bool extract_function_call(Node** child, Tk_view* tokens) {
    //log_tokens(LOG_TRACE, *tokens, 0);
    if (!tokens_start_with_function_call(*tokens)) {
        return false;
    }

    Node* function_call = node_new();
    function_call->type = NODE_FUNCTION_CALL;
    function_call->name = tokens->tokens[0].text;
    assert(tk_view_at(*tokens, 0).type == TOKEN_SYMBOL);
    assert(tk_view_at(*tokens, 1).type == TOKEN_OPEN_PAR);
    tk_view_chop_count(tokens, 2); // exclude outer ()

    size_t parameters_end;
    if (!get_idx_matching_token(&parameters_end, *tokens, false, TOKEN_CLOSE_PAR)) {
        todo();
    }
    Tk_view arguments_tokens = tk_view_chop_count(tokens, parameters_end);

    Node* argument;
    while (extract_function_argument(&argument, &arguments_tokens)) {
        nodes_append_child(function_call, argument);
    }

    assert(tk_view_front(*tokens).type == TOKEN_CLOSE_PAR);
    assert(tokens->count == 1 || tk_view_at(*tokens, 1).type == TOKEN_SEMICOLON);
    tk_view_chop_count_at_most(tokens, 2); // remove );

    *child = function_call;
    return true;
}

static Node* parse_function_return_statement(Tk_view tokens) {
    try(tk_view_try_consume_symbol(NULL, &tokens, "return"));

    Node* new_node = node_new();
    new_node->type = NODE_RETURN_STATEMENT;
    nodes_append_child(new_node, parse_expression(tokens));
    tk_view_try_consume(NULL, &tokens, TOKEN_SEMICOLON);
    return new_node;
}

static Node* extract_function_return_statement(Tk_view* tokens) {
    Tk_view return_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SEMICOLON);
    try(tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON));
    return parse_function_return_statement(return_tokens);
}

static Node* extract_assignment(Tk_view* tokens) {
    Node* assignment = node_new();
    assignment->type = NODE_ASSIGNMENT;
    
    Tk_view lhs_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_SINGLE_EQUAL);
    nodes_append_child(assignment, parse_expression(lhs_tokens));

    try(tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_EQUAL));

    Tk_view rhs_tokens = tk_view_chop_on_type_delim_or_all(tokens, TOKEN_SEMICOLON);
    nodes_append_child(assignment, parse_expression(rhs_tokens));

    if (!tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON)) {
        assert(tokens->count < 1);
    }

    return assignment;
}

static bool is_if_statement(Tk_view tokens) {
    if (tk_view_front(tokens).type == TOKEN_SYMBOL && str_view_cstr_is_equal(tk_view_front(tokens).text, "if")) {
        return true;
    }
    return false;
}

static Node* parse_if_condition(Tk_view tokens) {
    Node* condition = node_new();
    condition->type = NODE_IF_CONDITION;

    if (count_operators(tokens) > 0) {
        nodes_append_child(condition, parse_operation(tokens));
        return condition;
    }

    todo();
}

static bool extract_if_statement(Node** result, Tk_view* tokens) {
    if (!is_if_statement(*tokens)) {
        return false;
    }

    Node* if_statement = node_new();
    if_statement->type = NODE_IF_STATEMENT;

    tk_view_chop_front(tokens); // remove "if"
    Tk_view if_condition_tokens = tk_view_chop_on_type_delim(tokens, TOKEN_OPEN_CURLY_BRACE);
    Node* if_condition = parse_if_condition(if_condition_tokens);

    Tk_view if_body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE);
    Node* if_body = parse_block(if_body_tokens);

    nodes_append_child(if_statement, if_condition);
    nodes_append_child(if_statement, if_body);

    *result = if_statement;
    return true;
}

INLINE bool extract_statement(Node** child, Tk_view* tokens) {
    if (token_is_equal(tk_view_front(*tokens), "extern", TOKEN_SYMBOL)) {
        *child = extract_function_declaration(tokens);
        return true;
    }

    if (token_is_equal(tk_view_front(*tokens), "fn", TOKEN_SYMBOL)) {
        *child = extract_function_definition(tokens);
        return true;
    }

    if (token_is_equal(tk_view_front(*tokens), "return", TOKEN_SYMBOL)) {
        *child = extract_function_return_statement(tokens);
        return true;
    }

    if (token_is_equal(tk_view_front(*tokens), "if", TOKEN_SYMBOL)) {
        return extract_if_statement(child, tokens);
    }

    if (token_is_equal(tk_view_front(*tokens), "for", TOKEN_SYMBOL)) {
        *child = extract_for_loop(tokens);
        return true;
    }

    if (tk_view_front(*tokens).type == TOKEN_SYMBOL && tk_view_at(*tokens, 1).type == TOKEN_OPEN_PAR) {
        return extract_function_call(child, tokens);
    }

    Tk_view assignment_tokens = tk_view_chop_on_type_delim_or_all(tokens, TOKEN_SEMICOLON);
    if (get_idx_token(NULL, assignment_tokens, 0, TOKEN_SINGLE_EQUAL)) {
        *child = extract_assignment(&assignment_tokens);
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        assert(assignment_tokens.count < 1);
        return true;
    }

    if (token_is_equal(tk_view_front(assignment_tokens), "let", TOKEN_SYMBOL)) {
        bool status = extract_variable_declaration(child, &assignment_tokens);
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        return status;
    }

    log_tokens(LOG_ERROR, *tokens, 0);
    unreachable("");
}

static Node* parse_block(Tk_view tokens) {
    Node* block = node_new();
    block->type = NODE_BLOCK;
    while (tokens.count > 0) {
        Node* child;
        if (!extract_statement(&child, &tokens)) {
            log_tokens(LOG_ERROR, tokens, 0);
            todo();
        }
        assert(!tk_view_try_consume(NULL, &tokens, TOKEN_SEMICOLON));
        nodes_append_child(block, child);
    }
    return block;
}

// give block only if there is a need
static Node* parse_single_item_or_block(Tk_view tokens) {
    Node* block = parse_block(tokens);
    if (nodes_count_children(block) < 1) {
        unreachable("");
    }
    if (nodes_count_children(block) == 1) {
        return block->left_child;
    }
    return block;
}

static Node* parse_single_statement(Tk_view tokens) {
    unreachable("");
}

static Node* parse_statement(Tk_view tokens) {
    todo();
}

static Node* parse_expression(Tk_view tokens) {
    if (tokens.count < 1) {
        unreachable("");
    }

    while (tk_view_front(tokens).type == TOKEN_OPEN_PAR && tk_view_at(tokens, tokens.count - 1).type == TOKEN_CLOSE_PAR) {
        try(tk_view_try_consume(NULL, &tokens, TOKEN_OPEN_PAR));
        Tk_view inner_tokens = tk_view_chop_count(&tokens, tokens.count - 1);
        try(tk_view_try_consume(NULL, &tokens, TOKEN_CLOSE_PAR));
        tokens = inner_tokens;
    }

    if (tokens.count == 1 && token_is_literal(tokens.tokens[0])) {
        return parse_literal(tokens);
    }

    if (tokens.count == 1 && tk_view_front(tokens).type == TOKEN_SYMBOL) {
        return parse_symbol(tokens);
    }

    if (token_is_equal(tk_view_front(tokens), "let", TOKEN_SYMBOL)) {
        Node* var_declaration;
        log_tokens(LOG_DEBUG, tokens, 0);
        if (extract_variable_declaration(&var_declaration, &tokens)) {
            log_tokens(LOG_DEBUG, tokens, 0);
            assert(tokens.count < 1);
            return var_declaration;
        }
    }

    if (count_operators(tokens) > 0) {
        return parse_operation(tokens);
    }

    Node* fun_call;
    if (extract_function_call(&fun_call, &tokens)) {
        assert(tokens.count < 1);
        return fun_call;
    }

    log_tokens(LOG_DEBUG, tokens, 0);
    todo();
}

/*
    Node* block_element;
    if (extract_block_element(&block_element, &tokens)) {
        assert(tokens.count < 1);
        return block_element;
    }

    log_tokens(LOG_DEBUG, tokens, 0);
    log_tree(LOG_VERBOSE, root_of_tree);
    unreachable("");
    */

Node* parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Node* root = parse_block(token_view);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    return root;
}
