#include "util.h"
#include "parser.h"
#include "node.h"
#include "nodes.h"
#include "assert.h"
#include "token_view.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include "error_msg.h"

static Node_block* extract_block(Env* env, Tk_view* tokens);
INLINE bool extract_statement(Env* env, Node** child, Tk_view* tokens);
static bool try_extract_expression(Env* env, Node** result, Tk_view* tokens, bool defer_sym_add);
static bool try_extract_variable_declaration(
    Env* env, 
    Node_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool defer_sym_add // if true, symbol will not be added to symbol table 
                       // until the next extract_block call
);
static Node_if_condition* extract_if_condition(Env* env, Tk_view* tokens);

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
        case TOKEN_REFER:
            return true;
        case TOKEN_VOID:
            return false;
    }
}

static bool extract_function_parameter(Env* env, Node_variable_def** child, Tk_view* tokens) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return false;
    }

    Node_variable_def* param;
    try(try_extract_variable_declaration(env, &param, tokens, false, true));
    if (tk_view_try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        param->is_variadic = true;
    }
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);

    *child = param;
    return true;
}

static bool extract_function_parameters(Env* env, Node_function_params** result, Tk_view* tokens) {
    Node_function_params* fun_params = node_unwrap_function_params(node_new(tk_view_front(*tokens).pos, NODE_FUNCTION_PARAMETERS));

    Node_variable_def* param;
    while (extract_function_parameter(env, &param, tokens)) {
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
        Token rtn_type_token;
        Node_lang_type* return_type = node_unwrap_lang_type(node_new(rtn_type_token.pos, NODE_LANG_TYPE));
        if (!tk_view_try_consume(&rtn_type_token, tokens, TOKEN_SYMBOL)) {
            return_type->lang_type.str = str_view_from_cstr("void");
            return_types->child = return_type;
            break;
        }
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

static Node_function_declaration* extract_function_declaration_common(Env* env, Tk_view* tokens) {
    Node_function_declaration* fun_declaration = node_unwrap_function_declaration(node_new(tk_view_front(*tokens).pos, NODE_FUNCTION_DECLARATION));

    Token name_token = tk_view_consume(tokens);
    fun_declaration->name = name_token.text;
    if (!symbol_add(env, node_wrap(fun_declaration))) {
        msg_redefinition_of_symbol(env, node_wrap(fun_declaration));
        todo();
    }
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    if (!extract_function_parameters(env, &fun_declaration->parameters, tokens)) {
        todo();
    }
    if (!extract_function_return_types(&fun_declaration->return_types, tokens)) {
        todo();
    }

    return fun_declaration;
}

static Node_function_definition* extract_function_definition(Env* env, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "fn"));
    Node_function_declaration* fun_decl = extract_function_declaration_common(env, tokens);
    Node_function_definition* function = node_unwrap_function_definition(
        node_new(node_wrap(fun_decl)->pos, NODE_FUNCTION_DEFINITION)
    );
    function->declaration = fun_decl;
    function->body = extract_block(env, tokens);
    return function;
}

static Node_struct_def* extract_struct_definition(Env* env, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "struct")); // remove "struct"

    Token name = tk_view_consume(tokens);

    Node_struct_def* new_struct = node_unwrap_struct_def(node_new(name.pos, NODE_STRUCT_DEFINITION));
    new_struct->name = name.text;

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
    while (tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Node_variable_def* member;
        try(extract_function_parameter(env, &member, tokens));
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        vec_append(&a_main, &new_struct->members, node_wrap(member));
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    if (!symbol_add(env, node_wrap(new_struct))) {
        msg_redefinition_of_symbol(env, node_wrap(new_struct));
    }
    return new_struct;
}

static bool try_extract_variable_declaration(
    Env* env,
    Node_variable_def** result,
    Tk_view* tokens_input,
    bool require_let,
    bool defer_sym_add
) {
    (void) require_let;
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
    Node* dummy;
    if (defer_sym_add) {
        symbol_add_defer(env, node_wrap(variable_def));
    } else {
        if (symbol_lookup(&dummy, env, get_node_name(node_wrap(variable_def)))) {
            msg_redefinition_of_symbol(env, node_wrap(variable_def));
            todo();
        }
        symbol_add(env, node_wrap(variable_def));
    }

    tk_view_try_consume(NULL, &tokens, TOKEN_SEMICOLON);

    *tokens_input = tokens;
    *result = variable_def;
    return true;
}

static void extract_for_range(Env* env, Node_for_range* for_loop, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "in"));

    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));

    Node* lower_bound_child;
    try(try_extract_expression(env, &lower_bound_child, tokens, true));
    Node_for_lower_bound* lower_bound = node_unwrap_for_lower_bound(node_new(lower_bound_child->pos, NODE_FOR_LOWER_BOUND));
    lower_bound->child = lower_bound_child;
    for_loop->lower_bound = lower_bound;
    try(tk_view_try_consume(NULL, tokens, TOKEN_DOUBLE_DOT));

    Node* upper_bound_child;
    if (!try_extract_expression(env, &upper_bound_child, tokens, true)) {
        todo();
        msg(LOG_ERROR, EXPECT_FAIL_EXPECTED_EXPRESSION, tk_view_front(*tokens).pos, "expected expression\n");
        return;
    }
    Node_for_upper_bound* upper_bound = node_unwrap_for_upper_bound(node_new(upper_bound_child->pos, NODE_FOR_UPPER_BOUND));
    upper_bound->child = upper_bound_child;
    for_loop->upper_bound = upper_bound;
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));

    for_loop->body = extract_block(env, tokens);
}

static Node_for_with_condition* extract_for_with_condition(Env* env, Node_for_range* for_range, Tk_view* tokens) {
    Node_for_with_condition* for_with_cond = node_unwrap_for_with_condition(node_new(node_wrap(for_range)->pos, NODE_FOR_WITH_CONDITION));
    for_with_cond->condition = extract_if_condition(env, tokens);

    for_with_cond->body = extract_block(env, tokens);
    return for_with_cond;
}

static Node* extract_for_loop(Env* env, Tk_view* tokens) {
    Token for_token;
    try(tk_view_try_consume_symbol(&for_token, tokens, "for"));
    Node_for_range* for_loop = node_unwrap_for_range(node_new(for_token.pos, NODE_FOR_RANGE));
    
    if (try_extract_variable_declaration(env, &for_loop->var_def, tokens, false, true)) {
        extract_for_range(env, for_loop, tokens);
        return node_wrap(for_loop);
    } else {
        return node_wrap(extract_for_with_condition(env, for_loop, tokens));
    }
}

static Node_break* extract_break(Tk_view* tokens) {
    Token break_token = tk_view_consume(tokens);
    Node_break* bk_statement = node_unwrap_break(node_new(break_token.pos, NODE_BREAK));
    return bk_statement;
}

static Node_function_declaration* extract_function_declaration(Env* env, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "extern"));
    try(tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR));
    Token extern_type_token;
    try(tk_view_try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL));
    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        todo();
    }
    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
    try(tk_view_try_consume_symbol(NULL, tokens, "fn"));
    Node_function_declaration* fun_declaration = extract_function_declaration_common(env, tokens);

    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    symbol_ignore_defered(env);
    return fun_declaration;
}

static Node_literal* extract_literal(Env* env, Tk_view* tokens, bool defer_sym_add) {
    Token token = tk_view_consume(tokens);
    assert(token_is_literal(token));

    Node_literal* new_node = literal_new(token.text, token.type, token.pos);

    if (defer_sym_add) {
        symbol_add_defer(env, node_wrap(new_node));
    } else {
        try(symbol_add(env, node_wrap(new_node)));
    }
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

static bool extract_function_argument(Env* env, Node** child, Tk_view* tokens) {
    if (!try_extract_expression(env, child, tokens, true)) {
        return false;
    }
    tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    return true;
}

static bool try_extract_function_call(Env* env, Node_function_call** child, Tk_view* tokens) {
    Tk_view curr_tokens = *tokens;
    Token fun_name_token;
    if (!tk_view_try_consume(&fun_name_token, &curr_tokens, TOKEN_SYMBOL)) {
        return false;
    }
    if (!tk_view_try_consume(NULL, &curr_tokens, TOKEN_OPEN_PAR)) {
        return false;
    }
    Node_function_call* function_call = node_unwrap_function_call(
        node_new(fun_name_token.pos, NODE_FUNCTION_CALL)
    );
    function_call->name = fun_name_token.text;

    Node* argument;
    while (extract_function_argument(env, &argument, &curr_tokens)) {
        vec_append(&a_main, &function_call->args, argument);
    }

    if (!tk_view_try_consume(NULL, &curr_tokens, TOKEN_CLOSE_PAR)) {
        //msg_parser_expected(env, &tk_view_front(*curr_tokens), TOKEN_CLOSE_PAR, TOKEN_OPEN_CURLY_BRACE);
        msg_parser_expected(tk_view_front(curr_tokens), TOKEN_CLOSE_PAR);
        exit(EXIT_CODE_FAIL);
    }

    *child = function_call;
    *tokens = curr_tokens;
    return true;
}

static Node_return_statement* extract_function_return_statement(Env* env, Tk_view* tokens) {
    try(tk_view_try_consume_symbol(NULL, tokens, "return"));

    Node* expression;
    Node_return_statement* new_node;
    if (try_extract_expression(env, &expression, tokens, false)) {
        new_node = node_unwrap_return_statement(node_new(expression->pos, NODE_RETURN_STATEMENT));
        new_node->child = expression;
    } else {
        new_node = node_unwrap_return_statement(node_new(prev_pos, NODE_RETURN_STATEMENT));
        new_node->child = node_wrap(literal_new(str_view_from_cstr(""), TOKEN_VOID, prev_pos));
    }
    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return new_node;
}

static Node_assignment* extract_assignment(Env* env, Tk_view* tokens, Node* lhs) {
    Token equal_token;
    Node_assignment* assignment = NULL;
    if (lhs) {
        assignment = node_unwrap_assignment(node_new(lhs->pos, NODE_ASSIGNMENT));
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
    } else {
        try_extract_expression(env, &lhs, tokens, false);
        try(tk_view_try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL));
        assignment = node_unwrap_assignment(node_new(equal_token.pos, NODE_ASSIGNMENT));
    }
    assignment->lhs = lhs;
    try(try_extract_expression(env, &assignment->rhs, tokens, false));

    return assignment;
}

static Node_if_condition* extract_if_condition(Env* env, Tk_view* tokens) {
    Node* operation;
    try(try_extract_expression(env, &operation, tokens, false));
    Node_if_condition* condition = node_unwrap_if_condition(node_new(operation->pos, NODE_IF_CONDITION));
    condition->child = operation;
    return condition;
}

static Node_if* extract_if_statement(Env* env, Tk_view* tokens) {
    Token if_start_token;
    try(tk_view_try_consume_symbol(&if_start_token, tokens, "if"));
    Node_if* if_statement = node_unwrap_if(node_new(if_start_token.pos, NODE_IF_STATEMENT));

    if_statement->condition = extract_if_condition(env, tokens);
    if_statement->body = extract_block(env, tokens);

    return if_statement;
}

static bool extract_statement(Env* env, Node** child, Tk_view* tokens) {
    Node* lhs;
    if (token_is_equal(tk_view_front(*tokens), "struct", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_struct_definition(env, tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "extern", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_declaration(env, tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "fn", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_definition(env, tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "return", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_function_return_statement(env, tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "if", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_if_statement(env, tokens));
    } else if (token_is_equal(tk_view_front(*tokens), "for", TOKEN_SYMBOL)) {
        lhs = extract_for_loop(env, tokens);
    } else if (token_is_equal(tk_view_front(*tokens), "break", TOKEN_SYMBOL)) {
        lhs = node_wrap(extract_break(tokens));
    } else {
        if (!try_extract_expression(env, &lhs, tokens, false)) {
            todo();
            /*
            if (tokens->count < 1) {
                todo();
            }
            msg(LOG_ERROR, tk_view_front(*tokens).pos, "invalid or missing expression\n");
            exit(EXIT_CODE_FAIL);
            */
        }
    }

    // do assignment if applicible
    if (tokens->count > 0 && tk_view_front(*tokens).type == TOKEN_SINGLE_EQUAL) {
        *child = node_wrap(extract_assignment(env, tokens, lhs));
    } else {
        *child = lhs;
    }

    tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return true;
}

static Node_block* extract_block(Env* env, Tk_view* tokens) {
    tk_view_try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE);
    Node_block* block = node_unwrap_block(node_new(tk_view_front(*tokens).pos, NODE_BLOCK));
    vec_append(&a_main, &env->ancesters, node_wrap(block));
    Node* redefined_symbol;
    if (!symbol_do_add_defered(&redefined_symbol, env)) {
        msg_redefinition_of_symbol(env, redefined_symbol);
    }
    while (tokens->count > 0 && !tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        Node* child;
        if (!extract_statement(env, &child, tokens)) {
            todo();
        }
        tk_view_try_consume(NULL, tokens, TOKEN_SEMICOLON);
        vec_append(&a_main, &block->children, child);
    }

    Node* dummy = NULL;
    vec_pop(dummy, &env->ancesters);
    assert(dummy == node_wrap(block));
    assert(block);
    return block;
}

static Node_struct_literal* extract_struct_literal(Env* env, Tk_view* tokens) {
    Token start_token;
    try(tk_view_try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE));
    Node_struct_literal* struct_literal = node_unwrap_struct_literal(
        node_new(start_token.pos, NODE_STRUCT_LITERAL)
    );
    struct_literal->name = literal_name_new();

    while (tk_view_try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        vec_append(&a_main, &struct_literal->members, node_wrap(extract_assignment(env, tokens, NULL)));
        tk_view_try_consume(NULL, tokens, TOKEN_COMMA);
    }

    try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    try(symbol_add(env, node_wrap(struct_literal)));
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
        Node_struct_member_sym_piece_untyped* member = node_unwrap_struct_member_sym_piece_untyped(
            node_new(member_token.pos, NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED)
        );
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

static bool try_extract_expression_piece(Env* env, Node** result, Tk_view* tokens, bool defer_sym_add) {
    Node_function_call* fun_call;
    if (tk_view_try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        try(try_extract_expression(env, result, tokens, defer_sym_add));
        try(tk_view_try_consume(NULL, tokens, TOKEN_CLOSE_PAR));
        return true;
    } else if (is_unary(tk_view_front(*tokens).type)) {
        Token operation_token = tk_view_consume(tokens);
        Node* inside_unary;
        try(try_extract_expression_piece(env, &inside_unary, tokens, defer_sym_add));
        *result = node_wrap(parser_unary_new(operation_token, inside_unary));
        return true;
    } else if (token_is_equal(tk_view_front(*tokens), "let", TOKEN_SYMBOL)) {
        Node_variable_def* var_def;
        if (!try_extract_variable_declaration(env, &var_def, tokens, true, defer_sym_add)) {
            msg(LOG_ERROR, EXPECT_FAIL_INCOMPLETE_VAR_DEF, tk_view_front(*tokens).pos, "incomplete variable definition\n");
            return false;
        }
        *result = node_wrap(var_def);
        return true;
    } else if (try_extract_function_call(env, &fun_call, tokens)) {
        *result = node_wrap(fun_call);
        return true;
    } else if (tokens->count > 1 && tk_view_front(*tokens).type == TOKEN_SYMBOL &&
        token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)
    ) {
        *result = node_wrap(extract_struct_member_call(tokens));
        return true;
    } else if (token_is_literal(tk_view_front(*tokens))) {
        *result = node_wrap(extract_literal(env, tokens, defer_sym_add));
        return true;
    } else if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        *result = node_wrap(extract_symbol(tokens));
        return true;
    } else if (token_is_equal(tk_view_front(*tokens), "", TOKEN_OPEN_CURLY_BRACE) && 
        token_is_equal(tk_view_at(*tokens, 1), "", TOKEN_SINGLE_DOT)
    ) {
        *result = node_wrap(extract_struct_literal(env, tokens));
        return true;
    } else {
        return false;
    }
}

static bool try_extract_expression(Env* env, Node** result, Tk_view* tokens, bool defer_sym_add) {
    assert(tokens->tokens);
    if (tokens->count < 1) {
        return false;
    }

    Node* expression;
    if (!try_extract_expression_piece(env, &expression, tokens, defer_sym_add)) {
        return false;
    }
    Token prev_operator_token = {0};
    bool is_first_operator = true;
    while (tokens->count > 0 && token_is_operator(tk_view_front(*tokens)) && !is_unary(tk_view_front(*tokens).type)) {
        Token operator_token = tk_view_consume(tokens);
        if (is_unary(operator_token.type)) {
            unreachable("");
        } else if (!is_first_operator && node_is_binary(expression) &&
            operator_precedence(prev_operator_token) < operator_precedence(operator_token)
        ) {
            Node* rhs;
            try(try_extract_expression_piece(env, &rhs, tokens, defer_sym_add));
            Node_binary* operation = parser_binary_new(
                node_auto_unwrap_op_binary(expression)->rhs,
                operator_token,
                rhs
            );
            node_auto_unwrap_op_binary(expression)->rhs = node_wrap(operation);
        } else {
            Node* lhs = expression;
            Node* rhs;
            try(try_extract_expression_piece(env, &rhs, tokens, defer_sym_add));
            Node_binary* operation = parser_binary_new(lhs, operator_token, rhs);
            expression = node_wrap(operation);
        }

        is_first_operator = false;
        prev_operator_token = operator_token;
    }

    *result = expression;
    return true;
}

Node_block* parse(const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Env env = {0};
    Node_block* root = extract_block(&env, &token_view);
    log(LOG_DEBUG, "%zu\n", env.ancesters.info.count);
    assert(env.ancesters.info.count == 0);
    log(LOG_DEBUG, "done with parsing:\n");
    symbol_log(LOG_TRACE, &env);
    //log(LOG_VERBOSE, "completed parse tree:\n");
    //log_tree(LOG_VERBOSE, root);
    return root;
}
