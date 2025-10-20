#include <assert.h>
#include <util.h>
#include <uast.h>
#include <uast_utils.h>
#include <token_view.h>
#include <symbol_table.h>
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
#include <str_and_num_utils.h>
#include <ast_msg.h>

static Strv curr_mod_path; // mod_path of the file that is currently being parsed
static Name curr_mod_alias; // placeholder mod alias of the file that is currently being parsed

static Token prev_token;

static Name new_scope_name;
static Pos new_scope_name_pos;

static Name default_brk_label = {0};

static Uast_using_vec using_params = {0};

// TODO: use parent block for scope_ids instead of function calls everytime

static bool can_end_stmt(Token token);

static PARSE_STATUS parse_block(
    Uast_block** block,
    Tk_view* tokens,
    bool is_top_level,
    Scope_id new_scope,
    Uast_stmt_vec init_children
);

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

static PARSE_STATUS parse_variable_def_or_generic_param(
    Uast_def** result,
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

static bool parse_file(Uast_block** block, Strv file_path, Pos import_pos);

static bool prev_is_newline(void) {
    return prev_token.type == TOKEN_NEW_LINE || prev_token.type == TOKEN_SEMICOLON;
}

// TODO: inline this function?
static bool try_consume_internal(Token* result, Tk_view* tokens, bool allow_opaque_type, TOKEN_TYPE type, bool rm_newlines) {
    Token temp = {0};
    if (allow_opaque_type) {
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
    prev_token = consume(tokens);
    if (result) {
        *result = prev_token;
    }
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

static void msg_parser_expected_internal(const char* file, int line, Token got, const char* msg_suffix, int count_expected, TOKEN_TYPE args[]) {
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
        unwrap(idx < count_expected);
        string_extend_strv(&a_print, &message, token_type_to_strv(TOKEN_MODE_MSG, args[idx]));
        string_extend_cstr(&a_print, &message, "` ");
    }

    string_extend_cstr(&a_print, &message, msg_suffix);

    DIAG_TYPE expect_fail_type = DIAG_PARSER_EXPECTED;
    if (got.type == TOKEN_NONTYPE) {
        expect_fail_type = DIAG_INVALID_TOKEN;
    }
    msg_internal(file, line, expect_fail_type, got.pos, FMT"\n", strv_print(string_to_strv(message)));
}

#define msg_parser_expected(got, msg_suffix, ...) \
    do { \
        msg_parser_expected_internal(__FILE__, __LINE__, got, msg_suffix, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), (TOKEN_TYPE[]){__VA_ARGS__}); \
    } while(0)

bool consume_expect_internal(const char* file, int line, Token* result, Tk_view* tokens, const char* msg, int count_expected, TOKEN_TYPE args[]) {
    for (int idx = 0; idx < count_expected; idx++) {
        unwrap(idx < count_expected);
        if (try_consume(result, tokens, args[idx])) {
            return true;
        }
    }

    msg_parser_expected_internal(file, line, tk_view_front(*tokens), msg, count_expected, args);
    return false;
}

#define consume_expect(result, tokens, msg, ...) \
    consume_expect_internal(__FILE__, __LINE__, result, tokens, msg, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), (TOKEN_TYPE[]){__VA_ARGS__})
    
// TODO: give this function a better name
// returns the modified name of the label
static PARSE_STATUS label_thing(Name* new_name, Scope_id block_scope) {
    assert(new_scope_name.base.count > 0);
    new_scope_name.scope_id = block_scope;
    Uast_label* label = uast_label_new(new_scope_name_pos, new_scope_name, scope_to_name_tbl_lookup(block_scope));
    if (!usymbol_add(uast_label_wrap(label))) {
        msg_redefinition_of_symbol(uast_label_wrap(label));
        return PARSE_ERROR;
    }
    Name old_name = new_scope_name;
    memset(&new_scope_name, 0, sizeof(new_scope_name));
    *new_name = old_name;
    return PARSE_OK;
}

static bool get_mod_alias_from_path_token(
    Uast_mod_alias** mod_alias,
    Token alias_tk,
    Pos mod_path_pos,
    Strv mod_path,
    bool is_builtin_mod_path_alias, 
    bool is_main_mod,
    Pos import_pos
) {
    bool status = true;
    if (!params.do_prelude) {
        assert(!strv_is_equal(mod_path, MOD_PATH_PRELUDE));
    }
    assert(mod_path.count > 0);
    assert(alias_tk.text.count > 0);
    Uast_def* prev_def = NULL;
    String file_path = {0};

    Name old_mod_alias = curr_mod_alias;
    if (is_builtin_mod_path_alias) {
        curr_mod_alias = util_literal_name_new();
    } else if (is_main_mod) {
        assert(strv_is_equal(MOD_ALIAS_TOP_LEVEL.base, alias_tk.text));
        curr_mod_alias = name_new(MOD_ALIAS_TOP_LEVEL.mod_path, alias_tk.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
    } else {
        assert(curr_mod_path.count > 0);
        curr_mod_alias = name_new(curr_mod_path, alias_tk.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
    }
    *mod_alias = uast_mod_alias_new(alias_tk.pos, curr_mod_alias, mod_path, SCOPE_TOP_LEVEL);
    unwrap(usymbol_add(uast_mod_alias_wrap(*mod_alias)));

    Strv old_mod_path = curr_mod_path;
    curr_mod_path = mod_path;

    if (usymbol_lookup(&prev_def, name_new(MOD_PATH_OF_MOD_PATHS, mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL))) {
        goto finish;
    }

    unwrap(usym_tbl_add(uast_import_path_wrap(uast_import_path_new(
        mod_path_pos,
        NULL,
        mod_path
    ))));


    Name alias_name = name_new(MOD_PATH_AUX_ALIASES, mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
    unwrap(usymbol_add(uast_mod_alias_wrap(uast_mod_alias_new(
        POS_BUILTIN,
        alias_name,
        mod_path,
        SCOPE_TOP_LEVEL
    ))));

    string_extend_strv(&a_main, &file_path, mod_path);
    string_extend_cstr(&a_main, &file_path, ".own");
    Uast_block* block = NULL;
    if (!parse_file(&block, string_to_strv(file_path), import_pos)) {
        status = false;
        goto finish;
    }

    assert(block->scope_id != SCOPE_TOP_LEVEL && "this will cause infinite recursion");
    usym_tbl_update(uast_import_path_wrap(uast_import_path_new(
        mod_path_pos,
        block,
        mod_path
    )));

finish:
    curr_mod_path = old_mod_path;
    curr_mod_alias = old_mod_alias;
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

static bool starts_with_label(Tk_view tokens) {
    return try_consume(NULL, &tokens, TOKEN_SYMBOL) && try_consume(NULL, &tokens, TOKEN_COLON);
}

static bool starts_with_using(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_USING;
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

static bool starts_with_yield(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_YIELD;
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
        case TOKEN_LOGICAL_NOT_EQUAL:
            return false;
        case TOKEN_BITWISE_NOT:
            return false;
        case TOKEN_BITWISE_XOR:
            return false;
        case TOKEN_LOGICAL_NOT:
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
        case TOKEN_SIZEOF:
            return false;
        case TOKEN_YIELD:
            return false;
        case TOKEN_COUNTOF:
            return false;
        case TOKEN_DOUBLE_TICK:
            return false;
        case TOKEN_GENERIC_TYPE:
            return true;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
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
        case TOKEN_LOGICAL_NOT_EQUAL:
            return false;
        case TOKEN_LOGICAL_NOT:
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
        case TOKEN_SIZEOF:
            return true;
        case TOKEN_YIELD:
            return false;
        case TOKEN_COUNTOF:
            return true;
        case TOKEN_DOUBLE_TICK:
            return false;
        case TOKEN_GENERIC_TYPE:
            return false;
        case TOKEN_BITWISE_NOT:
            return true;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

static bool parse_lang_type_struct_atom(Pos* pos, Ulang_type_atom* lang_type, Tk_view* tokens, Scope_id scope_id) {
    memset(lang_type, 0, sizeof(*lang_type));
    Token lang_type_token = {0};
    Name mod_alias = curr_mod_alias;

    if (!try_consume(&lang_type_token, tokens, TOKEN_SYMBOL)) {
        return false;
    }

    if (try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        // TODO: mod_alias.mod = line below is a temporary hack.
        //   fix the actual bug with curr_mod_alias to prevent the need for this hack
        mod_alias.mod_path = curr_mod_path;
        mod_alias.base = lang_type_token.text;
        if (!consume_expect(&lang_type_token, tokens, "", TOKEN_SYMBOL)) {
            return false;
        }
    }

    *pos = lang_type_token.pos;

    lang_type->str = uname_new(mod_alias, lang_type_token.text, (Ulang_type_vec) {0}, scope_id);
    assert(mod_alias.mod_path.count > 0);
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
    unwrap(try_consume(&tk_start, tokens, TOKEN_OPEN_PAR));

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

    if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_PAR)) {
        return false;
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

    if (try_consume(NULL, tokens, TOKEN_OPEN_GENERIC)) {
        if (PARSE_OK != parse_generics_args(&atom.str.gen_args, tokens, scope_id)) {
            return false;
        }
    }

    *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(atom, pos));

    Token open_sq_tk = {0};
    if (tk_view_front(*tokens).type == TOKEN_OPEN_SQ_BRACKET || tk_view_front(*tokens).type == TOKEN_ASTERISK) {
        while (1) {
            if (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
                ulang_type_set_pointer_depth(lang_type, ulang_type_get_pointer_depth(*lang_type) + 1);
                continue;
            }

            if (!try_consume(&open_sq_tk, tokens, TOKEN_OPEN_SQ_BRACKET)) {
                break;
            }

            if (try_consume(NULL, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
                *lang_type = ulang_type_new_slice(pos, *lang_type, 0);
                continue;
            }

            Token count_tk = {0};
            if (!consume_expect(&count_tk, tokens, "after `[`", TOKEN_INT_LITERAL)) {
                return false;
            }
            if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_SQ_BRACKET)) {
                return false;
            }

            size_t count = 0;
            unwrap(try_strv_to_size_t(&count, count_tk.text));
            Ulang_type* item_type = arena_dup(&a_main, lang_type);
            *lang_type = ulang_type_array_const_wrap(ulang_type_array_new(item_type, count, open_sq_tk.pos));
        }
    }

    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        ulang_type_set_pointer_depth(lang_type, ulang_type_get_pointer_depth(*lang_type) + 1);
    }

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

static PARSE_EXPR_STATUS parse_function_parameter(Uast_param** child, Tk_view* tokens, Uast_generic_param_vec* gen_params, bool add_to_sym_table, Scope_id scope_id) {
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return PARSE_EXPR_NONE;
    }

    Uast_def* base = NULL;
    bool is_optional = false;
    bool is_variadic = false;
    Uast_expr* opt_default = NULL;
    if (PARSE_OK != parse_variable_def_or_generic_param(&base, tokens, false, add_to_sym_table, true, (Ulang_type) {0}, scope_id)) {
        return PARSE_EXPR_ERROR;
    }
    switch (base->type) {
        case UAST_VARIABLE_DEF:
            break;
        case UAST_GENERIC_PARAM:
            assert(base);
            vec_append(&a_main, gen_params, uast_generic_param_unwrap(base));
            break;
        default:
            unreachable("");
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

    switch (base->type) {
        case UAST_VARIABLE_DEF:
            *child = uast_param_new(uast_def_get_pos(base), uast_variable_def_unwrap(base), is_optional, is_variadic, opt_default);
            return PARSE_EXPR_OK;
        case UAST_GENERIC_PARAM: {
            Uast_variable_def* param_var_def = uast_variable_def_new(
                uast_def_get_pos(base),
                ulang_type_gen_param_const_wrap(ulang_type_gen_param_new(uast_def_get_pos(base))),
                name_new(curr_mod_path, uast_generic_param_unwrap(base)->name.base, (Ulang_type_vec) {0}, scope_id)
            );
            *child = uast_param_new(uast_def_get_pos(base), param_var_def, is_optional, is_variadic, opt_default);
            return PARSE_EXPR_OK;
        }
        default:
            unreachable("");
    }
}

static PARSE_STATUS parse_function_parameters(Uast_function_params** result, Tk_view* tokens, Uast_generic_param_vec* gen_params, bool add_to_sym_tbl, Scope_id scope_id) {
    Uast_param_vec params = {0};
    assert(
        using_params.info.count == 0 &&
        "this vector should have been emptied at the start of the previous block, "
        "and should only be added to when parsing function arguments"
    );

    Uast_param* param = NULL;
    bool done = false;
    while (!done) {
        switch (parse_function_parameter(&param, tokens, gen_params, add_to_sym_tbl, scope_id)) {
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
    if (!consume_expect(NULL, tokens,  " in function decl", TOKEN_OPEN_PAR)) {
        return PARSE_ERROR;
    }

    Uast_function_params* params = NULL;
    if (PARSE_OK != parse_function_parameters(&params, tokens, &gen_params, add_to_sym_table, block_scope)) {
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

    // nessessary because if return type in decl is opaque*, * is not considered to end stmt
    try_consume_newlines(tokens);

    return PARSE_OK;
}

static PARSE_STATUS parse_function_def(Uast_function_def** fun_def, Tk_view* tokens) {
    unwrap(try_consume(NULL, tokens, TOKEN_FN));

    Scope_id fn_scope = SCOPE_TOP_LEVEL;
    Scope_id block_scope = symbol_collection_new(fn_scope, util_literal_name_new());

    Uast_function_decl* fun_decl = NULL;
    if (PARSE_OK != parse_function_decl_common(&fun_decl, tokens, true, fn_scope, block_scope)) {
        return PARSE_ERROR;
    }
    if (strv_is_equal(fun_decl->name.base, sv("main"))) {
        env.mod_path_main_fn = curr_mod_path;
    }

    Uast_block* fun_body = NULL;
    if (PARSE_OK != parse_block(&fun_body, tokens, false, block_scope, (Uast_stmt_vec) {0})) {
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
        if (!consume_expect(&symbol, tokens, "", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        Uast_generic_param* param = uast_generic_param_new(
            symbol.pos,
            name_new(curr_mod_path, symbol.text, (Ulang_type_vec) {0}, block_scope)
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

    if (!consume_expect(NULL, tokens, "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE)) {
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

    if (!consume_expect(NULL, tokens, "to end struct, raw_union, or enum definition", TOKEN_CLOSE_CURLY_BRACE)) {
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

    if (!consume_expect(NULL, tokens, "in struct, raw_union, or enum definition", TOKEN_OPEN_CURLY_BRACE)) {
        return PARSE_ERROR;
    }
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));

    bool done = false;
    while (!done && tokens->count > 0 && tk_view_front(*tokens).type != TOKEN_CLOSE_CURLY_BRACE) {
        Token name_token = {0};
        if (!consume_expect(&name_token, tokens, "in variable definition", TOKEN_SYMBOL)) {
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
        todo();
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
        todo();
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
    if (!consume_expect(&dummy, tokens, "after `def`", TOKEN_SINGLE_EQUAL)) {
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

    *def = uast_lang_def_new(
        name.pos,
        name_new(curr_mod_path, name.text, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
        expr,
        false
    );
    assert((*def)->pos.file_path.count != SIZE_MAX);
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
    if (!consume_expect(&dummy, tokens, "after `import`", TOKEN_SINGLE_EQUAL)) {
        return PARSE_ERROR;
    }

    String mod_path = {0};
    Token path_tk = {0};
    if (!consume_expect(&path_tk, tokens, "after =", TOKEN_SYMBOL)) {
        return PARSE_ERROR;
    }
    string_extend_strv(&a_main, &mod_path, path_tk.text);
    Pos mod_path_pos = path_tk.pos;

    while (try_consume(&path_tk, tokens, TOKEN_SINGLE_DOT)) {
        if (!consume_expect(&path_tk, tokens, "after . in module path", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, &mod_path, PATH_SEPARATOR);
        string_extend_strv(&a_main, &mod_path, path_tk.text);
    }

    if (!get_mod_alias_from_path_token(alias, name, mod_path_pos, string_to_strv(mod_path), false, false, mod_path_pos)) {
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_type_def(Uast_def** def, Tk_view* tokens, Scope_id scope_id) {
    Token name = {0};
    if (!consume_expect(&name, tokens, "", TOKEN_SYMBOL)) {
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
    Uast_def* result_ = NULL;
    PARSE_STATUS status = parse_variable_def_or_generic_param(
        &result_,
        tokens,
        require_let,
        add_to_sym_table,
        require_type,
        default_lang_type,
        scope_id
    );
    if (status != PARSE_OK) {
        return PARSE_ERROR;
    }

    if (result_->type != UAST_VARIABLE_DEF) {
        // TODO: expected failure case
        todo();
    }
    *result = uast_variable_def_unwrap(result_);
    return PARSE_OK;
}

static PARSE_STATUS parse_variable_def_or_generic_param(
    Uast_def** result,
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

    Token dummy = {0};
    bool is_using = try_consume(&dummy, tokens, TOKEN_USING);

    try_consume_newlines(tokens);
    Token name_token = {0};
    if (!try_consume_no_rm_newlines(&name_token, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
        assert(tokens->count > 0);
        return PARSE_ERROR;
    }
    try_consume_no_rm_newlines(NULL, tokens, TOKEN_COLON);

    Ulang_type lang_type = {0};
    Token type_tk = {0};

    if (try_consume(&type_tk, tokens, TOKEN_GENERIC_TYPE)) {
        Uast_generic_param* var_def = uast_generic_param_new(
            name_token.pos,
            name_new(curr_mod_path, name_token.text, (Ulang_type_vec) {0}, scope_id)
        );

        *result = uast_generic_param_wrap(var_def);

        if (add_to_sym_table) {
            if (!usymbol_add(uast_generic_param_wrap(var_def))) {
                msg_redefinition_of_symbol(uast_generic_param_wrap(var_def));
                return PARSE_ERROR;
            }
        }
    } else {
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
        *result = uast_variable_def_wrap(var_def);

        if (add_to_sym_table) {
            if (!usymbol_add(uast_variable_def_wrap(var_def))) {
                msg_redefinition_of_symbol(uast_variable_def_wrap(var_def));
                return PARSE_ERROR;
            }
            if (is_using) {
                vec_append(&a_print /* TODO */, &using_params, uast_using_new(var_def->pos, var_def->name, var_def->name.mod_path));
            }
        } else if (is_using) {
            msg_todo("using in this situation", var_def->pos);
            return PARSE_ERROR;
        }
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_for_range_internal(
    Uast_block** result,
    Uast_variable_def* var_def_user,
    Uast_block* outer,
    Tk_view* tokens,
    Scope_id block_scope
) {
    Name user_name = var_def_user->name;
    Uast_variable_def* var_def_builtin = uast_variable_def_new(
        var_def_user->pos,
        ulang_type_clone(var_def_user->lang_type, true/* TODO */, user_name.scope_id),
        name_new(MOD_PATH_BUILTIN, util_literal_strv_new(), user_name.gen_args, user_name.scope_id)
    );
    unwrap(usymbol_add(uast_variable_def_wrap(var_def_builtin)));

    unwrap(try_consume(NULL, tokens, TOKEN_IN));

    Uast_expr* lower_bound = NULL;
    if (PARSE_EXPR_OK != parse_expr(&lower_bound, tokens, block_scope)) {
        msg_expected_expr(*tokens, "after in");
        return PARSE_ERROR;
    }

    if (!consume_expect(NULL, tokens, "after for loop lower bound", TOKEN_DOUBLE_DOT)) {
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

    Uast_assignment* increment = uast_assignment_new(
        uast_expr_get_pos(upper_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
        uast_operator_wrap(uast_binary_wrap(uast_binary_new(
            var_def_builtin->pos,
            uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
            util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, uast_expr_get_pos(upper_bound)),
            BINARY_ADD
        )))
    );
    Uast_stmt_vec init_children = {0};
    vec_append(&a_main, &init_children, uast_assignment_wrap(increment));

    Uast_block* inner = NULL;
    if (PARSE_OK != parse_block(&inner, tokens, false, symbol_collection_new(block_scope, util_literal_name_new()), init_children)) {
        return PARSE_ERROR;
    }

    Uast_assignment* init_assign = uast_assignment_new(
        uast_expr_get_pos(lower_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
        lower_bound
    );
    vec_append(&a_main, &outer->children, uast_assignment_wrap(init_assign));

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
        util_literal_name_new_prefix(sv("todo_remove_in_src_parser")),
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
    return parse_block(&(*for_new)->body, tokens, false, block_scope, (Uast_stmt_vec) {0});
}

static PARSE_STATUS parse_for_loop(Uast_stmt** result, Tk_view* tokens, Scope_id scope_id) {
    assert(new_scope_name.base.count > 0);
    Name old_default_brk_label = default_brk_label;
    // label_thing will be called when parsing the block (which is why label_thing is not called here)
    // default_brk_label is not set in label_thing, so it needs to be set here (for now)
    default_brk_label = new_scope_name;

    Token for_token = {0};
    unwrap(try_consume(&for_token, tokens, TOKEN_FOR));

    Scope_id block_scope = symbol_collection_new(scope_id, util_literal_name_new());
    
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
        *result = uast_expr_wrap(uast_block_wrap(new_for));

for_range_error:
        // TODO: deduplicate `default_brk_label = old_default_brk_label`?
        default_brk_label = old_default_brk_label;
        return status;
    } else {
        Uast_for_with_cond* new_for = NULL;
        if (PARSE_OK != parse_for_with_cond(&new_for, for_token.pos, tokens, block_scope)) {
            return PARSE_ERROR;
        }
        *result = uast_for_with_cond_wrap(new_for);
    }

    default_brk_label = old_default_brk_label;
    return PARSE_OK;
}

static PARSE_STATUS parse_break(Uast_yield** new_break, Tk_view* tokens, Scope_id scope_id) {
    Token break_token = consume(tokens);

    if (default_brk_label.base.count < 1/* TODO: consider switch statement, etc.*/) {
        msg(
            DIAG_BREAK_INVALID_LOCATION, break_token.pos,
            "break statement outside of a for loop\n"
        );
        return PARSE_ERROR;
    }

    Name break_out_of = default_brk_label;
    if (try_consume(NULL, tokens, TOKEN_DOUBLE_TICK)) {
        Token token = {0};
        if (!consume_expect(&token, tokens, "(scope name)", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        break_out_of = name_new(curr_mod_path, token.text, (Ulang_type_vec) {0}, scope_id);
    }

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

    *new_break = uast_yield_new(break_token.pos, do_break_expr, break_expr, name_clone(break_out_of, true, scope_id));
    return PARSE_OK;
}

static PARSE_STATUS parse_yield(Uast_yield** new_yield, Tk_view* tokens, Scope_id scope_id) {
    Token yield_token = consume(tokens);

    Name break_out_of = {0};
    if (try_consume(NULL, tokens, TOKEN_DOUBLE_TICK)) {
        Token token = {0};
        if (!consume_expect(&token, tokens, "(scope name)", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        break_out_of = name_new(curr_mod_path, token.text, (Ulang_type_vec) {0}, scope_id);
    } else if (PARSE_OK != label_thing(&break_out_of, scope_id)) {
        return PARSE_ERROR;
    }

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

    *new_yield = uast_yield_new(yield_token.pos, do_break_expr, break_expr, break_out_of);
    return PARSE_OK;
}

static PARSE_STATUS parse_continue(Uast_continue** new_cont, Tk_view* tokens, Scope_id scope_id) {
    Token cont_token = consume(tokens);

    Name break_out_of = {0};
    if (try_consume(NULL, tokens, TOKEN_DOUBLE_TICK)) {
        Token token = {0};
        if (!consume_expect(&token, tokens, "(scope name)", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        break_out_of = name_new(curr_mod_path, token.text, (Ulang_type_vec) {0}, scope_id);
    } else {
        if (default_brk_label.base.count < 1) {
            msg(
                DIAG_CONTINUE_INVALID_LOCATION, cont_token.pos,
                "continue statement outside of a for loop\n"
            );
            return PARSE_ERROR;
        }
        break_out_of = default_brk_label;
    }

    *new_cont = uast_continue_new(cont_token.pos, name_clone(break_out_of, true, scope_id));
    return PARSE_OK;
}

static PARSE_STATUS parse_label(Tk_view* tokens, Scope_id scope_id) {
    Token sym_name = {0};
    unwrap(try_consume(&sym_name, tokens, TOKEN_SYMBOL));
    unwrap(try_consume(NULL, tokens, TOKEN_COLON));
    // scope will be updated when parsing the statement
    Name label_name = name_new(curr_mod_path, sym_name.text, (Ulang_type_vec) {0}, scope_id);

    new_scope_name_pos = sym_name.pos;
    new_scope_name = label_name;
    return PARSE_OK;
}

static PARSE_STATUS parse_using(Uast_using** using, Tk_view* tokens, Scope_id scope_id) {
    Token using_tk = {0};
    unwrap(try_consume(&using_tk, tokens, TOKEN_USING));

    Token sym_name = {0};
    if (!consume_expect(&sym_name, tokens, "(module or struct name)", TOKEN_SYMBOL)) {
        return PARSE_ERROR;
    }

    *using = uast_using_new(using_tk.pos, name_new(curr_mod_path, sym_name.text, (Ulang_type_vec) {0}, scope_id), curr_mod_path);
    return PARSE_OK;
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
    if (!consume_expect(NULL, tokens, "in function decl", TOKEN_OPEN_PAR)) {
        goto error;
    }
    Token extern_type_token;
    if (!consume_expect(&extern_type_token, tokens, "in function decl", TOKEN_STRING_LITERAL)) {
        goto error;
    }
    if (!strv_is_equal(extern_type_token.text, sv("c"))) {
        msg(
            DIAG_INVALID_EXTERN_TYPE, extern_type_token.pos,
            "invalid extern type `"FMT"`\n", strv_print(extern_type_token.text)
        );
        goto error;
    }
    if (!consume_expect(NULL, tokens, "in function decl", TOKEN_CLOSE_PAR)) {
        goto error;
    }
    if (!consume_expect(NULL, tokens, "in function decl", TOKEN_FN)) {
        goto error;
    }
    if (PARSE_OK != parse_function_decl_common(fun_decl, tokens, false, SCOPE_TOP_LEVEL, symbol_collection_new(SCOPE_TOP_LEVEL, util_literal_name_new()) /* TODO */)) {
        goto error;
    }
    try_consume(NULL, tokens, TOKEN_SEMICOLON);

    status = PARSE_OK;
error:
    return status;
}

static PARSE_STATUS parse_literal(Uast_expr** lit, Tk_view* tokens) {
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
    if (callee->type == UAST_SYMBOL && uast_symbol_unwrap(callee)->name.gen_args.info.count > 0) {
        msg(DIAG_WRONG_GEN_TYPE, uast_expr_get_pos(callee), "`(<` and `>)` should not be used on function callee\n");
        return PARSE_ERROR;
    }

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

    try_consume_newlines(tokens);
    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        msg_parser_expected(tk_view_front(*tokens), "", TOKEN_CLOSE_PAR, TOKEN_COMMA);
        return PARSE_ERROR;
    }

    *child = uast_function_call_new(uast_expr_get_pos(callee), args, callee, true);
    return PARSE_OK;
}

static PARSE_STATUS parse_return(Uast_return** rtn_stmt, Tk_view* tokens, Scope_id scope_id) {
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
                util_uast_literal_new_from_strv(
                    sv(""),
                    TOKEN_VOID,
                    prev_token.pos
                ),
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

static PARSE_STATUS parse_if_else_chain_internal(
    Uast_block** if_else_chain,
    Token if_token,
    Tk_view* tokens,
    Scope_id grand_parent
) {
    Uast_if_vec ifs = {0};

    Scope_id parent = symbol_collection_new(grand_parent, util_literal_name_new());
    // TODO: (maybe not): extract this if and block_new into separate function
    Name dummy = {0};
    if (PARSE_OK != label_thing(&dummy, parent)) {
        return PARSE_ERROR;
    }

    Uast_if* if_stmt = uast_if_new(if_token.pos, NULL, NULL);
    if_stmt = uast_if_new(if_token.pos, NULL, NULL);
    
    switch (parse_condition(&if_stmt->condition, tokens, parent)) {
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
    if (
        if_stmt->condition->child->type == UAST_BINARY &&
        uast_binary_unwrap(if_stmt->condition->child)->token_type == BINARY_SINGLE_EQUAL
    ) {
        msg(
            DIAG_IF_SHOULD_BE_IF_LET,
            if_stmt->condition->pos,
            "assignment is not allowed for if statement condition; did you mean to use `if let` instead of `if`?\n"
        );
        return PARSE_ERROR;
    }
    if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(parent, util_literal_name_new()), (Uast_stmt_vec) {0})) {
        return PARSE_ERROR;
    }
    vec_append(&a_main, &ifs, if_stmt);

    if_else_chain_consume_newline(tokens);
    while (try_consume(NULL, tokens, TOKEN_ELSE)) {
        if_stmt = uast_if_new(if_token.pos, NULL, NULL);

        if (try_consume(&if_token, tokens, TOKEN_IF)) {
            switch (parse_condition(&if_stmt->condition, tokens, parent)) {
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
            if_stmt->condition = uast_condition_new(if_token.pos, NULL);
            if_stmt->condition->child = uast_condition_get_default_child(
                util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_token.pos)
            );
        }

        if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(parent, util_literal_name_new()), (Uast_stmt_vec) {0})) {
            return PARSE_ERROR;
        }
        vec_append(&a_main, &ifs, if_stmt);

        if_else_chain_consume_newline(tokens);
    }

    Uast_stmt_vec chain_ = {0};
    vec_append(&a_main, &chain_, uast_expr_wrap(uast_if_else_chain_wrap(uast_if_else_chain_new(if_token.pos, ifs))));
    *if_else_chain = uast_block_new(if_token.pos, chain_, if_token.pos, parent);
    return PARSE_OK;
}

static PARSE_STATUS parse_if_let_internal(Uast_switch** lang_switch, Token if_token, Tk_view* tokens, Scope_id scope_id) {
    unwrap(try_consume(NULL, tokens, TOKEN_LET));
    Scope_id if_true_scope = symbol_collection_new(scope_id, util_literal_name_new());
    Scope_id if_false_scope = symbol_collection_new(scope_id, util_literal_name_new());

    Uast_expr* is_true = NULL;
    switch (parse_generic_binary(&is_true, tokens, if_true_scope, 0, 0)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "");
            return PARSE_ERROR;
    }

    Token eq_token = {0};
    if (!consume_expect(&eq_token, tokens, "", TOKEN_SINGLE_EQUAL)) {
        return PARSE_ERROR;
    }

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

    Uast_block* if_true = NULL;
    if (PARSE_OK != parse_block(&if_true, tokens, false, if_true_scope, (Uast_stmt_vec) {0})) {
        return PARSE_ERROR;
    }

    Uast_stmt* if_false = uast_expr_wrap(uast_literal_wrap(uast_void_wrap(uast_void_new(if_token.pos))));
    if (try_consume(NULL, tokens, TOKEN_ELSE)) {
        if (try_consume(&if_token, tokens, TOKEN_IF)) {
            msg(DIAG_IF_ELSE_IN_IF_LET, if_token.pos, "`else if` causes of `if let` statements are not supported\n");
            return PARSE_ERROR;
        }

        Uast_block* if_false_block = NULL;
        if (PARSE_OK != parse_block(&if_false_block, tokens, false, symbol_collection_new(if_false_scope, util_literal_name_new()), (Uast_stmt_vec) {0})) {
            return PARSE_ERROR;
        }
        if_false = uast_expr_wrap(uast_block_wrap(if_false_block));

        if_else_chain_consume_newline(tokens);
    }

    Uast_case_vec cases = {0};

    Uast_case* if_true_case = uast_case_new(
        if_token.pos,
        false,
        is_true,
        uast_expr_wrap(uast_block_wrap(if_true)),
        if_true_scope
    );
    vec_append(&a_main, &cases, if_true_case);

    Uast_case* if_false_case = uast_case_new(
        if_token.pos,
        true,
        uast_literal_wrap(uast_int_wrap(uast_int_new(tk_view_front(*tokens).pos, 1))),
        if_false,
        if_false_scope
    );
    vec_append(&a_main, &cases, if_false_case);

    *lang_switch = uast_switch_new(if_token.pos, operand, cases);
    return PARSE_OK;
}

static PARSE_STATUS parse_if_else_chain(Uast_expr** expr, Tk_view* tokens, Scope_id scope_id) {
    Token if_start_token;
    unwrap(try_consume(&if_start_token, tokens, TOKEN_IF));

    if (tk_view_front(*tokens).type == TOKEN_LET) {
        Uast_switch* lang_switch = NULL;
        if (PARSE_OK != parse_if_let_internal(&lang_switch, if_start_token, tokens, scope_id)) {
            return PARSE_ERROR;
        }
        *expr = uast_switch_wrap(lang_switch);
        return PARSE_OK;
    }

    Uast_block* if_else = NULL;
    if (PARSE_OK != parse_if_else_chain_internal(&if_else, if_start_token, tokens, scope_id)) {
        return PARSE_ERROR;
    }
    *expr = uast_block_wrap(if_else);
    return PARSE_OK;
}


static PARSE_STATUS parse_switch(Uast_block** lang_switch, Tk_view* tokens, Scope_id grand_parent) {
    assert(new_scope_name.base.count > 0);
    Name old_default_brk_label = default_brk_label;

    Scope_id parent = symbol_collection_new(grand_parent, util_literal_name_new());
    // TODO: (maybe not): extract this if and block_new into separate function
    if (PARSE_OK != label_thing(&default_brk_label, parent)) {
        return PARSE_ERROR;
    }

    PARSE_STATUS status = PARSE_OK;

    Token start_token = {0};
    unwrap(try_consume(&start_token, tokens, TOKEN_SWITCH));

    Uast_expr* operand = NULL;
    switch (parse_expr(&operand, tokens, parent)) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_ERROR:
            status = PARSE_ERROR;
            goto error;
        case PARSE_EXPR_NONE:
            msg_expected_expr(*tokens, "");
            status = PARSE_ERROR;
            goto error;
        default:
            unreachable("");
    }

    if (!consume_expect(NULL, tokens, "after switch operand", TOKEN_OPEN_CURLY_BRACE)) {
        status = PARSE_ERROR;
        goto error;
    }
    try_consume_newlines(tokens);

    Uast_case_vec cases = {0};

    while (1) {
        Scope_id case_scope = symbol_collection_new(parent, util_literal_name_new());
        Uast_stmt* case_if_true = NULL;
        Uast_expr* case_operand = NULL;
        bool case_is_default = false;
        if (try_consume(NULL, tokens, TOKEN_CASE)) {
            switch (parse_expr(&case_operand, tokens, case_scope)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_ERROR:
                    status = PARSE_ERROR;
                    goto error;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(*tokens, "");
                    status = PARSE_ERROR;
                    goto error;
                default:
                    unreachable("");
            }
        } else if (try_consume(NULL, tokens, TOKEN_DEFAULT)) {
            case_operand = uast_literal_wrap(uast_int_wrap(uast_int_new(tk_view_front(*tokens).pos, 1)));
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
                status = PARSE_ERROR;
                goto error;
            case PARSE_EXPR_NONE:
                msg_expected_expr(*tokens, "");
                status = PARSE_ERROR;
                goto error;
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

    Uast_stmt_vec chain_ = {0};
    vec_append(&a_main, &chain_, uast_yield_wrap(uast_yield_new(start_token.pos, true, uast_switch_wrap(uast_switch_new(start_token.pos, operand, cases)), default_brk_label)));

    if (cases.info.count < 1) {
        msg(DIAG_SWITCH_NO_CASES, start_token.pos, "switch statement must have at least one case statement\n");
        status = PARSE_ERROR;
        goto error;
    }
    *lang_switch = uast_block_new(start_token.pos, chain_, start_token.pos /* TODO */, parent);

    log_tokens(LOG_DEBUG, *tokens);
    if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_CURLY_BRACE)) {
        // TODO: expeced failure case no close brace
        goto error;
    }

error:
    default_brk_label = old_default_brk_label;
    return status;
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
    // TODO: use try_consume_newlines(tokens) instead of try_consume(NULL, tokens, TOKEN_NEW_LINE)
    while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    if (starts_with_label(*tokens)) {
        if (PARSE_OK != parse_label(tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        assert(new_scope_name.base.count > 0);
    } else if (new_scope_name.base.count < 1) {
        new_scope_name_pos = POS_BUILTIN;
        new_scope_name = util_literal_name_new_prefix_scope(sv("scope_name"), scope_id);
    }

    Uast_stmt* lhs = NULL;
    if (starts_with_type_def_in_def(*tokens)) {
        assert(!try_consume(NULL, tokens, TOKEN_NEW_LINE));
        unwrap(try_consume(NULL, tokens, TOKEN_TYPE_DEF));
        Uast_def* fun_decl;
        if (PARSE_OK != parse_type_def(&fun_decl, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_def_wrap(fun_decl);
    } else if (starts_with_using(*tokens)) {
        Uast_using* using;
        if (PARSE_OK != parse_using(&using, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_using_wrap(using);
    } else if (starts_with_defer(*tokens)) {
        Uast_defer* defer;
        if (PARSE_OK != parse_defer(&defer, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_defer_wrap(defer);
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
        if (PARSE_OK != parse_return(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_return_wrap(rtn_stmt);
    } else if (starts_with_for(*tokens)) {
        assert(new_scope_name.base.count > 0);
        if (PARSE_OK != parse_for_loop(&lhs, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
    } else if (starts_with_break(*tokens)) {
        Uast_yield* rtn_stmt = NULL;
        if (PARSE_OK != parse_break(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_yield_wrap(rtn_stmt);
    } else if (starts_with_yield(*tokens)) {
        Uast_yield* rtn_stmt = NULL;
        if (PARSE_OK != parse_yield(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_yield_wrap(rtn_stmt);
    } else if (starts_with_continue(*tokens)) {
        Uast_continue* rtn_stmt = NULL;
        if (PARSE_OK != parse_continue(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_continue_wrap(rtn_stmt);
    } else if (starts_with_block(*tokens)) {
        Uast_block* block_def = NULL;
        if (PARSE_OK != parse_block(&block_def, tokens, false, symbol_collection_new(scope_id, util_literal_name_new()), (Uast_stmt_vec) {0})) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_expr_wrap(uast_block_wrap(block_def));
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

static PARSE_STATUS parse_block(
    Uast_block** block,
    Tk_view* tokens,
    bool is_top_level,
    Scope_id new_scope,
    Uast_stmt_vec init_children
) {
    PARSE_STATUS status = PARSE_OK;
    bool is_one_line = false;

    Name dummy = {0};
    if (new_scope_name.base.count > 0 && PARSE_OK != label_thing(&dummy, new_scope)) {
        status = PARSE_ERROR;
    }
    *block = uast_block_new(tk_view_front(*tokens).pos, init_children, POS_BUILTIN /* TODO */, new_scope);

    Token open_brace_token = {0};
    if (!is_top_level && !try_consume(&open_brace_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        if (try_consume(&open_brace_token, tokens, TOKEN_ONE_LINE_BLOCK_START)) {
            is_one_line = true;
        } else {
            msg_parser_expected(
                tk_view_front(*tokens),
                "at start of block",
                TOKEN_OPEN_CURLY_BRACE,
                TOKEN_ONE_LINE_BLOCK_START
            );
            status = PARSE_ERROR;
        }
    }
    // TODO: consider if these new lines should be allowed even with one line block
    if (!is_one_line) {
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    }

    // NOTE: this if statement should always run even if parse_block returns PARSE_ERROR
    while (using_params.info.count > 0) {
        vec_append(&a_main, &(*block)->children, uast_using_wrap(vec_pop(&using_params)));
    }
    if (status != PARSE_OK) {
        goto end;
    }

    do {
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
                assert(env.error_count > 0 && "error_count not incremented\n");
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
    } while (!is_one_line);

    Token block_end = {0};
    if (!is_top_level && !is_one_line && status == PARSE_OK && !try_consume(&block_end, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "at the end of the block", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }
    if (is_one_line) {
        assert(!is_top_level);
        block_end = prev_token;
    }
    (*block)->pos_end = block_end.pos;

    try_consume_newlines(tokens);

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
                if (members->info.count < 1) {
                    return PARSE_OK;
                }
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
    if (!consume_expect(&start_token, tokens, "at start of struct literal", TOKEN_OPEN_CURLY_BRACE)) {
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
    if (!consume_expect(&start_token, tokens, "at start of array literal", TOKEN_OPEN_SQ_BRACKET)) {
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
        if (PARSE_OK != parse_literal(result, tokens)) {
            return PARSE_EXPR_ERROR;
        }
    } else if (tk_view_front(*tokens).type == TOKEN_MACRO) {
        Pos pos = tk_view_front(*tokens).pos;
        *result = uast_macro_wrap(uast_macro_new(pos, tk_view_front(*tokens).text, pos));
        consume(tokens);
    } else if (tk_view_front(*tokens).type == TOKEN_SYMBOL) {
        *result = uast_symbol_wrap(parse_symbol(tokens, scope_id));
    } else if (starts_with_switch(*tokens)) {
        Uast_block* lang_switch = NULL;
        if (PARSE_OK != parse_switch(&lang_switch, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = uast_block_wrap(lang_switch);
    } else if (starts_with_if(*tokens)) {
        Uast_expr* expr = NULL;
        if (PARSE_OK != parse_if_else_chain(&expr, tokens ,scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        *result = expr;
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
        if (!consume_expect(&memb_name, tokens, "after `.` in member access", TOKEN_SYMBOL)) {
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
        unwrap(consume_expect(&memb_name, tokens, "", TOKEN_SYMBOL));

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
    Ulang_type unary_lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(
        ulang_type_atom_new(
            uname_new(
                MOD_ALIAS_BUILTIN,
                sv("i32"),
                (Ulang_type_vec) {0},
                SCOPE_TOP_LEVEL
            ),
            0
        ),
        oper.pos
    )); // this is a placeholder type

    static_assert(TOKEN_COUNT == 76, "exhausive handling of token types (only unary operators need to be handled here");
    switch (oper.type) {
        case TOKEN_BITWISE_NOT:
            break;
        case TOKEN_LOGICAL_NOT:
            break;
        case TOKEN_ASTERISK:
            break;
        case TOKEN_BITWISE_AND:
            break;
        case TOKEN_SINGLE_MINUS:
            break;
        case TOKEN_SIZEOF:
            break;
        case TOKEN_COUNTOF:
            break;
        case TOKEN_UNSAFE_CAST: {
            {
                Token temp = {0};
                if (!consume_expect(&temp, tokens, "", TOKEN_LESS_THAN)) {
                    return PARSE_EXPR_ERROR;
                }
            }
            // make expected success case for function pointer casting, etc.
            if (PARSE_OK != parse_lang_type_struct_require(&unary_lang_type, tokens, scope_id)) {
                return PARSE_EXPR_ERROR;
            }
            {
                Token temp = {0};
                if (!consume_expect(&temp, tokens, "", TOKEN_GREATER_THAN)) {
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

    static_assert(TOKEN_COUNT == 76, "exhausive handling of token types (only unary operators need to be handled here");
    switch (oper.type) {
        case TOKEN_BITWISE_NOT: {
            Uast_expr_vec args = {0};
            vec_append(&a_main, &args, child);
            *result = uast_function_call_wrap(uast_function_call_new(
                oper.pos,
                args,
                uast_symbol_wrap(uast_symbol_new(oper.pos, name_new(
                    MOD_PATH_RUNTIME,
                    sv("bitwise_not"),
                    (Ulang_type_vec) {0},
                    SCOPE_TOP_LEVEL
                ))),
                false
            ));
        } break;
        case TOKEN_LOGICAL_NOT:
            // fallthrough
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_BITWISE_AND:
            // fallthrough
        case TOKEN_UNSAFE_CAST:
            *result = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                oper.pos,
                child,
                token_type_to_unary_type(oper.type),
                unary_lang_type
            )));
            assert(*result);
            break;
        case TOKEN_SINGLE_MINUS: {
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(
                oper.pos,
                uast_literal_wrap(uast_int_wrap(uast_int_new(oper.pos, 0))),
                child,
                token_type_to_binary_type(oper.type)
            )));
            assert(*result);
            break;
        }
        case TOKEN_COUNTOF:
            // fallthrough
        case TOKEN_SIZEOF:
            *result = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                oper.pos,
                child,
                token_type_to_unary_type(oper.type),
                ulang_type_new_usize()
            )));
            assert(*result);
            break;
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
            index_index = uast_expr_removed_wrap(uast_expr_removed_new(tk_view_front(*tokens).pos, sv("after `[`")));
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
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

static_assert(TOKEN_COUNT == 76, "exhausive handling of token types; only binary operators need to be explicitly handled here");
// lower precedence operators are in earlier rows in the table
static const TOKEN_TYPE BIN_IDX_TO_TOKEN_TYPES[][4] = {
    // {bin_type_1, bin_type_2, bin_type_3, bin_type_4},
    {TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_OR},
    {TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND, TOKEN_LOGICAL_AND},
    {TOKEN_BITWISE_OR, TOKEN_BITWISE_OR, TOKEN_BITWISE_OR, TOKEN_BITWISE_OR},
    {TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR, TOKEN_BITWISE_XOR},
    {TOKEN_BITWISE_AND, TOKEN_BITWISE_AND, TOKEN_BITWISE_AND, TOKEN_BITWISE_AND},
    {TOKEN_LOGICAL_NOT_EQUAL, TOKEN_DOUBLE_EQUAL, TOKEN_DOUBLE_EQUAL, TOKEN_DOUBLE_EQUAL},
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
                final_rhs = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, uast_expr_clone(lhs, true, scope_id, oper.pos), rhs, binary_type_from_token_type(oper.type))));
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

static bool parse_file(Uast_block** block, Strv file_path, Pos import_pos) {
    bool status = true;

    if (strv_is_equal(MOD_PATH_BUILTIN, file_strip_extension(file_basename(file_path)))) {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN /* TODO */, "file path with basename `builtin.own` is not permitted\n");
        status = false;
        goto error;
    }

    if (file_basename(file_path).count < 1) {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN /* TODO */, "file path with basename length of zero is not permitted\n");
        status = false;
        goto error;
    }

    if (strv_at(file_basename(file_path), 0) == '.') {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN /* TODO */, "file path that starts with `.` is not permitted\n");
        status = false;
        goto error;
    }

    Uast_mod_alias* prelude_alias = NULL;
    if (params.do_prelude) {
        if (!get_mod_alias_from_path_token(
            &prelude_alias,
            token_new(MOD_ALIAS_PRELUDE.base, TOKEN_SYMBOL),
            POS_BUILTIN,
            MOD_PATH_PRELUDE,
            true,
            false,
            POS_BUILTIN
        )) {
            return false;
        }
    }

    Scope_id new_scope = symbol_collection_new(SCOPE_TOP_LEVEL, util_literal_name_new());

    // TODO: DNDEBUG should be spelled NDEBUG
#ifndef DNDEBUG
    // TODO: reenable
    //parser_do_tests();
#endif // DNDEBUG

    Strv* file_con = arena_alloc(&a_main, sizeof(*file_con));
    if (!read_file(file_con, file_path)) {
        msg(DIAG_NOTE, import_pos, "error occured when attempting to import module\n");
        status = false;
        goto error;
    }
    unwrap(file_path_to_text_tbl_add(file_con, file_path) && "parse_file should not be called with the same file path more than once");

    Tk_view tokens = {0};
    if (!tokenize(&tokens, file_path)) {
        status = false;
        goto error;
    }
    // NOTE: scope_id of block in the top level of the file should always be SCOPE_TOP_LEVEL, regardless of if it is the main module
    if (params.do_prelude) {
        vec_append(
            &a_print /* TODO: make arena called "a_pass" or similar to reset after each pass */,
            &using_params,
            uast_using_new((Pos) {.line = 0, .file_path = sv("std/runtime.own") /* TODO: do not hardcode path */} /* TODO: change this to prelude_alias->pos */, prelude_alias->name, file_strip_extension(file_path))
        );
    }
    if (PARSE_OK != parse_block(block, &tokens, true, new_scope, (Uast_stmt_vec) {0})) {
        status = false;
        goto error;
    }

    while (tokens.count > 0) {
        Token curr = consume(&tokens);
        if (curr.type == TOKEN_CLOSE_CURLY_BRACE) {
            msg(DIAG_MISMATCHED_CLOSING_CURLY_BRACE, curr.pos, "closing `}` is unmatched\n");
            status = false;
        }
    }

error:
    return status;
}

bool parse(void) {
    Name alias_name = name_new(MOD_PATH_AUX_ALIASES, MOD_PATH_BUILTIN, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
    unwrap(usymbol_add(uast_mod_alias_wrap(uast_mod_alias_new(
        POS_BUILTIN,
        alias_name,
        MOD_PATH_BUILTIN,
        SCOPE_TOP_LEVEL
    ))));

    Uast_mod_alias* dummy = NULL;
    return get_mod_alias_from_path_token(
        &dummy,
        token_new(MOD_ALIAS_TOP_LEVEL.base, TOKEN_SYMBOL),
        POS_BUILTIN,
        file_strip_extension(params.input_file_path),
        false,
        true,
        POS_BUILTIN
    );
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
