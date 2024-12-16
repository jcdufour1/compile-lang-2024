#include <assert.h>
#include <util.h>
#include <node.h>
#include <nodes.h>
#include <token_view.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <tokens.h>

static Token prev_token = {0};

// functions return bool if they do not report error to the user
// functions return PARSE_STATUS if they report error to the user
// functions return PARSE_EXPR_STATUS if they may report error to the user or do nothing without reporting an error
typedef enum {
    PARSE_OK, // no need for callers to sync tokens
    PARSE_ERROR, // tokens need to be synced by callers
} PARSE_STATUS;

typedef enum {
    PARSE_EXPR_OK, // no need for callers to sync tokens
    PARSE_EXPR_NONE, // no expression parsed; no message reported to the user, and no need for callers to sync tokens
    PARSE_EXPR_ERROR, // tokens need to be synced by callers
} PARSE_EXPR_STATUS;

static PARSE_STATUS extract_block(Env* env, Node_block** block, Tk_view* tokens, bool is_top_level);
static PARSE_EXPR_STATUS extract_statement(Env* env, Node** child, Tk_view* tokens, bool defer_sym_add);
static PARSE_EXPR_STATUS try_extract_expression(Env* env, Node_expr** result, Tk_view* tokens, bool defer_sym_add);
static PARSE_STATUS try_extract_variable_declaration(
    Env* env, 
    Node_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool defer_sym_add // if true, symbol will not be added to symbol table 
                       // until the next extract_block call
);
static Node_condition* extract_condition(Env* env, Tk_view* tokens);

static bool try_consume(Token* result, Tk_view* tokens, TOKEN_TYPE type) {
    Token temp;
    if (!tk_view_try_consume(&temp, tokens, type)) {
        return false;
    }
    prev_token = temp;
    if (result) {
        *result = temp;
    }
    return true;
}

static void consume(Tk_view* tokens) {
    Token temp = tk_view_front(*tokens);
    tk_view_consume(tokens);
    prev_token = temp;
}

static Token get_curr_token(Tk_view tokens) {
    if (tokens.count < 1) {
        return prev_token;
    }
    return tk_view_front(tokens);
}

static Pos get_curr_pos(Tk_view tokens) {
    return get_curr_token(tokens).pos;
}

#define msg_expected_expression(file_text, tokens) \
    do { \
        msg(LOG_ERROR, EXPECT_FAIL_EXPECTED_EXPRESSION, file_text, get_curr_pos(tokens), "expected expression\n"); \
    } while (0)

static void msg_parser_expected_internal(Str_view file_text, const char* file, int line, Token got, int count_expected, ...) {
    va_list args;
    va_start(args, count_expected);

    String message = {0};
    string_extend_cstr(&print_arena, &message, "got token `");
    string_extend_strv(&print_arena, &message, token_print_internal(&print_arena, got, true));
    string_extend_cstr(&print_arena, &message, "`, but expected ");

    for (int idx = 0; idx < count_expected; idx++) {
        if (idx > 0) {
            if (idx == count_expected - 1) {
                string_extend_cstr(&print_arena, &message, " or ");
            } else {
                string_extend_cstr(&print_arena, &message, ", ");
            }
        }
        string_extend_cstr(&print_arena, &message, "`");
        string_extend_strv(&print_arena, &message, token_type_to_str_view(va_arg(args, TOKEN_TYPE)));
        string_extend_cstr(&print_arena, &message, "`");
    }

    EXPECT_FAIL_TYPE expect_fail_type = EXPECT_FAIL_PARSER_EXPECTED;
    if (got.type == TOKEN_NONTYPE) {
        expect_fail_type = EXPECT_FAIL_INVALID_TOKEN;
    }
    msg_internal(file, line, LOG_ERROR, expect_fail_type, file_text, got.pos, STR_VIEW_FMT"\n", str_view_print(string_to_strv(message)));

    va_end(args);
}

#define msg_parser_expected(file_text, got, ...) \
    do { \
        msg_parser_expected_internal(file_text, __FILE__, __LINE__, got, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), __VA_ARGS__); \
    } while(0)

static PARSE_STATUS msg_redefinition_of_symbol(const Env* env, const Node_def* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, env->file_text, node_wrap_def_const(new_sym_def)->pos,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(get_def_name(new_sym_def))
    );

    Node_def* original_def;
    try(symbol_lookup(&original_def, env, get_def_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_def(original_def)->pos,
        STR_VIEW_FMT " originally defined here\n", str_view_print(get_def_name(original_def))
    );

    return PARSE_ERROR;
}

static bool starts_with_struct_def(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_STRUCT;
}

static bool starts_with_raw_union_definition(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_RAW_UNION;
}

static bool starts_with_enum_definition(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_ENUM;
}

static bool starts_with_function_decl(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_EXTERN;
}

static bool starts_with_function_def(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_FN;
}

static bool starts_with_return(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_RETURN;
}

static bool starts_with_if(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_IF;
}

static bool starts_with_for(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_FOR;
}

static bool starts_with_break(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_BREAK;
}

static bool starts_with_function_call(Tk_view tokens) {
    if (!try_consume(NULL, &tokens, TOKEN_SYMBOL)) {
        return false;
    }
    if (!try_consume(NULL, &tokens, TOKEN_OPEN_PAR)) {
        return false;
    }
    return true;
}

static bool starts_with_variable_declaration(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_LET;
}

static bool starts_with_variable_type_declaration(Tk_view tokens, bool require_let) {
    if (!try_consume(NULL, &tokens, TOKEN_LET) && require_let) {
        return false;
    }
    if (!try_consume(NULL, &tokens, TOKEN_SYMBOL)) {
        return false;
    }
    try_consume(NULL, &tokens, TOKEN_COLON);
    return try_consume(NULL, &tokens, TOKEN_SYMBOL);
}

static bool starts_with_struct_literal(Tk_view tokens) {
    if (!try_consume(NULL, &tokens, TOKEN_OPEN_CURLY_BRACE)) {
        return false;
    }
    while (try_consume(NULL, &tokens, TOKEN_NEW_LINE));
    return try_consume(NULL, &tokens, TOKEN_SINGLE_DOT);
}

static void sync(Tk_view* tokens) {
    int bracket_depth = 0;
    while (tokens->count > 0) {
        if (try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
            bracket_depth++;
        } else if (try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
            if (bracket_depth == 0) {
                return;
            }
            bracket_depth--;
        } else {
            consume(tokens);
        }
        assert(bracket_depth >= 0);

        if (prev_token.type != TOKEN_NEW_LINE || bracket_depth != 0) {
            continue;
        }

        if (
            starts_with_struct_def(*tokens) ||
            starts_with_function_decl(*tokens) ||
            starts_with_function_def(*tokens) ||
            starts_with_return(*tokens) ||
            starts_with_if(*tokens) ||
            starts_with_for(*tokens) ||
            starts_with_break(*tokens) ||
            starts_with_function_call(*tokens) ||
            starts_with_variable_declaration(*tokens)
        ) {
            return;
        }
    }
}

static void sync_past_next_bracket(Token token_to_match, Tk_view* tokens) {
    int bracket_count = 1;
    while (tokens->count > 0) {
        if (try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
            bracket_count++;
        } else if (try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
            bracket_count--;
            if (bracket_count == 0) {
                return;
            }
        } else {
            consume(tokens);
        }
        assert(bracket_count > 0);
    }

    (void) token_to_match;
    todo();
}

// try_consume tokens from { to } (inclusive) and discard outer {}
static Tk_view extract_items_inside_brackets(Tk_view* tokens, TOKEN_TYPE closing_bracket_type) {
    // the opening_bracket type should be the opening bracket type that corresponds to closing_brace_type
    switch (closing_bracket_type) {
        case TOKEN_CLOSE_CURLY_BRACE:
            try(try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
            break;
        case TOKEN_CLOSE_PAR:
            try(try_consume(NULL, tokens, TOKEN_OPEN_PAR));
            break;
        default:
            unreachable("invalid or unimplemented bracket type");
    }

    size_t idx_closing_bracket;
    if (!get_idx_matching_token(&idx_closing_bracket, *tokens, false, closing_bracket_type)) {
        unreachable("closing bracket not found");
    }
    Tk_view inside_brackets = tk_view_consume_count(tokens, idx_closing_bracket);

    try(try_consume(NULL, tokens, closing_bracket_type));
    return inside_brackets;
}

// TODO: consider case of being inside brackets
static bool can_end_statement(Token token) {
    switch (token.type) {
        case TOKEN_NONTYPE:
            unreachable("");
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
        case TOKEN_XOR:
            return false;
        case TOKEN_NOT:
            return false;
        case TOKEN_DEREF:
            return false;
        case TOKEN_REFER:
            return false;
        case TOKEN_UNSAFE_CAST:
            return false;
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_INT_LITERAL:
            return true;
        case TOKEN_VOID:
            return true;
        case TOKEN_NEW_LINE:
            todo();
        case TOKEN_SYMBOL:
            return true;
        case TOKEN_OPEN_PAR:
            return false;
        case TOKEN_CLOSE_PAR:
            return true;
        case TOKEN_OPEN_CURLY_BRACE:
            return false;
        case TOKEN_CLOSE_CURLY_BRACE:
            return true;
        case TOKEN_DOUBLE_QUOTE:
            return true;
        case TOKEN_SEMICOLON:
            return true;
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
        case TOKEN_FN:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_RETURN:
            return true;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return true;
        case TOKEN_COMMENT:
            return true;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
        case TOKEN_CLOSE_SQ_BRACKET:
            return true;
        case TOKEN_CHAR_LITERAL:
            return true;
    }
    unreachable("");
}

// higher number returned from this function means that operator has higher precedence
static uint32_t operator_precedence(Token token) {
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
        case TOKEN_INT_LITERAL:
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
        case TOKEN_UNSAFE_CAST:
            return true;
        case TOKEN_FN:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_RETURN:
            return false;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
        case TOKEN_CLOSE_SQ_BRACKET:
            return false;
        case TOKEN_CHAR_LITERAL:
            return false;
    }
    unreachable("");
}

// type will be parsed if possible
static bool extract_lang_type_struct(Lang_type* lang_type, Tk_view* tokens) {
    memset(lang_type, 0, sizeof(*lang_type));

    Token lang_type_token;
    if (!try_consume(&lang_type_token, tokens, TOKEN_SYMBOL)) {
        return false;
    }

    lang_type->str = lang_type_token.text;
    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        lang_type->pointer_depth++;
    }
    return true;
}

// require type to be parsed
static PARSE_STATUS extract_lang_type_struct_require(Env* env, Lang_type* lang_type, Tk_view* tokens) {
    if (extract_lang_type_struct(lang_type, tokens)) {
        return PARSE_OK;
    } else {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
}

static PARSE_EXPR_STATUS extract_function_parameter(Env* env, Node_variable_def** child, Tk_view* tokens) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return PARSE_EXPR_NONE;
    }

    Node_variable_def* param;
    if (PARSE_OK != try_extract_variable_declaration(env, &param, tokens, false, true)) {
        return PARSE_EXPR_ERROR;
    }
    if (try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        param->is_variadic = true;
    }
    try_consume(NULL, tokens, TOKEN_COMMA);

    *child = param;
    return PARSE_EXPR_OK;
}

static PARSE_STATUS extract_function_parameters(Env* env, Node_function_params** result, Tk_view* tokens) {
    Node_function_params* fun_params = node_function_params_new(tk_view_front(*tokens).pos);

    Node_variable_def* param;
    bool done = false;
    while (!done) {
        switch (extract_function_parameter(env, &param, tokens)) {
            case PARSE_EXPR_OK:
                vec_append_safe(&a_main, &fun_params->params, node_wrap_def(node_wrap_variable_def(param)));
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                done = true;
                break;
            default:
                unreachable("");
        }
    }

    if (tokens->count > 0) {
        if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
            unreachable("message not implemented\n");
        }
    }
    *result = fun_params;
    return PARSE_OK;
}

static void extract_return_type(Node_lang_type** result, Tk_view* tokens) {
    *result = node_lang_type_new(tk_view_front(*tokens).pos);

    bool is_comma = true;
    while (is_comma) {
        // a return type is only one token, at least for now
        if (!extract_lang_type_struct(&(*result)->lang_type, tokens)) {
            (*result)->lang_type.str = str_view_from_cstr("void");
            break;
        }
        is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
    }
    assert((*result)->lang_type.str.count > 0);
}

static PARSE_STATUS extract_function_decl_common(
    Env* env,
    Node_function_decl** fun_decl,
    Tk_view* tokens
) {
    *fun_decl = node_function_decl_new(tk_view_front(*tokens).pos);

    Token name_token = tk_view_consume(tokens);
    (*fun_decl)->name = name_token.text;
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_OPEN_PAR);
        return PARSE_ERROR;
    }
    if (PARSE_OK != extract_function_parameters(env, &(*fun_decl)->parameters, tokens)) {
        return PARSE_ERROR;
    }

    extract_return_type(&(*fun_decl)->return_type, tokens);
    assert((*fun_decl)->return_type->lang_type.str.count > 0);
    if (!symbol_add(env, node_wrap_function_decl(*fun_decl))) {
        return msg_redefinition_of_symbol(env, node_wrap_function_decl(*fun_decl));
    }

    return PARSE_OK;
}

static PARSE_STATUS extract_function_def(Env* env, Node_function_def** fun_def, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_FN));
    Node_function_decl* fun_decl;
    if (PARSE_OK != extract_function_decl_common(env, &fun_decl, tokens)) {
        return PARSE_ERROR;
    }
    (*fun_def) = node_function_def_new(node_wrap_def(node_wrap_function_decl(fun_decl))->pos);
    (*fun_def)->declaration = fun_decl;
    return extract_block(env, &(*fun_def)->body, tokens, false);
}

static PARSE_STATUS extract_struct_base_def(Env* env, Struct_def_base* base, Str_view name, Tk_view* tokens) {
    base->name = name;

    if (!try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        log_tokens(LOG_DEBUG, *tokens);
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Node_variable_def* member;
        switch (extract_function_parameter(env, &member, tokens)) {
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                done = true;
            case PARSE_EXPR_OK:
                break;
        }
        try_consume(NULL, tokens, TOKEN_SEMICOLON);
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
        vec_append_safe(&a_main, &base->members, node_wrap_def(node_wrap_variable_def(member)));
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS extract_struct_def(Env* env, Node_struct_def** struct_def, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_STRUCT));

    Token name = {0};
    if (!try_consume(&name, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_SYMBOL);
        return PARSE_ERROR;
    }

    *struct_def = node_struct_def_new(name.pos);

    if (PARSE_OK != extract_struct_base_def(env, &(*struct_def)->base, name.text, tokens)) {
        return PARSE_ERROR;
    }

    if (!symbol_add(env, node_wrap_struct_def(*struct_def))) {
        msg_redefinition_of_symbol(env, node_wrap_struct_def(*struct_def));
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS extract_raw_union_def(Env* env, Node_raw_union_def** raw_union_def, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_RAW_UNION));

    Token name = tk_view_consume(tokens);

    *raw_union_def = node_raw_union_def_new(name.pos);

    if (PARSE_OK != extract_struct_base_def(env, &(*raw_union_def)->base, name.text, tokens)) {
        return PARSE_ERROR;
    }

    if (!symbol_add(env, node_wrap_raw_union_def(*raw_union_def))) {
        msg_redefinition_of_symbol(env, node_wrap_raw_union_def(*raw_union_def));
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS extract_enum_def(Env* env, Node_enum_def** enum_def, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_ENUM));

    Token name = tk_view_consume(tokens);

    *enum_def = node_enum_def_new(name.pos);

    if (PARSE_OK != extract_struct_base_def(env, &(*enum_def)->base, name.text, tokens)) {
        return PARSE_ERROR;
    }

    if (!symbol_add(env, node_wrap_enum_def(*enum_def))) {
        msg_redefinition_of_symbol(env, node_wrap_enum_def(*enum_def));
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS try_extract_variable_declaration(
    Env* env,
    Node_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool defer_sym_add
) {
    (void) require_let;
    if (!try_consume(NULL, tokens, TOKEN_LET)) {
        assert(!require_let);
    }

    try_consume(NULL, tokens, TOKEN_NEW_LINE);
    Token name_token;
    if (!try_consume(&name_token, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
    Node_variable_def* variable_def = node_variable_def_new(name_token.pos);
    variable_def->name = name_token.text;
    try_consume(NULL, tokens, TOKEN_COLON);

    if (PARSE_OK != extract_lang_type_struct_require(env, &variable_def->lang_type, tokens)) {
        return PARSE_ERROR;
    }

    Node_def* dummy;
    if (defer_sym_add) {
        symbol_add_defer(env, node_wrap_variable_def(variable_def));
    } else {
        if (symbol_lookup(&dummy, env, variable_def->name)) {
            msg_redefinition_of_symbol(env, node_wrap_variable_def(variable_def));
            return PARSE_ERROR;
        }
        symbol_add(env, node_wrap_variable_def(variable_def));
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    *result = variable_def;
    return PARSE_OK;
}

static PARSE_STATUS extract_for_range_internal(Env* env, Node_for_range* for_loop, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_IN));

    Node_expr* lower_bound_child;
    if (PARSE_EXPR_OK != try_extract_expression(env, &lower_bound_child, tokens, true)) {
        msg_expected_expression(env->file_text, *tokens);
        return PARSE_ERROR;
    }
    Node_for_lower_bound* lower_bound = node_for_lower_bound_new(node_wrap_expr(lower_bound_child)->pos);
    lower_bound->child = lower_bound_child;
    for_loop->lower_bound = lower_bound;
    if (!try_consume(NULL, tokens, TOKEN_DOUBLE_DOT)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_DOUBLE_DOT);
        return PARSE_ERROR;
    }

    Node_expr* upper_bound_child;
    if (PARSE_EXPR_OK != try_extract_expression(env, &upper_bound_child, tokens, true)) {
        msg_expected_expression(env->file_text, *tokens);
        return PARSE_ERROR;
    }
    Node_for_upper_bound* upper_bound = node_for_upper_bound_new(node_wrap_expr(upper_bound_child)->pos);
    upper_bound->child = upper_bound_child;
    for_loop->upper_bound = upper_bound;

    return extract_block(env, &for_loop->body, tokens, false);
}

static PARSE_STATUS extract_for_with_cond(Env* env, Node_for_with_cond** for_new, Node_for_range* for_range, Tk_view* tokens) {
    *for_new = node_for_with_cond_new(node_wrap_for_range(for_range)->pos);
    (*for_new)->condition = extract_condition(env, tokens);
    return extract_block(env, &(*for_new)->body, tokens, false);
}

static PARSE_STATUS extract_for_loop(Env* env, Node** for_loop_result, Tk_view* tokens) {
    Token for_token;
    try(try_consume(&for_token, tokens, TOKEN_FOR));
    Node_for_range* for_loop = node_for_range_new(for_token.pos);
    
    if (starts_with_variable_type_declaration(*tokens, false)) {
        if (PARSE_OK != try_extract_variable_declaration(env, &for_loop->var_def, tokens, false, true)) {
            todo();
            return PARSE_ERROR;
        }
        if (PARSE_OK != extract_for_range_internal(env, for_loop, tokens)) {
            return PARSE_ERROR;
        }
        *for_loop_result = node_wrap_for_range(for_loop);
    } else {
        Node_for_with_cond* for_with_cond;
        if (PARSE_OK != extract_for_with_cond(env, &for_with_cond, for_loop, tokens)) {
            return PARSE_ERROR;
        }
        *for_loop_result = node_wrap_for_with_cond(for_with_cond);
    }

    return PARSE_OK;
}

static Node_break* extract_break(Tk_view* tokens) {
    Token break_token = tk_view_consume(tokens);
    Node_break* bk_statement = node_break_new(break_token.pos);
    return bk_statement;
}

static PARSE_STATUS extract_function_decl(Env* env, Node_function_decl** fun_decl, Tk_view* tokens) {
    PARSE_STATUS status = PARSE_ERROR;

    try(try_consume(NULL, tokens, TOKEN_EXTERN));
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_OPEN_PAR);
        goto error;
    }
    Token extern_type_token;
    if (!try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_STRING_LITERAL);
        goto error;
    }
    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        msg(
            LOG_ERROR, EXPECT_FAIL_INVALID_EXTERN_TYPE, env->file_text, extern_type_token.pos,
            "invalid extern type `"STR_VIEW_FMT"`\n", str_view_print(extern_type_token.text)
        );
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_CLOSE_PAR);
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_FN)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_FN);
        goto error;
    }
    if (PARSE_OK != extract_function_decl_common(env, fun_decl, tokens)) {
        goto error;
    }
    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    status = PARSE_OK;
error:
    symbol_ignore_defered(env);
    return status;
}

static Node_literal* extract_literal(Env* env, Tk_view* tokens, bool defer_sym_add) {
    (void) env;
    Token token = tk_view_consume(tokens);
    assert(token_is_literal(token));

    Node_literal* new_node = util_literal_new_from_strv(token.text, token.type, token.pos);

    if (defer_sym_add) {
        // TODO: is this nessessary?
        //symbol_add_defer(env, node_wrap_expr(node_wrap_literal(new_node)));
    } else {
        // TODO: is this nessessary?
        //try(symbol_add(env, node_wrap_expr(node_wrap_literal(new_node))));
    }
    return new_node;
}

static Node_symbol_untyped* extract_symbol(Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token.type == TOKEN_SYMBOL);

    Node_expr* sym_node_ = node_expr_new(token.pos);
    sym_node_->type = NODE_SYMBOL_UNTYPED;
    Node_symbol_untyped* sym_node = node_unwrap_symbol_untyped(sym_node_);
    sym_node->name = token.text;
    return sym_node;
}

static PARSE_STATUS try_extract_function_call(Env* env, Node_function_call** child, Tk_view* tokens) {
    Token fun_name_token;
    if (!try_consume(&fun_name_token, tokens, TOKEN_SYMBOL)) {
        unreachable("this is not a function call");
    }
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        unreachable("this is not a function call");
    }
    Node_function_call* function_call = node_function_call_new(fun_name_token.pos);
    function_call->name = fun_name_token.text;

    bool is_first_time = true;
    bool prev_is_comma = false;
    while (is_first_time || prev_is_comma) {
        Node_expr* arg;
        switch (try_extract_expression(env, &arg, tokens, true)) {
            case PARSE_EXPR_OK:
                assert(arg);
                vec_append_safe(&a_main, &function_call->args, arg);
                prev_is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
                break;
            case PARSE_EXPR_NONE:
                if (prev_is_comma) {
                    msg_expected_expression(env->file_text, *tokens);
                    return PARSE_ERROR;
                }
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        is_first_time = false;
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_CLOSE_PAR, TOKEN_COMMA);
        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_expr(node_wrap_function_call(function_call))->pos,
            "when parsing arguments for call to function `"STR_VIEW_FMT"`\n", 
            str_view_print(function_call->name)
        );
        return PARSE_ERROR;
    }

    *child = function_call;
    return PARSE_OK;
}

static PARSE_STATUS extract_function_return(Env* env, Node_return** rtn_statement, Tk_view* tokens) {
    try(try_consume(NULL, tokens, TOKEN_RETURN));

    Node_expr* expression;
    switch (try_extract_expression(env, &expression, tokens, false)) {
        case PARSE_EXPR_OK:
            *rtn_statement = node_return_new(node_wrap_expr(expression)->pos);
            (*rtn_statement)->child = expression;
            break;
        case PARSE_EXPR_NONE:
            *rtn_statement = node_return_new(prev_token.pos);
            (*rtn_statement)->child = node_wrap_literal(util_literal_new_from_strv(str_view_from_cstr(""), TOKEN_VOID, prev_token.pos));
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return PARSE_OK;
}

static PARSE_STATUS extract_assignment(Env* env, Node_assignment** assign, Tk_view* tokens, Node* lhs) {
    Token equal_token;
    *assign = NULL;
    if (lhs) {
        *assign = node_assignment_new(lhs->pos);
        if (!try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL)) {
            unreachable("extract_assignment should possibly never be called with no `=`, but it was");
        }
    } else {
        Node_expr* lhs_ = NULL;
        switch (try_extract_expression(env, &lhs_, tokens, false)) {
            case PARSE_EXPR_NONE:
                unreachable("extract_assignment should possibly never be called without lhs expression present, but it was");
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_OK:
                lhs = node_wrap_expr(lhs_);
                break;
            default:
                unreachable("");
        }
        if (!try_consume(&equal_token, tokens, TOKEN_SINGLE_EQUAL)) {
            todo();
            msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_SINGLE_EQUAL);
            return PARSE_ERROR;
        }
        *assign = node_assignment_new(equal_token.pos);
    }

    (*assign)->lhs = lhs;

    switch (try_extract_expression(env, &(*assign)->rhs, tokens, false)) {
        case PARSE_EXPR_NONE:
            msg_expected_expression(env->file_text, *tokens);
            return PARSE_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_OK:
            return PARSE_OK;
    }
    unreachable("");
}

static Node_condition* extract_condition(Env* env, Tk_view* tokens) {
    Node_expr* cond_child;
    if (PARSE_EXPR_OK != try_extract_expression(env, &cond_child, tokens, false)) {
        todo();
    }
    Node_condition* condition = node_condition_new(node_wrap_expr(cond_child)->pos);

    // TODO: make helper function that does this
    switch (cond_child->type) {
        case NODE_OPERATOR:
            condition->child = node_unwrap_operator(cond_child);
            break;
        case NODE_LITERAL:
            condition->child = condition_get_default_child(cond_child);
            break;
        case NODE_FUNCTION_CALL:
            condition->child = condition_get_default_child(cond_child);
            break;
        case NODE_SYMBOL_UNTYPED:
            condition->child = condition_get_default_child(cond_child);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr(cond_child)));
    }

    return condition;
}

static void if_else_chain_consume_newline(Tk_view* tokens) {
    while (1) {
        if (tokens->count < 2) {
            break;
        }
        if (tk_view_at(*tokens, 1).type != TOKEN_NEW_LINE && tk_view_at(*tokens, 1).type != TOKEN_ELSE) {
            break;
        }
        if (!try_consume(NULL, tokens, TOKEN_NEW_LINE)) {
            break;
        }
    }
}

static PARSE_STATUS extract_if_else_chain(Env* env, Node_if_else_chain** if_else_chain, Tk_view* tokens) {
    Token if_start_token;
    try(try_consume(&if_start_token, tokens, TOKEN_IF));

    *if_else_chain = node_if_else_chain_new(if_start_token.pos);

    Node_if* if_statement = NULL;
    if_statement = node_if_new(if_start_token.pos);
    if_statement->condition = extract_condition(env, tokens);
    if (PARSE_OK != extract_block(env, &if_statement->body, tokens, false)) {
        return PARSE_ERROR;
    }
    vec_append(&a_main, &(*if_else_chain)->nodes, if_statement);

    if_else_chain_consume_newline(tokens);
    while (try_consume(NULL, tokens, TOKEN_ELSE)) {
        if_statement = node_if_new(if_start_token.pos);

        if (try_consume(&if_start_token, tokens, TOKEN_IF)) {
            if_statement->condition = extract_condition(env, tokens);
        } else {
            if_statement->condition = node_condition_new(if_start_token.pos);
            if_statement->condition->child = condition_get_default_child(node_wrap_literal(
                util_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_start_token.pos)
            ));
        }

        if (PARSE_OK != extract_block(env, &if_statement->body, tokens, false)) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, &(*if_else_chain)->nodes, if_statement);

        if_else_chain_consume_newline(tokens);
    }

    log(LOG_DEBUG, "thing 1356\n");
    return PARSE_OK;
}

static PARSE_EXPR_STATUS extract_statement(Env* env, Node** child, Tk_view* tokens, bool defer_sym_add) {
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Node* lhs;
    if (starts_with_struct_def(*tokens)) {
        Node_struct_def* struct_def;
        if (PARSE_OK != extract_struct_def(env, &struct_def, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_struct_def(struct_def));
    } else if (starts_with_raw_union_definition(*tokens)) {
        Node_raw_union_def* raw_union_def;
        if (PARSE_OK != extract_raw_union_def(env, &raw_union_def, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_raw_union_def(raw_union_def));
    } else if (starts_with_enum_definition(*tokens)) {
        Node_enum_def* enum_def;
        if (PARSE_OK != extract_enum_def(env, &enum_def, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_enum_def(enum_def));
    } else if (starts_with_function_decl(*tokens)) {
        Node_function_decl* fun_decl;
        if (PARSE_OK != extract_function_decl(env, &fun_decl, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_function_decl(fun_decl));
    } else if (starts_with_function_def(*tokens)) {
        Node_function_def* fun_def;
        if (PARSE_OK != extract_function_def(env, &fun_def, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_function_def(fun_def));
    } else if (starts_with_return(*tokens)) {
        Node_return* rtn_statement = NULL;
        if (PARSE_OK != extract_function_return(env, &rtn_statement, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_return(rtn_statement);
    } else if (starts_with_if(*tokens)) {
        Node_if_else_chain* if_else_chain = NULL;
        if (PARSE_OK != extract_if_else_chain(env, &if_else_chain, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_if_else_chain(if_else_chain);
    } else if (starts_with_for(*tokens)) {
        Node* for_loop;
        if (PARSE_OK != extract_for_loop(env, &for_loop, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = for_loop;
    } else if (starts_with_break(*tokens)) {
        lhs = node_wrap_break(extract_break(tokens));
    } else if (starts_with_variable_declaration(*tokens)) {
        Node_variable_def* var_def;
        if (PARSE_OK != try_extract_variable_declaration(env, &var_def, tokens, true, defer_sym_add)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = node_wrap_def(node_wrap_variable_def(var_def));
    } else {
        Node_expr* lhs_ = NULL;
        switch (try_extract_expression(env, &lhs_, tokens, false)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                return PARSE_EXPR_NONE;
            default:
                unreachable("");
        }

        switch (lhs_->type) {
            case NODE_FUNCTION_CALL:
                break;
            case NODE_SYMBOL_UNTYPED:
                break;
            case NODE_MEMBER_ACCESS_UNTYPED:
                break;
            case NODE_OPERATOR:
                break;
            case NODE_INDEX_UNTYPED:
                break;
            default:
                unreachable(NODE_FMT"\n", node_print(node_wrap_expr(lhs_)));
        }

        lhs = node_wrap_expr(lhs_);
    }

    // do assignment if applicible
    if (tokens->count > 0 && tk_view_front(*tokens).type == TOKEN_SINGLE_EQUAL) {
        Node_assignment* assign;
        if (PARSE_OK != extract_assignment(env, &assign, tokens, lhs)) {
            return PARSE_EXPR_ERROR;
        }
        *child = node_wrap_assignment(assign);
    } else {
        *child = lhs;
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);
    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }

    // TODO: allow else to be after closing bracket
    if (!try_consume(NULL, tokens, TOKEN_NEW_LINE)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT, env->file_text, tk_view_front(*tokens).pos,
            "expected newline after statement\n"
        );
        return PARSE_EXPR_ERROR;
    }

    return PARSE_EXPR_OK;
}

static PARSE_STATUS extract_block(Env* env, Node_block** block, Tk_view* tokens, bool is_top_level) {
    PARSE_STATUS status = PARSE_OK;

    *block = node_block_new(tk_view_front(*tokens).pos);
    vec_append_safe(&a_main, &env->ancesters, node_wrap_block(*block));

    Node_def* redefined_symbol;
    if (!symbol_do_add_defered(&redefined_symbol, env)) {
        msg_redefinition_of_symbol(env, redefined_symbol);
        status = PARSE_ERROR;
    }

    if (tokens->count < 1) {
        unreachable("empty file not implemented\n");
    }

    Token open_brace_token = {0};
    if (!is_top_level && !try_consume(&open_brace_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_OPEN_CURLY_BRACE);
        status = PARSE_ERROR;
        goto end;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Token close_brace_token = {0};
    while (!try_consume(&close_brace_token, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        if (tokens->count < 1) {
            // this means that there is no matching `}` found
            if (!is_top_level) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE, env->file_text, open_brace_token.pos, 
                    "opening `{` is unmatched\n"
                );
                status = PARSE_ERROR;
            }
            break;
        }
        Node* child;
        bool should_stop = false;
        switch (extract_statement(env, &child, tokens, false)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                assert(error_count > 0 && "error_count not incremented\n");
                sync(tokens);
                if (tokens->count < 1) {
                    should_stop = true;
                }
                status = PARSE_ERROR;
                continue;
            case PARSE_EXPR_NONE:
                should_stop = true;
                break;
            default:
                unreachable("");
        }
        if (should_stop) {
            break;
        }
        try_consume(NULL, tokens, TOKEN_SEMICOLON);
        vec_append_safe(&a_main, &(*block)->children, child);
    }
    (*block)->pos_end = close_brace_token.pos;

    Node* dummy = NULL;
end:
    vec_pop(dummy, &env->ancesters);
    assert(dummy == node_wrap_block(*block));
    assert(*block);
    return status;
}

static PARSE_STATUS extract_struct_literal(Env* env, Node_struct_literal** struct_lit, Tk_view* tokens) {
    Token start_token;
    if (!try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env->file_text, tk_view_front(*tokens), TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }
    *struct_lit = node_struct_literal_new(start_token.pos);
    (*struct_lit)->name = util_literal_name_new();

    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    while (try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        bool delim_is_present = false;
        Node_assignment* assign;
        extract_assignment(env, &assign, tokens, NULL);
        vec_append_safe(&a_main, &(*struct_lit)->members, node_wrap_assignment(assign));
        delim_is_present = try_consume(NULL, tokens, TOKEN_COMMA);
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE)) {
            delim_is_present = true;
        }
        if (!delim_is_present) {
            break;
        }
    }

    if (!try_consume(&start_token, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(
            env->file_text,
            tk_view_front(*tokens),
            TOKEN_COMMA,
            TOKEN_NEW_LINE,
            TOKEN_CLOSE_CURLY_BRACE
        );
        return PARSE_ERROR;
    }

    // TODO: is this nessessary?
    // try(symbol_add(env, node_wrap_expr(node_wrap_struct_literal(*struct_lit))));
    return PARSE_OK;
}

static Node_unary* parser_util_unary_new(Token operator_token, Node_expr* child, Lang_type init_lang_type) {
    Node_unary* unary = node_unary_new(operator_token.pos);
    unary->token_type = operator_token.type;
    unary->child = child;
    unary->lang_type = init_lang_type;
    return unary;
}

static Node_binary* parser_binary_new(Node_expr* lhs, Token operator_token, Node_expr* rhs) {
    Node_binary* binary = node_binary_new(operator_token.pos);
    binary->token_type = operator_token.type;
    binary->lhs = lhs;
    binary->rhs = rhs;
    return binary;
}

static PARSE_EXPR_STATUS try_extract_expression_piece(
    Env* env, Node_expr** result, Tk_view* tokens, bool defer_sym_add
) {
    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }

    Node_function_call* fun_call;
    Token open_par_token = {0};
    if (try_consume(&open_par_token, tokens, TOKEN_OPEN_PAR)) {
        switch (try_extract_expression(env, result, tokens, defer_sym_add)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                // TODO: remove this if if it causes too many issues
                if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
                    msg(
                        LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_PAR, env->file_text, open_par_token.pos, 
                        "opening `(` is unmatched\n"
                    );
                }
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expression(env->file_text, *tokens);
                return PARSE_EXPR_ERROR;
        }
        if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_PAR, env->file_text, open_par_token.pos, 
                "opening `(` is unmatched\n"
            );
            return PARSE_EXPR_ERROR;
        }
    } else if (is_unary(tk_view_front(*tokens).type)) {
        Token operator_token = tk_view_consume(tokens);
        Lang_type unary_lang_type = {0};
        switch (operator_token.type) {
            case TOKEN_DEREF:
                break;
            case TOKEN_REFER:
                break;
            case TOKEN_NOT:
                break;
            case TOKEN_UNSAFE_CAST: {
                {
                    Token temp;
                    if (!try_consume(&temp, tokens, TOKEN_LESS_THAN)) {
                        msg_parser_expected(env->file_text, temp, TOKEN_LESS_THAN);
                        return PARSE_EXPR_ERROR;
                    }
                }
                if (PARSE_OK != extract_lang_type_struct_require(env, &unary_lang_type, tokens)) {
                    return PARSE_EXPR_ERROR;
                }
                {
                    Token temp;
                    if (!try_consume(&temp, tokens, TOKEN_GREATER_THAN)) {
                        msg_parser_expected(env->file_text, temp, TOKEN_GREATER_THAN);
                        return PARSE_EXPR_ERROR;
                    }
                }
                break;
            }
            default:
                unreachable(TOKEN_TYPE_FMT"\n", token_type_print(operator_token.type));
        }
        Node_expr* inside_unary;
        if (PARSE_EXPR_OK != try_extract_expression_piece(env, &inside_unary, tokens, defer_sym_add)) {
            todo();
        }
        *result = node_wrap_operator(node_wrap_unary(parser_util_unary_new(operator_token, inside_unary, unary_lang_type)));
    } else if (starts_with_function_call(*tokens)) {
        if (PARSE_OK != try_extract_function_call(env, &fun_call, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        *result = node_wrap_function_call(fun_call);
    } else if (token_is_literal(tk_view_front(*tokens))) {
        *result = node_wrap_literal(extract_literal(env, tokens, defer_sym_add));
    } else if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        *result = node_wrap_symbol_untyped(extract_symbol(tokens));
    } else if (starts_with_struct_literal(*tokens)) {
        Node_struct_literal* struct_lit;
        if (PARSE_OK != extract_struct_literal(env, &struct_lit, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        *result = node_wrap_struct_literal(struct_lit);
    } else if (tk_view_front(*tokens).type == TOKEN_NONTYPE) {
        return PARSE_EXPR_NONE;
    } else {
        return PARSE_EXPR_NONE;
    }

    while (1) {
        Token bracket = {0};
        if (try_consume(&bracket, tokens, TOKEN_SINGLE_DOT)) {
            Token member = {0};
            if (!try_consume(&member, tokens, TOKEN_SYMBOL)) {
                unreachable("");
            }

            Node_member_access_untyped* access = node_member_access_untyped_new(bracket.pos);
            access->callee = *result;
            access->member_name = member.text;

            *result = node_wrap_member_access_untyped(access);
        } else if (try_consume(&bracket, tokens, TOKEN_OPEN_SQ_BRACKET)) {
            Node_index_untyped* access = node_index_untyped_new(bracket.pos);

            PARSE_EXPR_STATUS status = try_extract_expression(
                env, &access->index, tokens, defer_sym_add
            );
            if (status != PARSE_EXPR_OK) {
                return status;
            }

            access->callee = *result;

            if (!try_consume(&bracket, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_SQ_BRACKET, env->file_text,
                    get_curr_pos(*tokens), "expected closing `]` after expression"
                );
                return PARSE_EXPR_ERROR;
            }

            *result = node_wrap_index_untyped(access);
        } else {
            break;
        }
    }

    return PARSE_EXPR_OK;
}

static bool expr_is_unary(const Node_expr* expr) {
    if (expr->type != NODE_OPERATOR) {
        return false;
    }
    const Node_operator* operator = node_unwrap_operator_const(expr);
    return operator->type == NODE_UNARY;
}

static bool expr_is_binary(const Node_expr* expr) {
    if (expr->type != NODE_OPERATOR) {
        return false;
    }
    const Node_operator* operator = node_unwrap_operator_const(expr);
    return operator->type == NODE_BINARY;
}

static PARSE_EXPR_STATUS try_extract_expression(Env* env, Node_expr** result, Tk_view* tokens, bool defer_sym_add) {
    assert(tokens->tokens);
    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }

    Node_expr* expression = NULL;
    switch (try_extract_expression_piece(env, &expression, tokens, defer_sym_add)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            assert(error_count > 0 && "error_count not incremented\n");
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_NONE:
            return PARSE_EXPR_NONE;
        default:
            unreachable("");
    }
    assert(expression);

    Token prev_operator_token = {0};
    bool is_first_operator = true;
    while (tokens->count > 0 && token_is_operator(tk_view_front(*tokens)) && !is_unary(tk_view_front(*tokens).type)) {
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE)) {
            todo();
        }

        Token operator_token = tk_view_consume(tokens);
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
        if (is_unary(operator_token.type)) {
            unreachable("");
        } else if (!is_first_operator && expr_is_binary(expression) &&
            operator_precedence(prev_operator_token) < operator_precedence(operator_token)
        ) {
            Node_expr* rhs;
            switch (try_extract_expression_piece(env, &rhs, tokens, defer_sym_add)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_NONE: {
                    msg_expected_expression(env->file_text, *tokens);
                    return PARSE_EXPR_ERROR;
                }
                case PARSE_EXPR_ERROR: {
                    assert(error_count > 0 && "error_count not incremented\n");
                    return PARSE_EXPR_ERROR;
                }
                default:
                    unreachable("");

            }
            Node_binary* operator = parser_binary_new(
                node_unwrap_binary(node_unwrap_operator(expression))->rhs,
                operator_token,
                rhs
            );
            node_unwrap_binary(node_unwrap_operator(expression))->rhs = node_wrap_operator(node_wrap_binary(operator));
        } else {
            Node_expr* lhs = expression;
            Node_expr* rhs;
            switch (try_extract_expression_piece(env, &rhs, tokens, defer_sym_add)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_NONE: {
                    msg_expected_expression(env->file_text, *tokens);
                    return PARSE_EXPR_ERROR;
                }
                case PARSE_EXPR_ERROR: {
                    assert(error_count > 0 && "error_count not incremented\n");
                    return PARSE_EXPR_ERROR;
                }
                default:
                    unreachable("");
            }
            Node_binary* operator = parser_binary_new(lhs, operator_token, rhs);
            expression = node_wrap_operator(node_wrap_binary(operator));
        }

        is_first_operator = false;
        prev_operator_token = operator_token;
    }

    *result = expression;
    return PARSE_EXPR_OK;
}

Node_block* parse(Env* env, const Tokens tokens) {
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    Node_block* root;
    extract_block(env, &root, &token_view, true);
    log(LOG_DEBUG, "%zu\n", env->ancesters.info.count);
    assert(env->ancesters.info.count == 0);
    log(LOG_DEBUG, "done with parsing:\n");
    symbol_log(LOG_TRACE, env);
    return root;
}
