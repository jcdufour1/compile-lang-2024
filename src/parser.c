#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include "error_msg.h"

static Node_block* extract_block(Tk_view* tokens);
INLINE bool extract_statement(Node** child, Tk_view* tokens);
static Node* extract_expression(Tk_view* tokens);
static bool try_extract_variable_declaration(Node_variable_def** result, Tk_view* tokens, bool require_let);
static Node_if_condition* extract_if_condition(Tk_view* tokens);

// consume tokens from { to } (inclusive) and discard outer {}
static Tk_view extract_items_inside_brackets(Tk_view* tokens, TOKEN_TYPE closing_bracket_type) {
    // the opening_bracket type should be the opening bracket type that corresponds to closing_brace_type
    switch (closing_bracket_type) {
        case TOKEN_CLOSE_CURLY_BRACE:
            try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
            break;
        case TOKEN_CLOSE_PAR:
            try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
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

// higher number returned from this function means that operator has higher precedence
static inline uint32_t operator_precedence(Token token) {
    switch (token.type) {
        case TOKEN_LESS_THAN:
            // fallthrough
        case TOKEN_GREATER_THAN:
            return 1;
        case TOKEN_SINGLE_PLUS:
            // fallthrough
        case TOKEN_SINGLE_MINUS:
            return 2;
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_SLASH:
            return 3;
        case TOKEN_DOUBLE_EQUAL:
            return 4;
        case TOKEN_NOT_EQUAL:
            return 4;
        default:
            unreachable(TOKEN_FMT, token_print(token));
    }
}

static bool is_unary(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        case TOKEN_DOUBLE_EQUAL:
            return false;
        case TOKEN_NOT_EQUAL:
            return false;
        case TOKEN_NOT:
            return true;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_NUM_LITERAL:
            return false;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_OPEN_PAR:
            return false;
        case TOKEN_CLOSE_PAR:
            return false;
        case TOKEN_OPEN_CURLY_BRACE:
            return false;
        case TOKEN_CLOSE_CURLY_BRACE:
            return false;
        case TOKEN_DOUBLE_QUOTE:
            return false;
        case TOKEN_SEMICOLON:
            return false;
        case TOKEN_COMMA:
            return false;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_SINGLE_DOT:
            return false;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_XOR:
            return false;
        case TOKEN_DEREF:
            return true;
    }
}

static bool extract_function_parameter(Node_variable_def** child, Tk_view* tokens) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return false;
    }

    Node_variable_def* param;
    try(try_extract_variable_declaration(&param, tokens, false));
    if (tk_view_try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        param->is_variadic = true;
    }
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);

    *child = param;
    return true;
}

static bool extract_function_parameters(Node_function_params** result, Tk_view* tokens) {
    Node_function_params* fun_params = node_unwrap_function_params(node_new(tk_view_front(*tokens).pos, NODE_FUNCTION_PARAMETERS));

    Node_variable_def* param;
    while (extract_function_parameter(&param, tokens)) {
        vec_append(&a_main, &fun_params->params, node_wrap(param));
    }

    if (tokens->count > 0) {
        try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
    }
    *result = fun_params;
    return true;
}

static bool extract_function_return_types(Node_function_return_types** result, Tk_view* tokens) {
    Node_function_return_types* return_types = node_unwrap_function_return_types(node_new(tk_view_front(*tokens).pos, NODE_FUNCTION_RETURN_TYPES));

    bool is_comma = true;
    while (is_comma) {
        // a return type is only one token, at least for now
        Token rtn_type_token = tk_view_consume(tokens);
        Node_lang_type* return_type = node_unwrap_lang_type(node_new(rtn_type_token.pos, NODE_LANG_TYPE));
        return_type->lang_type.str = rtn_type_token.text;
        if (tk_view_try_consume(NULL, tokens, TOKEN_ASTERISK)) {
            todo();
        }
        assert(return_type->lang_type.str.count > 0);
        return_types->child = return_type;
        is_comma = tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    }

    *result = return_types;
    return true;
}

static Node_function_declaration* extract_function_declaration_common(Tk_view* tokens) {
    Node_function_declaration* fun_declaration = node_unwrap_function_declaration(node_new(tk_view_front(*tokens).pos, NODE_FUNCTION_DECLARATION));

    Token name_token = tk_view_consume(tokens);
    fun_declaration->name = name_token.text;
    if (!sym_tbl_add(node_wrap(fun_declaration))) {
        msg_redefinition_of_symbol(node_wrap(fun_declaration));
        todo();
    }
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    if (!extract_function_parameters(&fun_declaration->parameters, tokens)) {
        todo();
    }
    if (!extract_function_return_types(&fun_declaration->return_types, tokens)) {
        todo();
    }

    return fun_declaration;
}

static Node_function_definition* parse_function_definition(Tk_view tokens) {
    tk_view_try_consume_symbol(NULL, &tokens, "fn");
    Node_function_declaration* fun_decl = extract_function_declaration_common(&tokens);
    try(tk_view_try_consume(NULL, &tokens, TOKEN_OPEN_CURLY_BRACE));
    Node_function_definition* function = node_unwrap_function_definition(node_new(node_wrap(fun_decl)->pos, NODE_FUNCTION_DEFINITION));
    function->declaration = fun_decl;
    sym_tbl_update(node_wrap(function));
    function->body = extract_block(&tokens);
    return function;
}

static Node_function_definition* extract_function_definition(Tk_view* tokens) {
    size_t close_curly_brace_idx;
    if (!get_idx_matching_token(&close_curly_brace_idx, *tokens, true, TOKEN_CLOSE_CURLY_BRACE)) {
        todo();
    }
    Tk_view function_tokens = tk_view_consume_count(tokens, close_curly_brace_idx);
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));

    return parse_function_definition(function_tokens);
}

static Node_struct_def* extract_struct_definition(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "struct")); // remove "struct"

    Token name = tk_view_consume(tokens);

    Node_struct_def* new_struct = node_unwrap_struct_def(node_new(name.pos, NODE_STRUCT_DEFINITION));
    new_struct->name = name.text;

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
    while (tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Node_variable_def* member;
        try(extract_function_parameter(&member, tokens));
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        vec_append(&a_main, &new_struct->members, node_wrap(member));
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    if (!sym_tbl_add(node_wrap(new_struct))) {
        msg_redefinition_of_symbol(node_wrap(new_struct));
    }
    return new_struct;
}

static bool try_extract_variable_declaration(Node_variable_def** result, Tk_view* tokens_input, bool require_let) {
    Tk_view tokens = *tokens_input;
    if (!tk_view_try_consume_symbol(NULL, &tokens, "let")) {
        assert(!require_let);
    }

    Token name_token;
    if (!tk_view_try_consume(&name_token, &tokens, TOKEN_SYMBOL)) {
        return false;
    }
    Node_variable_def* variable_def = node_unwrap_variable_def(node_new(name_token.pos, NODE_VARIABLE_DEFINITION));
    variable_def->name = name_token.text;
    if (!tk_view_try_consume(NULL, &tokens, TOKEN_COLON)) {
        return false;
    }
    variable_def->lang_type.str = tk_view_consume(&tokens).text;
    while (tk_view_try_consume(NULL, &tokens, TOKEN_ASTERISK)) {
        variable_def->lang_type.pointer_depth++;
    }
    if (!sym_tbl_add(node_wrap(variable_def))) {
        msg_redefinition_of_symbol(node_wrap(variable_def));
        todo();
    }

    if (tokens.count > 0 && !tk_view_try_consume(NULL, &tokens, TOKEN_SEMICOLON)) {
        Token front = tk_view_front(tokens);
        if (front.type == TOKEN_SYMBOL && str_view_cstr_is_equal(front.text, "if")) {
            todo();
            //nodes_append_child(node_wrap(variable_def), extract_expression(tokens));
        }
    }
    tk_view_try_consume(NULL, &tokens, TOKEN_SEMICOLON);

    *tokens_input = tokens;
    *result = variable_def;
    return true;
}

static void extract_for_range(Node_for_range* for_loop, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "in"));

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));

    Node* lower_bound_child = extract_expression(tokens);
    Node_for_lower_bound* lower_bound = node_unwrap_for_lower_bound(node_new(lower_bound_child->pos, NODE_FOR_LOWER_BOUND));
    lower_bound->child = lower_bound_child;
    for_loop->lower_bound = lower_bound;
    try(tk_view_try_consume(NULL, tokens, TOKEN_DOUBLE_DOT));

    Node* upper_bound_child = extract_expression(tokens);
    Node_for_upper_bound* upper_bound = node_unwrap_for_upper_bound(node_new(upper_bound_child->pos, NODE_FOR_UPPER_BOUND));
    upper_bound->child = upper_bound_child;
    for_loop->upper_bound = upper_bound;
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));

    Tk_view body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE); // TODO: remove this line?
    for_loop->body = extract_block(&body_tokens);
}

static Node_for_with_condition* extract_for_with_condition(Node_for_range* for_range, Tk_view* tokens) {
    Node_for_with_condition* for_with_cond = node_unwrap_for_with_condition(node_new(node_wrap(for_range)->pos, NODE_FOR_WITH_CONDITION));
    for_with_cond->condition = extract_if_condition(tokens);

    Tk_view body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE);
    for_with_cond->body = extract_block(&body_tokens);
    return for_with_cond;
}

static Node* extract_for_loop(Tk_view* tokens) {
    Token for_token;
    try(tk_view_try_consume_symbol(&for_token, tokens, "for"));
    Node_for_range* for_loop = node_unwrap_for_range(node_new(for_token.pos, NODE_FOR_RANGE));
    
    if (try_extract_variable_declaration(&for_loop->var_def, tokens, false)) {
        extract_for_range(for_loop, tokens);
        return node_wrap(for_loop);
    } else {
        return node_wrap(extract_for_with_condition(for_loop, tokens));
    }
}

static Node_break* extract_break(Tk_view* tokens) {
    Token break_token = tk_view_consume(tokens);
    Node_break* bk_statement = node_unwrap_break(node_new(break_token.pos, NODE_BREAK));
    return bk_statement;
}

static Node_function_declaration* extract_function_declaration(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "extern"));
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    Token extern_type_token;
    try(tk_view_try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL));
    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        todo();
    }
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
    try(tk_view_try_consume_symbol(NULL, tokens, "fn"));

    Node_function_declaration* fun_declaration = extract_function_declaration_common(tokens);

    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return fun_declaration;
}

static Node_literal* extract_literal(Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token_is_literal(token));

    Node_literal* new_node = literal_new(token.text, token.type, token.pos);
    assert(new_node->str_data.count > 0);

    try(sym_tbl_add(node_wrap(new_node)));
    return new_node;
}

static Node_symbol_untyped* extract_symbol(Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token.type == TOKEN_SYMBOL);

    Node_symbol_untyped* sym_node = node_unwrap_symbol_untyped(node_new(token.pos, NODE_SYMBOL_UNTYPED));
    sym_node->name = token.text;
    assert(node_wrap(sym_node)->pos.line > 0);
    return sym_node;
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

static bool extract_function_call(Node_function_call** child, Tk_view* tokens) {
    if (!tokens_start_with_function_call(*tokens)) {
        return false;
    }

    Token fun_name_token;
    try(tk_view_try_consume(&fun_name_token, tokens, TOKEN_SYMBOL));
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    Node_function_call* function_call = node_unwrap_function_call(node_new(fun_name_token.pos, NODE_FUNCTION_CALL));
    function_call->name = fun_name_token.text;

    Node* argument;
    while (extract_function_argument(&argument, tokens)) {
        vec_append(&a_main, &function_call->args, argument);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));

    *child = function_call;
    return true;
}

static Node_return_statement* extract_function_return_statement(Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "return"));

    Node* expression = extract_expression(tokens);
    Node_return_statement* new_node = node_unwrap_return_statement(node_new(expression->pos, NODE_RETURN_STATEMENT));
    new_node->child = expression;
    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return new_node;
}

static Node_assignment* extract_assignment(Tk_view* tokens, Node* lhs) {
    Token equal_token;
    Node_assignment* assignment = NULL;
    if (lhs) {
        assignment = node_unwrap_assignment(node_new(lhs->pos, NODE_ASSIGNMENT));
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
    } else {
        lhs = extract_expression(tokens);
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
        assignment = node_unwrap_assignment(node_new(equal_token.pos, NODE_ASSIGNMENT));
    }
    assignment->lhs = lhs;
    assignment->rhs = extract_expression(tokens);

    return assignment;
}

static Node_if_condition* extract_if_condition(Tk_view* tokens) {
    Node* operation = extract_expression(tokens);
    Node_if_condition* condition = node_unwrap_if_condition(node_new(operation->pos, NODE_IF_CONDITION));
    condition->child = operation;
    return condition;
}

static Node_if* extract_if_statement(Tk_view* tokens) {
    Token if_start_token;
    try(tk_view_try_consume_symbol(&if_start_token, tokens, "if"));
    Node_if* if_statement = node_unwrap_if(node_new(if_start_token.pos, NODE_IF_STATEMENT));

    if_statement->condition = extract_if_condition(tokens);
    Tk_view if_body_tokens = extract_items_inside_brackets(tokens, TOKEN_CLOSE_CURLY_BRACE);
    if_statement->body = extract_block(&if_body_tokens);

    return if_statement;
}

static bool extract_statement(Node** child, Tk_view* tokens) {
    Node* lhs;
    if (token_is_equal(tk_view_front(*tokens), "struct", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_struct_definition(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "extern", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_declaration(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "fn", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_definition(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "return", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_return_statement(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "if", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_if_statement(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "for", TOKEN_SYMBOL)) {
        lhs = extract_for_loop(tokens);
    } else if (token_is_equal(tk_view_front(*tokens), "break", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_break(tokens));
    } else {
        lhs = extract_expression(tokens);
    }

    // do assignment if applicible
    if (tokens->count > 0 && tk_view_front(*tokens).type == TOKEN_SINGLE_EQUAL) {
        *child = node_wrap(extract_assignment(tokens, lhs));
    } else {
        *child = lhs;
    }

    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return true;
}

static Node_block* extract_block(Tk_view* tokens) {
    Node_block* block = NULL;
    bool is_first = true;
    while (tokens->count > 0 && !tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        Node* child;
        if (!extract_statement(&child, tokens)) {
            todo();
        }
        if (is_first) {
            block = node_unwrap_block(node_new(child->pos, NODE_BLOCK));
        }
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        vec_append(&a_main, &block->children, child);
        is_first = false;
    }
    assert(block);
    return block;
}

static Node_struct_literal* extract_struct_literal(Tk_view* tokens) {
    Token start_token;
    try(tk_view_try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE));
    Node_struct_literal* struct_literal = node_unwrap_struct_literal(node_new(start_token.pos, NODE_STRUCT_LITERAL));
    struct_literal->name = literal_name_new();

    while (tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        vec_append(&a_main, &struct_literal->members, node_wrap(extract_assignment(tokens, NULL)));
        tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    try(sym_tbl_add(node_wrap(struct_literal)));
    return struct_literal;
}

static Node_struct_member_sym_untyped* extract_struct_member_call(Tk_view* tokens) {
    Token start_token = tk_view_consume(tokens);
    Node_struct_member_sym_untyped* member_call = node_unwrap_struct_member_sym_untyped(
        node_new(start_token.pos, NODE_STRUCT_MEMBER_SYM_UNTYPED)
    );
    member_call->name = start_token.text;
    while (tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        Token member_token = tk_view_consume(tokens);
        Node_struct_member_sym_piece_untyped* member = node_unwrap_struct_member_sym_piece_untyped(node_new(member_token.pos, NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED));
        member->name = member_token.text;
        vec_append(&a_main, &member_call->children, node_wrap(member));
    }
    return member_call;
}

static Node_unary* parser_unary_new(Token operation_token, Node* child) {
    Node_operator* operation = node_unwrap_operation(node_new(operation_token.pos, NODE_OPERATOR));
    operation->type = NODE_OP_UNARY;
    Node_unary* unary = node_unwrap_op_unary(operation);
    unary->token_type = operation_token.type;
    unary->child = child;
    return unary;
}

static Node_binary* parser_binary_new(Node* lhs, Token operation_token, Node* rhs) {
    Node_operator* operation = node_unwrap_operation(node_new(operation_token.pos, NODE_OPERATOR));
    operation->type = NODE_OP_BINARY;
    Node_binary* binary = node_unwrap_op_binary(operation);
    binary->token_type = operation_token.type;
    binary->lhs = lhs;
    binary->rhs = rhs;
    return binary;
}

static Node* extract_expression_piece(Tk_view* tokens) {
    Node_function_call* fun_call;
    if (tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        Node* expr_in_par = extract_expression(tokens);
        try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
        return expr_in_par;
    } else if (is_unary(tk_view_front(*tokens).type)) {
        Token operation_token = tk_view_consume(tokens);
        return node_wrap(parser_unary_new(operation_token, extract_expression_piece(tokens)));
    } else if (token_is_equal(tk_view_front(*tokens), "let", TOKEN_SYMBOL)) {
        Node_variable_def* result;
        try(try_extract_variable_declaration(&result, tokens, true));
        return node_wrap(result);
    } else if (extract_function_call(&fun_call, tokens)) {
        return node_wrap(fun_call);
    } else if (tokens->count > 1 && tk_view_front(*tokens).type == TOKEN_SYMBOL &&
        token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)
    ) {
        return node_wrap(extract_struct_member_call(tokens));
    } else if (token_is_literal(tk_view_front(*tokens))) {
        return node_wrap(extract_literal(tokens));
    } else if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        return node_wrap(extract_symbol(tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "", TOKEN_OPEN_CURLY_BRACE) && token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)) {
        return node_wrap(extract_struct_literal(tokens));
    } else {
        log_tokens(LOG_ERROR, *tokens);
        todo();
    }
}

static Node* extract_expression(Tk_view* tokens) {
    assert(tokens->tokens);
    if (tokens->count < 1) {
        unreachable("");
    }

    Node* expression = extract_expression_piece(tokens);
    Token prev_operator_token = {0};
    bool is_first_operator = true;
    while (tokens->count > 0 && token_is_operator(tk_view_front(*tokens)) && !is_unary(tk_view_front(*tokens).type)) {
        Token operator_token = tk_view_consume(tokens);
        if (is_unary(operator_token.type)) {
            unreachable("");
        } else if (!is_first_operator && node_is_binary(expression) &&
            operator_precedence(prev_operator_token) < operator_precedence(operator_token)
        ) {
            Node* rhs = extract_expression_piece(tokens);
            Node_binary* operation = parser_binary_new(
                node_auto_unwrap_op_binary(expression)->rhs,
                operator_token,
                rhs
            );
            node_auto_unwrap_op_binary(expression)->rhs = node_wrap(operation);
        } else {
            Node* lhs = expression;
            Node* rhs = extract_expression_piece(tokens);
            Node_binary* operation = parser_binary_new(lhs, operator_token, rhs);
            expression = node_wrap(operation);
        }

        is_first_operator = false;
        prev_operator_token = operator_token;
    }

    return expression;
}

Node_block* parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Node_block* root = extract_block(&token_view);
    //log(LOG_VERBOSE, "completed parse tree:\n");
    //log_tree(LOG_VERBOSE, root);
    return root;
}
