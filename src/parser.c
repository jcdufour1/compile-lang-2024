#include <assert.h>
#include <util.h>
#include <uast.h>
#include <uast_utils.h>
#include <token_view.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <do_passes.h>
#include <ulang_type.h>
#include <token_type_to_operator_type.h>
#include <do_passes.h>
#include <uast_clone.h>
#include <symbol_log.h>
#include <file.h>
#include <errno.h>

// TODO: use parent block for scope_ids instead of function calls everytime

static Token prev_token = {0};

// functions return bool if they do not report error to the user
// functions return PARSE_STATUS if they report error to the user
// functions return PARSE_EXPR_STATUS if they may report error to the user or do nothing without reporting an error
typedef enum {
    PARSE_OK, // no need for callers to sync tokens
    PARSE_ERROR, // tokens need to be synced by callers
} PARSE_STATUS;

typedef enum {
    PARSE_EXPR_OK, // no need for callers to sync tokens, and no message reported to the user
    PARSE_EXPR_NONE, // no expr parsed; no message reported to the user, and no need for callers to sync tokens
    PARSE_EXPR_ERROR, // tokens need to be synced by callers
} PARSE_EXPR_STATUS;

static void set_right_child_expr(Uast_expr* expr, Uast_expr* new_rhs);
static PARSE_STATUS parse_block(Uast_block** block, Tk_view* tokens, bool is_top_level, Scope_id scope_id);
static PARSE_EXPR_STATUS parse_stmt(Uast_stmt** child, Tk_view* tokens, bool defer_sym_add, Scope_id scope_id);
static PARSE_EXPR_STATUS parse_expr(
    Uast_expr** result,
    Tk_view* tokens,
    bool defer_sym_add, /* TODO: remove defer variables and functions, and just put items in hash table immediately */
    bool can_be_tuple,
    Scope_id scope_id
);
static PARSE_STATUS parse_variable_decl(
    Uast_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool defer_sym_add, // if true, symbol will not be added to symbol table 
                       // until the next parse_block call
    bool add_to_sym_table,
    bool require_type,
    Ulang_type lang_type_if_not_required,
    Scope_id scope_id
);
static PARSE_EXPR_STATUS parse_condition(Uast_condition**, Tk_view* tokens, Scope_id scope_id);
static PARSE_STATUS parse_generics_args(Ulang_type_vec* args, Tk_view* tokens);
static PARSE_STATUS parse_generics_params(Uast_generic_param_vec* params, Tk_view* tokens);

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

static Token consume(Tk_view* tokens) {
    Token temp = tk_view_front(*tokens);
    tk_view_consume(tokens);
    prev_token = temp;
    return temp;
}

static Token consume_operator(Tk_view* tokens) {
    Token temp = consume(tokens);
    try_consume(NULL, tokens, TOKEN_NEW_LINE);
    return temp;
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

static void msg_expected_expr_internal(File_path_to_text file_text, const char* file, int line, Tk_view tokens, const char* msg_suffix) {
    String message = {0};
    string_extend_cstr(&print_arena, &message, "expected expr ");
    string_extend_cstr(&print_arena, &message, msg_suffix);

    msg_internal(file, line, LOG_ERROR, EXPECT_FAIL_EXPECTED_EXPRESSION, file_text, get_curr_pos(tokens), STRING_FMT"\n", string_print(message)); \
}

#define msg_expected_expr(file_text, tokens, msg_suffix) \
    msg_expected_expr_internal(file_text, __FILE__, __LINE__, tokens, msg_suffix)

static void msg_parser_expected_internal(File_path_to_text file_text, const char* file, int line, Token got, const char* msg_suffix, int count_expected, ...) {
    va_list args;
    va_start(args, count_expected);

    String message = {0};
    string_extend_cstr(&print_arena, &message, "got token `");
    string_extend_strv(&print_arena, &message, token_print_internal(&print_arena, TOKEN_MODE_MSG, got));
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
        string_extend_strv(&print_arena, &message, token_type_to_str_view(TOKEN_MODE_MSG, va_arg(args, TOKEN_TYPE)));
        string_extend_cstr(&print_arena, &message, "` ");
    }

    string_extend_cstr(&print_arena, &message, msg_suffix);

    EXPECT_FAIL_TYPE expect_fail_type = EXPECT_FAIL_PARSER_EXPECTED;
    if (got.type == TOKEN_NONTYPE) {
        expect_fail_type = EXPECT_FAIL_INVALID_TOKEN;
    }
    msg_internal(file, line, LOG_ERROR, expect_fail_type, file_text, got.pos, STR_VIEW_FMT"\n", str_view_print(string_to_strv(message)));

    va_end(args);
}

#define msg_parser_expected(file_text, got, msg_suffix, ...) \
    do { \
        msg_parser_expected_internal(file_text, __FILE__, __LINE__, got, msg_suffix, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), __VA_ARGS__); \
    } while(0)

static PARSE_STATUS msg_redefinition_of_symbol(const Uast_def* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, env.file_path_to_text, uast_def_get_pos(new_sym_def),
        "redefinition of symbol "STR_VIEW_FMT"\n", name_print(uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, uast_def_get_pos(original_def),
        STR_VIEW_FMT " originally defined here\n", name_print(uast_def_get_name(original_def))
    );

    return PARSE_ERROR;
}

static bool get_mod_alias_from_path_token(Uast_mod_alias** mod_alias, Token alias_tk, Pos mod_path_pos, Str_view mod_path) {
    Uast_def* prev_def = NULL;
    if (usymbol_lookup(&prev_def, name_new((Str_view) {0}, mod_path, (Ulang_type_vec) {0}, 0 /* TODO */))) {
        goto finish;
    }

    String file_path = {0};
    string_extend_strv(&a_main, &file_path, mod_path);
    string_extend_cstr(&a_main, &file_path, ".own");
    (void) mod_path_pos;
    todo();
    //Sym_coll_vec old_ances = env.ancesters;
    //Str_view old_mod_path = env.curr_mod_path;
    //env.ancesters.info.count = 1; // TODO: make macro to do this
    //env.curr_mod_path = mod_path;
    //Uast_block* block = NULL;
    //if (!parse_file(&block, string_to_strv(file_path), false)) {
    //    // TODO: expected failure test
    //    todo();
    //}
    //env.ancesters = old_ances;
    //env.curr_mod_path = old_mod_path;

    //unwrap(usym_tbl_add(&vec_at(&env.ancesters, 0)->usymbol_table, uast_import_path_wrap(uast_import_path_new(
    //    mod_path_pos,
    //    block,
    //    name_new((Str_view) {0}, mod_path, (Ulang_type_vec) {0}, 0)
    //))));

finish:
    *mod_alias = uast_mod_alias_new(
        alias_tk.pos,
        name_new(env.curr_mod_path, alias_tk.text, (Ulang_type_vec) {0}, 0),
        name_new(env.curr_mod_path, mod_path, (Ulang_type_vec) {0}, 0)
    );
    unwrap(usymbol_add(uast_mod_alias_wrap(*mod_alias)));
    return true;
}

static bool starts_with_mod_alias(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_IMPORT;
}

static bool starts_with_struct_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_STRUCT;
}

static bool starts_with_raw_union_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_RAW_UNION;
}

static bool starts_with_enum_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_ENUM;
}

static bool starts_with_sum_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_SUM;
}

static bool starts_with_lang_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_DEF;
}

static bool starts_with_type_def(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_TYPE_DEF;
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

static bool starts_with_switch(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_SWITCH;
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

static bool starts_with_continue(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_CONTINUE;
}

static bool starts_with_block(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_OPEN_CURLY_BRACE;
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

// TODO: rename variable_decl to variable_def for consistancy
static bool starts_with_variable_decl(Tk_view tokens) {
    if (tokens.count < 1) {
        return false;
    }
    return tk_view_front(tokens).type == TOKEN_LET;
}

static bool starts_with_variable_type_decl(Tk_view tokens, bool require_let) {
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
    return try_consume(NULL, &tokens, TOKEN_OPEN_CURLY_BRACE);
}

static bool starts_with_array_literal(Tk_view tokens) {
    return try_consume(NULL, &tokens, TOKEN_OPEN_SQ_BRACKET);
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
            starts_with_type_def(*tokens) ||
            starts_with_function_decl(*tokens) ||
            starts_with_function_def(*tokens) ||
            starts_with_return(*tokens) ||
            starts_with_if(*tokens) ||
            starts_with_for(*tokens) ||
            starts_with_break(*tokens) ||
            starts_with_function_call(*tokens) ||
            starts_with_variable_decl(*tokens)
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

// TODO: consider case of being inside brackets
static bool can_end_stmt(Token token) {
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
        case TOKEN_BITWISE_XOR:
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
        case TOKEN_CONTINUE:
            return true;
        case TOKEN_LESS_OR_EQUAL:
            return false;
        case TOKEN_GREATER_OR_EQUAL:
            return false;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
        case TOKEN_SUM:
            return false;
        case TOKEN_MODULO:
            return false;
        case TOKEN_BITWISE_AND:
            return false;
        case TOKEN_BITWISE_OR:
            return false;
        case TOKEN_LOGICAL_AND:
            return false;
        case TOKEN_LOGICAL_OR:
            return false;
        case TOKEN_SHIFT_LEFT:
            return false;
        case TOKEN_SHIFT_RIGHT:
            return false;
        case TOKEN_OPEN_GENERIC:
            return false;
        case TOKEN_CLOSE_GENERIC:
            return true;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
    }
    unreachable("");
}

// higher number returned from this function means that operator has higher precedence
static int32_t get_operator_precedence(TOKEN_TYPE type) {
    switch (type) {
        case TOKEN_COMMA:
            return 1;
        case TOKEN_SINGLE_EQUAL:
            unreachable("not a regular operator");
        case TOKEN_LOGICAL_OR:
            return 8;
        case TOKEN_LOGICAL_AND:
            return 9;
        case TOKEN_BITWISE_OR:
            return 10;
        case TOKEN_BITWISE_XOR:
            return 11;
        case TOKEN_BITWISE_AND:
            return 12;
        case TOKEN_NOT_EQUAL:
            return 13;
        case TOKEN_DOUBLE_EQUAL:
            return 13;
        case TOKEN_LESS_THAN:
            return 15;
        case TOKEN_GREATER_THAN:
            return 15;
        case TOKEN_LESS_OR_EQUAL:
            return 15;
        case TOKEN_GREATER_OR_EQUAL:
            return 15;
        case TOKEN_SHIFT_LEFT:
            return 16;
        case TOKEN_SHIFT_RIGHT:
            return 16;
        case TOKEN_SINGLE_PLUS:
            return 17;
        case TOKEN_SINGLE_MINUS:
            return 17;
        case TOKEN_ASTERISK:
            return 18;
        case TOKEN_SLASH:
            return 18;
        case TOKEN_MODULO:
            return 18;
        case TOKEN_NOT:
            return 28;
        case TOKEN_DEREF:
            return 28;
        case TOKEN_REFER:
            return 28;
        case TOKEN_UNSAFE_CAST:
            return 28;
        case TOKEN_SINGLE_DOT:
            return 33;
        case TOKEN_OPEN_SQ_BRACKET:
            return 33;
        case TOKEN_OPEN_PAR:
            return 33;
        case TOKEN_OPEN_GENERIC:
            return 33;
        default:
            unreachable(TOKEN_TYPE_FMT, token_type_print(TOKEN_MODE_LOG, type));
    }
}

static bool is_unary(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return true;
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
        case TOKEN_BITWISE_XOR:
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
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return false;
        case TOKEN_GREATER_OR_EQUAL:
            return false;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
        case TOKEN_SUM:
            return false;
        case TOKEN_MODULO:
            return false;
        case TOKEN_BITWISE_AND:
            return false;
        case TOKEN_BITWISE_OR:
            return false;
        case TOKEN_LOGICAL_AND:
            return false;
        case TOKEN_LOGICAL_OR:
            return false;
        case TOKEN_SHIFT_LEFT:
            return false;
        case TOKEN_SHIFT_RIGHT:
            return false;
        case TOKEN_OPEN_GENERIC:
            return false;
        case TOKEN_CLOSE_GENERIC:
            return false;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
    }
    unreachable("");
}

// TOKEN_SINGLE_PLUS is treated differently from other binary operators for now
static bool is_binary(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return true;
        case TOKEN_SINGLE_MINUS:
            return true;
        case TOKEN_ASTERISK:
            return true;
        case TOKEN_SLASH:
            return true;
        case TOKEN_LESS_THAN:
            return true;
        case TOKEN_GREATER_THAN:
            return true;
        case TOKEN_DOUBLE_EQUAL:
            return true;
        case TOKEN_NOT_EQUAL:
            return true;
        case TOKEN_NOT:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_INT_LITERAL:
            return false;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_OPEN_PAR:
            return true;
        case TOKEN_CLOSE_PAR:
            return true;
        case TOKEN_OPEN_CURLY_BRACE:
            return true;
        case TOKEN_CLOSE_CURLY_BRACE:
            return true;
        case TOKEN_DOUBLE_QUOTE:
            return false;
        case TOKEN_SEMICOLON:
            return false;
        case TOKEN_COMMA:
            return true;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_SINGLE_DOT:
            return true;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_BITWISE_XOR:
            return true;
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
            return true;
        case TOKEN_CLOSE_SQ_BRACKET:
            return true;
        case TOKEN_CHAR_LITERAL:
            return false;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return true;
        case TOKEN_GREATER_OR_EQUAL:
            return true;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
        case TOKEN_SUM:
            return false;
        case TOKEN_MODULO:
            return true;
        case TOKEN_BITWISE_AND:
            return true;
        case TOKEN_BITWISE_OR:
            return true;
        case TOKEN_LOGICAL_AND:
            return true;
        case TOKEN_LOGICAL_OR:
            return true;
        case TOKEN_SHIFT_LEFT:
            return true;
        case TOKEN_SHIFT_RIGHT:
            return true;
        case TOKEN_OPEN_GENERIC:
            return true;
        case TOKEN_CLOSE_GENERIC:
            return true;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
    }
    unreachable("");
}

static bool parse_lang_type_struct_atom(Pos* pos, Ulang_type_atom* lang_type, Tk_view* tokens) {
    (void) env;
    memset(lang_type, 0, sizeof(*lang_type));
    Token lang_type_token = {0};
    Str_view mod_alias = {0};

    if (!try_consume(&lang_type_token, tokens, TOKEN_SYMBOL)) {
        return false;
    }

    if (try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        mod_alias = lang_type_token.text;
        if (!try_consume(&lang_type_token, tokens, TOKEN_SYMBOL)) {
            todo();
            return false;
        }
    }

    *pos = lang_type_token.pos;

    lang_type->str = (Uname) {.mod_alias = name_new(env.curr_mod_path, mod_alias, (Ulang_type_vec) {0}, 0), .base = lang_type_token.text};
    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        lang_type->pointer_depth++;
    }

    return true;
}

// type will be parsed if possible
static bool parse_lang_type_struct_tuple(Ulang_type_tuple* lang_type, Tk_view* tokens) {
    Ulang_type_atom atom = {0};
    Ulang_type_vec types = {0};
    bool is_comma = true;

    // TODO: make token naming more consistant
    Token tk_start = {0};
    if (!try_consume(&tk_start, tokens, TOKEN_OPEN_PAR)) {
        log_tokens(LOG_DEBUG, *tokens);
        todo();
    }

    while (is_comma) {
        // a return type is only one token, at least for now
        Pos atom_pos = {0};
        if (!parse_lang_type_struct_atom(&atom_pos, &atom, tokens)) {
            todo();
            //atom.str = (Uname) {.mod_alias = (Str_view) {0}, .base = str_view_from_cstr("void")};
            break;
        }
        Ulang_type new_child = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, atom_pos));
        vec_append(&a_main, &types, new_child);
        is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        log_tokens(LOG_DEBUG, *tokens);
        todo();
    }

    *lang_type = ulang_type_tuple_new(types, tk_start.pos);
    return true;
}

// type will be parsed if possible
static bool parse_lang_type_struct(Ulang_type* lang_type, Tk_view* tokens) {
    memset(lang_type, 0, sizeof(*lang_type));

    Token lang_type_token = {0};
    if (try_consume(&lang_type_token, tokens, TOKEN_FN)) {
        Ulang_type_tuple params = {0};
        if (!parse_lang_type_struct_tuple(&params, tokens)) {
            return false;
        }
        Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type)); // TODO: make function, etc. to call arena_alloc automatically
        if (!parse_lang_type_struct(rtn_type, tokens)) {
            return false;
        }
        *lang_type = ulang_type_fn_const_wrap(ulang_type_fn_new(params, rtn_type, lang_type_token.pos));
        return true;
    }

    Ulang_type_atom atom = {0};
    if (tk_view_front(*tokens).type ==  TOKEN_OPEN_PAR) {
        Ulang_type_tuple new_tuple = {0};
        if (!parse_lang_type_struct_tuple(&new_tuple, tokens)) {
            return false;
        }
        *lang_type = ulang_type_tuple_const_wrap(new_tuple);
        return true;
    }

    Pos pos = {0};
    if (!parse_lang_type_struct_atom(&pos, &atom, tokens)) {
        return false;
    }

    if (tk_view_front(*tokens).type != TOKEN_OPEN_GENERIC) {
        *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, pos));
        return true;
    }

    if (PARSE_OK != parse_generics_args(&atom.str.gen_args, tokens)) {
        return false;
    }
    *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, pos));
    return true;
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_atom_require(Ulang_type_atom* lang_type, Tk_view* tokens) {
    Pos pos = {0};
    if (parse_lang_type_struct_atom(&pos, lang_type, tokens)) {
        return PARSE_OK;
    } else {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_require(Ulang_type* lang_type, Tk_view* tokens) {
    if (parse_lang_type_struct(lang_type, tokens)) {
        return PARSE_OK;
    } else {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
}

static PARSE_EXPR_STATUS parse_function_parameter(Uast_param** child, Tk_view* tokens, bool add_to_sym_table, Scope_id scope_id) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return PARSE_EXPR_NONE;
    }

    Uast_variable_def* base = NULL;
    bool is_optional = false;
    bool is_variadic = false;
    Uast_expr* opt_default = NULL;
    if (PARSE_OK != parse_variable_decl(&base, tokens, false, true, add_to_sym_table, true, (Ulang_type) {0}, scope_id)) {
        return PARSE_EXPR_ERROR;
    }
    if (try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        is_variadic = true;
    }
    if (try_consume(NULL, tokens, TOKEN_SINGLE_EQUAL)) {
        if (is_variadic) {
            // TODO: expected failure case
            todo();
        }
        is_optional = true;
        switch (parse_expr(&opt_default, tokens, false, false, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "after = for optional argument");
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            default:
                unreachable("");
        }
    }
    try_consume(NULL, tokens, TOKEN_COMMA);

    *child = uast_param_new(base->pos, base, is_optional, is_variadic, opt_default);
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_optional_lang_type_parameter(Uast_variable_def** child, Tk_view* tokens, Ulang_type default_lang_type, Scope_id scope_id) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return PARSE_EXPR_NONE;
    }

    Uast_variable_def* param;
    if (PARSE_OK != parse_variable_decl(&param, tokens, false, true, false, false, default_lang_type, scope_id)) {
        return PARSE_EXPR_ERROR;
    }
    try_consume(NULL, tokens, TOKEN_COMMA);

    *child = param;
    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_function_parameters(Uast_function_params** result, Tk_view* tokens, Scope_id scope_id) {
    Uast_param_vec params = {0};

    Uast_param* param = NULL;
    bool done = false;
    while (!done) {
        switch (parse_function_parameter(&param, tokens, true, scope_id)) {
            case PARSE_EXPR_OK:
                vec_append(&a_main, &params, param);
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

    *result = uast_function_params_new(tk_view_front(*tokens).pos, params);
    return PARSE_OK;
}

static PARSE_STATUS parse_function_decl_common(
    Uast_function_decl** fun_decl,
    Tk_view* tokens,
    Scope_id fn_scope,
    Scope_id block_scope
) {
    Token name_token = tk_view_consume(tokens);

    Uast_generic_param_vec gen_params = {0};
    if (tk_view_front(*tokens).type ==  TOKEN_OPEN_GENERIC) {
        if (PARSE_OK != parse_generics_params(&gen_params, tokens)) {
            return PARSE_ERROR;
        }
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), " in function decl", TOKEN_OPEN_PAR);
        return PARSE_ERROR;
    }

    Uast_function_params* params = NULL;
    if (PARSE_OK != parse_function_parameters(&params, tokens, block_scope)) {
        return PARSE_ERROR;
    }

    Token close_par_tk = {0};
    if (tokens->count > 0) {
        if (!try_consume(&close_par_tk, tokens, TOKEN_CLOSE_PAR)) {
            unreachable("message not implemented\n");
        }
    }

    Ulang_type rtn_type = {0};
    if (!parse_lang_type_struct(&rtn_type, tokens)) {
        rtn_type = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new_from_cstr("void", 0), close_par_tk.pos));
    }

    *fun_decl = uast_function_decl_new(name_token.pos, gen_params, params, rtn_type, name_new(env.curr_mod_path, name_token.text, (Ulang_type_vec) {0}, fn_scope));
    if (!usymbol_add(uast_function_decl_wrap(*fun_decl))) {
        return msg_redefinition_of_symbol(uast_function_decl_wrap(*fun_decl));
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_function_def(Uast_function_def** fun_def, Tk_view* tokens) {
    unwrap(try_consume(NULL, tokens, TOKEN_FN));

    Scope_id fn_scope = 0;
    Scope_id block_scope = symbol_collection_new(0);

    Uast_function_decl* fun_decl = NULL;
    if (PARSE_OK != parse_function_decl_common(&fun_decl, tokens, fn_scope, block_scope)) {
        return PARSE_ERROR;
    }

    Uast_block* fun_body = NULL;
    if (PARSE_OK != parse_block(&fun_body, tokens, false, block_scope)) {
        return PARSE_ERROR;
    }

    *fun_def = uast_function_def_new(fun_decl->pos, fun_decl, fun_body);
    usymbol_update(uast_function_def_wrap(*fun_def));
    return PARSE_OK;
}

static PARSE_STATUS parse_generics_params(Uast_generic_param_vec* params, Tk_view* tokens) {
    memset(params, 0, sizeof(*params));
    unwrap(try_consume(NULL, tokens, TOKEN_OPEN_GENERIC));

    do {
        Token symbol = {0};
        if (!try_consume(&symbol, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_SYMBOL);
            return PARSE_ERROR;
        }
        Uast_generic_param* param = uast_generic_param_new(
            symbol.pos,
            uast_symbol_new(
                symbol.pos,
                name_new(env.curr_mod_path, symbol.text, (Ulang_type_vec) {0}, 0 /* TODO */)
            )
        );
        vec_append(&a_main, params, param);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
        );
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_generics_args(Ulang_type_vec* args, Tk_view* tokens) {
    memset(args, 0, sizeof(*args));
    unwrap(try_consume(NULL, tokens, TOKEN_OPEN_GENERIC));

    do {
        Ulang_type arg = {0};
        if (PARSE_ERROR == parse_lang_type_struct_require(&arg, tokens)) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, args, arg);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
        );
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_struct_base_def(
    Ustruct_def_base* base,
    Name name,
    Tk_view* tokens,
    bool require_sub_types,
    Ulang_type default_lang_type,
    Scope_id scope_id
) {
    memset(base, 0, sizeof(*base));
    base->name = name;
    log(LOG_DEBUG, "THING: "TAST_FMT"\n", name_print(name));

    if (tk_view_front(*tokens).type == TOKEN_OPEN_GENERIC) {
        parse_generics_params(&base->generics, tokens);
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE, TOKEN_OPEN_GENERIC);
        return PARSE_ERROR;
    }

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Uast_variable_def* member;
        switch (parse_variable_decl(&member, tokens, false, false, false, require_sub_types, default_lang_type, scope_id)) {
            case PARSE_ERROR:
                return PARSE_ERROR;
            case PARSE_OK:
                break;
            default:
                unreachable("");
        }
        try_consume(NULL, tokens, TOKEN_SEMICOLON);
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
        vec_append(&a_main, &base->members, member);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "to end struct, raw_union, or enum definition", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_struct_base_def_implicit_type(
    Ustruct_def_base* base,
    Name name,
    Tk_view* tokens,
    Ulang_type_atom lang_type
) {
    (void) lang_type;
    base->name = name;

    if (!try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Token name_token = {0};
        if (!try_consume(&name_token, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
            return PARSE_ERROR;
        }
        try_consume(NULL, tokens, TOKEN_SEMICOLON);
        if (!try_consume(NULL, tokens, TOKEN_NEW_LINE) && !try_consume(NULL, tokens, TOKEN_COMMA)) {
            msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_NEW_LINE, TOKEN_COMMA);
            return PARSE_ERROR;
        }

        Uast_variable_def* member = uast_variable_def_new(
            name_token.pos,
            ulang_type_regular_const_wrap(ulang_type_regular_new(lang_type, name_token.pos)),
            name_new(env.curr_mod_path, name_token.text, (Ulang_type_vec) {0}, 0)
        );

        vec_append(&a_main, &base->members, member);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "to end struct, raw_union, or enum definition", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_struct_def(Uast_struct_def** struct_def, Tk_view* tokens, Token name) {
    unwrap(try_consume(NULL, tokens, TOKEN_STRUCT));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def(&base, name_new(env.curr_mod_path, name.text, (Ulang_type_vec) {0}, 0), tokens, true, (Ulang_type) {0}, 0 /* TODO */)) {
        return PARSE_ERROR;
    }

    *struct_def = uast_struct_def_new(name.pos, base);
    if (!usymbol_add(uast_struct_def_wrap(*struct_def))) {
        msg_redefinition_of_symbol(uast_struct_def_wrap(*struct_def));
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_raw_union_def(Uast_raw_union_def** raw_union_def, Tk_view* tokens, Token name) {
    unwrap(try_consume(NULL, tokens, TOKEN_RAW_UNION));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def(&base, name_new(env.curr_mod_path, name.text, (Ulang_type_vec) {0}, 0), tokens, true, (Ulang_type) {0}, 0)) {
        return PARSE_ERROR;
    }

    *raw_union_def = uast_raw_union_def_new(name.pos, base);
    if (!usymbol_add(uast_raw_union_def_wrap(*raw_union_def))) {
        msg_redefinition_of_symbol(uast_raw_union_def_wrap(*raw_union_def));
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_enum_def(Uast_enum_def** enum_def, Tk_view* tokens, Token name) {
    unwrap(try_consume(NULL, tokens, TOKEN_ENUM));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def_implicit_type(
        &base,
        name_new(env.curr_mod_path, name.text, (Ulang_type_vec) {0}, 0),
        tokens,
        ulang_type_atom_new((Uname) {.base = name.text}, 0)
    )) {
        return PARSE_ERROR;
    }

    *enum_def = uast_enum_def_new(name.pos, base);
    if (!usymbol_add(uast_enum_def_wrap(*enum_def))) {
        msg_redefinition_of_symbol(uast_enum_def_wrap(*enum_def));
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_sum_def(Uast_sum_def** sum_def, Tk_view* tokens, Token name) {
    Token sum_tk = {0};
    unwrap(try_consume(&sum_tk, tokens, TOKEN_SUM));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def(
        &base,
        name_new(env.curr_mod_path, name.text, (Ulang_type_vec) {0}, 0),
        tokens,
        false,
        ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new_from_cstr("void", 0), sum_tk.pos)),
        0
    )) {
        return PARSE_ERROR;
    }

    *sum_def = uast_sum_def_new(name.pos, base);
    if (!usymbol_add(uast_sum_def_wrap(*sum_def))) {
        msg_redefinition_of_symbol(uast_sum_def_wrap(*sum_def));
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_lang_def(Uast_lang_def** def, Tk_view* tokens, Token name, Scope_id scope_id) {
    Token lang_def_tk = {0};
    unwrap(try_consume(&lang_def_tk, tokens, TOKEN_DEF));

    Token dummy = {0};
    if (!try_consume(&dummy, tokens, TOKEN_SINGLE_EQUAL)) {
        // TODO
        todo();
    }
    Uast_expr* expr = NULL;
    if (PARSE_EXPR_OK != parse_expr(&expr, tokens, false, false, scope_id)) {
        todo();
    }

    *def = uast_lang_def_new(name.pos, name_new(env.curr_mod_path, name.text, (Ulang_type_vec) {0}, 0), expr);
    if (!usymbol_add(uast_lang_def_wrap(*def))) {
        msg_redefinition_of_symbol(uast_lang_def_wrap(*def));
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_import(Uast_mod_alias** alias, Tk_view* tokens, Token name) {
    Token import_tk = {0};
    unwrap(try_consume(&import_tk, tokens, TOKEN_IMPORT));

    Token dummy = {0};
    if (!try_consume(&dummy, tokens, TOKEN_SINGLE_EQUAL)) {
        // TODO
        todo();
    }

    String mod_path = {0};
    Token path_tk = {0};
    if (!try_consume(&path_tk, tokens, TOKEN_SYMBOL)) {
        // TODO
        todo();
    }
    string_extend_strv(&a_main, &mod_path, path_tk.text);
    Pos mod_path_pos = path_tk.pos;

    while (try_consume(&path_tk, tokens, TOKEN_SINGLE_DOT)) {
        if (!try_consume(&path_tk, tokens, TOKEN_SYMBOL)) {
            // TODO
            todo();
        }
        vec_append(&a_main, &mod_path, PATH_SEPARATOR);
        string_extend_strv(&a_main, &mod_path, path_tk.text);
    }

    if (!get_mod_alias_from_path_token(alias, name, mod_path_pos, string_to_strv(mod_path))) {
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_type_def(Uast_def** def, Tk_view* tokens, Scope_id scope_id) {
    Token name = {0};
    if (!try_consume(&name, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }

    if (starts_with_struct_def(*tokens)) {
        Uast_struct_def* struct_def;
        if (PARSE_OK != parse_struct_def(&struct_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_struct_def_wrap(struct_def);
    } else if (starts_with_raw_union_def(*tokens)) {
        Uast_raw_union_def* raw_union_def;
        if (PARSE_OK != parse_raw_union_def(&raw_union_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_raw_union_def_wrap(raw_union_def);
    } else if (starts_with_enum_def(*tokens)) {
        Uast_enum_def* enum_def;
        if (PARSE_OK != parse_enum_def(&enum_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_enum_def_wrap(enum_def);
    } else if (starts_with_sum_def(*tokens)) {
        Uast_sum_def* sum_def;
        if (PARSE_OK != parse_sum_def(&sum_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_sum_def_wrap(sum_def);
    } else if (starts_with_mod_alias(*tokens)) {
        Uast_mod_alias* import = NULL;
        if (PARSE_OK != parse_import(&import, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_mod_alias_wrap(import);
    } else if (starts_with_lang_def(*tokens)) {
        Uast_lang_def* lang_def = NULL;
        if (PARSE_OK != parse_lang_def(&lang_def, tokens, name, scope_id)) {
            return PARSE_ERROR;
        }
        *def = uast_lang_def_wrap(lang_def);
    } else {
        msg_parser_expected(
            env.file_path_to_text, tk_view_front(*tokens), "",
            TOKEN_STRUCT, TOKEN_RAW_UNION, TOKEN_ENUM, TOKEN_SUM, TOKEN_DEF, TOKEN_IMPORT
        );
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_variable_decl(
    Uast_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool defer_sym_add,
    bool add_to_sym_table,
    bool require_type,
    Ulang_type default_lang_type,
    Scope_id scope_id
) {
    (void) require_let;
    if (!try_consume(NULL, tokens, TOKEN_LET)) {
        assert(!require_let);
    }

    try_consume(NULL, tokens, TOKEN_NEW_LINE);
    Token name_token = {0};
    if (!try_consume(&name_token, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
    try_consume(NULL, tokens, TOKEN_COLON);

    Ulang_type lang_type = {0};
    if (require_type) {
        if (PARSE_OK != parse_lang_type_struct_require(&lang_type, tokens)) {
            return PARSE_ERROR;
        }
    } else {
        if (!parse_lang_type_struct(&lang_type, tokens)) {
            lang_type = default_lang_type;
        }
    }

    Uast_variable_def* variable_def = uast_variable_def_new(
        name_token.pos,
        lang_type,
        name_new(env.curr_mod_path, name_token.text, (Ulang_type_vec) {0}, scope_id)
    );

    if (add_to_sym_table) {
        Uast_def* dummy;
        if (defer_sym_add) {
            usymbol_add_defer(uast_variable_def_wrap(variable_def));
        } else {
            log(LOG_DEBUG, TAST_FMT, uast_variable_def_print(variable_def));
            if (usymbol_lookup(&dummy, variable_def->name)) {
                msg_redefinition_of_symbol(uast_variable_def_wrap(variable_def));
                return PARSE_ERROR;
            }
            usymbol_add(uast_variable_def_wrap(variable_def));
        }
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    *result = variable_def;
    return PARSE_OK;
}

static PARSE_STATUS parse_for_range_internal(
    Uast_block** result,
    Uast_variable_def* var_def,
    Uast_block* outer,
    Tk_view* tokens,
    Scope_id block_scope
) {
    unwrap(try_consume(NULL, tokens, TOKEN_IN));

    Uast_expr* lower_bound = NULL;
    if (PARSE_EXPR_OK != parse_expr(&lower_bound, tokens, true, false, block_scope)) {
        msg_expected_expr(env.file_path_to_text, *tokens, "after in");
        return PARSE_ERROR;
    }

    if (!try_consume(NULL, tokens, TOKEN_DOUBLE_DOT)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "after for loop lower bound", TOKEN_DOUBLE_DOT);
        return PARSE_ERROR;
    }

    Uast_expr* upper_bound = NULL;
    switch (parse_expr(&upper_bound, tokens, true, false, block_scope)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(env.file_path_to_text, *tokens, "after ..");
            return PARSE_ERROR;
        default:
            unreachable("");
    }

    Uast_block* inner = NULL;
    if (PARSE_OK != parse_block(&inner, tokens, false, block_scope)) {
        return PARSE_ERROR;
    }

    Uast_assignment* init_assign = uast_assignment_new(
        uast_expr_get_pos(lower_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
        lower_bound
    );
    vec_append(&a_main, &outer->children, uast_assignment_wrap(init_assign));

    Name incre_name = name_new(env.curr_mod_path, util_literal_name_new(), (Ulang_type_vec) {0}, 0);

    Uast_for_with_cond* inner_for = uast_for_with_cond_new(
        outer->pos,
        uast_condition_new(
            outer->pos,
            uast_binary_wrap(uast_binary_new(
                outer->pos,
                uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
                upper_bound,
                BINARY_LESS_THAN
            ))
        ),
        inner,
        incre_name,
        true
    );
    vec_append(&a_main, &outer->children, uast_for_with_cond_wrap(inner_for));

    Uast_continue* cont = uast_continue_new(uast_expr_get_pos(upper_bound));
    vec_append(&a_main, &inner->children, uast_continue_wrap(cont));

    Uast_label* incre_label = uast_label_new(uast_expr_get_pos(upper_bound), incre_name.base);
    vec_append(&a_main, &inner->children, uast_label_wrap(incre_label));

    Uast_assignment* increment = uast_assignment_new(
        uast_expr_get_pos(upper_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
        uast_operator_wrap(uast_binary_wrap(uast_binary_new(
            var_def->pos,
            uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
            uast_literal_wrap(util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, uast_expr_get_pos(upper_bound))),
            BINARY_ADD
        )))
    );
    vec_append(&a_main, &inner->children, uast_assignment_wrap(increment));
    log(LOG_DEBUG, TAST_FMT, uast_block_print(outer));

    *result = outer;
    return PARSE_OK;
}

static PARSE_STATUS parse_for_with_cond(Uast_for_with_cond** for_new, Pos pos, Tk_view* tokens, Scope_id block_scope) {
    *for_new = uast_for_with_cond_new(pos, NULL, NULL, (Name) {0}, false);
    
    switch (parse_condition(&(*for_new)->condition, tokens, block_scope)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            msg_expected_expr(env.file_path_to_text, *tokens, "");
            return PARSE_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        default:
            unreachable("");
    }
    return parse_block(&(*for_new)->body, tokens, false, block_scope);
}

static PARSE_STATUS parse_for_loop(Uast_stmt** result, Tk_view* tokens) {
    todo();
    //Token for_token;
    //unwrap(try_consume(&for_token, tokens, TOKEN_FOR));

    //Scope_id block_scope = symbol_collection_new();
    //
    if (starts_with_variable_type_decl(*tokens, false)) {
    //    todo();
    //    //PARSE_STATUS status = PARSE_OK;
    //    //Uast_block* outer = uast_block_new(for_token.pos, (Uast_stmt_vec) {0}, (Symbol_collection) {0}, for_token.pos, scope_id_new());
    //    //Uast_variable_def* var_def = NULL;
    //    //if (PARSE_OK != parse_variable_decl(&var_def, tokens, false, false, true, true, (Ulang_type) {0}, block_scope)) {
    //    //    status = PARSE_ERROR;
    //    //    goto for_range_error;
    //    //}

    //    //Uast_block* new_for = NULL;
    //    //if (PARSE_OK != parse_for_range_internal(&new_for, var_def, outer, tokens, block_scope)) {
    //    //    status = PARSE_ERROR;
    //    //    goto for_range_error;
    //    //}
    //    //*result = uast_block_wrap(new_for);

for_range_error:
        todo();
        //return status;
    } else {
        todo();
        //Uast_for_with_cond* new_for = NULL;
        //if (PARSE_OK != parse_for_with_cond(&new_for, for_token.pos, tokens, block_scope)) {
        //    return PARSE_ERROR;
        //}
        //*result = uast_for_with_cond_wrap(new_for);
    }

    todo();
    //return PARSE_OK;
}

static PARSE_STATUS parse_break(Uast_break** new_break, Tk_view* tokens, Scope_id scope_id) {
    Token break_token = tk_view_consume(tokens);

    Uast_expr* break_expr = NULL;
    bool do_break_expr = true;
    switch (parse_expr(&break_expr, tokens, true, false, scope_id)) {
        case PARSE_EXPR_OK:
            do_break_expr = true;
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            do_break_expr = false;
            break;
        default:
            unreachable("");
    }

    *new_break = uast_break_new(break_token.pos, do_break_expr, break_expr);
    return PARSE_OK;
}

static Uast_continue* parse_continue(Tk_view* tokens) {
    Token continue_token = tk_view_consume(tokens);
    Uast_continue* cont_stmt = uast_continue_new(continue_token.pos);
    return cont_stmt;
}

static PARSE_STATUS parse_function_decl(Uast_function_decl** fun_decl, Tk_view* tokens) {
    PARSE_STATUS status = PARSE_ERROR;

    unwrap(try_consume(NULL, tokens, TOKEN_EXTERN));
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in function decl", TOKEN_OPEN_PAR);
        goto error;
    }
    Token extern_type_token;
    if (!try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in function decl", TOKEN_STRING_LITERAL);
        goto error;
    }
    if (!str_view_cstr_is_equal(extern_type_token.text, "c")) {
        msg(
            LOG_ERROR, EXPECT_FAIL_INVALID_EXTERN_TYPE, env.file_path_to_text, extern_type_token.pos,
            "invalid extern type `"STR_VIEW_FMT"`\n", str_view_print(extern_type_token.text)
        );
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in function decl", TOKEN_CLOSE_PAR);
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_FN)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "in function decl", TOKEN_FN);
        goto error;
    }
    if (PARSE_OK != parse_function_decl_common(fun_decl, tokens, 0, SIZE_MAX /* TODO */)) {
        goto error;
    }
    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    status = PARSE_OK;
error:
    usymbol_ignore_defered();
    return status;
}

static PARSE_STATUS parse_literal(Uast_literal** lit, Tk_view* tokens) {
    Token token = tk_view_consume(tokens);
    assert(token_is_literal(token));

    if (util_try_uast_literal_new_from_strv(lit, token.text, token.type, token.pos)) {
        return PARSE_OK;
    }
    return PARSE_ERROR;
}

static Uast_symbol* parse_symbol(Tk_view* tokens, Scope_id scope_id) {
    Token token = tk_view_consume(tokens);
    assert(token.type == TOKEN_SYMBOL);

    return uast_symbol_new(token.pos, name_new(env.curr_mod_path, token.text, (Ulang_type_vec) {0}, scope_id));
}

static PARSE_STATUS parse_function_call(Uast_function_call** child, Tk_view* tokens, Uast_expr* callee, Scope_id scope_id) {
    //Token fun_name_token;
    //if (!try_consume(&fun_name_token, tokens, TOKEN_SYMBOL)) {
    //    unreachable("this is not a function call");
    //}
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        unreachable("this is not a function call");
    }

    bool is_first_time = true;
    bool prev_is_comma = false;
    Uast_expr_vec args = {0};
    while (is_first_time || prev_is_comma) {
        Uast_expr* arg;
        switch (parse_expr(&arg, tokens, true, false, scope_id)) {
            case PARSE_EXPR_OK:
                assert(arg);
                vec_append(&a_main, &args, arg);
                prev_is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
                break;
            case PARSE_EXPR_NONE:
                if (prev_is_comma) {
                    msg_expected_expr(env.file_path_to_text, *tokens, "");
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
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "", TOKEN_CLOSE_PAR, TOKEN_COMMA);
        return PARSE_ERROR;
    }

    *child = uast_function_call_new(uast_expr_get_pos(callee), args, callee);
    return PARSE_OK;
}

static PARSE_STATUS parse_function_return(Uast_return** rtn_stmt, Tk_view* tokens, Scope_id scope_id) {
    unwrap(try_consume(NULL, tokens, TOKEN_RETURN));

    Uast_expr* expr;
    switch (parse_expr(&expr, tokens, false, true, scope_id)) {
        case PARSE_EXPR_OK:
            *rtn_stmt = uast_return_new(uast_expr_get_pos(expr), expr, false);
            break;
        case PARSE_EXPR_NONE:
            *rtn_stmt = uast_return_new(
                prev_token.pos,
                uast_literal_wrap(util_uast_literal_new_from_strv(
                    
                    str_view_from_cstr(""),
                    TOKEN_VOID,
                    prev_token.pos
                )),
                false
            );
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);
    return PARSE_OK;
}

static PARSE_EXPR_STATUS parse_condition(Uast_condition** result, Tk_view* tokens, Scope_id scope_id) {
    Uast_expr* cond_child;
    switch (parse_expr(&cond_child, tokens, false, false, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_NONE:
            return PARSE_EXPR_NONE;
        default:
            unreachable("");
    }

    Uast_operator* cond_oper = NULL;

    // TODO: make helper function that does this
    switch (cond_child->type) {
        case UAST_OPERATOR:
            cond_oper = uast_operator_unwrap(cond_child);
            break;
        case UAST_LITERAL:
            cond_oper = uast_condition_get_default_child(cond_child);
            break;
        case UAST_FUNCTION_CALL:
            cond_oper = uast_condition_get_default_child(cond_child);
            break;
        case UAST_SYMBOL:
            cond_oper = uast_condition_get_default_child(cond_child);
            break;
        default:
            unreachable(UAST_FMT"\n", uast_expr_print(cond_child));
    }

    *result = uast_condition_new(uast_expr_get_pos(cond_child), cond_oper);
    return PARSE_EXPR_OK;
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

static PARSE_STATUS parse_if_else_chain(Uast_if_else_chain** if_else_chain, Tk_view* tokens, Scope_id scope_id) {
    todo();
    //Token if_start_token;
    //unwrap(try_consume(&if_start_token, tokens, TOKEN_IF));

    //Uast_if_vec ifs = {0};

    //Uast_if* if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);
    //if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);
    //
    //switch (parse_condition(&if_stmt->condition, tokens, scope_id)) {
    //    case PARSE_EXPR_OK:
    //        break;
    //    case PARSE_EXPR_ERROR:
    //        return PARSE_ERROR;
    //    case PARSE_EXPR_NONE:
    //        msg_expected_expr(env.file_path_to_text, *tokens, "");
    //        return PARSE_ERROR;
    //    default:
    //        unreachable("");
    //}
    //if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, scope_id_new())) {
    //    return PARSE_ERROR;
    //}
    //vec_append(&a_main, &ifs, if_stmt);

    //if_else_chain_consume_newline(tokens);
    //while (try_consume(NULL, tokens, TOKEN_ELSE)) {
    //    if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);

    //    if (try_consume(&if_start_token, tokens, TOKEN_IF)) {
    //        switch (parse_condition(&if_stmt->condition, tokens, scope_id)) {
    //            case PARSE_EXPR_OK:
    //                break;
    //            case PARSE_EXPR_ERROR:
    //                return PARSE_ERROR;
    //            case PARSE_EXPR_NONE:
    //                msg_expected_expr(env.file_path_to_text, *tokens, "");
    //                return PARSE_ERROR;
    //            default:
    //                unreachable("");
    //        }
    //    } else {
    //        if_stmt->condition = uast_condition_new(if_start_token.pos, NULL);
    //        if_stmt->condition->child = uast_condition_get_default_child( uast_literal_wrap(
    //            util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_start_token.pos)
    //        ));
    //    }

    //    if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, scope_id_new())) {
    //        return PARSE_ERROR;
    //    }
    //    vec_append(&a_main, &ifs, if_stmt);

    //    if_else_chain_consume_newline(tokens);
    //}

    //*if_else_chain = uast_if_else_chain_new(if_start_token.pos, ifs);
    //return PARSE_OK;
}


static PARSE_STATUS parse_switch(Uast_switch** lang_switch, Tk_view* tokens, Scope_id scope_id) {
    Token start_token = {0};
    unwrap(try_consume(&start_token, tokens, TOKEN_SWITCH));

    Uast_expr* operand = NULL;
    switch (parse_expr(&operand, tokens, false, false, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(env.file_path_to_text, *tokens, "");
            return PARSE_ERROR;
        default:
            unreachable("");
    }

    unwrap(try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE));
    unwrap(try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Uast_case_vec cases = {0};

    while (1) {
        Uast_stmt* case_if_true = NULL;
        Uast_expr* case_operand = NULL;
        bool case_is_default = false;
        if (try_consume(NULL, tokens, TOKEN_CASE)) {
            switch (parse_expr(&case_operand, tokens, false, false, scope_id)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_ERROR:
                    return PARSE_ERROR;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(env.file_path_to_text, *tokens, "");
                    return PARSE_ERROR;
                default:
                    unreachable("");
            }
        } else if (try_consume(NULL, tokens, TOKEN_DEFAULT)) {
            case_is_default = true;
        } else {
            break;
        }

        Token case_start_token = {0};
        unwrap(try_consume(&case_start_token, tokens, TOKEN_COLON));
        switch (parse_stmt(&case_if_true, tokens, false, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "");
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        Uast_case* curr_case = uast_case_new(
            case_start_token.pos,
            case_is_default,
            case_operand,
            case_if_true
        );
        vec_append(&a_main, &cases, curr_case);
    }

    *lang_switch = uast_switch_new(start_token.pos, operand, cases);
    unwrap(try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE));
    return PARSE_OK;
}

static Uast_expr* get_expr_or_symbol(Uast_stmt* stmt) {
    if (stmt->type == UAST_DEF) {
        return uast_symbol_wrap(uast_symbol_new(
            uast_stmt_get_pos(stmt),
            uast_variable_def_unwrap(uast_def_unwrap(stmt))->name
        ));
    }

    return uast_expr_unwrap(stmt);
}

static PARSE_EXPR_STATUS parse_stmt(Uast_stmt** child, Tk_view* tokens, bool defer_sym_add, Scope_id scope_id) {
    log(LOG_DEBUG, BOOL_FMT"\n", bool_print(defer_sym_add));
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Uast_stmt* lhs = NULL;
    if (starts_with_type_def(*tokens)) {
        assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));
        unwrap(try_consume(NULL, tokens, TOKEN_TYPE_DEF));
        Uast_def* fun_decl;
        if (PARSE_OK != parse_type_def(&fun_decl, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(fun_decl);
    } else if (starts_with_function_decl(*tokens)) {
        Uast_function_decl* fun_decl;
        if (PARSE_OK != parse_function_decl(&fun_decl, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(uast_function_decl_wrap(fun_decl));
    } else if (starts_with_function_def(*tokens)) {
        Uast_function_def* fun_def;
        if (PARSE_OK != parse_function_def(&fun_def, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(uast_function_def_wrap(fun_def));
    } else if (starts_with_return(*tokens)) {
        Uast_return* rtn_stmt = NULL;
        if (PARSE_OK != parse_function_return(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_return_wrap(rtn_stmt);
    } else if (starts_with_for(*tokens)) {
        if (PARSE_OK != parse_for_loop(&lhs, tokens)) {
            return PARSE_EXPR_ERROR;
        }
    } else if (starts_with_break(*tokens)) {
        Uast_break* rtn_stmt = NULL;
        if (PARSE_OK != parse_break(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_break_wrap(rtn_stmt);
    } else if (starts_with_continue(*tokens)) {
        lhs = uast_continue_wrap(parse_continue(tokens));
    } else if (starts_with_block(*tokens)) {
        Uast_block* block_def = NULL;
        if (PARSE_OK != parse_block(&block_def, tokens, false, symbol_collection_new(scope_id))) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_block_wrap(block_def);
    } else if (starts_with_variable_decl(*tokens)) {
        Uast_variable_def* var_def = NULL;
        if (PARSE_OK != parse_variable_decl(&var_def, tokens, true, defer_sym_add, true, true, (Ulang_type) {0}, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(uast_variable_def_wrap(var_def));
    } else {
        Uast_expr* lhs_ = NULL;
        switch (parse_expr(&lhs_, tokens, false, true, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                return PARSE_EXPR_NONE;
            default:
                unreachable("");
        }

        lhs = uast_expr_wrap(lhs_);
    }

    Token equal_tk = {0};
    if (try_consume(&equal_tk, tokens, TOKEN_SINGLE_EQUAL)) {
        Uast_expr* rhs = NULL;
        switch (parse_expr(&rhs, tokens, false, false, scope_id)) {
            case PARSE_EXPR_OK:
                *child = uast_expr_wrap(uast_operator_wrap(uast_binary_wrap(uast_binary_new(
                    equal_tk.pos, get_expr_or_symbol(lhs), rhs, BINARY_SINGLE_EQUAL
                ))));
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "");
                return PARSE_EXPR_ERROR;
            default:
                unreachable("");
        }
    } else {
        *child = lhs;
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);
    if (tokens->count < 1 || !try_consume(NULL, tokens, TOKEN_NEW_LINE)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT, env.file_path_to_text, tk_view_front(*tokens).pos,
            "expected newline after stmt\n"
        );
        return PARSE_EXPR_ERROR;
    }
    assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_block(Uast_block** block, Tk_view* tokens, bool is_top_level, Scope_id new_scope) {
    PARSE_STATUS status = PARSE_OK;
    bool did_consume_close_brace = false;

    *block = uast_block_new(
        tk_view_front(*tokens).pos,
        (Uast_stmt_vec) {0},
        (Pos) {0},
        new_scope
    );

    if (tokens->count < 1) {
        // TODO: expected test case
        unreachable("empty file not implemented\n");
    }

    Token open_brace_token = {0};
    if (!is_top_level && !try_consume(&open_brace_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "at start of block", TOKEN_OPEN_CURLY_BRACE);
        status = PARSE_ERROR;
        goto end;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Token close_brace_token = {0};
    while (1) {
        if (try_consume(&close_brace_token, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
            did_consume_close_brace = true;
            assert(close_brace_token.pos.line > 0);
            (*block)->pos_end = close_brace_token.pos;
            assert((*block)->pos_end.line > 0);
            break;
        }
        if (tokens->count < 1) {
            // this means that there is no matching `}` found
            if (!is_top_level) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE, env.file_path_to_text, open_brace_token.pos, 
                    "opening `{` is unmatched\n"
                );
                status = PARSE_ERROR;
            }
            break;
        }
        Uast_stmt* child;
        bool should_stop = false;
        switch (parse_stmt(&child, tokens, false, new_scope)) {
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
        assert(!try_consume(NULL, tokens, TOKEN_SEMICOLON) && !try_consume(NULL, tokens, TOKEN_NEW_LINE));
        vec_append(&a_main, &(*block)->children, child);
    }

end:
    if (did_consume_close_brace) {
        unwrap((*block)->pos_end.line > 0);
    } else if (!is_top_level && status == PARSE_OK) {
        unreachable("");
    }

    assert(*block);
    return status;
}

static PARSE_STATUS parse_struct_literal_members(Uast_expr_vec* members, Tk_view* tokens, Scope_id scope_id) {
    memset(members, 0, sizeof(*members));

    try_consume(NULL, tokens, TOKEN_NEW_LINE);
    do {
        try_consume(NULL, tokens, TOKEN_NEW_LINE);
        Uast_expr* memb = NULL;
        switch (parse_expr(&memb, tokens, false, false, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "");
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        vec_append(&a_main, members, memb);
        try_consume(NULL, tokens, TOKEN_NEW_LINE);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    return PARSE_OK;
}


static PARSE_STATUS parse_struct_literal(Uast_struct_literal** struct_lit, Tk_view* tokens, Scope_id scope_id) {
    Token start_token;
    if (!try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "at start of struct literal", TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }

    Uast_expr_vec members = {0};
    PARSE_STATUS status = parse_struct_literal_members(&members, tokens, scope_id);
    if (status != PARSE_OK) {
        return status;
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        if (members.info.count > 0) {
            msg_parser_expected(
                env.file_path_to_text,
                tk_view_front(*tokens),
                "after expression in struct literal",
                TOKEN_COMMA,
                TOKEN_NEW_LINE,
                TOKEN_CLOSE_CURLY_BRACE
            );
        } else {
            msg_parser_expected(
                env.file_path_to_text,
                tk_view_front(*tokens),
                "in struct literal",
                TOKEN_SINGLE_DOT,
                TOKEN_CLOSE_CURLY_BRACE
            );
        }
        return PARSE_ERROR;
    }

    *struct_lit = uast_struct_literal_new(start_token.pos, members, util_literal_name_new());
    return PARSE_OK;
}

static PARSE_STATUS parse_array_literal(Uast_array_literal** arr_lit, Tk_view* tokens, Scope_id scope_id) {
    Token start_token;
    if (!try_consume(&start_token, tokens, TOKEN_OPEN_SQ_BRACKET)) {
        msg_parser_expected(env.file_path_to_text, tk_view_front(*tokens), "at start of array literal", TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }
    
    Uast_expr_vec members = {0};
    PARSE_STATUS status = parse_struct_literal_members(&members, tokens, scope_id);
    if (status != PARSE_OK) {
        return status;
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
        if (members.info.count > 0) {
            msg_parser_expected(
                env.file_path_to_text,
                tk_view_front(*tokens),
                "after expression in array literal",
                TOKEN_COMMA,
                TOKEN_NEW_LINE,
                TOKEN_CLOSE_SQ_BRACKET
            );
        } else {
            msg_parser_expected(
                env.file_path_to_text,
                tk_view_front(*tokens),
                "in array literal",
                TOKEN_SINGLE_DOT,
                TOKEN_CLOSE_SQ_BRACKET
            );
        }
        return PARSE_ERROR;
    }

    *arr_lit = uast_array_literal_new(start_token.pos, members, util_literal_name_new());
    return PARSE_OK;
}

static Uast_binary* parser_binary_new(Uast_expr* lhs, Token operator_token, Uast_expr* rhs) {
    return uast_binary_new(operator_token.pos, lhs, rhs, token_type_to_binary_type(operator_token.type));
}

static PARSE_EXPR_STATUS parse_expr_piece(
    Uast_expr** result, Tk_view* tokens, int32_t* prev_oper_pres, bool defer_sym_add, Scope_id scope_id
) {
    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }

    Token open_par_token = {0};
    if (try_consume(&open_par_token, tokens, TOKEN_OPEN_PAR)) {
        switch (parse_expr(result, tokens, defer_sym_add, false ,scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                // TODO: remove this if if it causes too many issues
                if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
                    msg(
                        LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_PAR, env.file_path_to_text, open_par_token.pos, 
                        "opening `(` is unmatched\n"
                    );
                }
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "");
                return PARSE_EXPR_ERROR;
        }
        if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_MISSING_CLOSE_PAR, env.file_path_to_text, open_par_token.pos, 
                "opening `(` is unmatched\n"
            );
            return PARSE_EXPR_ERROR;
        }
        *prev_oper_pres = get_operator_precedence(TOKEN_SINGLE_DOT);
    } else if (token_is_literal(tk_view_front(*tokens))) {
        Uast_literal* lit = NULL;
        if (PARSE_OK != parse_literal(&lit, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_literal_wrap(lit);
    } else if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        *result = uast_symbol_wrap(parse_symbol(tokens, scope_id));
    } else if (starts_with_switch(*tokens)) {
        Uast_switch* lang_switch = NULL;
        if (PARSE_OK != parse_switch(&lang_switch, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_switch_wrap(lang_switch);
    } else if (starts_with_if(*tokens)) {
        Uast_if_else_chain* if_else = NULL;
        if (PARSE_OK != parse_if_else_chain(&if_else, tokens ,scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_if_else_chain_wrap(if_else);
    } else if (starts_with_struct_literal(*tokens)) {
        Uast_struct_literal* struct_lit;
        if (PARSE_OK != parse_struct_literal(&struct_lit, tokens ,scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_struct_literal_wrap(struct_lit);
    } else if (starts_with_array_literal(*tokens)) {
        Uast_array_literal* arr_lit;
        if (PARSE_OK != parse_array_literal(&arr_lit, tokens ,scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_array_literal_wrap(arr_lit);
    } else if (tk_view_front(*tokens).type == TOKEN_NONTYPE) {
        return PARSE_EXPR_NONE;
    } else {
        return PARSE_EXPR_NONE;
    }

    return PARSE_EXPR_OK;
}

static bool expr_is_unary(const Uast_expr* expr) {
    if (expr->type != UAST_OPERATOR) {
        return false;
    }
    const Uast_operator* operator = uast_operator_const_unwrap(expr);
    return operator->type == UAST_UNARY;
}

static bool expr_is_binary(const Uast_expr* expr) {
    if (expr->type != UAST_OPERATOR) {
        return false;
    }
    const Uast_operator* operator = uast_operator_const_unwrap(expr);
    return operator->type == UAST_BINARY;
}

static PARSE_EXPR_STATUS parse_unary(
    Uast_operator** result,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    bool can_be_tuple,
    Scope_id scope_id
) {
    (void) can_be_tuple;

    Token oper = consume_operator(tokens);
    unwrap(is_unary(oper.type));

    *prev_oper_pres = get_operator_precedence(oper.type);

    Uast_expr* child = NULL;
    Ulang_type_atom unary_lang_type = ulang_type_atom_new_from_cstr("i32", 0);

    switch (oper.type) {
        case TOKEN_NOT:
            break;
        case TOKEN_DEREF:
            break;
        case TOKEN_REFER:
            break;
        case TOKEN_SINGLE_MINUS:
            break;
        case TOKEN_UNSAFE_CAST: {
            {
                Token temp;
                if (!try_consume(&temp, tokens, TOKEN_LESS_THAN)) {
                    msg_parser_expected(env.file_path_to_text, temp, "", TOKEN_LESS_THAN);
                    return PARSE_EXPR_ERROR;
                }
            }
            if (PARSE_OK != parse_lang_type_struct_atom_require(&unary_lang_type, tokens)) {
                return PARSE_EXPR_ERROR;
            }
            {
                Token temp;
                if (!try_consume(&temp, tokens, TOKEN_GREATER_THAN)) {
                    msg_parser_expected(env.file_path_to_text, temp, "", TOKEN_GREATER_THAN);
                    return PARSE_EXPR_ERROR;
                }
            }
            break;
        }
        default:
            unreachable(TOKEN_FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    switch (parse_expr_piece(&child, tokens, prev_oper_pres, defer_sym_add, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            return PARSE_EXPR_NONE;
        default:
            todo();
    }

    switch (oper.type) {
        case TOKEN_NOT:
            // fallthrough
        case TOKEN_DEREF:
            // fallthrough
        case TOKEN_REFER:
            // fallthrough
        case TOKEN_UNSAFE_CAST:
            *result = uast_unary_wrap(uast_unary_new(oper.pos, child, token_type_to_unary_type(oper.type), ulang_type_regular_const_wrap(ulang_type_regular_new(unary_lang_type, oper.pos))));
            assert(*result);
            break;
        case TOKEN_SINGLE_MINUS: {
            *result = uast_binary_wrap(uast_binary_new(oper.pos, uast_literal_wrap(uast_number_wrap(uast_number_new(oper.pos, 0))), child, token_type_to_binary_type(oper.type)));
            assert(*result);
            break;
        }
        default:
            unreachable(TOKEN_FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_binary(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    bool can_be_tuple,
    Scope_id scope_id
) {
    Token oper = consume_operator(tokens);

    bool is_equal_thing = false;
    if (try_consume(NULL, tokens, TOKEN_SINGLE_EQUAL)) {
        is_equal_thing = true;
    }

    unwrap(is_binary(oper.type));

    Uast_expr* rhs = NULL;

    if (is_unary(tk_view_front(*tokens).type)) {
        int32_t unary_pres = get_operator_precedence(tk_view_front(*tokens).type);
        log_tokens(LOG_DEBUG, *tokens);
        Uast_operator* unary = NULL;
        switch (parse_unary(&unary, tokens, prev_oper_pres, defer_sym_add, can_be_tuple, scope_id)) {
            case PARSE_EXPR_OK:
                rhs = uast_operator_wrap(unary);
                break;
            case PARSE_EXPR_ERROR:
                todo();
            case PARSE_EXPR_NONE:
                todo();
            default:
                todo();
        }

        unwrap(
            (unary_pres > get_operator_precedence(oper.type)) &&
            "case of unary operator having lower or equal precedence than adjacent binary not implemented"
        );

        rhs = uast_operator_wrap(unary);
        log_tokens(LOG_DEBUG, *tokens);
        log(LOG_DEBUG, TAST_FMT, uast_expr_print(rhs));
        //if (is_unary(tk_view_front(*tokens).type)) {
        //    todo();
        //}
    } else {
        switch (parse_expr_piece(&rhs, tokens, prev_oper_pres, defer_sym_add, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(env.file_path_to_text, *tokens, "");
                return PARSE_EXPR_ERROR;
            default:
                unreachable("");
        }
    }

    switch (oper.type) {
        case TOKEN_SINGLE_DOT:
            *result = uast_member_access_wrap(uast_member_access_new(oper.pos, uast_symbol_unwrap(rhs), lhs));
            break;
        case TOKEN_SINGLE_PLUS:
            // fallthrough
        case TOKEN_SINGLE_MINUS:
            // fallthrough
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_SLASH:
            // fallthrough
        case TOKEN_MODULO:
            // fallthrough
        case TOKEN_GREATER_OR_EQUAL:
            // fallthrough
        case TOKEN_LESS_OR_EQUAL:
            // fallthrough
        case TOKEN_LESS_THAN:
            // fallthrough
        case TOKEN_GREATER_THAN:
            // fallthrough
        case TOKEN_NOT_EQUAL:
            // fallthrough
        case TOKEN_BITWISE_AND:
            // fallthrough
        case TOKEN_BITWISE_OR:
            // fallthrough
        case TOKEN_BITWISE_XOR:
            // fallthrough
        case TOKEN_LOGICAL_AND:
            // fallthrough
        case TOKEN_LOGICAL_OR:
            // fallthrough
        case TOKEN_SHIFT_LEFT:
            // fallthrough
        case TOKEN_SHIFT_RIGHT:
            // fallthrough
        case TOKEN_SINGLE_EQUAL:
            // fallthrough
        case TOKEN_DOUBLE_EQUAL:
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, lhs, rhs, token_type_to_binary_type(oper.type))));
            break;
        case TOKEN_COMMA:
            if (lhs->type == UAST_TUPLE) {
                vec_append(&a_main, &uast_tuple_unwrap(lhs)->members, rhs);
                *result = lhs;
            } else {
                Uast_expr_vec members = {0};
                vec_append(&a_main, &members, lhs);
                vec_append(&a_main, &members, rhs);
                *result = uast_tuple_wrap(uast_tuple_new(
                    oper.pos,
                    members
                ));
            }
            break;
        default:
            unreachable(TOKEN_FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    if (is_equal_thing) {
        todo();
        //*result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(
        //    oper.pos, uast_expr_clone(lhs, scope_id_new()), *result, BINARY_SINGLE_EQUAL
        //)));
        //return PARSE_EXPR_OK;
    } else {
        assert(*result);
        *prev_oper_pres = get_operator_precedence(oper.type);
        return PARSE_EXPR_OK;
    }
    unreachable("");
}

static Uast_expr* get_right_child_operator(Uast_operator* oper) {
    switch (oper->type) {
        case UAST_UNARY:
            return uast_unary_unwrap(oper)->child;
        case UAST_BINARY:
            return uast_binary_unwrap(oper)->rhs;
        default:
            unreachable("");
    }
    unreachable("");
}

static Uast_expr* get_right_child_expr(Uast_expr* expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            return get_right_child_operator(uast_operator_unwrap(expr));
        case UAST_TUPLE: {
            Uast_tuple* tuple = uast_tuple_unwrap(expr);
            unwrap(tuple->members.info.count > 0);
            return vec_at(&tuple->members, tuple->members.info.count - 1);
        }
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(expr);
            return access->callee;
        }
        default:
            unreachable("");
    }
    unreachable("");
}


static void set_right_child_operator(Uast_operator* oper, Uast_expr* new_expr) {
    switch (oper->type) {
        case UAST_UNARY:
            uast_unary_unwrap(oper)->child = new_expr;
            return;
        case UAST_BINARY:
            uast_binary_unwrap(oper)->rhs = new_expr;
            return;
        default:
            unreachable("");
    }
    unreachable("");
}

static void set_right_child_expr(Uast_expr* expr, Uast_expr* new_expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            set_right_child_operator(uast_operator_unwrap(expr), new_expr);
            return;
        case UAST_TUPLE: {
            Uast_tuple* tuple = uast_tuple_unwrap(expr);
            unwrap(tuple->members.info.count > 0);
            *vec_at_ref(&tuple->members, tuple->members.info.count - 1) = new_expr;
            return;
        }
        case UAST_MEMBER_ACCESS: {
            uast_member_access_unwrap(expr)->callee = new_expr;
            return;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

static PARSE_EXPR_STATUS parse_expr_function_call(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id
) {
    Uast_function_call* fun_call = NULL;
    switch (lhs->type) {
        case UAST_SYMBOL:
            switch (parse_function_call(&fun_call, tokens, lhs ,scope_id)) {
                case PARSE_OK:
                    *result = uast_function_call_wrap(fun_call);
                    lhs = *result;
                    return PARSE_EXPR_OK;
                case PARSE_ERROR:
                    return PARSE_EXPR_ERROR;
                default:
                    unreachable("");
            }
            unreachable("");
        case UAST_OPERATOR:
            // fallthrough
        case UAST_MEMBER_ACCESS:
            switch (parse_function_call(&fun_call, tokens, lhs, scope_id)) {
                case PARSE_OK:
                    lhs = uast_function_call_wrap(fun_call);
                    *result = lhs;
                    return PARSE_EXPR_OK;
                case PARSE_ERROR:
                    return PARSE_EXPR_ERROR;
                default:
                    unreachable("");
            }
            unreachable("");
        case UAST_TUPLE: {
            Uast_tuple* tuple = uast_tuple_unwrap(lhs);
            unwrap(tuple->members.info.count > 0);
            Uast_expr* new_last = NULL;
            PARSE_EXPR_STATUS status = parse_expr_function_call(&new_last, vec_top(&tuple->members), tokens, scope_id);
            if (status != PARSE_EXPR_OK) {
                return status;
            }
            vec_rem_last(&tuple->members);
            vec_append(&a_main, &tuple->members, new_last);
            *result = uast_tuple_wrap(tuple);
            return PARSE_EXPR_OK;
        }
            unreachable(UAST_FMT, uast_expr_print(lhs));
        default:
            unreachable(UAST_FMT, uast_expr_print(lhs));
    }
    unreachable("");
}

static PARSE_EXPR_STATUS parse_expr_index(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    Scope_id scope_id
) {
    Token oper = consume_operator(tokens);
    unwrap(oper.type == TOKEN_OPEN_SQ_BRACKET);

    Uast_expr* index_index = NULL;
    switch (parse_expr(&index_index, tokens, prev_oper_pres, defer_sym_add, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            return PARSE_EXPR_NONE;
        default:
            todo();
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
        msg(
            LOG_ERROR,
            EXPECT_FAIL_MISSING_CLOSE_SQ_BRACKET,
            env.file_path_to_text,
            tk_view_front(*tokens).pos,
            "expected closing `]` after expr\n"
        );
        return PARSE_EXPR_ERROR;
    }

    *result = uast_index_wrap(uast_index_new(oper.pos, index_index, lhs));
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_expr_generic(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add
) {
    (void) result;
    (void) prev_oper_pres;
    (void) defer_sym_add;

    Uast_symbol* sym = NULL;

    switch (lhs->type) {
        case UAST_SYMBOL:
            sym = uast_symbol_unwrap(lhs);
            break;
        case UAST_MEMBER_ACCESS:
            sym = uast_member_access_unwrap(lhs)->member_name;
            break;
        default:
            todo();
    }

    if (PARSE_OK != parse_generics_args(&sym->name.gen_args, tokens)) {
        return PARSE_EXPR_ERROR;
    }
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_expr_opening_prev_less_pres(
    Uast_expr** result,
    Uast_expr** lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    Scope_id scope_id
) {
    switch (tk_view_front(*tokens).type) {
        case TOKEN_OPEN_PAR: {
            Uast_expr* new_right = NULL;
            PARSE_EXPR_STATUS status = parse_expr_function_call(&new_right, get_right_child_expr(*lhs), tokens, scope_id);
            if (status != PARSE_EXPR_OK) {
                return status;
            }
            assert(new_right);
            set_right_child_expr(*result, new_right);
            return PARSE_EXPR_OK;
        }
        case TOKEN_OPEN_SQ_BRACKET: {
            Uast_expr* new_right = NULL;
            PARSE_EXPR_STATUS status = parse_expr_index(&new_right, get_right_child_expr(*lhs), tokens, prev_oper_pres, defer_sym_add, scope_id);
            if (status != PARSE_EXPR_OK) {
                return status;
            }
            assert(new_right);
            set_right_child_expr(*result, new_right);
            return PARSE_EXPR_OK;
        }
        default:
            unreachable(TOKEN_FMT, token_print(TOKEN_MODE_LOG, tk_view_front(*tokens)));
    }
    unreachable("");
}

static PARSE_EXPR_STATUS parse_expr_opening_prev_equal_pres(
    Uast_expr** result,
    Uast_expr** lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    Scope_id scope_id
) {
    switch (tk_view_front(*tokens).type) {
        case TOKEN_OPEN_PAR: {
            return parse_expr_function_call(result, *lhs, tokens, scope_id);
        }
        case TOKEN_OPEN_SQ_BRACKET: {
            return parse_expr_index(result, *lhs, tokens, prev_oper_pres, defer_sym_add, scope_id);
        }
        case TOKEN_OPEN_GENERIC: {
            return parse_expr_generic(result, *lhs, tokens, prev_oper_pres, defer_sym_add);
        }
        default:
            unreachable(TOKEN_FMT, token_print(TOKEN_MODE_LOG, tk_view_front(*tokens)));
    }
    unreachable("");
}

static PARSE_EXPR_STATUS parse_expr_opening(
    Uast_expr** result,
    Uast_expr** lhs,
    Tk_view* tokens,
    int32_t* prev_oper_pres,
    bool defer_sym_add,
    Scope_id scope_id
) {
    if (*prev_oper_pres < get_operator_precedence(tk_view_front(*tokens).type)) {
        return parse_expr_opening_prev_less_pres(result, lhs, tokens, prev_oper_pres, defer_sym_add, scope_id);
    } else {
        return parse_expr_opening_prev_equal_pres(result, lhs, tokens, prev_oper_pres, defer_sym_add, scope_id);
    }
}

// parse expression, ignoring single equal
static PARSE_EXPR_STATUS parse_expr_side(
    Uast_expr** result,
    Tk_view* tokens,
    bool defer_sym_add,
    bool can_be_tuple,
    Scope_id scope_id
) {
    Uast_expr* lhs = NULL;

    int32_t prev_oper_pres = INT32_MAX;

    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }
    
    if (tk_view_front(*tokens).type == TOKEN_SINGLE_DOT) {
        lhs = uast_unknown_wrap(uast_unknown_new(tk_view_front(*tokens).pos));
    } else if (is_unary(tk_view_front(*tokens).type)) {
        prev_oper_pres = get_operator_precedence(tk_view_front(*tokens).type);
        Uast_operator* unary = NULL;
        switch (parse_unary(&unary, tokens, &prev_oper_pres, defer_sym_add, can_be_tuple, scope_id)) {
            case PARSE_EXPR_OK:
                lhs = uast_operator_wrap(unary);
                *result = lhs;
                break;
            case PARSE_EXPR_ERROR:
                // TODO
                todo();
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                // TODO
                todo();
                return PARSE_EXPR_NONE;
            default:
                unreachable("");
        }
    } else {
        switch (parse_expr_piece(&lhs, tokens, &prev_oper_pres, defer_sym_add, scope_id)) {
            case PARSE_EXPR_OK:
                *result = lhs;
                break;
            case PARSE_EXPR_NONE:
                return PARSE_EXPR_NONE;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            default:
                todo();
        }
    }

    if (!token_is_operator(tk_view_front(*tokens), can_be_tuple)) {
        if (lhs) {
            *result = lhs;
            return PARSE_EXPR_OK;
        } else {
            todo();
        }
    }

    while (token_is_operator(tk_view_front(*tokens), can_be_tuple)) {
        if (!is_binary(tk_view_front(*tokens).type)) {
            todo();
        } else if (token_is_opening(tk_view_front(*tokens))) {
            switch (parse_expr_opening(result, &lhs, tokens, &prev_oper_pres, defer_sym_add, scope_id)) {
                case PARSE_EXPR_OK:
                    lhs = *result;
                    break;
                case PARSE_EXPR_NONE:
                    todo();
                case PARSE_EXPR_ERROR:
                    return PARSE_EXPR_ERROR;
                default:
                    unreachable("");
            }
        } else {
            log(LOG_DEBUG, "prev_oper_pres: %"PRIi32"\n", prev_oper_pres);
            log(LOG_DEBUG, "curr_oper_pres: %"PRIi32"\n", get_operator_precedence(tk_view_front(*tokens).type));
            if (prev_oper_pres < get_operator_precedence(tk_view_front(*tokens).type)) {
                prev_oper_pres = get_operator_precedence(tk_view_front(*tokens).type);
                Uast_expr* binary = NULL;
                switch (parse_binary(
                    &binary,
                    get_right_child_expr(lhs), tokens, &prev_oper_pres, defer_sym_add, can_be_tuple, scope_id)
                ) {
                    case PARSE_EXPR_OK:
                        set_right_child_expr(lhs, binary);
                        *result = lhs;
                        assert(*result);
                        break;
                    case PARSE_EXPR_NONE:
                        todo();
                    case PARSE_EXPR_ERROR:
                        return PARSE_EXPR_ERROR;
                    default:
                        todo();
                }
            } else {
                prev_oper_pres = get_operator_precedence(tk_view_front(*tokens).type);
                Uast_expr* binary = NULL;
                switch (parse_binary(&binary, lhs, tokens, &prev_oper_pres, defer_sym_add, can_be_tuple, scope_id)) {
                    case PARSE_EXPR_OK:
                        *result = binary;
                        lhs = *result;
                        assert(*result);
                        break;
                    case PARSE_EXPR_NONE:
                        return PARSE_EXPR_NONE;
                    case PARSE_EXPR_ERROR:
                        return PARSE_EXPR_ERROR;
                    default:
                        todo();
                }
            }
        }
    }

    assert(*result);
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_expr(
    Uast_expr** result,
    Tk_view* tokens,
    bool defer_sym_add,
    bool can_be_tuple,
    Scope_id scope_id
) {
    Uast_expr* lhs = NULL;
    PARSE_EXPR_STATUS status = parse_expr_side(&lhs, tokens, defer_sym_add, can_be_tuple, scope_id);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    Token equal_tk = {0};
    if (!try_consume(&equal_tk, tokens, TOKEN_SINGLE_EQUAL)) {
        *result = lhs;
        return PARSE_EXPR_OK;
    }
    
    // TODO: remove BINARY_SINGLE_EQUAL?
    Uast_expr* rhs = NULL;
    switch (parse_expr_side(&rhs, tokens, defer_sym_add, can_be_tuple ,scope_id)) {
        case PARSE_EXPR_OK:
            // TODO: do uast_assignment?
            //*result = uast_assignment_wrap(uast_assignment_new(equal_tk.pos, uast_expr_wrap(lhs), rhs));
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(equal_tk.pos, lhs, rhs, BINARY_SINGLE_EQUAL)));
            return PARSE_EXPR_OK;
        case PARSE_EXPR_NONE:
            msg_expected_expr(env.file_path_to_text, *tokens, "after `=`");
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_ERROR;
    }
    unreachable("");
}

static void parser_do_tests(void);

bool parse_file(Uast_block** block, Str_view file_path, bool do_new_sym_coll) {
    bool status = true;
#ifndef DNDEBUG
    // TODO: reenable
    //parser_do_tests();
#endif // DNDEBUG

    Str_view* file_con = arena_alloc(&a_main, sizeof(*file_con));
    if (!read_file(file_con, file_path)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos,
            "could not open file `"STR_VIEW_FMT"`: %s\n",
            str_view_print(file_path), strerror(errno)
        );
        status = false;
        goto error;
    }
    unwrap(file_path_to_text_tbl_add(&env.file_path_to_text, file_con, file_path) && "parse_file should not be called with the same file path more than once");

    Token_vec tokens = {0};
    if (!tokenize(&tokens, file_path)) {
        status = false;
        goto error;
    }
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    if (PARSE_OK != parse_block(block, &token_view, true, symbol_collection_new(0))) {
        status = false;
        goto error;
    }
    log(LOG_DEBUG, "done with parsing:\n");
    symbol_log(LOG_TRACE, (*block)->scope_id);

error:
    return status;
}

// TODO: put this in src/tokens.h or whatever
static inline Tk_view tokens_to_tk_view(Token_vec tokens) {
    Tk_view tk_view = {.tokens = tokens.buf, .count = tokens.info.count};
    return tk_view;
}

static void parser_test_parse_expr(const char* input, int test) {
    (void) input;
    (void) test;
    todo();
    //log(LOG_DEBUG, "start parser_test_%d:\n", test);
    //Env env = {0};
    //Str_view file_text = str_view_from_cstr(input);
    //env.file_path_to_text = file_text;

    //Token_vec tokens_ = {0};
    //unwrap(tokenize(&tokens_, & (Str_view) {0}));
    //Tk_view tokens = tokens_to_tk_view(tokens_);
    //log_tokens(LOG_DEBUG, tokens);
    //Uast_expr* result = NULL;
    //switch (parse_expr(& &result, &tokens, false, false)) {
    //    case PARSE_EXPR_OK:
    //        log(LOG_DEBUG, "\n"TAST_FMT, uast_expr_print(result));
    //        break;
    //    default:
    //        unreachable("");
    //}
    //log(LOG_DEBUG, "end parser_test_%d:\n\n", test);
}

static void parser_test_parse_stmt(const char* input, int test) {
    (void) input;
    (void) test;
    log(LOG_DEBUG, "start parser_test_%d:\n", test);
    //Env env = {0};
    //Str_view file_text = str_view_from_cstr(input);
    //env.file_text = file_text;

    //Token_vec tokens_ = {0};
    //unwrap(tokenize(&tokens_, & (Str_view) {0}));
    //Tk_view tokens = tokens_to_tk_view(tokens_);
    //log_tokens(LOG_DEBUG, tokens);
    //Uast_stmt* result = NULL;
    //switch (parse_stmt(& &result, &tokens, true)) {
    //    case PARSE_EXPR_OK:
    //        log(LOG_DEBUG, "\n"TAST_FMT, uast_stmt_print(result));
    //        break;
    //    default:
    //        assert(error_count > 0);
    //        unreachable("");
    //}
    //log(LOG_DEBUG, "end parser_test_%d:\n\n", test);
}

static void parser_do_tests(void) {
    // TODO: automate checking if these tests fail
    // TODO: expected failures?
    int test = 1;
    parser_test_parse_expr("4\n", test++);
    parser_test_parse_expr("5*3\n", test++);
    parser_test_parse_expr("num = 5\n", test++);
    parser_test_parse_expr("deref(num) = 5*2\n", test++);
    parser_test_parse_expr("deref(num) = deref(5)\n", test++);
    parser_test_parse_expr("deref(num) + deref(5)/6\n", test++);
    parser_test_parse_expr("deref(num) = 2*num\n", test++);
    parser_test_parse_expr("deref(num) = 2 + num*3\n", test++);
    parser_test_parse_expr("deref(num) = 2*num + 1\n", test++);

    parser_test_parse_stmt("deref(num) = 2*num + 1\n", test++);
    parser_test_parse_stmt("let num i32\n", test++);
    parser_test_parse_stmt("let num i32 = 0\n", test++);
    parser_test_parse_stmt("num += deref(num2)\n", test++);
}
