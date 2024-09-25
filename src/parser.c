#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include "error_msg.h"

static Node* parse_function_single_return_type(Token tokens);
static Node* extract_block(Tk_view* tokens);
INLINE bool extract_statement(Node** child, Tk_view* tokens);
static Node* extract_expression(Tk_view* tokens);
static Node* extract_variable_declaration(Tk_view* tokens, bool require_let);

// consume tokens from { to } (inclusive) and discard outer {}
static Tk_view extract_items_inside_brackets(Tk_view* tokens, TOKEN_TYPE closing_bracket_type) {
    Token opening_bracket = tk_view_consume(tokens);
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
    Tk_view inside_brackets = tk_view_consume_count(tokens, idx_closing_bracket);

    try(tk_view_try_consume(NULL, tokens, closing_bracket_type));
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

    return true;
}

static size_t get_idx_lowest_precedence_operator(Tk_view tokens) {
    size_t idx_lowest = SIZE_MAX;
    uint32_t lowest_pre = UINT32_MAX; // higher numbers have higher precedence
    int16_t par_level = 1;

    for (size_t idx_ = tokens.count; idx_ > 0; idx_--) {
        size_t actual_idx = idx_ - 1;
        Token curr_token = tk_view_at(tokens, actual_idx);

        if (curr_token.type == TOKEN_OPEN_PAR) {
            par_level--;
            continue;
        }
        if (curr_token.type == TOKEN_CLOSE_PAR) {
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

static bool extract_function_parameter(Node** child, Tk_view* tokens) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return false;
    }

    Node* param = extract_variable_declaration(tokens, false);
    if (tk_view_try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        param->is_variadic = true;
    }
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);

    *child = param;
    return true;
}

static bool extract_function_parameters(Node** result, Tk_view* tokens) {
    Node* fun_params = node_new(tk_view_front(*tokens).file_path, tk_view_front(*tokens).line_num);
    fun_params->type = NODE_FUNCTION_PARAMETERS;

    Node* param;
    while (extract_function_parameter(&param, tokens)) {
        nodes_append_child(fun_params, param);
    }

    if (tokens->count > 0) {
        try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
    }
    *result = fun_params;
    return true;
}

static bool extract_function_return_types(Node** result, Tk_view* tokens) {
    Node* return_types = node_new(tk_view_front(*tokens).file_path, tk_view_front(*tokens).line_num);
    return_types->type = NODE_FUNCTION_RETURN_TYPES;

    bool is_comma = true;
    while (is_comma) {
        // a return type is only one token, at least for now
        Token rtn_type_token = tk_view_consume(tokens);
        Node* return_type = node_new(rtn_type_token.file_path, rtn_type_token.line_num);
        return_type->type = NODE_LANG_TYPE;
        return_type->lang_type = rtn_type_token.text;
        assert(return_type->lang_type.count > 0);
        nodes_append_child(return_types, return_type);
        is_comma = tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    }

    *result = return_types;
    return true;
}

static Node* extract_function_declaration_common(Tk_view* tokens) {
    Node* fun_declaration = node_new(tk_view_front(*tokens).file_path, tk_view_front(*tokens).line_num);

    Token name_token = tk_view_consume(tokens);
    fun_declaration->name = name_token.text;
    fun_declaration->line_num = name_token.line_num;
    fun_declaration->file_path = name_token.file_path;
    assert(fun_declaration->name.count > 0);

    if (!sym_tbl_add(fun_declaration)) {
        msg_redefinition_of_symbol(fun_declaration);
        todo();
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));

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
    tk_view_try_consume_symbol(NULL, &tokens, "fn");
    Node* function = extract_function_declaration_common(&tokens);
    try(tk_view_try_consume(NULL, &tokens, TOKEN_OPEN_CURLY_BRACE));
    function->type = NODE_FUNCTION_DEFINITION;
    nodes_append_child(function, extract_block(&tokens));

    return function;
}

static Node* extract_function_definition(Tk_view* tokens) {
    size_t close_curly_brace_idx;
    if (!get_idx_matching_token(&close_curly_brace_idx, *tokens, true, TOKEN_CLOSE_CURLY_BRACE)) {
        todo();
    }
    Tk_view function_tokens = tk_view_consume_count(tokens, close_curly_brace_idx);
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));

    return parse_function_definition(function_tokens);
}

static Node* extract_struct_definition(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "struct")); // remove "struct"

    Token name = tk_view_consume(tokens);

    Node* new_struct = node_new(name.file_path, name.line_num);
    new_struct->name = name.text;
    new_struct->type = NODE_STRUCT_DEFINITION;
    new_struct->line_num = name.line_num;
    new_struct->file_path = name.file_path;

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
    while (tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Node* member;
        try(extract_function_parameter(&member, tokens));
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        nodes_append_child(new_struct, member);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    if (!sym_tbl_add(new_struct)) {
        msg_redefinition_of_symbol(new_struct);
        todo();
    }
    return new_struct;
}

static Node* extract_variable_declaration(Tk_view* tokens, bool require_let) {
    if (!tk_view_try_consume_symbol(NULL, tokens, "let")) {
        assert(!require_let);
    }

    Token name_token = tk_view_consume(tokens);
    Node* variable_def = node_new(name_token.file_path, name_token.line_num);
    variable_def->type = NODE_VARIABLE_DEFINITION;
    variable_def->name = name_token.text;
    variable_def->line_num = name_token.line_num;
    variable_def->file_path = name_token.file_path;
    try(tk_view_try_consume(NULL, tokens, TOKEN_COLON));
    variable_def->lang_type = tk_view_consume(tokens).text;
    while (tk_view_try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        variable_def->pointer_depth++;
    }
    if (!sym_tbl_add(variable_def)) {
        msg_redefinition_of_symbol(variable_def);
        todo();
    }

    if (tokens->count > 0 && !tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON)) {
        Token front = tk_view_front(*tokens);
        if (front.type == TOKEN_SYMBOL && str_view_cstr_is_equal(front.text, "if")) {
            nodes_append_child(variable_def, extract_expression(tokens));
        }
    }
    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return variable_def;
}

static bool is_not_symbol_in(const Token* prev, const Token* curr) {
    (void) prev;
    if (curr->type != TOKEN_SYMBOL) {
        return true;
    }
    return !str_view_cstr_is_equal(curr->text, "in");
}

static Node* extract_for_loop(Tk_view* tokens) {
    Token for_token;
    try(tk_view_try_consume_symbol(&for_token, tokens, "for"));
    Node* for_loop = node_new(for_token.file_path, for_token.line_num);
    for_loop->type = NODE_FOR_LOOP;

    Node* var_def_child = extract_variable_declaration(tokens, false);
    Node* var_def = node_new(var_def_child->file_path, var_def_child->line_num);
    var_def->type = NODE_FOR_VARIABLE_DEF;
    nodes_append_child(var_def, var_def_child);
    nodes_append_child(for_loop, var_def);
    try(tk_view_try_consume_symbol(NULL, tokens, "in"));

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));

    Node* lower_bound_child = extract_expression(tokens);
    Node* lower_bound = node_new(lower_bound_child->file_path, lower_bound_child->line_num);
    lower_bound->type = NODE_FOR_LOWER_BOUND;
    nodes_append_child(lower_bound, lower_bound_child);
    nodes_append_child(for_loop, lower_bound);
    try(tk_view_try_consume(NULL, tokens, TOKEN_DOUBLE_DOT));

    Node* upper_bound_child = extract_expression(tokens);
    Node* upper_bound = node_new(upper_bound_child->file_path, upper_bound_child->line_num);
    upper_bound->type = NODE_FOR_UPPER_BOUND;
    nodes_append_child(upper_bound, upper_bound_child);
    nodes_append_child(for_loop, upper_bound);
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));

    Tk_view body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE); // TODO: remove this line?
    nodes_append_child(for_loop, extract_block(&body_tokens));

    return for_loop;
}

static Node* extract_function_declaration(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "extern"));
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    Token extern_type_token;
    try(tk_view_try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL));
    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        todo();
    }
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
    try(tk_view_try_consume_symbol(NULL, tokens, "fn"));

    Node* fun_declaration = extract_function_declaration_common(tokens);
    fun_declaration->type = NODE_FUNCTION_DECLARATION;

    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return fun_declaration;
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

static Node* extract_literal(Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token_is_literal(token));

    Node* new_node = node_new(token.file_path, token.line_num);
    new_node->type = NODE_LITERAL;
    new_node->name = literal_name_new();
    assert(new_node->str_data.count < 1);
    new_node->str_data = token.text;
    new_node->token_type = token.type;
    new_node->line_num = token.line_num;
    new_node->lang_type = get_literal_lang_type_from_token_type(new_node->token_type);

    try(sym_tbl_add(new_node));
    return new_node;
}

static Node* extract_symbol(Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token.type == TOKEN_SYMBOL);

    Node* sym_node = node_new(token.file_path, token.line_num);
    sym_node->type = NODE_SYMBOL;
    sym_node->name = token.text;
    sym_node->line_num = token.line_num;
    assert(sym_node->line_num > 0);
    return sym_node;
}

typedef enum {
    PAR_OPERAND,
    PAR_OPERATOR,
    PAR_PAR,
} PAR_STATUS;

static bool try_extract_operation_tokens(Tk_view* operation, Tk_view* tokens) {
    bool is_operation = false;
    size_t idx = 0;
    PAR_STATUS par_status = PAR_PAR;
    for (idx = 0; idx < tokens->count; idx++) {
        TOKEN_TYPE prev_token_type = idx > 0 ? tk_view_at(*tokens, idx - 1).type : TOKEN_NONTYPE;
        Token curr_token = tk_view_at(*tokens, idx);
        if (token_is_operator(curr_token)) {
            assert(par_status != PAR_OPERATOR);
            par_status = PAR_OPERATOR;
            is_operation = true;
            continue;
        }
        switch (curr_token.type) {
            case TOKEN_NUM_LITERAL:
                // fallthrough
            case TOKEN_STRING_LITERAL:
                // fallthrough
            case TOKEN_SYMBOL:
                if (prev_token_type == TOKEN_CLOSE_PAR || par_status == PAR_OPERAND) {
                    goto after_for_extract_operation_tokens;
                }
                par_status = PAR_OPERAND;
                continue;
            case TOKEN_OPEN_PAR:
                par_status = PAR_PAR;
                continue;
            case TOKEN_CLOSE_PAR:
                par_status = PAR_PAR;
                continue;
            case TOKEN_OPEN_CURLY_BRACE:
                // fallthrough
            case TOKEN_SEMICOLON:
                // fallthrough
            case TOKEN_SINGLE_DOT:
                // fallthrough
            case TOKEN_COLON:
                // fallthrough
            case TOKEN_CLOSE_CURLY_BRACE:
                // fallthrough
            case TOKEN_DOUBLE_DOT:
                // fallthrough
            case TOKEN_SINGLE_EQUAL:
                // fallthrough
            case TOKEN_COMMA:
                assert(par_status != PAR_OPERATOR);
                goto after_for_extract_operation_tokens;
            default:
                unreachable(TOKEN_FMT"\n", token_print(curr_token));
        }
    }

after_for_extract_operation_tokens:
    if (!is_operation) {
        return false;
    }
    *operation = tk_view_consume_count(tokens, idx);
    return true;
}

static Node* parse_operation(Tk_view tokens) {
    while (tk_view_front(tokens).type == TOKEN_OPEN_PAR) {
        Tk_view temp = tokens;
        Tk_view inner_tokens = extract_items_inside_brackets(&temp, TOKEN_CLOSE_PAR);
        if (inner_tokens.count + 2 != tokens.count) {
            // this means that () group at beginning of expression does not wrap the entirty of tokens
            break;
        }
        tokens = inner_tokens;
    }

    size_t idx_operator = get_idx_lowest_precedence_operator(tokens);
    Tk_view left_tokens = tk_view_consume_count(&tokens, idx_operator);
    Token operator_token = tk_view_consume(&tokens);
    Tk_view right_tokens = tk_view_consume_count(&tokens, tokens.count);

    assert(left_tokens.count > 0);
    assert(right_tokens.count > 0);

    Node* operator_node = node_new(operator_token.file_path, operator_token.line_num);
    operator_node->type = NODE_OPERATOR;
    operator_node->token_type = operator_token.type;
    operator_node->line_num = operator_token.line_num;

    Node* left_node = extract_expression(&left_tokens);
    Node* right_node = extract_expression(&right_tokens);
    nodes_append_child(operator_node, left_node);
    nodes_append_child(operator_node, right_node);
    return operator_node;
}

INLINE bool try_extract_operation(Node** operation, Tk_view* tokens) {
    Tk_view operation_tokens;
    if (!try_extract_operation_tokens(&operation_tokens, tokens)) {
        return false;
    }
    *operation = parse_operation(operation_tokens);
    return true;
}

// returns true if the parse was successful
static bool extract_function_argument(Node** child, Tk_view* tokens) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return false;
    }

    assert(tk_view_front(*tokens).type != TOKEN_COMMA);
    Tk_view curr_arg_tokens = tk_view_consume_until_matching_type_delims_or_all(
        tokens,
        TOKEN_COMMA,
        TOKEN_CLOSE_PAR,
        false
    );
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    *child = extract_expression(&curr_arg_tokens);
    return true;
}

static bool extract_function_call(Node** child, Tk_view* tokens) {
    if (!tokens_start_with_function_call(*tokens)) {
        return false;
    }

    Token fun_name_token;
    try(tk_view_try_consume(&fun_name_token, tokens, TOKEN_SYMBOL));
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    Node* function_call = node_new(fun_name_token.file_path, fun_name_token.line_num);
    function_call->type = NODE_FUNCTION_CALL;
    function_call->name = fun_name_token.text;

    Node* argument;
    while (extract_function_argument(&argument, tokens)) {
        nodes_append_child(function_call, argument);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));

    *child = function_call;
    return true;
}

static Node* extract_function_return_statement(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "return"));

    Node* expression = extract_expression(tokens);
    Node* new_node = node_new(expression->file_path, expression->line_num);
    new_node->type = NODE_RETURN_STATEMENT;
    nodes_append_child(new_node, expression);
    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return new_node;
}

static Node* extract_assignment(Tk_view* tokens, Node* lhs) {
    Token equal_token;
    Node* assignment = NULL;
    if (lhs) {
        assignment = node_new(lhs->file_path, lhs->line_num);
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
    } else {
        lhs = extract_expression(tokens);
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
        assignment = node_new(equal_token.file_path, equal_token.line_num);
    }
    assignment->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, extract_expression(tokens)); // rhs

    return assignment;
}

static bool is_if_statement(Tk_view tokens) {
    if (tk_view_front(tokens).type == TOKEN_SYMBOL && str_view_cstr_is_equal(tk_view_front(tokens).text, "if")) {
        return true;
    }
    return false;
}

static Node* extract_if_condition(Tk_view* tokens) {

    Node* operation;
    try(try_extract_operation(&operation, tokens))
    Node* condition = node_new(operation->file_path, operation->line_num);
    condition->type = NODE_IF_CONDITION;
    nodes_append_child(condition, operation);
    return condition;
}

static Node* extract_if_statement(Tk_view* tokens) {
    Token if_start_token;
    try(tk_view_try_consume_symbol(&if_start_token, tokens, "if"));
    Node* if_statement = node_new(if_start_token.file_path, if_start_token.line_num);
    if_statement->type = NODE_IF_STATEMENT;

    nodes_append_child(if_statement, extract_if_condition(tokens));
    Tk_view if_body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE);
    nodes_append_child(if_statement, extract_block(&if_body_tokens));

    return if_statement;
}

static bool extract_statement(Node** child, Tk_view* tokens) {
    if (token_is_equal(tk_view_front(*tokens), "struct", TOKEN_SYMBOL)) {
        *child = extract_struct_definition(tokens);
        return true;
    }

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
        *child = extract_if_statement(tokens);
        return true;
    }

    if (token_is_equal(tk_view_front(*tokens), "for", TOKEN_SYMBOL)) {
        *child = extract_for_loop(tokens);
        return true;
    }

    if (tk_view_front(*tokens).type == TOKEN_SYMBOL && tk_view_at(*tokens, 1).type == TOKEN_OPEN_PAR) {
        return extract_function_call(child, tokens);
    }

    Node* var_decl = NULL;
    if (token_is_equal(tk_view_front(*tokens), "let", TOKEN_SYMBOL)) {
        var_decl = extract_expression(tokens);
        if (tk_view_front(*tokens).type == TOKEN_SINGLE_EQUAL) {
            *child = extract_assignment(tokens, var_decl);
            tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
            return true;
        }
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        *child = var_decl;
        return true;
    }
    *child = extract_assignment(tokens, NULL);
    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return true;

    log_tokens(LOG_ERROR, *tokens);
    unreachable("");
}

static Node* extract_block(Tk_view* tokens) {
    Node* block = NULL;
    bool is_first = true;
    while (tokens->count > 0 && !tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        Node* child;
        if (!extract_statement(&child, tokens)) {
            todo();
        }
        if (is_first) {
            block = node_new(child->file_path, child->line_num);
            block->type = NODE_BLOCK;
        }
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        nodes_append_child(block, child);
        is_first = false;
    }
    return block;
}

static Node* extract_struct_literal(Tk_view* tokens) {
    Token start_token;
    try(tk_view_try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE));
    Node* struct_literal = node_new(start_token.file_path, start_token.line_num);
    struct_literal->type = NODE_STRUCT_LITERAL;
    struct_literal->name = literal_name_new();

    while (tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        nodes_append_child(struct_literal, extract_assignment(tokens, NULL));
        tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    try(sym_tbl_add(struct_literal));
    return struct_literal;
}

static Node* extract_struct_member_call(Tk_view* tokens) {
    Token start_token = tk_view_consume(tokens);
    Node* member_call = node_new(start_token.file_path, start_token.line_num);
    member_call->type = NODE_STRUCT_MEMBER_SYM;
    member_call->name = start_token.text;
    while (tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        Token member_token = tk_view_consume(tokens);
        Node* member = symbol_new(member_token.text, member_token.file_path, member_token.line_num);
        nodes_append_child(member_call, member);
    }
    return member_call;
}

static Node* extract_expression(Tk_view* tokens) {
    assert(tokens->tokens);
    if (tokens->count < 1) {
        unreachable("");
    }

    assert(tokens->count > 0);
    while (tk_view_front(*tokens).type == TOKEN_OPEN_PAR) {
        Tk_view temp = *tokens;
        Tk_view inner_tokens = extract_items_inside_brackets(&temp, TOKEN_CLOSE_PAR);
        if (inner_tokens.count + 2 != tokens->count) {
            // this means that () group at beginning of expression does not wrap the entirty of tokens
            break;
        }
        *tokens = inner_tokens;
    }

    Node* operation;
    if (try_extract_operation(&operation, tokens)) {
        return operation;
    }

    if (token_is_equal(tk_view_front(*tokens), "let", TOKEN_SYMBOL)) {
        return extract_variable_declaration(tokens, true);
    }

    Node* fun_call;
    if (extract_function_call(&fun_call, tokens)) {
        return fun_call;
    }

    if (tokens->count > 1 && tk_view_front(*tokens).type == TOKEN_SYMBOL &&
        token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)
    ) {
        return extract_struct_member_call(tokens);
    }

    if (token_is_literal(tk_view_front(*tokens))) {
        return extract_literal(tokens);
    }

    if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        return extract_symbol(tokens);
    }

    if (token_is_equal(tk_view_front(*tokens), "", TOKEN_OPEN_CURLY_BRACE) &&
        token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)
    ) {
        return extract_struct_literal(tokens);
    }

    log_tokens(LOG_ERROR, *tokens);
    todo();
}

Node* parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Node* root = extract_block(&token_view);
    log(LOG_VERBOSE, "completed parse tree:\n");
    log_tree(LOG_VERBOSE, root);
    return root;
}
