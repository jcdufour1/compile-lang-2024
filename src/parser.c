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
#include <name.h>
#include <ulang_type_clone.h>

static Strv curr_mod_path;

static Token prev_token;

// TODO: make consume_expect function to print error automatically

// TODO: use parent block for scope_ids instead of function calls everytime

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

static bool can_end_stmt(Token token);

static PARSE_STATUS parse_block(Uast_block** block, Tk_view* tokens, bool is_top_level, Scope_id scope_id);

static PARSE_EXPR_STATUS parse_stmt(Uast_stmt** child, Tk_view* tokens, Scope_id scope_id);

static PARSE_EXPR_STATUS parse_expr(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id
);

static PARSE_STATUS parse_variable_def(
    Uast_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool add_to_sym_table,
    bool require_type,
    Ulang_type lang_type_if_not_required,
    Scope_id scope_id
);

static PARSE_EXPR_STATUS parse_condition(Uast_condition**, Tk_view* tokens, Scope_id scope_id);

static PARSE_STATUS parse_generics_args(Ulang_type_vec* args, Tk_view* tokens, Scope_id scope_id);

static PARSE_STATUS parse_generics_params(Uast_generic_param_vec* params, Tk_view* tokens, Scope_id block_scope);

static PARSE_STATUS parse_expr_generic(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id
);

static PARSE_STATUS parse_expr_index(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id,
    Pos oper_pos
);

static PARSE_EXPR_STATUS parse_generic_binary(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id,
    size_t bin_idx, // idx of BIN_IDX_TO_TOKEN_TYPES that will be used in this function invocation
    int depth
);

static bool is_unary(TOKEN_TYPE token_type);

static bool prev_is_newline(void) {
    return prev_token.type == TOKEN_NEW_LINE || prev_token.type == TOKEN_SEMICOLON;
}

// TODO: inline this function?
static bool try_consume_internal(Token* result, Tk_view* tokens, bool allow_any_type, TOKEN_TYPE type, bool rm_newlines) {
    Token temp = {0};
    if (allow_any_type) {
        temp = tk_view_consume(tokens);
    } else {
        if (!tk_view_try_consume(&temp, tokens, type)) {
            return false;
        }
    }
    prev_token = temp;
    if (result) {
        *result = temp;
    }
    if (rm_newlines && !can_end_stmt(prev_token)) {
        while (try_consume_internal(NULL, tokens, false, TOKEN_SEMICOLON, false) || try_consume_internal(NULL, tokens, false, TOKEN_NEW_LINE, false));
    }
    return true;
}

static Token consume(Tk_view* tokens) {
    Token result = {0};
    unwrap(try_consume_internal(&result, tokens, true, TOKEN_NONTYPE, true));
    return result;
}

// also will automatically remove semicolon and newline if present
static bool try_consume(Token* result, Tk_view* tokens, TOKEN_TYPE type) {
    return try_consume_internal(result, tokens, false, type, true);
}

static bool try_consume_no_rm_newlines(Token* result, Tk_view* tokens, TOKEN_TYPE type) {
    return try_consume_internal(result, tokens, false, type, false);
}

static bool try_consume_newlines(Tk_view* tokens) {
    Token dummy = {0};
    bool is_newline = false;
    if (can_end_stmt(tk_view_front(*tokens))) {
        while (try_consume_internal(&dummy, tokens, false, TOKEN_NEW_LINE, false) || try_consume_internal(&dummy, tokens, false, TOKEN_SEMICOLON, false)) {
            is_newline = true;
        }
    }
    return is_newline;
}

static bool try_consume_1_of_4(Token* result, Tk_view* tokens, TOKEN_TYPE a, TOKEN_TYPE b, TOKEN_TYPE c, TOKEN_TYPE d) {
    if (try_consume(result, tokens, a)) {
        return true;
    }
    if (try_consume(result, tokens, b)) {
        return true;
    }
    if (try_consume(result, tokens, c)) {
        return true;
    }
    if (try_consume(result, tokens, d)) {
        return true;
    }
    return false;
}

static bool try_peek_1_of_4(Token* result, Tk_view* tokens, TOKEN_TYPE a, TOKEN_TYPE b, TOKEN_TYPE c, TOKEN_TYPE d) {
    if (tk_view_front(*tokens).type == a) {
        *result = tk_view_front(*tokens);
        return true;
    }
    if (tk_view_front(*tokens).type == b) {
        return true;
    }
    if (tk_view_front(*tokens).type == c) {
        return true;
    }
    if (tk_view_front(*tokens).type == d) {
        return true;
    }
    return false;
}

static bool try_consume_if_not(Token* result, Tk_view* tokens, TOKEN_TYPE type) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == type) {
        return false;
    }
    if (result) {
        *result = consume(tokens);
    }
    prev_token = *result;
    return true;
}

static Token consume_unary(Tk_view* tokens) {
    Token result = {0};
    unwrap(try_consume_if_not(&result, tokens, TOKEN_EOF));
    try_consume_newlines(tokens);
    assert(is_unary(result.type) && "there is a bug somewhere in the parser");
    return result;
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

static void msg_expected_expr_internal(const char* file, int line, Tk_view tokens, const char* msg_suffix) {
    String message = {0};
    string_extend_cstr(&a_print, &message, "expected expression ");
    string_extend_cstr(&a_print, &message, msg_suffix);

    msg_internal(file, line, DIAG_EXPECTED_EXPRESSION, get_curr_pos(tokens), FMT"\n", string_print(message)); \
}

#define msg_expected_expr(tokens, msg_suffix) \
    msg_expected_expr_internal(__FILE__, __LINE__, tokens, msg_suffix)

static void msg_parser_expected_internal(const char* file, int line, Token got, const char* msg_suffix, int count_expected, ...) {
    va_list args;
    va_start(args, count_expected);

    String message = {0};
    string_extend_cstr(&a_print, &message, "got token `");
    string_extend_strv(&a_print, &message, token_print_internal(&a_print, TOKEN_MODE_MSG, got));
    string_extend_cstr(&a_print, &message, "`, but expected ");

    for (int idx = 0; idx < count_expected; idx++) {
        if (idx > 0) {
            if (idx == count_expected - 1) {
                string_extend_cstr(&a_print, &message, " or ");
            } else {
                string_extend_cstr(&a_print, &message, ", ");
            }
        }
        string_extend_cstr(&a_print, &message, "`");
        string_extend_strv(&a_print, &message, token_type_to_strv(TOKEN_MODE_MSG, va_arg(args, TOKEN_TYPE)));
        string_extend_cstr(&a_print, &message, "` ");
    }

    string_extend_cstr(&a_print, &message, msg_suffix);

    DIAG_TYPE expect_fail_type = DIAG_PARSER_EXPECTED;
    if (got.type == TOKEN_NONTYPE) {
        expect_fail_type = DIAG_INVALID_TOKEN;
    }
    msg_internal(file, line, expect_fail_type, got.pos, FMT"\n", strv_print(string_to_strv(message)));

    va_end(args);
}

#define msg_parser_expected(got, msg_suffix, ...) \
    do { \
        msg_parser_expected_internal(__FILE__, __LINE__, got, msg_suffix, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), __VA_ARGS__); \
    } while(0)

static PARSE_STATUS msg_redefinition_of_symbol(const Uast_def* new_sym_def) {
    msg(
        DIAG_REDEFINITION_SYMBOL, uast_def_get_pos(new_sym_def),
        "redefinition of symbol `"FMT"`\n", name_print(NAME_MSG, uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg(
        DIAG_NOTE, uast_def_get_pos(original_def),
        "`"FMT"` originally defined here\n", name_print(NAME_MSG, uast_def_get_name(original_def))
    );

    return PARSE_ERROR;
}

static bool get_mod_alias_from_path_token(Uast_mod_alias** mod_alias, Token alias_tk, Pos mod_path_pos, Strv mod_path) {
    bool status = true;
    Uast_def* prev_def = NULL;
    String file_path = {0};
    Strv old_mod_path = curr_mod_path;
    curr_mod_path = mod_path;
    if (usymbol_lookup(&prev_def, name_new((Strv) {0}, mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL /* TODO */))) {
        goto finish;
    }

    string_extend_strv(&a_main, &file_path, mod_path);
    string_extend_cstr(&a_main, &file_path, ".own");
    Uast_block* block = NULL;
    if (!parse_file(&block, string_to_strv(file_path))) {
        status = false;
        goto finish;
    }

    unwrap(usym_tbl_add(uast_import_path_wrap(uast_import_path_new(
        mod_path_pos,
        block,
        name_new((Strv) {0}, mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL)
    ))));

finish:
    curr_mod_path = old_mod_path;
    *mod_alias = uast_mod_alias_new(
        alias_tk.pos,
        name_new(curr_mod_path, alias_tk.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
        name_new(curr_mod_path, mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL)
    );
    unwrap(usymbol_add(uast_mod_alias_wrap(*mod_alias)));
    return status;
}

static bool starts_with_mod_alias_in_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_IMPORT;
}

static bool starts_with_struct_def_in_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_STRUCT;
}

static bool starts_with_raw_union_def_in_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_RAW_UNION;
}

static bool starts_with_enum_def_in_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_ENUM;
}

static bool starts_with_type_def_in_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_TYPE_DEF;
}

static bool starts_with_lang_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_DEF;
}

static bool starts_with_defer(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_DEFER;
}

static bool starts_with_function_decl(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_EXTERN;
}

static bool starts_with_function_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_FN;
}

static bool starts_with_return(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_RETURN;
}

static bool starts_with_if(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_IF;
}

static bool starts_with_switch(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_SWITCH;
}

static bool starts_with_for(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_FOR;
}

static bool starts_with_break(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_BREAK;
}

static bool starts_with_continue(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_CONTINUE;
}

static bool starts_with_block(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_OPEN_CURLY_BRACE;
}

static bool starts_with_function_call(Tk_view tokens) {
    return try_consume(NULL, &tokens, TOKEN_SYMBOL) && try_consume(NULL, &tokens, TOKEN_OPEN_PAR);
}

static bool starts_with_variable_def(Tk_view tokens) {
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
    bool is_repeat = false;
    while (tokens->count > 0) {
        Token prev = {0};
        if (try_consume(&prev, tokens, TOKEN_OPEN_CURLY_BRACE)) {
            bracket_depth++;
        } else if (try_consume(&prev, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
            if (bracket_depth == 0) {
                return;
            }
            bracket_depth--;
        } else if (is_repeat && !try_consume_if_not(&prev, tokens, TOKEN_EOF)) {
            return;
        }
        //assert(token_is_equal(prev, prev_token) && "prev_token is not being updated properly");
        assert(bracket_depth >= 0);
        is_repeat = true;

        if (!prev_is_newline() || bracket_depth != 0) {
            continue;
        }

        if (
            starts_with_type_def_in_def(*tokens) ||
            starts_with_function_decl(*tokens) ||
            starts_with_function_def(*tokens) ||
            starts_with_return(*tokens) ||
            starts_with_if(*tokens) ||
            starts_with_for(*tokens) ||
            starts_with_break(*tokens) ||
            starts_with_function_call(*tokens) ||
            starts_with_variable_def(*tokens)
        ) {
            return;
        }
    }
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
        case TOKEN_UNSAFE_CAST:
            return false;
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_INT_LITERAL:
            return true;
        case TOKEN_FLOAT_LITERAL:
            return true;
        case TOKEN_VOID:
            return true;
        case TOKEN_NEW_LINE:
            return true;
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
        case TOKEN_ENUM:
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
        case TOKEN_EOF:
            return true;
        case TOKEN_ASSIGN_BY_BIN:
            return false;
        case TOKEN_MACRO:
            return true;
        case TOKEN_DEFER:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
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
            return true;
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
        case TOKEN_FLOAT_LITERAL:
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
        case TOKEN_ENUM:
            return false;
        case TOKEN_MODULO:
            return false;
        case TOKEN_BITWISE_AND:
            return true;
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
        case TOKEN_EOF:
            return false;
        case TOKEN_ASSIGN_BY_BIN:
            return false;
        case TOKEN_MACRO:
            return false;
        case TOKEN_DEFER:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

static bool parse_lang_type_struct_atom(Pos* pos, Ulang_type_atom* lang_type, Tk_view* tokens, Scope_id scope_id) {
    (void) env;
    memset(lang_type, 0, sizeof(*lang_type));
    Token lang_type_token = {0};
    Strv mod_alias = {0};

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

    lang_type->str = uname_new(
        name_new(curr_mod_path, mod_alias, (Ulang_type_vec) {0}, scope_id),
        lang_type_token.text,
        (Ulang_type_vec) {0},
        scope_id
    );
    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        lang_type->pointer_depth++;
    }

    return true;
}

// type will be parsed if possible
static bool parse_lang_type_struct_tuple(Ulang_type_tuple* lang_type, Tk_view* tokens, Scope_id scope_id) {
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
        if (!parse_lang_type_struct_atom(&atom_pos, &atom, tokens, scope_id)) {
            todo();
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
static bool parse_lang_type_struct(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id) {
    memset(lang_type, 0, sizeof(*lang_type));

    Token lang_type_token = {0};
    if (try_consume(&lang_type_token, tokens, TOKEN_FN)) {
        Ulang_type_tuple params = {0};
        if (!parse_lang_type_struct_tuple(&params, tokens, scope_id)) {
            return false;
        }
        Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type));
        if (!parse_lang_type_struct(rtn_type, tokens, scope_id)) {
            return false;
        }
        *lang_type = ulang_type_fn_const_wrap(ulang_type_fn_new(params, rtn_type, lang_type_token.pos));
        return true;
    }

    Ulang_type_atom atom = {0};
    if (tk_view_front(*tokens).type ==  TOKEN_OPEN_PAR) {
        Ulang_type_tuple new_tuple = {0};
        if (!parse_lang_type_struct_tuple(&new_tuple, tokens, scope_id)) {
            return false;
        }
        *lang_type = ulang_type_tuple_const_wrap(new_tuple);
        return true;
    }

    Pos pos = {0};
    if (!parse_lang_type_struct_atom(&pos, &atom, tokens, scope_id)) {
        return false;
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_GENERIC)) {
        *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, pos));
        return true;
    }

    if (PARSE_OK != parse_generics_args(&atom.str.gen_args, tokens, scope_id)) {
        return false;
    }
    *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, pos));
    return true;
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_atom_require(Ulang_type_atom* lang_type, Tk_view* tokens, Scope_id scope_id) {
    Pos pos = {0};
    if (parse_lang_type_struct_atom(&pos, lang_type, tokens, scope_id)) {
        return PARSE_OK;
    } else {
        msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_require(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id) {
    if (parse_lang_type_struct(lang_type, tokens, scope_id)) {
        return PARSE_OK;
    } else {
        msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
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
    if (PARSE_OK != parse_variable_def(&base, tokens, false, add_to_sym_table, true, (Ulang_type) {0}, scope_id)) {
        return PARSE_EXPR_ERROR;
    }
    if (try_consume(NULL, tokens, TOKEN_TRIPLE_DOT)) {
        is_variadic = true;
    }
    Token tk_equal = {0};
    if (try_consume(&tk_equal, tokens, TOKEN_SINGLE_EQUAL)) {
        if (is_variadic) {
            msg(
                DIAG_OPTIONAL_ARGS_FOR_VARIADIC_ARGS, tk_equal.pos,
                "optional arguments cannot be used with variadic parameters\n"
            );
            return PARSE_EXPR_ERROR;
        }
        is_optional = true;
        switch (parse_expr(&opt_default, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "after = for optional argument");
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

static PARSE_STATUS parse_function_parameters(Uast_function_params** result, Tk_view* tokens, bool add_to_sym_tbl, Scope_id scope_id) {
    Uast_param_vec params = {0};

    Uast_param* param = NULL;
    bool done = false;
    while (!done) {
        switch (parse_function_parameter(&param, tokens, add_to_sym_tbl, scope_id)) {
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
    bool add_to_sym_table,
    Scope_id fn_scope,
    Scope_id block_scope
) {
    Token name_token = consume(tokens);

    Uast_generic_param_vec gen_params = {0};
    if (tk_view_front(*tokens).type ==  TOKEN_OPEN_GENERIC) {
        if (PARSE_OK != parse_generics_params(&gen_params, tokens, block_scope)) {
            return PARSE_ERROR;
        }
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(tk_view_front(*tokens), " in function decl", TOKEN_OPEN_PAR);
        return PARSE_ERROR;
    }

    Uast_function_params* params = NULL;
    if (PARSE_OK != parse_function_parameters(&params, tokens, add_to_sym_table, block_scope)) {
        return PARSE_ERROR;
    }

    Token close_par_tk = {0};
    if (tokens->count > 0) {
        if (!try_consume_no_rm_newlines(&close_par_tk, tokens, TOKEN_CLOSE_PAR)) {
            unreachable("message not implemented\n");
        }
    }

    Ulang_type rtn_type = {0};
    if (!parse_lang_type_struct(&rtn_type, tokens, fn_scope)) {
        rtn_type = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new_from_cstr("void", 0), close_par_tk.pos));
    }

    *fun_decl = uast_function_decl_new(name_token.pos, gen_params, params, rtn_type, name_new(curr_mod_path, name_token.text, (Ulang_type_vec) {0}, fn_scope));
    if (!usymbol_add(uast_function_decl_wrap(*fun_decl))) {
        return msg_redefinition_of_symbol(uast_function_decl_wrap(*fun_decl));
    }

    // nessessary because if return type in decl is any*, * is not considered to end stmt
    try_consume_newlines(tokens);

    return PARSE_OK;
}

static PARSE_STATUS parse_function_def(Uast_function_def** fun_def, Tk_view* tokens) {
    unwrap(try_consume(NULL, tokens, TOKEN_FN));

    Scope_id fn_scope = SCOPE_TOP_LEVEL;
    Scope_id block_scope = symbol_collection_new(fn_scope);

    Uast_function_decl* fun_decl = NULL;
    if (PARSE_OK != parse_function_decl_common(&fun_decl, tokens, true, fn_scope, block_scope)) {
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

static PARSE_STATUS parse_generics_params(Uast_generic_param_vec* params, Tk_view* tokens, Scope_id block_scope) {
    memset(params, 0, sizeof(*params));
    unwrap(try_consume(NULL, tokens, TOKEN_OPEN_GENERIC));

    do {
        Token symbol = {0};
        if (!try_consume(&symbol, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
            return PARSE_ERROR;
        }
        Uast_generic_param* param = uast_generic_param_new(
            symbol.pos,
            uast_symbol_new(
                symbol.pos,
                name_new(curr_mod_path, symbol.text, (Ulang_type_vec) {0}, block_scope)
            )
        );
        vec_append(&a_main, params, param);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
        );
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_generics_args(Ulang_type_vec* args, Tk_view* tokens, Scope_id scope_id) {
    memset(args, 0, sizeof(*args));

    do {
        Ulang_type arg = {0};
        if (PARSE_ERROR == parse_lang_type_struct_require(&arg, tokens, scope_id)) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, args, arg);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
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
    Ulang_type default_lang_type
) {
    memset(base, 0, sizeof(*base));
    base->name = name;

    if (tk_view_front(*tokens).type == TOKEN_OPEN_GENERIC) {
        parse_generics_params(&base->generics, tokens, name.scope_id);
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE, TOKEN_OPEN_GENERIC);
        return PARSE_ERROR;
    }

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Uast_variable_def* member = NULL;
        switch (parse_variable_def(&member, tokens, false, false, require_sub_types, default_lang_type, name.scope_id)) {
            case PARSE_ERROR:
                return PARSE_ERROR;
            case PARSE_OK:
                break;
            default:
                unreachable("");
        }
        if (require_sub_types) {
            while (try_consume_newlines(tokens));
        } else {
            while (try_consume(NULL, tokens, TOKEN_COMMA) || try_consume_newlines(tokens));
        }
        vec_append(&a_main, &base->members, member);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "to end struct, raw_union, or enum definition", TOKEN_CLOSE_CURLY_BRACE);
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
        msg_parser_expected(tk_view_front(*tokens), "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Token name_token = {0};
        if (!try_consume(&name_token, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
            return PARSE_ERROR;
        }
        try_consume(NULL, tokens, TOKEN_SEMICOLON);
        if (!try_consume_newlines(tokens) && !try_consume(NULL, tokens, TOKEN_COMMA)) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_NEW_LINE, TOKEN_COMMA);
            return PARSE_ERROR;
        }

        Uast_variable_def* member = uast_variable_def_new(
            name_token.pos,
            ulang_type_regular_const_wrap(ulang_type_regular_new(lang_type, name_token.pos)),
            name_new(curr_mod_path, name_token.text, (Ulang_type_vec) {0}, 0)
        );

        vec_append(&a_main, &base->members, member);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "to end struct, raw_union, or enum definition", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_struct_def(Uast_struct_def** struct_def, Tk_view* tokens, Token name) {
    unwrap(try_consume(NULL, tokens, TOKEN_STRUCT));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def(&base, name_new(curr_mod_path, name.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL), tokens, true, (Ulang_type) {0})) {
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
    if (PARSE_OK != parse_struct_base_def(&base, name_new(curr_mod_path, name.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL), tokens, true, (Ulang_type) {0})) {
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
    Token enum_tk = {0};
    unwrap(try_consume(&enum_tk, tokens, TOKEN_ENUM));

    Ustruct_def_base base = {0};
    if (PARSE_OK != parse_struct_base_def(
        &base,
        name_new(curr_mod_path, name.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
        tokens,
        false,
        ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new_from_cstr("void", 0), enum_tk.pos))
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

static PARSE_STATUS parse_lang_def(Uast_lang_def** def, Tk_view* tokens, Token name, Scope_id scope_id) {
    Token lang_def_tk = {0};
    unwrap(try_consume(&lang_def_tk, tokens, TOKEN_DEF));

    Token dummy = {0};
    if (!try_consume(&dummy, tokens, TOKEN_SINGLE_EQUAL)) {
        msg_parser_expected(tk_view_front(*tokens), "after `def`", TOKEN_SINGLE_EQUAL);
        return PARSE_ERROR;
    }
    Uast_expr* expr = NULL;
    switch (parse_expr(&expr, tokens, scope_id)) {
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "after `=` in def definition");
            return PARSE_ERROR;
        case PARSE_EXPR_OK:
            break;
        default:
            unreachable("");
    }

    *def = uast_lang_def_new(name.pos, name_new(curr_mod_path, name.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL), expr);
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
        msg_parser_expected(tk_view_front(*tokens), "after `import`", TOKEN_SINGLE_EQUAL);
        return PARSE_ERROR;
    }

    String mod_path = {0};
    Token path_tk = {0};
    if (!try_consume(&path_tk, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(tk_view_front(*tokens), "after =", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }
    string_extend_strv(&a_main, &mod_path, path_tk.text);
    Pos mod_path_pos = path_tk.pos;

    while (try_consume(&path_tk, tokens, TOKEN_SINGLE_DOT)) {
        if (!try_consume(&path_tk, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(tk_view_front(*tokens), "after . in module path", TOKEN_SYMBOL);
            return PARSE_ERROR;
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
        msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
        return PARSE_ERROR;
    }

    if (starts_with_struct_def_in_def(*tokens)) {
        Uast_struct_def* struct_def;
        if (PARSE_OK != parse_struct_def(&struct_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_struct_def_wrap(struct_def);
    } else if (starts_with_raw_union_def_in_def(*tokens)) {
        Uast_raw_union_def* raw_union_def;
        if (PARSE_OK != parse_raw_union_def(&raw_union_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_raw_union_def_wrap(raw_union_def);
    } else if (starts_with_enum_def_in_def(*tokens)) {
        Uast_enum_def* enum_def;
        if (PARSE_OK != parse_enum_def(&enum_def, tokens, name)) {
            return PARSE_ERROR;
        }
        *def = uast_enum_def_wrap(enum_def);
    } else if (starts_with_mod_alias_in_def(*tokens)) {
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
            tk_view_front(*tokens), "",
            TOKEN_STRUCT, TOKEN_RAW_UNION, TOKEN_ENUM, TOKEN_DEF, TOKEN_IMPORT
        );
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_variable_def(
    Uast_variable_def** result,
    Tk_view* tokens,
    bool require_let,
    bool add_to_sym_table,
    bool require_type,
    Ulang_type default_lang_type,
    Scope_id scope_id
) {
    (void) require_let;
    if (!try_consume(NULL, tokens, TOKEN_LET)) {
        assert(!require_let);
    }

    try_consume_newlines(tokens);
    Token name_token = {0};
    if (!try_consume_no_rm_newlines(&name_token, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
        assert(tokens->count > 0);
        return PARSE_ERROR;
    }
    try_consume_no_rm_newlines(NULL, tokens, TOKEN_COLON);

    Ulang_type lang_type = {0};
    if (require_type) {
        if (PARSE_OK != parse_lang_type_struct_require(&lang_type, tokens, scope_id)) {
            return PARSE_ERROR;
        }
    } else {
        if (!parse_lang_type_struct(&lang_type, tokens, scope_id)) {
            lang_type = default_lang_type;
        }
    }

    Uast_variable_def* var_def = uast_variable_def_new(
        name_token.pos,
        lang_type,
        name_new(curr_mod_path, name_token.text, (Ulang_type_vec) {0}, scope_id)
    );

    if (add_to_sym_table) {
        if (!usymbol_add(uast_variable_def_wrap(var_def))) {
            msg_redefinition_of_symbol(uast_variable_def_wrap(var_def));
            return PARSE_ERROR;
        }
    }

    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    *result = var_def;
    return PARSE_OK;
}

static PARSE_STATUS parse_for_range_internal(
    Uast_block** result,
    Uast_variable_def* var_def_user,
    Uast_block* outer,
    Tk_view* tokens,
    Scope_id block_scope
) {
    // TODO: create mod_path "builtin" for all builtin symbols (to prevent collisions between user defined symbols and internal symbols)

    Name user_name = var_def_user->name;
    Uast_variable_def* var_def_builtin = uast_variable_def_new(
        var_def_user->pos,
        ulang_type_clone(var_def_user->lang_type, user_name.scope_id),
        name_new(sv("builtin"), util_literal_strv_new(), user_name.gen_args, user_name.scope_id)
    );
    unwrap(usymbol_add(uast_variable_def_wrap(var_def_builtin)));

    unwrap(try_consume(NULL, tokens, TOKEN_IN));

    Uast_expr* lower_bound = NULL;
    if (PARSE_EXPR_OK != parse_expr(&lower_bound, tokens, block_scope)) {
        msg_expected_expr(*tokens, "after in");
        return PARSE_ERROR;
    }

    if (!try_consume(NULL, tokens, TOKEN_DOUBLE_DOT)) {
        msg_parser_expected(tk_view_front(*tokens), "after for loop lower bound", TOKEN_DOUBLE_DOT);
        return PARSE_ERROR;
    }

    Uast_expr* upper_bound = NULL;
    switch (parse_expr(&upper_bound, tokens, block_scope)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "after ..");
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
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
        lower_bound
    );
    vec_append(&a_main, &outer->children, uast_assignment_wrap(init_assign));

    Uast_assignment* increment = uast_assignment_new(
        uast_expr_get_pos(upper_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
        uast_operator_wrap(uast_binary_wrap(uast_binary_new(
            var_def_builtin->pos,
            uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
            uast_literal_wrap(util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, uast_expr_get_pos(upper_bound))),
            BINARY_ADD
        )))
    );
    // TODO: make new vector instead to avoid O(n) insert time
    vec_insert(&a_main, &inner->children, 0, uast_assignment_wrap(increment));

    Uast_assignment* user_assign = uast_assignment_new(
        uast_expr_get_pos(lower_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_user->name)),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name))
    );
    vec_insert(&a_main, &inner->children, 0, uast_assignment_wrap(user_assign));

    // this is a way to compensate for increment occuring on the first iteration
    // TODO: this will not work properly for unsigned if the initial value is 0
    //Uast_assignment* init_decre = uast_assignment_new(
    //    uast_expr_get_pos(upper_bound),
    //    uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
    //    uast_operator_wrap(uast_binary_wrap(uast_binary_new(
    //        var_def->pos,
    //        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def->name)),
    //        uast_literal_wrap(util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, uast_expr_get_pos(upper_bound))),
    //        BINARY_SUB
    //    )))
    //);
    //vec_append(&a_main, &outer->children, uast_assignment_wrap(init_decre));


    Uast_for_with_cond* inner_for = uast_for_with_cond_new(
        outer->pos,
        uast_condition_new(
            outer->pos,
            uast_binary_wrap(uast_binary_new(
                outer->pos,
                uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
                upper_bound,
                BINARY_LESS_THAN
            ))
        ),
        inner,
        util_literal_name_new_prefix2(sv("todo_remove_in_src_parser")),
        true
    );
    vec_append(&a_main, &outer->children, uast_for_with_cond_wrap(inner_for));

    *result = outer;
    return PARSE_OK;
}

static PARSE_STATUS parse_for_with_cond(Uast_for_with_cond** for_new, Pos pos, Tk_view* tokens, Scope_id block_scope) {
    *for_new = uast_for_with_cond_new(pos, NULL, NULL, (Name) {0}, false);
    
    switch (parse_condition(&(*for_new)->condition, tokens, block_scope)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "");
            return PARSE_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        default:
            unreachable("");
    }
    return parse_block(&(*for_new)->body, tokens, false, block_scope);
}

static PARSE_STATUS parse_for_loop(Uast_stmt** result, Tk_view* tokens, Scope_id scope_id) {
    Token for_token = {0};
    unwrap(try_consume(&for_token, tokens, TOKEN_FOR));

    Scope_id block_scope = symbol_collection_new(scope_id);
    
    if (starts_with_variable_type_decl(*tokens, false)) {
        PARSE_STATUS status = PARSE_OK;
        Uast_block* outer = uast_block_new(for_token.pos, (Uast_stmt_vec) {0}, for_token.pos, block_scope);
        Uast_variable_def* var_def = NULL;
        if (PARSE_OK != parse_variable_def(&var_def, tokens, false, true, true, (Ulang_type) {0}, block_scope)) {
            status = PARSE_ERROR;
            goto for_range_error;
        }

        Uast_block* new_for = NULL;
        if (PARSE_OK != parse_for_range_internal(&new_for, var_def, outer, tokens, block_scope)) {
            status = PARSE_ERROR;
            goto for_range_error;
        }
        *result = uast_block_wrap(new_for);

for_range_error:
        return status;
    } else {
        Uast_for_with_cond* new_for = NULL;
        if (PARSE_OK != parse_for_with_cond(&new_for, for_token.pos, tokens, block_scope)) {
            return PARSE_ERROR;
        }
        *result = uast_for_with_cond_wrap(new_for);
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_break(Uast_break** new_break, Tk_view* tokens, Scope_id scope_id) {
    Token break_token = consume(tokens);

    Uast_expr* break_expr = NULL;
    bool do_break_expr = true;
    switch (parse_expr(&break_expr, tokens, scope_id)) {
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
    Token continue_token = consume(tokens);
    Uast_continue* cont_stmt = uast_continue_new(continue_token.pos);
    return cont_stmt;
}

static PARSE_STATUS parse_defer(Uast_defer** defer, Tk_view* tokens, Scope_id scope_id) {
    // TODO: expected failure case for return in defer block
    Token defer_tk = {0};
    unwrap(try_consume(&defer_tk, tokens, TOKEN_DEFER));
    Uast_stmt* child = NULL;
    switch (parse_stmt(&child, tokens, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "after `defer`");
            return PARSE_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        default:
            unreachable("");
    }
    *defer = uast_defer_new(defer_tk.pos, child);
    return PARSE_OK;
}

static PARSE_STATUS parse_function_decl(Uast_function_decl** fun_decl, Tk_view* tokens) {
    PARSE_STATUS status = PARSE_ERROR;

    unwrap(try_consume(NULL, tokens, TOKEN_EXTERN));
    if (!try_consume(NULL, tokens, TOKEN_OPEN_PAR)) {
        msg_parser_expected(tk_view_front(*tokens), "in function decl", TOKEN_OPEN_PAR);
        goto error;
    }
    Token extern_type_token;
    if (!try_consume(&extern_type_token, tokens, TOKEN_STRING_LITERAL)) {
        msg_parser_expected(tk_view_front(*tokens), "in function decl", TOKEN_STRING_LITERAL);
        goto error;
    }
    if (!strv_cstr_is_equal(extern_type_token.text, "c")) {
        msg(
            DIAG_INVALID_EXTERN_TYPE, extern_type_token.pos,
            "invalid extern type `"FMT"`\n", strv_print(extern_type_token.text)
        );
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        msg_parser_expected(tk_view_front(*tokens), "in function decl", TOKEN_CLOSE_PAR);
        goto error;
    }
    if (!try_consume(NULL, tokens, TOKEN_FN)) {
        msg_parser_expected(tk_view_front(*tokens), "in function decl", TOKEN_FN);
        goto error;
    }
    if (PARSE_OK != parse_function_decl_common(fun_decl, tokens, false, SCOPE_TOP_LEVEL, symbol_collection_new(SCOPE_TOP_LEVEL) /* TODO */)) {
        goto error;
    }
    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    status = PARSE_OK;
error:
    return status;
}

static PARSE_STATUS parse_literal(Uast_literal** lit, Tk_view* tokens) {
    Token token = consume(tokens);
    assert(token_is_literal(token));

    if (util_try_uast_literal_new_from_strv(lit, token.text, token.type, token.pos)) {
        return PARSE_OK;
    }
    return PARSE_ERROR;
}

static Uast_symbol* parse_symbol(Tk_view* tokens, Scope_id scope_id) {
    Token token = consume(tokens);
    assert(token.type == TOKEN_SYMBOL);

    return uast_symbol_new(token.pos, name_new(curr_mod_path, token.text, (Ulang_type_vec) {0}, scope_id));
}

static PARSE_STATUS parse_function_call(Uast_function_call** child, Tk_view* tokens, Uast_expr* callee, Scope_id scope_id) {
    bool is_first_time = true;
    bool prev_is_comma = false;
    Uast_expr_vec args = {0};
    while (is_first_time || prev_is_comma) {
        Uast_expr* arg;
        switch (parse_expr(&arg, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                assert(arg);
                vec_append(&a_main, &args, arg);
                prev_is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
                break;
            case PARSE_EXPR_NONE:
                if (prev_is_comma) {
                    msg_expected_expr(*tokens, "");
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
        msg_parser_expected(tk_view_front(*tokens), "", TOKEN_CLOSE_PAR, TOKEN_COMMA);
        return PARSE_ERROR;
    }

    *child = uast_function_call_new(uast_expr_get_pos(callee), args, callee);
    return PARSE_OK;
}

static PARSE_STATUS parse_function_return(Uast_return** rtn_stmt, Tk_view* tokens, Scope_id scope_id) {
    unwrap(try_consume(NULL, tokens, TOKEN_RETURN));

    Uast_expr* expr = NULL;
    switch (parse_expr(&expr, tokens, scope_id)) {
        case PARSE_EXPR_OK:
            assert(expr);
            *rtn_stmt = uast_return_new(uast_expr_get_pos(expr), expr, false);
            break;
        case PARSE_EXPR_NONE:
            *rtn_stmt = uast_return_new(
                prev_token.pos,
                uast_literal_wrap(util_uast_literal_new_from_strv(
                    sv(""),
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
    switch (parse_expr(&cond_child, tokens, scope_id)) {
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
            unreachable(FMT"\n", uast_expr_print(cond_child));
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
    Token if_start_token;
    unwrap(try_consume(&if_start_token, tokens, TOKEN_IF));

    Uast_if_vec ifs = {0};

    Uast_if* if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);
    if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);
    
    switch (parse_condition(&if_stmt->condition, tokens, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "");
            return PARSE_ERROR;
        default:
            unreachable("");
    }
    if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(scope_id))) {
        return PARSE_ERROR;
    }
    vec_append(&a_main, &ifs, if_stmt);

    if_else_chain_consume_newline(tokens);
    while (try_consume(NULL, tokens, TOKEN_ELSE)) {
        if_stmt = uast_if_new(if_start_token.pos, NULL, NULL);

        if (try_consume(&if_start_token, tokens, TOKEN_IF)) {
            switch (parse_condition(&if_stmt->condition, tokens, scope_id)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_ERROR:
                    return PARSE_ERROR;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(*tokens, "");
                    return PARSE_ERROR;
                default:
                    unreachable("");
            }
        } else {
            if_stmt->condition = uast_condition_new(if_start_token.pos, NULL);
            if_stmt->condition->child = uast_condition_get_default_child( uast_literal_wrap(
                util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_start_token.pos)
            ));
        }

        if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(scope_id))) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, &ifs, if_stmt);

        if_else_chain_consume_newline(tokens);
    }

    *if_else_chain = uast_if_else_chain_new(if_start_token.pos, ifs);
    return PARSE_OK;
}


static PARSE_STATUS parse_switch(Uast_switch** lang_switch, Tk_view* tokens, Scope_id scope_id) {
    Token start_token = {0};
    unwrap(try_consume(&start_token, tokens, TOKEN_SWITCH));

    Uast_expr* operand = NULL;
    switch (parse_expr(&operand, tokens, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "");
            return PARSE_ERROR;
        default:
            unreachable("");
    }

    if (!try_consume(NULL, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "after switch operand", TOKEN_OPEN_CURLY_BRACE);
        return PARSE_ERROR;
    }
    try_consume_newlines(tokens);

    Uast_case_vec cases = {0};

    while (1) {
        Scope_id case_scope = symbol_collection_new(scope_id);

        Uast_stmt* case_if_true = NULL;
        Uast_expr* case_operand = NULL;
        bool case_is_default = false;
        if (try_consume(NULL, tokens, TOKEN_CASE)) {
            switch (parse_expr(&case_operand, tokens, case_scope)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_ERROR:
                    return PARSE_ERROR;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(*tokens, "");
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
        // TODO: expected failure case no colon
        unwrap(try_consume(&case_start_token, tokens, TOKEN_COLON));
        switch (parse_stmt(&case_if_true, tokens, case_scope)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "");
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        Uast_case* curr_case = uast_case_new(
            case_start_token.pos,
            case_is_default,
            case_operand,
            case_if_true,
            case_scope
        );
        vec_append(&a_main, &cases, curr_case);
    }

    *lang_switch = uast_switch_new(start_token.pos, operand, cases);
    // TODO: expeced failure case no close brace
    log_tokens(LOG_DEBUG, *tokens);
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

static PARSE_EXPR_STATUS parse_stmt(Uast_stmt** child, Tk_view* tokens, Scope_id scope_id) {
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    Uast_stmt* lhs = NULL;
    if (starts_with_type_def_in_def(*tokens)) {
        assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));
        unwrap(try_consume(NULL, tokens, TOKEN_TYPE_DEF));
        Uast_def* fun_decl;
        if (PARSE_OK != parse_type_def(&fun_decl, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(fun_decl);
    } else if (starts_with_defer(*tokens)) {
        Uast_defer* fun_decl;
        if (PARSE_OK != parse_defer(&fun_decl, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_defer_wrap(fun_decl);
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
        if (PARSE_OK != parse_for_loop(&lhs, tokens, scope_id)) {
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
    } else if (starts_with_variable_def(*tokens)) {
        Uast_variable_def* var_def = NULL;
        if (PARSE_OK != parse_variable_def(&var_def, tokens, true, true, true, (Ulang_type) {0}, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(uast_variable_def_wrap(var_def));
    } else {
        Uast_expr* lhs_ = NULL;
        switch (parse_expr(&lhs_, tokens, scope_id)) {
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
        switch (parse_expr(&rhs, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                *child = uast_expr_wrap(uast_operator_wrap(uast_binary_wrap(uast_binary_new(
                    equal_tk.pos, get_expr_or_symbol(lhs), rhs, BINARY_SINGLE_EQUAL
                ))));
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "");
                return PARSE_EXPR_ERROR;
            default:
                unreachable("");
        }
    } else {
        *child = lhs;
    }

    try_consume_newlines(tokens);
    if (!prev_is_newline()) {
        msg(
            DIAG_NO_NEW_LINE_AFTER_STATEMENT, tk_view_front(*tokens).pos,
            "expected newline after statement\n"
        );
        return PARSE_EXPR_ERROR;
    }
    assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_block(Uast_block** block, Tk_view* tokens, bool is_top_level, Scope_id new_scope) {
    PARSE_STATUS status = PARSE_OK;

    *block = uast_block_new(tk_view_front(*tokens).pos, (Uast_stmt_vec) {0}, (Pos) {0}, new_scope);

    Token open_brace_token = {0};
    if (!is_top_level && !try_consume(&open_brace_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "at start of block", TOKEN_OPEN_CURLY_BRACE);
        status = PARSE_ERROR;
        goto end;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    while (1) {
        if (tk_view_front(*tokens).type == TOKEN_EOF) {
            // this means that there is no matching `}` found
            if (!is_top_level) {
                msg(
                    DIAG_MISSING_CLOSE_CURLY_BRACE, open_brace_token.pos, 
                    "opening `{` is unmatched\n"
                );
                status = PARSE_ERROR;
            }
            break;
        }
        Uast_stmt* child;
        bool should_stop = false;
        switch (parse_stmt(&child, tokens, new_scope)) {
            case PARSE_EXPR_OK:
                assert(child);
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
        assert(!try_consume_newlines(tokens));
        vec_append(&a_main, &(*block)->children, child);
    }
    Token block_end = {0};
    if (!is_top_level && status == PARSE_OK && !try_consume(&block_end, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "at the end of the block", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }
    (*block)->pos_end = block_end.pos;

end:
    return status;
}

static PARSE_STATUS parse_struct_literal_members(Uast_expr_vec* members, Tk_view* tokens, Scope_id scope_id) {
    memset(members, 0, sizeof(*members));

    try_consume_newlines(tokens);
    do {
        try_consume_newlines(tokens);
        Uast_expr* memb = NULL;
        switch (parse_expr(&memb, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "");
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        vec_append(&a_main, members, memb);
        try_consume_newlines(tokens);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    return PARSE_OK;
}


static PARSE_STATUS parse_struct_literal(Uast_struct_literal** struct_lit, Tk_view* tokens, Scope_id scope_id) {
    Token start_token;
    if (!try_consume(&start_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "at start of struct literal", TOKEN_OPEN_CURLY_BRACE);
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
                tk_view_front(*tokens),
                "after expression in struct literal",
                TOKEN_COMMA,
                TOKEN_NEW_LINE,
                TOKEN_CLOSE_CURLY_BRACE
            );
        } else {
            msg_parser_expected(
                tk_view_front(*tokens),
                "in struct literal",
                TOKEN_SINGLE_DOT,
                TOKEN_CLOSE_CURLY_BRACE
            );
        }
        return PARSE_ERROR;
    }

    *struct_lit = uast_struct_literal_new(start_token.pos, members);
    return PARSE_OK;
}

static PARSE_STATUS parse_array_literal(Uast_array_literal** arr_lit, Tk_view* tokens, Scope_id scope_id) {
    Token start_token;
    if (!try_consume(&start_token, tokens, TOKEN_OPEN_SQ_BRACKET)) {
        msg_parser_expected(tk_view_front(*tokens), "at start of array literal", TOKEN_OPEN_CURLY_BRACE);
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
                tk_view_front(*tokens),
                "after expression in array literal",
                TOKEN_COMMA,
                TOKEN_NEW_LINE,
                TOKEN_CLOSE_SQ_BRACKET
            );
        } else {
            msg_parser_expected(
                tk_view_front(*tokens),
                "in array literal",
                TOKEN_SINGLE_DOT,
                TOKEN_CLOSE_SQ_BRACKET
            );
        }
        return PARSE_ERROR;
    }

    *arr_lit = uast_array_literal_new(start_token.pos, members);
    return PARSE_OK;
}

static PARSE_EXPR_STATUS parse_expr_piece(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id
) {
    if (tokens->count < 1) {
        return PARSE_EXPR_NONE;
    }

    Token open_par_token = {0};
    if (try_consume(&open_par_token, tokens, TOKEN_OPEN_PAR)) {
        switch (parse_expr(result, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_ERROR:
                // TODO: remove this if if it causes too many issues
                if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
                    msg(
                        DIAG_MISSING_CLOSE_PAR, open_par_token.pos, 
                        "opening `(` is unmatched\n"
                    );
                }
                return PARSE_EXPR_ERROR;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "");
                return PARSE_EXPR_ERROR;
        }
        if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
            msg(
                DIAG_MISSING_CLOSE_PAR, open_par_token.pos, 
                "opening `(` is unmatched\n"
            );
            return PARSE_EXPR_ERROR;
        }
    } else if (token_is_literal(tk_view_front(*tokens))) {
        Uast_literal* lit = NULL;
        if (PARSE_OK != parse_literal(&lit, tokens)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_literal_wrap(lit);
    } else if (tk_view_front(*tokens).type == TOKEN_MACRO) {
        Pos pos = tk_view_front(*tokens).pos;
        *result = uast_macro_wrap(uast_macro_new(pos, tk_view_front(*tokens).text, pos));
        consume(tokens);
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

static PARSE_EXPR_STATUS parse_high_presidence_internal(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id
) {
    Token oper = {0};
    if (try_consume(&oper, tokens, TOKEN_SINGLE_DOT)) {
        try_consume_newlines(tokens);
        Token memb_name = {0};
        if (!try_consume(&memb_name, tokens, TOKEN_SYMBOL)) {
            msg_parser_expected(tk_view_front(*tokens), "after `.` in member access", TOKEN_SYMBOL);
            return PARSE_EXPR_ERROR;
        }

        lhs = uast_member_access_wrap(uast_member_access_new(
            oper.pos,
            uast_symbol_new(
                memb_name.pos,
                name_new(curr_mod_path, memb_name.text, (Ulang_type_vec) {0}, scope_id)
            ),
            lhs
        ));

        return parse_high_presidence_internal(result, lhs, tokens, scope_id);
    }

    if (try_consume(&oper, tokens, TOKEN_OPEN_PAR)) {
        Uast_function_call* new_call = NULL;
        if (PARSE_OK != parse_function_call(&new_call, tokens, lhs, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        return parse_high_presidence_internal(result, uast_function_call_wrap(new_call), tokens, scope_id);
    }

    if (try_consume(&oper, tokens, TOKEN_OPEN_GENERIC)) {
        if (PARSE_OK != parse_expr_generic(&lhs, lhs, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        // TODO: also consume TOKEN_CLOSE_GENERIC here
        return parse_high_presidence_internal(result, lhs, tokens, scope_id);
    }

    if (try_consume(&oper, tokens, TOKEN_OPEN_SQ_BRACKET)) {
        if (PARSE_OK != parse_expr_index(&lhs, lhs, tokens, scope_id, oper.pos)) {
            return PARSE_EXPR_ERROR;
        }
        return parse_high_presidence_internal(result, lhs, tokens, scope_id);
        // TODO: also consume TOKEN_CLOSE_SQ_BRACKET here
    }
    
    *result = lhs;
    assert(*result);
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_high_presidence(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id
) {
    Uast_expr* lhs = NULL;
    Token oper = {0};
    if (try_consume(&oper, tokens, TOKEN_SINGLE_DOT)) {
        // eg. `.some` in `let opt Optional(<i32>) = .some`
        Token memb_name = {0};
        if (!try_consume(&memb_name, tokens, TOKEN_SYMBOL)) {
            todo();
        }

        lhs = uast_member_access_wrap(uast_member_access_new(
            oper.pos,
            uast_symbol_new(
                memb_name.pos,
                name_new(curr_mod_path, memb_name.text, (Ulang_type_vec) {0}, scope_id)
            ),
            uast_unknown_wrap(uast_unknown_new(oper.pos))
        ));
    } else {
        PARSE_EXPR_STATUS status = parse_expr_piece(&lhs, tokens, scope_id);
        if (status != PARSE_EXPR_OK) {
            return status;
        }
    }

    return parse_high_presidence_internal(result, lhs, tokens, scope_id);
}

static PARSE_EXPR_STATUS parse_unary(
    Uast_expr** result,
    Tk_view* tokens,
    bool can_be_tuple,
    Scope_id scope_id
) {
    (void) can_be_tuple;

    if (!is_unary(tk_view_front(*tokens).type)) {
        return parse_high_presidence(result, tokens, scope_id);
    }
    Token oper = consume_unary(tokens);

    Uast_expr* child = NULL;
    Ulang_type_atom unary_lang_type = ulang_type_atom_new_from_cstr("i32", 0); // this is a placeholder type

    switch (oper.type) {
        case TOKEN_NOT:
            break;
        case TOKEN_ASTERISK:
            break;
        case TOKEN_BITWISE_AND:
            break;
        case TOKEN_SINGLE_MINUS:
            break;
        case TOKEN_UNSAFE_CAST: {
            {
                Token temp;
                if (!try_consume(&temp, tokens, TOKEN_LESS_THAN)) {
                    msg_parser_expected(temp, "", TOKEN_LESS_THAN);
                    return PARSE_EXPR_ERROR;
                }
            }
            // TODO: parse just Lang_type instead of Lang_type_atom
            // make expected success case for function pointer casting, etc.
            if (PARSE_OK != parse_lang_type_struct_atom_require(&unary_lang_type, tokens, scope_id)) {
                return PARSE_EXPR_ERROR;
            }
            {
                Token temp;
                if (!try_consume(&temp, tokens, TOKEN_GREATER_THAN)) {
                    msg_parser_expected(temp, "", TOKEN_GREATER_THAN);
                    return PARSE_EXPR_ERROR;
                }
            }
            break;
        }
        default:
            unreachable(FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    PARSE_EXPR_STATUS status = parse_unary(&child, tokens, false, scope_id);
    switch (status) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "after unary operator");
            return PARSE_EXPR_ERROR;
        default:
            unreachable("");
    }

    switch (oper.type) {
        case TOKEN_NOT:
            // fallthrough
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_BITWISE_AND:
            // fallthrough
        case TOKEN_UNSAFE_CAST:
            *result = uast_operator_wrap(uast_unary_wrap(uast_unary_new(oper.pos, child, token_type_to_unary_type(oper.type), ulang_type_regular_const_wrap(ulang_type_regular_new(unary_lang_type, oper.pos)))));
            assert(*result);
            break;
        case TOKEN_SINGLE_MINUS: {
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, uast_literal_wrap(uast_int_wrap(uast_int_new(oper.pos, 0))), child, token_type_to_binary_type(oper.type))));
            assert(*result);
            break;
        }
        default:
            unreachable(FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_expr_index(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id,
    Pos oper_pos
) {
    Uast_expr* index_index = NULL;
    switch (parse_expr(&index_index, tokens, scope_id)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            // TODO: expected expr
            todo();
        default:
            todo();
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
        msg(
            DIAG_MISSING_CLOSE_SQ_BRACKET,
            tk_view_front(*tokens).pos,
            "expected closing `]` after expr\n"
        );
        return PARSE_ERROR;
    }

    *result = uast_index_wrap(uast_index_new(oper_pos, index_index, lhs));
    return PARSE_OK;
}

static PARSE_STATUS parse_expr_generic(
    Uast_expr** result,
    Uast_expr* lhs,
    Tk_view* tokens,
    Scope_id scope_id
) {
    Uast_symbol* sym = NULL;

    switch (lhs->type) {
        case UAST_SYMBOL:
            sym = uast_symbol_unwrap(lhs);
            break;
        case UAST_MEMBER_ACCESS:
            sym = uast_member_access_unwrap(lhs)->member_name;
            break;
        default:
            // TODO
            todo();
    }

    if (PARSE_OK != parse_generics_args(&sym->name.gen_args, tokens, scope_id)) {
        return PARSE_ERROR;
    }
    *result = lhs;
    return PARSE_OK;
}

//static_assert(TOKEN_COUNT == 67, "exhausive handling of token types; note that only binary operators need to be explicitly handled here");
//static PARSE_EXPR_STATUS(*binary_fns[])(Uast_expr**, Tk_view*, Scope_id) = {
//    // lowest precedence binary operator should be put closed to the top
//    parse_logical_or
//    parse_logical_and
//    parse_bitwise_or
//    parse_bitwise_and
//};

static_assert(TOKEN_COUNT == 68, "exhausive handling of token types; note that only binary operators need to be explicitly handled here");
// lower precedence operators are in earlier rows in the table
static const TOKEN_TYPE BIN_IDX_TO_TOKEN_TYPES[][4] = {
    // {bin_type_1, bin_type_2, bin_type_3, bin_type_4},
    {TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR},
    {TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND},
    {TOKEN_BITWISE_OR, TOKEN_BITWISE_OR, TOKEN_BITWISE_OR, TOKEN_BITWISE_OR},
    {TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR},
    {TOKEN_BITWISE_AND, TOKEN_BITWISE_AND, TOKEN_BITWISE_AND, TOKEN_BITWISE_AND},
    {TOKEN_NOT_EQUAL, TOKEN_DOUBLE_EQUAL, TOKEN_DOUBLE_EQUAL, TOKEN_DOUBLE_EQUAL},
    {TOKEN_LESS_THAN, TOKEN_LESS_OR_EQUAL, TOKEN_GREATER_THAN, TOKEN_GREATER_OR_EQUAL},
    {TOKEN_SHIFT_LEFT, TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT},
    {TOKEN_SINGLE_PLUS, TOKEN_SINGLE_MINUS, TOKEN_SINGLE_MINUS, TOKEN_SINGLE_MINUS},
    {TOKEN_SLASH, TOKEN_ASTERISK, TOKEN_MODULO, TOKEN_MODULO},
};

static PARSE_EXPR_STATUS parse_generic_binary_internal(
    Uast_expr** result,
    Uast_expr* lhs, // this is only used if bin_idx thing matches /* TODO: better description here */
    Tk_view* tokens,
    Scope_id scope_id,
    size_t bin_idx, // idx of BIN_IDX_TO_TOKEN_TYPES that will be used in this function invocation
    int depth
) {
    if (bin_idx >= sizeof(BIN_IDX_TO_TOKEN_TYPES)/sizeof(BIN_IDX_TO_TOKEN_TYPES[0])) {
        unreachable("");
    }

    TOKEN_TYPE bin_type_1 = BIN_IDX_TO_TOKEN_TYPES[bin_idx][0];
    TOKEN_TYPE bin_type_2 = BIN_IDX_TO_TOKEN_TYPES[bin_idx][1];
    TOKEN_TYPE bin_type_3 = BIN_IDX_TO_TOKEN_TYPES[bin_idx][2];
    TOKEN_TYPE bin_type_4 = BIN_IDX_TO_TOKEN_TYPES[bin_idx][3];
                         
    // new_lhs has 1 when parsing &&
    // tokens left are || 2
    Token oper = {0};
    if (!try_consume_1_of_4(&oper, tokens, bin_type_1, bin_type_2, bin_type_3, bin_type_4)) {
        *result = lhs;
        assert(*result);
        return PARSE_EXPR_OK;
    }
    try_consume_newlines(tokens);

    Uast_expr* rhs = NULL;
    PARSE_EXPR_STATUS status = parse_generic_binary(&rhs, tokens, scope_id, bin_idx + 1, depth + 1);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, lhs, rhs, binary_type_from_token_type(oper.type))));

    if (!try_peek_1_of_4(&oper, tokens, bin_type_1, bin_type_2, bin_type_3, bin_type_4)) {
        assert(*result);
        return PARSE_EXPR_OK;
    }

    status = parse_generic_binary_internal(result, *result, tokens, scope_id, bin_idx, depth + 1);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    return PARSE_EXPR_OK;

}

static PARSE_EXPR_STATUS parse_generic_binary(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id,
    size_t bin_idx, // idx of BIN_IDX_TO_TOKEN_TYPES that will be used in this function invocation
    int depth
) {
    if (bin_idx >= sizeof(BIN_IDX_TO_TOKEN_TYPES)/sizeof(BIN_IDX_TO_TOKEN_TYPES[0])) {
        return parse_unary(result, tokens, false, scope_id);
    }

    Uast_expr* new_lhs = NULL;
    PARSE_EXPR_STATUS status = parse_generic_binary(&new_lhs, tokens, scope_id, bin_idx + 1, depth + 1);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    return parse_generic_binary_internal(result, new_lhs, tokens, scope_id, bin_idx, depth);
}

static PARSE_EXPR_STATUS parse_expr(Uast_expr** result, Tk_view* tokens, Scope_id scope_id) {
    Uast_expr* lhs = NULL;
    PARSE_EXPR_STATUS status = parse_generic_binary(&lhs, tokens, scope_id, 0, 0);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    Token equal_tk = {0};
    bool is_assign_bin = false;
    Token oper = {0};
    if (try_consume(&equal_tk, tokens, TOKEN_ASSIGN_BY_BIN)) {
        is_assign_bin = true;
        oper = consume(tokens);
        assert(token_is_binary(oper.type) && "this is likely a bug in the tokenizer");
    } else if (!try_consume(&equal_tk, tokens, TOKEN_SINGLE_EQUAL)) {
        *result = lhs;
        assert(*result);
        return PARSE_EXPR_OK;
    }
    
    Uast_expr* rhs = NULL;
    Uast_expr* final_rhs = NULL;
    switch (parse_generic_binary(&rhs, tokens, scope_id, 0, 0)) {
        case PARSE_EXPR_OK:
            if (is_assign_bin) {
                final_rhs = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, uast_expr_clone(lhs, scope_id, oper.pos), rhs, binary_type_from_token_type(oper.type))));
            } else {
                final_rhs = rhs;
            }
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(equal_tk.pos, lhs, final_rhs, BINARY_SINGLE_EQUAL)));
            assert(*result);
            return PARSE_EXPR_OK;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "after `=`");
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_ERROR;
    }
    unreachable("");
}

static void parser_do_tests(void);

bool parse_file(Uast_block** block, Strv file_path) {
    bool status = true;
#ifndef DNDEBUG
    // TODO: reenable
    //parser_do_tests();
#endif // DNDEBUG

    Strv* file_con = arena_alloc(&a_main, sizeof(*file_con));
    if (!read_file(file_con, file_path)) {
        msg(
            DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN,
            "could not open file `"FMT"`: %s\n",
            strv_print(file_path), strerror(errno)
        );
        status = false;
        goto error;
    }
    unwrap(file_path_to_text_tbl_add(file_con, file_path) && "parse_file should not be called with the same file path more than once");

    Token_vec tokens = {0};
    if (!tokenize(&tokens, file_path)) {
        status = false;
        goto error;
    }
    Tk_view token_view = {.tokens = tokens.buf, .count = tokens.info.count};
    if (PARSE_OK != parse_block(block, &token_view, true, symbol_collection_new(SCOPE_BUILTIN))) {
        status = false;
        goto error;
    }
    symbol_log(LOG_TRACE, (*block)->scope_id);

error:
    return status;
}

static void parser_test_parse_expr(const char* input, int test) {
    (void) input;
    (void) test;
    todo();
    //Env env = {0};
    //Strv file_text = sv(input);
    //env.file_path_to_text = file_text;

    //Token_vec tokens_ = {0};
    //unwrap(tokenize(&tokens_, & (Strv) {0}));
    //Tk_view tokens = tokens_to_tk_view(tokens_);
    //log_tokens(LOG_DEBUG, tokens);
    //Uast_expr* result = NULL;
    //switch (parse_expr(& &result, &tokens, false, false)) {
    //    case PARSE_EXPR_OK:
    //        log(LOG_DEBUG, "\n"FMT, uast_expr_print(result));
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
    //Strv file_text = sv(input);
    //env.file_text = file_text;

    //Token_vec tokens_ = {0};
    //unwrap(tokenize(&tokens_, & (Strv) {0}));
    //Tk_view tokens = tokens_to_tk_view(tokens_);
    //log_tokens(LOG_DEBUG, tokens);
    //Uast_stmt* result = NULL;
    //switch (parse_stmt(& &result, &tokens, true)) {
    //    case PARSE_EXPR_OK:
    //        log(LOG_DEBUG, "\n"FMT, uast_stmt_print(result));
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
