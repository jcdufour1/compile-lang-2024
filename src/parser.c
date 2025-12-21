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
#include <pos_util.h>
#include <did_you_mean.h>

typedef struct {
    Strv mod_path;
    Pos import_pos;
    Pos mod_path_pos;
    Strv curr_mod_path;
    Name curr_mod_alias;
} Import_strv;

typedef struct {
    Vec_base info;
    Import_strv* buf;
} Import_strv_darr;

// TODO: make global variable for parse state

typedef struct {
    Strv curr_mod_path; // mod_path of the file that is currently being parsed
    Scope_id scope_id_curr_mod_path; // top most level scope id of the file that is currently being parsed
    Name curr_mod_alias; // placeholder mod alias of the file that is currently being parsed

    Token prev_token;

    Name new_scope_name;
    Name new_scope_name_when_leaving_block;
    Pos new_scope_name_pos;

    Name default_brk_label;

    Uast_using_darr using_params;
    Import_strv_darr mod_paths_to_parse;
} Parse_state;

static Parse_state parse_state;

static Tk_view parse_state_restore(Parse_state saved_point, Tk_view saved_tk_view) {
    unwrap(0 == memcmp(&saved_point.using_params, &parse_state.using_params, sizeof(parse_state.using_params)));
    unwrap(0 == memcmp(&saved_point.mod_paths_to_parse, &parse_state.mod_paths_to_parse, sizeof(parse_state.mod_paths_to_parse)));
    parse_state = saved_point;
    return saved_tk_view;
}

// TODO: use parent block for scope_ids instead of function calls everytime

static bool can_end_stmt(Token token);

static PARSE_STATUS parse_block(
    Uast_block** block,
    Tk_view* tokens,
    bool is_top_level,
    Scope_id new_scope,
    Uast_stmt_darr init_children
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
    bool actually_require_type,
    Ulang_type lang_type_if_not_required,
    Scope_id scope_id
);

static PARSE_STATUS parse_variable_def_or_generic_param(
    Uast_def** result,
    Tk_view* tokens,
    bool require_let,
    bool add_to_sym_table,
    bool require_type,
    bool actually_require_type,
    Ulang_type lang_type_if_not_required,
    Scope_id scope_id
);

static PARSE_EXPR_STATUS parse_condition(Uast_condition**, Tk_view* tokens, Scope_id scope_id);

static PARSE_STATUS parse_generics_args(Ulang_type_darr* args, Tk_view* tokens, Scope_id scope_id);

static PARSE_STATUS parse_generics_params(Uast_generic_param_darr* params, Tk_view* tokens, Scope_id block_scope);

static bool parse_lang_type_struct(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id);

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

// TODO: rename to parse_general_binary?
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
    return parse_state.prev_token.type == TOKEN_NEW_LINE;
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
    parse_state.prev_token = temp;
    if (result) {
        *result = temp;
    }
    if (rm_newlines && !can_end_stmt(parse_state.prev_token)) {
        while (try_consume_internal(NULL, tokens, false, TOKEN_NEW_LINE, false));
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
        while (try_consume_internal(&dummy, tokens, false, TOKEN_NEW_LINE, false)) {
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
    parse_state.prev_token = consume(tokens);
    if (result) {
        *result = parse_state.prev_token;
    }
    return true;
}

static Token consume_unary(Tk_view* tokens) {
    Token result = {0};
    unwrap(try_consume_if_not(&result, tokens, TOKEN_EOF));
    try_consume_newlines(tokens);
    unwrap(is_unary(result.type) && "there is a bug somewhere in the parser");
    return result;
}

static Token get_curr_token(Tk_view tokens) {
    if (tokens.count < 1) {
        return parse_state.prev_token;
    }
    return tk_view_front(tokens);
}

static Pos get_curr_pos(Tk_view tokens) {
    return get_curr_token(tokens).pos;
}

static void msg_expected_expr_internal(const char* file, int line, Tk_view tokens, const char* msg_suffix) {
    String message = {0};
    string_extend_cstr(&a_temp, &message, "expected expression ");
    string_extend_cstr(&a_temp, &message, msg_suffix);

    msg_internal(file, line, DIAG_EXPECTED_EXPRESSION, get_curr_pos(tokens), FMT"\n", string_print(message)); \
}

#define msg_expected_expr(tokens, msg_suffix) \
    msg_expected_expr_internal(__FILE__, __LINE__, tokens, msg_suffix)

static void msg_parser_expected_internal(const char* file, int line, Token got, const char* msg_suffix, int count_expected, TOKEN_TYPE args[]) {
    String message = {0};
    string_extend_cstr(&a_temp, &message, "got token `");
    string_extend_strv(&a_temp, &message, token_print_internal(&a_temp, TOKEN_MODE_MSG, got));
    string_extend_cstr(&a_temp, &message, "`, but expected ");

    for (int idx = 0; idx < count_expected; idx++) {
        if (idx > 0) {
            if (idx == count_expected - 1) {
                string_extend_cstr(&a_temp, &message, " or ");
            } else {
                string_extend_cstr(&a_temp, &message, ", ");
            }
        }
        string_extend_cstr(&a_temp, &message, "`");
        unwrap(idx < count_expected);
        string_extend_strv(&a_temp, &message, token_type_to_strv(TOKEN_MODE_MSG, args[idx]));
        string_extend_cstr(&a_temp, &message, "` ");
    }

    string_extend_cstr(&a_temp, &message, msg_suffix);

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
    assert(parse_state.new_scope_name.base.count > 0);
    // TODO: remove label->block_scope and use label->name.scope_id instead
    parse_state.new_scope_name.scope_id = block_scope;
    Uast_label* label = uast_label_new(parse_state.new_scope_name_pos, parse_state.new_scope_name, scope_to_name_tbl_lookup(block_scope));
    if (!usymbol_add(uast_label_wrap(label))) {
        msg_redefinition_of_symbol(uast_label_wrap(label));
        return PARSE_ERROR;
    }
    Name old_name = parse_state.new_scope_name;
    memset(&parse_state.new_scope_name, 0, sizeof(parse_state.new_scope_name));
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

    Name old_mod_alias = parse_state.curr_mod_alias;
    if (is_builtin_mod_path_alias) {
        parse_state.curr_mod_alias = util_literal_name_new();
    } else if (is_main_mod) {
        assert(strv_is_equal(MOD_ALIAS_TOP_LEVEL.base, alias_tk.text));
        parse_state.curr_mod_alias = name_new(MOD_ALIAS_TOP_LEVEL.mod_path, alias_tk.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL);
    } else {
        assert(parse_state.curr_mod_path.count > 0);
        parse_state.curr_mod_alias = name_new(parse_state.curr_mod_path, alias_tk.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL);
    }
    *mod_alias = uast_mod_alias_new(alias_tk.pos, parse_state.curr_mod_alias, mod_path, SCOPE_TOP_LEVEL, true);
    Strv old_mod_path = parse_state.curr_mod_path;
    parse_state.curr_mod_path = mod_path;
    if (!usymbol_add(uast_mod_alias_wrap(*mod_alias))) {
        msg_redefinition_of_symbol(uast_mod_alias_wrap(*mod_alias));
        status = false;
        goto finish;
    }

    if (usymbol_lookup(&prev_def, name_new(MOD_PATH_OF_MOD_PATHS, mod_path, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL))) {
        goto finish;
    }

    unwrap(usym_tbl_add(uast_import_path_wrap(uast_import_path_new(
        mod_path_pos,
        NULL,
        mod_path
    ))));
    darr_append(&a_pass, &parse_state.mod_paths_to_parse, ((Import_strv) {
        .mod_path = mod_path,
        .import_pos = import_pos,
        .mod_path_pos = mod_path_pos,
        .curr_mod_path = parse_state.curr_mod_path,
        .curr_mod_alias = parse_state.curr_mod_alias
    }));

    Name alias_name = name_new(MOD_PATH_AUX_ALIASES, mod_path, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL);
    unwrap(usymbol_add(uast_mod_alias_wrap(uast_mod_alias_new(
        POS_BUILTIN,
        alias_name,
        mod_path,
        SCOPE_TOP_LEVEL,
        false
    ))));

finish:
    parse_state.curr_mod_path = old_mod_path;
    parse_state.curr_mod_alias = old_mod_alias;

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

static bool starts_with_lang_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_DEF;
}

static bool starts_with_label(Tk_view tokens, Scope_id scope_id) {
    return scope_id != parse_state.scope_id_curr_mod_path && \
       try_consume(NULL, &tokens, TOKEN_SYMBOL) && \
       try_consume(NULL, &tokens, TOKEN_COLON);
}

static bool starts_with_general_def(Tk_view tokens) {
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
    return tk_view_front(tokens).type == TOKEN_OPEN_CURLY_BRACE ||
        tk_view_front(tokens).type == TOKEN_ONE_LINE_BLOCK_START ||
        tk_view_front(tokens).type == TOKEN_COLON;
}

static bool starts_with_function_call(Tk_view tokens) {
    return try_consume(NULL, &tokens, TOKEN_SYMBOL) && try_consume(NULL, &tokens, TOKEN_OPEN_PAR);
}

static bool starts_with_variable_def(Tk_view tokens) {
    return tk_view_front(tokens).type == TOKEN_LET;
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
        //unwrap(token_is_equal(prev, parse_state.prev_token) && "parse_state.prev_token is not being updated properly");
        unwrap(bracket_depth >= 0);
        is_repeat = true;

        if (!prev_is_newline() || bracket_depth != 0) {
            continue;
        }

        // TODO: see if more things could be added here
        if (
            starts_with_general_def(*tokens) ||
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
        case TOKEN_ORELSE:
            return false;
        case TOKEN_QUESTION_MARK:
            return true;
        case TOKEN_UNDERSCORE:
            // TODO
            return false;
        case TOKEN_AT_SIGN:
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
        case TOKEN_AT_SIGN:
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
        case TOKEN_ORELSE:
            return false;
        case TOKEN_QUESTION_MARK:
            return false;
        case TOKEN_UNDERSCORE:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

static bool is_right_unary(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_AT_SIGN:
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
        case TOKEN_LOGICAL_NOT_EQUAL:
            return false;
        case TOKEN_LOGICAL_NOT:
            return false;
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
            return false;
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
        case TOKEN_EOF:
            return false;
        case TOKEN_ASSIGN_BY_BIN:
            return false;
        case TOKEN_MACRO:
            return false;
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
            return false;
        case TOKEN_BITWISE_NOT:
            return false;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
            return false;
        case TOKEN_ORELSE:
            return true;
        case TOKEN_QUESTION_MARK:
            return true;
        case TOKEN_UNDERSCORE:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

static PARSE_STATUS parse_attrs(Attrs* result, Tk_view* tokens) {
    *result = (Attrs) {0};

    Token at_tk = {0};
    while (try_consume(&at_tk, tokens, TOKEN_AT_SIGN)) {
        Token sym_tk = {0};
        if (!consume_expect(&sym_tk, tokens, "(attribute name) after '@'", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }

        Strv attr_name = sym_tk.text;

        static_assert(ATTR_COUNT == 1, "exhausive handling of attributes in if-else");
        if (strv_is_equal(attr_name, sv("maybe_uninit"))) {
            *result |= ATTR_ALLOW_UNINIT;
        } else {
            Strv_darr candidates = {0};
            static_assert(ATTR_COUNT == 1, "exhausive handling of appending possible attributes to candidates darr");
            darr_append(&a_temp, &candidates, sv("maybe_uninit"));

            msg(
                DIAG_INVALID_ATTR,
                sym_tk.pos,
                "invalid attribute `"FMT"`"FMT"\n",
                strv_print(attr_name),
                did_you_mean_strv_choice_print(attr_name, candidates)
            );
            return PARSE_ERROR;
        }
    }

    return PARSE_OK;
}

static bool parse_lang_type_struct_atom(Pos* pos, Uname* lang_type_name, int16_t* lang_type_ptr_depth, Tk_view* tokens, Scope_id scope_id) {
    *lang_type_name = (Uname) {0};
    *lang_type_ptr_depth = 0;
    Token lang_type_token = {0};
    Name mod_alias = parse_state.curr_mod_alias;

    if (!try_consume(&lang_type_token, tokens, TOKEN_SYMBOL)) {
        return false;
    }

    if (try_consume(NULL, tokens, TOKEN_SINGLE_DOT)) {
        // TODO: mod_alias.mod = line below is a temporary hack.
        //   fix the actual bug with parse_state.curr_mod_alias to prevent the need for this hack
        mod_alias.mod_path = parse_state.curr_mod_path;
        mod_alias.base = lang_type_token.text;
        if (!consume_expect(&lang_type_token, tokens, "", TOKEN_SYMBOL)) {
            return false;
        }
    }

    *pos = lang_type_token.pos;

    *lang_type_name = uname_new(mod_alias, lang_type_token.text, (Ulang_type_darr) {0}, scope_id);
    assert(mod_alias.mod_path.count > 0);
    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        (*lang_type_ptr_depth)++;
    }

    return true;
}

static bool parse_lang_type_struct_ex(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id, bool print_error);

// type will be parsed if possible
static bool parse_lang_type_struct_tuple(Ulang_type_tuple* lang_type, Tk_view* tokens, Scope_id scope_id, bool print_error) {
    Ulang_type inner = {0};
    Ulang_type_darr types = {0};
    bool is_comma = true;

    // TODO: make token naming more consistant
    Token tk_start = {0};
    unwrap(try_consume(&tk_start, tokens, TOKEN_OPEN_PAR));

    while (is_comma) {
        if (!parse_lang_type_struct_ex(&inner, tokens, scope_id, print_error)) {
            break;
        }
        darr_append(&a_main, &types, inner);
        is_comma = try_consume(NULL, tokens, TOKEN_COMMA);
    }

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_PAR)) {
        if (print_error) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_CLOSE_PAR);
        }
        return false;
    }

    *lang_type = ulang_type_tuple_new(tk_start.pos, types, 0);
    return true;
}

// type will be parsed if possible
static bool parse_lang_type_struct_ex(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id, bool print_error) {
    memset(lang_type, 0, sizeof(*lang_type));

    Token lang_type_token = {0};
    if (try_consume(&lang_type_token, tokens, TOKEN_FN)) {
        Ulang_type_tuple params = {0};
        if (!parse_lang_type_struct_tuple(&params, tokens, scope_id, print_error)) {
            return false;
        }
        Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type));
        if (!parse_lang_type_struct_ex(rtn_type, tokens, scope_id, print_error)) {
            return false;
        }
        *lang_type = ulang_type_fn_const_wrap(ulang_type_fn_new(lang_type_token.pos, params, rtn_type, 1));
        return true;
    }

    if (tk_view_front(*tokens).type ==  TOKEN_OPEN_PAR) {
        Ulang_type_tuple new_tuple = {0};
        if (!parse_lang_type_struct_tuple(&new_tuple, tokens, scope_id, print_error)) {
            return false;
        }
        *lang_type = ulang_type_tuple_const_wrap(new_tuple);
        return true;
    }

    Uname atom_name = {0};
    int16_t atom_ptr_depth = {0};
    Pos pos = {0};
    if (!parse_lang_type_struct_atom(&pos, &atom_name, &atom_ptr_depth, tokens, scope_id)) {
        return false;
    }

    if (try_consume(NULL, tokens, TOKEN_OPEN_GENERIC)) {
        if (PARSE_OK != parse_generics_args(&atom_name.gen_args, tokens, scope_id)) {
            return false;
        }
    }

    *lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(pos, atom_name, atom_ptr_depth));

    Token open_sq_tk = {0};
    if (tk_view_front(*tokens).type == TOKEN_OPEN_SQ_BRACKET || tk_view_front(*tokens).type == TOKEN_ASTERISK || tk_view_front(*tokens).type == TOKEN_QUESTION_MARK) {
        while (1) {
            if (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
                ulang_type_set_pointer_depth(lang_type, ulang_type_get_pointer_depth(*lang_type) + 1);
                continue;
            }

            if (try_consume(&open_sq_tk, tokens, TOKEN_QUESTION_MARK)) {
                *lang_type = ulang_type_new_optional(open_sq_tk.pos, *lang_type);
                continue;
            }

            if (!try_consume(&open_sq_tk, tokens, TOKEN_OPEN_SQ_BRACKET)) {
                break;
            }

            if (try_consume(NULL, tokens, TOKEN_CLOSE_SQ_BRACKET)) {
                *lang_type = ulang_type_new_slice(pos, *lang_type, 0);
                continue;
            }

            Uast_expr* count = NULL;
            switch (parse_expr(&count, tokens, scope_id)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(*tokens, "after `[`");
                    return false;
                case PARSE_EXPR_ERROR:
                    return false;
            }
            if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_SQ_BRACKET)) {
                return false;
            }

            *lang_type = ulang_type_array_const_wrap(ulang_type_array_new(
                open_sq_tk.pos,
                arena_dup(&a_main, lang_type),
                count,
                0
            ));
        }
    }

    while (try_consume(NULL, tokens, TOKEN_ASTERISK)) {
        ulang_type_set_pointer_depth(lang_type, ulang_type_get_pointer_depth(*lang_type) + 1);
    }

    return true;
}

static bool parse_lang_type_struct(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id) {
    return parse_lang_type_struct_ex(lang_type, tokens, scope_id, true);
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_require_ex(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id, bool print_error) {
    if (parse_lang_type_struct_ex(lang_type, tokens, scope_id, print_error)) {
        return PARSE_OK;
    } else {
        if (print_error) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
        }
        return PARSE_ERROR;
    }
}

// require type to be parsed
static PARSE_STATUS parse_lang_type_struct_require(Ulang_type* lang_type, Tk_view* tokens, Scope_id scope_id) {
    return parse_lang_type_struct_require_ex(lang_type, tokens, scope_id, true);
}

static PARSE_EXPR_STATUS parse_function_parameter(Uast_param** child, Tk_view* tokens, Uast_generic_param_darr* gen_params /* TODO: remove gen_params */, bool add_to_sym_table, Scope_id scope_id) {
    (void) gen_params;
    if (tokens->count < 1 || tk_view_front(*tokens).type == TOKEN_CLOSE_PAR) {
        return PARSE_EXPR_NONE;
    }

    Uast_variable_def* var_def = NULL;
    bool is_optional = false;
    bool is_variadic = false;
    Uast_expr* opt_default = NULL;
    if (PARSE_OK != parse_variable_def(&var_def, tokens, false, add_to_sym_table, true, true, (Ulang_type) {0}, scope_id)) {
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

    *child = uast_param_new(var_def->pos, var_def, is_optional, is_variadic, opt_default);
    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_function_parameters(Uast_function_params** result, Tk_view* tokens, Uast_generic_param_darr* gen_params, bool add_to_sym_tbl, Scope_id scope_id) {
    Uast_param_darr params = {0};
    assert(
        parse_state.using_params.info.count == 0 &&
        "this darr should have been emptied at the start of the previous block, "
        "and should only be added to when parsing function arguments"
    );

    Uast_param* param = NULL;
    bool done = false;
    while (!done) {
        switch (parse_function_parameter(&param, tokens, gen_params, add_to_sym_tbl, scope_id)) {
            case PARSE_EXPR_OK:
                darr_append(&a_main, &params, param);
                break;
            case PARSE_EXPR_ERROR:
                darr_reset(&parse_state.using_params);
                assert(parse_state.using_params.info.count == 0);
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
    Scope_id block_scope,
    bool is_lambda,
    Token fn_name_tk_if_not_lambda
) {
    Token name_token = {0};
    if (is_lambda) {
        name_token = (Token) {.text = util_literal_strv_new(), .type = TOKEN_SYMBOL, .pos = tk_view_front(*tokens).pos};
    } else {
        name_token = fn_name_tk_if_not_lambda;
    }

    Uast_generic_param_darr gen_params = {0};
    if (tk_view_front(*tokens).type == TOKEN_OPEN_GENERIC) {
        parse_generics_params(&gen_params, tokens, block_scope);
    }

    if (!consume_expect(NULL, tokens,  " in function decl", TOKEN_OPEN_PAR)) {
        return PARSE_ERROR;
    }

    Uast_function_params* params = NULL;
    if (PARSE_OK != parse_function_parameters(&params, tokens, &gen_params, add_to_sym_table, block_scope)) {
        assert(parse_state.using_params.info.count == 0);
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
        rtn_type = ulang_type_new_void(tk_view_front(*tokens).pos);
        assert(!pos_is_equal(ulang_type_get_pos(rtn_type), POS_BUILTIN));
    }

    *fun_decl = uast_function_decl_new(name_token.pos, gen_params, params, rtn_type, name_new(parse_state.curr_mod_path, name_token.text, (Ulang_type_darr) {0}, fn_scope));
    if (!usymbol_add(uast_function_decl_wrap(*fun_decl))) {
        assert(parse_state.using_params.info.count == 0);
        return msg_redefinition_of_symbol(uast_function_decl_wrap(*fun_decl));
    }

    // nessessary because if return type in decl is opaque*, * is not considered to end stmt
    try_consume_newlines(tokens);

    return PARSE_OK;
}

static PARSE_STATUS parse_function_def_internal(Uast_function_def** fun_def, Tk_view* tokens, bool is_lambda, Token fn_name_tk_if_not_lambda) {
    Scope_id fn_scope = SCOPE_TOP_LEVEL;
    Scope_id block_scope = symbol_collection_new(fn_scope, util_literal_name_new());

    Uast_function_decl* fun_decl = NULL;
    if (PARSE_OK != parse_function_decl_common(&fun_decl, tokens, true, fn_scope, block_scope, is_lambda, fn_name_tk_if_not_lambda)) {
        return PARSE_ERROR;
    }
    if (strv_is_equal(fun_decl->name.base, sv("main"))) {
        env.mod_path_main_fn = parse_state.curr_mod_path;
    }

    Uast_block* fun_body = NULL;
    if (PARSE_OK != parse_block(&fun_body, tokens, false, block_scope, (Uast_stmt_darr) {0})) {
        return PARSE_ERROR;
    }

    *fun_def = uast_function_def_new(fun_decl->pos, fun_decl, fun_body);
    usymbol_update(uast_function_def_wrap(*fun_def));
    return PARSE_OK;
}

static PARSE_STATUS parse_function_def(Uast_stmt** result, Tk_view* tokens, bool is_lambda, Scope_id scope_id, Token fn_name_tk_if_not_lambda) {
    Token fn_tk = {0};
    unwrap(try_consume(&fn_tk, tokens, TOKEN_FN));

    if (!is_lambda && scope_id != parse_state.scope_id_curr_mod_path) {
        Name fun_name = name_new(
            parse_state.curr_mod_path,
            fn_name_tk_if_not_lambda.text,
            (Ulang_type_darr) {0},
            scope_id
        );
        Uast_function_def* fun_def = NULL;
        if (PARSE_OK != parse_function_def_internal(&fun_def, tokens, true, fn_name_tk_if_not_lambda)) {
            return PARSE_ERROR;
        }

        Uast_variable_def* var_def = uast_variable_def_new(
            fn_tk.pos,
            ulang_type_from_uast_function_decl(fun_def->decl),
            fun_name,
            (Attrs) {0}
        );
        if (!usymbol_add(uast_variable_def_wrap(var_def))) {
            msg_redefinition_of_symbol(uast_variable_def_wrap(var_def));
            return PARSE_ERROR;
        }
        *result = uast_assignment_wrap(uast_assignment_new(
            fn_tk.pos,
            uast_symbol_wrap(uast_symbol_new(var_def->pos, var_def->name)),
            uast_symbol_wrap(uast_symbol_new(fn_tk.pos, fun_def->decl->name))
        ));
        return PARSE_OK;
    }

    Uast_function_def* fun_def = NULL;
    if (PARSE_OK != parse_function_def_internal(&fun_def, tokens, is_lambda, fn_name_tk_if_not_lambda)) {
        return PARSE_ERROR;
    }
    *result = uast_def_wrap(uast_function_def_wrap(fun_def));
    return PARSE_OK;
}

static PARSE_STATUS parse_generics_params(Uast_generic_param_darr* params, Tk_view* tokens, Scope_id block_scope) {
    memset(params, 0, sizeof(*params));
    unwrap(try_consume(NULL, tokens, TOKEN_OPEN_GENERIC));

    do {
        Token symbol = {0};
        if (!consume_expect(&symbol, tokens, "", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }

        Token ticks = {0};
        bool is_expr = false;
        Ulang_type expr_lang_type = {0};
        if (try_consume(&ticks, tokens, TOKEN_DOUBLE_TICK)) {
            if (!parse_lang_type_struct(&expr_lang_type, tokens, block_scope)) {
                return PARSE_ERROR;
            }
            is_expr = true;
        }

        Uast_generic_param* param = uast_generic_param_new(
            symbol.pos,
            name_new(parse_state.curr_mod_path, symbol.text, (Ulang_type_darr) {0}, block_scope),
            is_expr,
            expr_lang_type
        );

        darr_append(&a_main, params, param);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
        );
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static PARSE_STATUS parse_generics_args(Ulang_type_darr* args, Tk_view* tokens, Scope_id scope_id) {
    memset(args, 0, sizeof(*args));

    do {
        Uast_expr* arg = {0};
        switch (parse_expr(&arg, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                break;
            case PARSE_EXPR_NONE:
                msg(DIAG_EXPECTED_TYPE_BUT_GOT_EXPR, tk_view_front(*tokens).pos, "expected type or expression\n");
                return PARSE_ERROR;
            case PARSE_EXPR_ERROR:
                return PARSE_ERROR;
            default:
                unreachable("");
        }
        darr_append(&a_main, args, ulang_type_expr_const_wrap(ulang_type_expr_new(uast_expr_get_pos(arg), arg, 0)));
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    if (!try_consume(NULL, tokens, TOKEN_CLOSE_GENERIC)) {
        msg_parser_expected(
            tk_view_front(*tokens), "", TOKEN_COMMA, TOKEN_CLOSE_GENERIC
        );
        return PARSE_ERROR;
    }
    return PARSE_OK;
}

static PARSE_STATUS parse_struct_def_base(
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
        switch (parse_variable_def(&member, tokens, false, false, require_sub_types, require_sub_types, default_lang_type, SCOPE_NOT)) {
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
        darr_append(&a_main, &base->members, member);
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
    Uname lang_type_name // TODO: pass Ulang_type here instead of Uname
) {
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
        try_consume_newlines(tokens);
        if (!try_consume_newlines(tokens) && !try_consume(NULL, tokens, TOKEN_COMMA)) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_NEW_LINE, TOKEN_COMMA);
            return PARSE_ERROR;
        }

        Attrs attrs = {0};
        if (PARSE_OK != parse_attrs(&attrs, tokens)) {
            return PARSE_ERROR;
        }

        Uast_variable_def* member = uast_variable_def_new(
            name_token.pos,
            ulang_type_regular_const_wrap(ulang_type_regular_new(name_token.pos, lang_type_name, 0)), 
              // TODO: set member lang_type to lang_type_name instead of 
              // ulang_type_regular_const_wrap(ulang_type_regular_new( when lang_type_name is changed to Ulang_type
            name_new(parse_state.curr_mod_path, name_token.text, (Ulang_type_darr) {0}, 0),
            attrs
        );

        darr_append(&a_main, &base->members, member);
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
    if (PARSE_OK != parse_struct_def_base(&base, name_new(parse_state.curr_mod_path, name.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL), tokens, true, (Ulang_type) {0})) {
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
    if (PARSE_OK != parse_struct_def_base(&base, name_new(parse_state.curr_mod_path, name.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL), tokens, true, (Ulang_type) {0})) {
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
    if (PARSE_OK != parse_struct_def_base(
        &base,
        name_new(parse_state.curr_mod_path, name.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL),
        tokens,
        false,
        ulang_type_new_void(tk_view_front(*tokens).pos)
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

    if (strv_is_equal(parse_state.curr_mod_path, MOD_PATH_RUNTIME)) {
        msg(DIAG_LANG_DEF_IN_RUNTIME, lang_def_tk.pos, "`def` cannot be used in "FMT"\n", strv_print(MOD_PATH_RUNTIME));
    }

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
        name_new(parse_state.curr_mod_path, name.text, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL),
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
        darr_append(&a_main, &mod_path, PATH_SEPARATOR);
        string_extend_strv(&a_main, &mod_path, path_tk.text);
    }

    if (!get_mod_alias_from_path_token(alias, name, mod_path_pos, string_to_strv(mod_path), false, false, mod_path_pos)) {
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
    bool actually_require_type,
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
        actually_require_type,
        default_lang_type,
        scope_id
    );
    if (status != PARSE_OK) {
        return PARSE_ERROR;
    }

    if (result_->type != UAST_VARIABLE_DEF) {
        msg(DIAG_EXPECTED_VARIABLE_DEF, uast_def_get_pos(result_), "expected variable definition\n");
        return PARSE_ERROR;
    }
    *result = uast_variable_def_unwrap(result_);
    return PARSE_OK;
}

static PARSE_STATUS parse_variable_def_or_generic_param(
    Uast_def** result,
    Tk_view* tokens,
    bool require_let,
    bool add_to_sym_table,
    bool require_type, // TODO: rename this variable
    bool actually_require_type,
    Ulang_type default_lang_type,
    Scope_id scope_id
) {
    (void) require_let;
    if (!try_consume(NULL, tokens, TOKEN_LET)) {
        unwrap(!require_let);
    }

    Token dummy = {0};
    bool is_using = try_consume(&dummy, tokens, TOKEN_USING);

    try_consume_newlines(tokens);
    Token name_token = {0};
    if (!try_consume_no_rm_newlines(&name_token, tokens, TOKEN_SYMBOL)) {
        msg_parser_expected(tk_view_front(*tokens), "in variable definition", TOKEN_SYMBOL);
        unwrap(tokens->count > 0);
        return PARSE_ERROR;
    }
    try_consume_no_rm_newlines(NULL, tokens, TOKEN_COLON);

    Ulang_type lang_type = {0};
    Token type_tk = {0};

    if (try_consume(&type_tk, tokens, TOKEN_DOUBLE_TICK)) {
        Ulang_type lang_type_expr = {0};
        if (!parse_lang_type_struct(&lang_type_expr, tokens, scope_id)) {
            return PARSE_ERROR;
        }

        Uast_generic_param* var_def = uast_generic_param_new(
            name_token.pos,
            name_new(parse_state.curr_mod_path, name_token.text, (Ulang_type_darr) {0}, scope_id),
            true,
            lang_type_expr
        );

        *result = uast_generic_param_wrap(var_def);

        if (add_to_sym_table) {
            if (!usymbol_add(uast_generic_param_wrap(var_def))) {
                msg_redefinition_of_symbol(uast_generic_param_wrap(var_def));
                return PARSE_ERROR;
            }
        }
    } else {
        if (actually_require_type) {
            if (PARSE_OK != parse_lang_type_struct_require(&lang_type, tokens, scope_id)) {
                return PARSE_ERROR;
            }
        } else if (require_type) {
            if (!parse_lang_type_struct(&lang_type, tokens, scope_id)) {
                lang_type = ulang_type_removed_const_wrap(ulang_type_removed_new(name_token.pos, 0));
            }
        } else {
            if (!parse_lang_type_struct(&lang_type, tokens, scope_id)) {
                lang_type = default_lang_type;
            }
        }

        Attrs attrs = {0};
        if (PARSE_OK != parse_attrs(&attrs, tokens)) {
            return PARSE_ERROR;
        }

        Uast_variable_def* var_def = uast_variable_def_new(
            name_token.pos,
            lang_type,
            name_new(parse_state.curr_mod_path, name_token.text, (Ulang_type_darr) {0}, scope_id),
            attrs
        );
        *result = uast_variable_def_wrap(var_def);

        if (add_to_sym_table) {
            if (!usymbol_add(uast_variable_def_wrap(var_def))) {
                msg_redefinition_of_symbol(uast_variable_def_wrap(var_def));
                return PARSE_ERROR;
            }
            // TODO: should the "is_using" if statement also be ran for other two cases (Type and '')?
            if (is_using) {
                darr_append(&a_pass, &parse_state.using_params, uast_using_new(var_def->pos, var_def->name, var_def->name.mod_path));
            }
        } else if (is_using) {
            // TODO: should this msg_todo also be ran for other two cases (Type and '')?
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
        name_new(MOD_PATH_BUILTIN, util_literal_strv_new(), user_name.gen_args, user_name.scope_id),
        (Attrs) {0}
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
    Uast_stmt_darr init_children = {0};
    darr_append(&a_main, &init_children, uast_assignment_wrap(increment));

    Uast_block* inner = NULL;
    if (PARSE_OK != parse_block(&inner, tokens, false, symbol_collection_new(block_scope, util_literal_name_new()), init_children)) {
        return PARSE_ERROR;
    }

    Uast_assignment* init_assign = uast_assignment_new(
        uast_expr_get_pos(lower_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name)),
        lower_bound
    );
    darr_append(&a_main, &outer->children, uast_assignment_wrap(init_assign));

    Uast_assignment* user_assign = uast_assignment_new(
        uast_expr_get_pos(lower_bound),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_user->name)),
        uast_symbol_wrap(uast_symbol_new(uast_expr_get_pos(lower_bound), var_def_builtin->name))
    );
    darr_insert(&a_main, &inner->children, 0, uast_assignment_wrap(user_assign));

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
    darr_append(&a_main, &outer->children, uast_for_with_cond_wrap(inner_for));

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
    return parse_block(&(*for_new)->body, tokens, false, block_scope, (Uast_stmt_darr) {0});
}

static bool is_for_range(Tk_view tokens, Scope_id block_scope) {
    Uast_variable_def* var_def = NULL;

    LOG_LEVEL old_params_log_level = params_log_level;
    params_log_level = LOG_FATAL;
    uint32_t old_error_count = env.error_count;
    if (PARSE_OK != parse_variable_def(&var_def, &tokens, false, false, true, false, (Ulang_type) {0}, block_scope)) {
        params_log_level = old_params_log_level;
        env.error_count = old_error_count;
        return false;
    }
    params_log_level = old_params_log_level;
    env.error_count = old_error_count;

    return tk_view_front(tokens).type == TOKEN_IN;
}

static PARSE_STATUS parse_for_loop(Uast_stmt** result, Tk_view* tokens, Scope_id scope_id) {
    unwrap(parse_state.new_scope_name.base.count > 0);
    Name old_default_brk_label = parse_state.default_brk_label;
    // label_thing will be called when parsing the block (which is why label_thing is not called here)
    // parse_state.default_brk_label is not set in label_thing, so it needs to be set here (for now)
    parse_state.default_brk_label = parse_state.new_scope_name;

    Token for_token = {0};
    unwrap(try_consume(&for_token, tokens, TOKEN_FOR));

    Scope_id block_scope = symbol_collection_new(scope_id, util_literal_name_new());
    
    if (is_for_range(*tokens, block_scope)) {
        PARSE_STATUS status = PARSE_OK;
        Uast_block* outer = uast_block_new(for_token.pos, (Uast_stmt_darr) {0}, for_token.pos, block_scope);
        Uast_variable_def* var_def = NULL;
        if (PARSE_OK != parse_variable_def(&var_def, tokens, false, true, true, false, (Ulang_type) {0}, block_scope)) {
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
        // TODO: deduplicate `parse_state.default_brk_label = old_default_brk_label`?
        parse_state.default_brk_label = old_default_brk_label;
        return status;
    } else {
        Uast_for_with_cond* new_for = NULL;
        if (PARSE_OK != parse_for_with_cond(&new_for, for_token.pos, tokens, block_scope)) {
            return PARSE_ERROR;
        }
        *result = uast_for_with_cond_wrap(new_for);
    }

    parse_state.default_brk_label = old_default_brk_label;
    return PARSE_OK;
}

static PARSE_STATUS parse_break(Uast_yield** new_break, Tk_view* tokens, Scope_id scope_id) {
    Token break_token = consume(tokens);

    if (parse_state.default_brk_label.base.count < 1) {
        msg(
            DIAG_BREAK_INVALID_LOCATION, break_token.pos,
            "break statement outside of a for loop\n"
        );
        return PARSE_ERROR;
    }

    Name break_out_of = parse_state.default_brk_label;
    if (try_consume(NULL, tokens, TOKEN_DOUBLE_TICK)) {
        Token token = {0};
        if (!consume_expect(&token, tokens, "(scope name)", TOKEN_SYMBOL)) {
            return PARSE_ERROR;
        }
        break_out_of = name_new(parse_state.curr_mod_path, token.text, (Ulang_type_darr) {0}, scope_id);
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

    *new_break = uast_yield_new(break_token.pos, do_break_expr, break_expr, name_clone(break_out_of, true, scope_id), true);
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
        break_out_of = name_new(parse_state.curr_mod_path, token.text, (Ulang_type_darr) {0}, scope_id);
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

    *new_yield = uast_yield_new(yield_token.pos, do_break_expr, break_expr, break_out_of, true);
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
        break_out_of = name_new(parse_state.curr_mod_path, token.text, (Ulang_type_darr) {0}, scope_id);
    } else {
        if (parse_state.default_brk_label.base.count < 1) {
            msg(
                DIAG_CONTINUE_INVALID_LOCATION, cont_token.pos,
                "continue statement outside of a for loop\n"
            );
            return PARSE_ERROR;
        }
        break_out_of = parse_state.default_brk_label;
    }

    *new_cont = uast_continue_new(cont_token.pos, name_clone(break_out_of, true, scope_id));
    return PARSE_OK;
}

static PARSE_STATUS parse_label(Tk_view* tokens, Scope_id scope_id) {
    Token sym_name = {0};
    unwrap(try_consume(&sym_name, tokens, TOKEN_SYMBOL));
    unwrap(try_consume(NULL, tokens, TOKEN_COLON));
    // scope will be updated when parsing the statement
    Name label_name = name_new(parse_state.curr_mod_path, sym_name.text, (Ulang_type_darr) {0}, scope_id);

    parse_state.new_scope_name_pos = sym_name.pos;
    parse_state.new_scope_name = label_name;
    return PARSE_OK;
}

static PARSE_STATUS parse_using(Uast_using** using, Tk_view* tokens, Scope_id scope_id) {
    Token using_tk = {0};
    unwrap(try_consume(&using_tk, tokens, TOKEN_USING));

    Token sym_name = {0};
    if (!consume_expect(&sym_name, tokens, "(module or struct name)", TOKEN_SYMBOL)) {
        return PARSE_ERROR;
    }

    *using = uast_using_new(using_tk.pos, name_new(parse_state.curr_mod_path, sym_name.text, (Ulang_type_darr) {0}, scope_id), parse_state.curr_mod_path);
    return PARSE_OK;
}

static PARSE_STATUS parse_defer(Uast_defer** defer, Tk_view* tokens, Scope_id scope_id) {
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

static PARSE_STATUS parse_function_decl(Uast_function_decl** fun_decl, Tk_view* tokens, Token fn_name_tk) {
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
    if (PARSE_OK != parse_function_decl_common(fun_decl, tokens, false, SCOPE_TOP_LEVEL, symbol_collection_new(SCOPE_TOP_LEVEL, util_literal_name_new()) /* TODO */, false, fn_name_tk)) {
        goto error;
    }
    try_consume_newlines(tokens);

    status = PARSE_OK;
error:
    return status;
}

static PARSE_STATUS parse_literal(Uast_expr** lit, Tk_view* tokens) {
    Token token = consume(tokens);
    unwrap(token_is_literal(token));

    if (util_try_uast_literal_new_from_strv(lit, token.text, token.type, token.pos)) {
        return PARSE_OK;
    }
    return PARSE_ERROR;
}

// TODO: parse_symbol should return PARSE_STATUS for consistancy?
static Uast_symbol* parse_symbol(Tk_view* tokens, Scope_id scope_id) {
    Token token = consume(tokens);
    unwrap(token.type == TOKEN_SYMBOL);

    return uast_symbol_new(token.pos, name_new(parse_state.curr_mod_path, token.text, (Ulang_type_darr) {0}, scope_id));
}

static PARSE_STATUS parse_function_call(Uast_function_call** child, Tk_view* tokens, Uast_expr* callee, Scope_id scope_id) {
    bool is_first_time = true;
    bool prev_is_comma = false;
    Uast_expr_darr args = {0};
    while (is_first_time || prev_is_comma) {
        Uast_expr* arg;
        switch (parse_expr(&arg, tokens, scope_id)) {
            case PARSE_EXPR_OK:
                unwrap(arg);
                darr_append(&a_main, &args, arg);
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
            unwrap(expr);
            *rtn_stmt = uast_return_new(uast_expr_get_pos(expr), expr, false);
            break;
        case PARSE_EXPR_NONE:
            *rtn_stmt = uast_return_new(
                parse_state.prev_token.pos,
                util_uast_literal_new_from_strv(
                    sv(""),
                    TOKEN_VOID,
                    parse_state.prev_token.pos
                ),
                false
            );
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_ERROR;
    }

    try_consume_newlines(tokens);
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
        case UAST_IF_ELSE_CHAIN:
            fallthrough;
        case UAST_BLOCK:
            fallthrough;
        case UAST_SWITCH:
            fallthrough;
        case UAST_UNKNOWN:
            fallthrough;
        case UAST_MEMBER_ACCESS:
            fallthrough;
        case UAST_INDEX:
            fallthrough;
        case UAST_STRUCT_LITERAL:
            fallthrough;
        case UAST_ARRAY_LITERAL:
            fallthrough;
        case UAST_TUPLE:
            fallthrough;
        case UAST_DIRECTIVE:
            fallthrough;
        case UAST_ENUM_ACCESS:
            fallthrough;
        case UAST_ENUM_GET_TAG:
            fallthrough;
        case UAST_ORELSE:
            fallthrough;
        case UAST_FN:
            fallthrough;
        case UAST_QUESTION_MARK:
            fallthrough;
        case UAST_UNDERSCORE:
            fallthrough;
        case UAST_EXPR_REMOVED:
            msg_todo("", uast_expr_get_pos(cond_child));
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
    Uast_if_darr ifs = {0};

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
    if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(parent, util_literal_name_new()), (Uast_stmt_darr) {0})) {
        return PARSE_ERROR;
    }
    darr_append(&a_main, &ifs, if_stmt);

    if_else_chain_consume_newline(tokens);
    bool has_appended_default = false;
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
            has_appended_default = true;
            if_stmt->condition = uast_condition_new(if_token.pos, NULL);
            if_stmt->condition->child = uast_condition_get_default_child(
                util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_token.pos)
            );
        }

        if (PARSE_OK != parse_block(&if_stmt->body, tokens, false, symbol_collection_new(parent, util_literal_name_new()), (Uast_stmt_darr) {0})) {
            return PARSE_ERROR;
        }
        darr_append(&a_main, &ifs, if_stmt);

        if_else_chain_consume_newline(tokens);
    }

    if (!has_appended_default) {
        if_stmt = uast_if_new(
            if_token.pos,
            uast_condition_new(if_token.pos, uast_condition_get_default_child(
                util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, if_token.pos)
            )),
            uast_block_new(
                if_stmt->pos,
                (Uast_stmt_darr) {0},
                if_stmt->pos,
                symbol_collection_new(parent, util_literal_name_new())
            )
        );
        darr_append(&a_main, &ifs, if_stmt);
    }

    Uast_stmt_darr chain_ = {0};
    darr_append(&a_main, &chain_, uast_expr_wrap(uast_if_else_chain_wrap(uast_if_else_chain_new(if_token.pos, ifs))));
    *if_else_chain = uast_block_new(if_token.pos, chain_, if_token.pos, parent);
    return PARSE_OK;
}

static PARSE_STATUS parse_if_let_internal(Uast_switch** lang_switch, Token if_token, Tk_view* tokens, Scope_id scope_id) {
    unwrap(try_consume(NULL, tokens, TOKEN_LET));
    Scope_id if_true_scope = symbol_collection_new(scope_id, util_literal_name_new());
    Scope_id if_false_scope = symbol_collection_new(scope_id, util_literal_name_new());

    Uast_expr* is_true_cond = NULL;
    switch (parse_generic_binary(&is_true_cond, tokens, if_true_scope, 0, 0)) {
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
    if (PARSE_OK != parse_block(&if_true, tokens, false, if_true_scope, (Uast_stmt_darr) {0})) {
        return PARSE_ERROR;
    }

    Uast_expr* is_false_cond = NULL;
    Uast_block* if_false = NULL;
    if (try_consume(NULL, tokens, TOKEN_ELSE)) {
        if (try_consume(&if_token, tokens, TOKEN_IF)) {
            msg(DIAG_IF_ELSE_IN_IF_LET, if_token.pos, "`else if` causes of `if let` statements are not supported\n");
            return PARSE_ERROR;
        }

        if (!starts_with_block(*tokens)) {
            Token open_par_tk = {0};
            if (!try_consume(&open_par_tk, tokens, TOKEN_OPEN_PAR)) {
                msg_parser_expected(
                    tk_view_front(*tokens),
                    "after `else` in if let statement",
                    TOKEN_OPEN_PAR,
                    TOKEN_ONE_LINE_BLOCK_START,
                    TOKEN_COLON,
                    TOKEN_OPEN_CURLY_BRACE
                );
                return false;
            }
            Uast_expr* arg = NULL;
            switch (parse_expr(&arg, tokens, if_false_scope)) {
                case PARSE_EXPR_OK:
                    break;
                case PARSE_EXPR_ERROR:
                    return PARSE_ERROR;
                case PARSE_EXPR_NONE:
                    msg_expected_expr(*tokens, "or block");
                    return PARSE_ERROR;
                default:
                    unreachable("");
            }
            Uast_expr_darr args = {0};
            darr_append(&a_main, &args, arg);
            is_false_cond = uast_function_call_wrap(uast_function_call_new(
                open_par_tk.pos,
                args,
                uast_unknown_wrap(uast_unknown_new(open_par_tk.pos)),
                true
            ));
            if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_PAR)) {
                return false;
            }
        }

        Uast_block* if_false_block = NULL;
        if (PARSE_OK != parse_block(&if_false_block, tokens, false, if_false_scope, (Uast_stmt_darr) {0})) {
            return PARSE_ERROR;
        }
        if_false = if_false_block;

        if_else_chain_consume_newline(tokens);
    }

    if (!if_false) {
        // TODO: make function for making empty block, and call it here?
        if_false = uast_block_new(if_token.pos, (Uast_stmt_darr) {0}, if_token.pos, if_false_scope);
    }


    Uast_case_darr cases = {0};

    if (is_true_cond->type != UAST_MEMBER_ACCESS && is_true_cond->type != UAST_FUNCTION_CALL) {
        msg(
            DIAG_IF_LET_INVALID_SYNTAX,
            uast_expr_get_pos(is_true_cond),
            "expected enum case (e.g. `.some(num)`) here\n"
        );
        return PARSE_ERROR;
    }

    Uast_case* if_true_case = uast_case_new(
        if_token.pos,
        false,
        is_true_cond,
        if_true,
        if_true_scope
    );
    darr_append(&a_main, &cases, if_true_case);

    Uast_case* if_false_case = NULL;
    if (is_false_cond) {
        if_false_case = uast_case_new(
            if_token.pos,
            false,
            is_false_cond,
            if_false,
            if_false_scope
        );
    } else {
        if_false_case = uast_case_new(
            if_token.pos,
            true,
            is_false_cond,
            if_false,
            if_false_scope
        );
    }
    darr_append(&a_main, &cases, if_false_case);

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
    unwrap(parse_state.new_scope_name.base.count > 0);
    Name old_default_brk_label = parse_state.default_brk_label;

    Scope_id parent = symbol_collection_new(grand_parent, util_literal_name_new());
    // TODO: (maybe not): extract this if and block_new into separate function
    if (PARSE_OK != label_thing(&parse_state.default_brk_label, parent)) {
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

    Uast_case_darr cases = {0};

    while (1) {
        Scope_id case_scope = symbol_collection_new(parent, util_literal_name_new());
        Uast_block* case_if_true = NULL;
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
            case_operand = NULL;
            case_is_default = true;
        } else {
            break;
        }
        
        switch (parse_block(&case_if_true, tokens, false, case_scope, (Uast_stmt_darr) {0})) {
            case PARSE_OK:
                break;
            case PARSE_ERROR:
                status = PARSE_ERROR;
                goto error;
            default:
                unreachable("");
        }
        Uast_case* curr_case = uast_case_new(
            case_if_true->pos,
            case_is_default,
            case_operand,
            case_if_true,
            case_scope
        );
        darr_append(&a_main, &cases, curr_case);
    }

    Uast_stmt_darr chain_ = {0};
    darr_append(&a_main, &chain_, uast_yield_wrap(uast_yield_new(
        start_token.pos,
        true,
        uast_switch_wrap(uast_switch_new(start_token.pos, operand, cases)),
        parse_state.default_brk_label,
        true /* TODO */
    )));

    if (cases.info.count < 1) {
        msg(DIAG_SWITCH_NO_CASES, start_token.pos, "switch statement must have at least one case statement\n");
        status = PARSE_ERROR;
        goto error;
    }
    *lang_switch = uast_block_new(start_token.pos, chain_, start_token.pos /* TODO */, parent);

    if (!consume_expect(NULL, tokens, "", TOKEN_CLOSE_CURLY_BRACE)) {
        status = PARSE_ERROR;
        // TODO: expeced failure case no close brace
        goto error;
    }

error:
    parse_state.default_brk_label = old_default_brk_label;
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

static PARSE_STATUS parse_general_def(Uast_stmt** result, Tk_view* tokens, Scope_id scope_id) {
    Token name_tk = {0};
    Token colon_tk = {0};
    unwrap(try_consume(&name_tk, tokens, TOKEN_SYMBOL));
    unwrap(try_consume(&colon_tk, tokens, TOKEN_COLON));
    if (!try_consume(NULL, tokens, TOKEN_COLON)) {
        msg_todo("single `:` here (`::` should be used for now)", colon_tk.pos);
        return PARSE_ERROR;
    }

    if (starts_with_struct_def_in_def(*tokens)) {
        Uast_struct_def* struct_def = NULL;
        if (PARSE_OK != parse_struct_def(&struct_def, tokens, name_tk)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_struct_def_wrap(struct_def));
    } else if (starts_with_raw_union_def_in_def(*tokens)) {
        Uast_raw_union_def* raw_union_def = NULL;
        if (PARSE_OK != parse_raw_union_def(&raw_union_def, tokens, name_tk)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_raw_union_def_wrap(raw_union_def));
    } else if (starts_with_enum_def_in_def(*tokens)) {
        Uast_enum_def* enum_def = NULL;
        if (PARSE_OK != parse_enum_def(&enum_def, tokens, name_tk)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_enum_def_wrap(enum_def));
    } else if (starts_with_mod_alias_in_def(*tokens)) {
        Uast_mod_alias* import = NULL;
        if (PARSE_OK != parse_import(&import, tokens, name_tk)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_mod_alias_wrap(import));
    } else if (starts_with_lang_def(*tokens)) {
        Uast_lang_def* lang_def = NULL;
        if (PARSE_OK != parse_lang_def(&lang_def, tokens, name_tk, scope_id)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_lang_def_wrap(lang_def));
    } else if (starts_with_function_decl(*tokens)) {
        Uast_function_decl* fun_decl;
        if (PARSE_OK != parse_function_decl(&fun_decl, tokens, name_tk)) {
            return PARSE_ERROR;
        }
        *result = uast_def_wrap(uast_function_decl_wrap(fun_decl));
    } else if (starts_with_function_def(*tokens)) {
        if (PARSE_OK != parse_function_def(result, tokens, false, scope_id, name_tk)) {
            return PARSE_ERROR;
        }
    } else {
        msg_parser_expected(
            tk_view_front(*tokens), "",
            TOKEN_STRUCT, TOKEN_RAW_UNION, TOKEN_ENUM, TOKEN_DEF, TOKEN_IMPORT
        );
        return PARSE_ERROR;
    }
    unwrap(*result);
    return PARSE_OK;
}

static PARSE_EXPR_STATUS parse_stmt(Uast_stmt** child, Tk_view* tokens, Scope_id scope_id) {
    // TODO: use try_consume_newlines(tokens) instead of try_consume(NULL, tokens, TOKEN_NEW_LINE)
    try_consume_newlines(tokens);
    unwrap(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    if (starts_with_label(*tokens, scope_id)) {
        if (PARSE_OK != parse_label(tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        unwrap(parse_state.new_scope_name.base.count > 0);
    } else if (parse_state.new_scope_name.base.count < 1) {
        parse_state.new_scope_name_pos = POS_BUILTIN;
        parse_state.new_scope_name = util_literal_name_new_prefix_scope(sv("scope_name"), scope_id);
    }

    Uast_stmt* lhs = NULL;
    if (starts_with_general_def(*tokens)) {
        if (PARSE_OK != parse_general_def(&lhs, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
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
    } else if (starts_with_return(*tokens)) {
        Uast_return* rtn_stmt = NULL;
        if (PARSE_OK != parse_return(&rtn_stmt, tokens, scope_id)) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_return_wrap(rtn_stmt);
    } else if (starts_with_for(*tokens)) {
        unwrap(parse_state.new_scope_name.base.count > 0);
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
        if (PARSE_OK != parse_block(&block_def, tokens, false, symbol_collection_new(scope_id, util_literal_name_new()), (Uast_stmt_darr) {0})) {
            return PARSE_EXPR_ERROR;
        }
        lhs = uast_expr_wrap(uast_block_wrap(block_def));
    } else if (starts_with_variable_def(*tokens)) {
        Uast_variable_def* var_def = NULL;
        if (PARSE_OK != parse_variable_def(&var_def, tokens, true, true, true, false, (Ulang_type) {0}, scope_id)) {
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
    unwrap(!try_consume(NULL, tokens, TOKEN_NEW_LINE));

    return PARSE_EXPR_OK;
}

static PARSE_STATUS parse_block(
    Uast_block** block,
    Tk_view* tokens,
    bool is_top_level,
    Scope_id new_scope,
    Uast_stmt_darr init_children
) {
    PARSE_STATUS status = PARSE_OK;
    bool is_one_line = false;

    Name dummy = {0};
    if (parse_state.new_scope_name.base.count > 0 && PARSE_OK != label_thing(&dummy, new_scope)) {
        status = PARSE_ERROR;
    }
    *block = uast_block_new(tk_view_front(*tokens).pos, init_children, tk_view_front(*tokens).pos, new_scope);

    Token open_brace_token = {0};
    if (!is_top_level && !try_consume(&open_brace_token, tokens, TOKEN_OPEN_CURLY_BRACE)) {
        if (try_consume(&open_brace_token, tokens, TOKEN_ONE_LINE_BLOCK_START) || try_consume(&open_brace_token, tokens, TOKEN_COLON)) {
            is_one_line = true;
        } else {
            msg_parser_expected(
                tk_view_front(*tokens),
                "at start of block",
                TOKEN_OPEN_CURLY_BRACE,
                TOKEN_ONE_LINE_BLOCK_START,
                TOKEN_COLON
            );
            status = PARSE_ERROR;
        }
    }
    // TODO: consider if these new lines should be allowed even with one line block
    if (!is_one_line) {
        while (try_consume(NULL, tokens, TOKEN_NEW_LINE));
    }

    // NOTE: this if statement should always run even if parse_block returns PARSE_ERROR
    while (parse_state.using_params.info.count > 0) {
        darr_append(&a_main, &(*block)->children, uast_using_wrap(darr_pop(&parse_state.using_params)));
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
                unwrap(child);
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
        unwrap(!try_consume_newlines(tokens));
        darr_append(&a_main, &(*block)->children, child);
    } while (!is_one_line);

    Token block_end = {0};
    if (!is_top_level && !is_one_line && status == PARSE_OK && !try_consume(&block_end, tokens, TOKEN_CLOSE_CURLY_BRACE)) {
        msg_parser_expected(tk_view_front(*tokens), "at the end of the block", TOKEN_CLOSE_CURLY_BRACE);
        return PARSE_ERROR;
    }
    if (is_one_line) {
        assert(!is_top_level);
        block_end = parse_state.prev_token;
    }
    (*block)->pos_end = block_end.pos;

    try_consume_newlines(tokens);

end:
    parse_state.new_scope_name_when_leaving_block = parse_state.new_scope_name;
    return status;
}

static PARSE_STATUS parse_struct_literal_members(Uast_expr_darr* members, Tk_view* tokens, Scope_id scope_id) {
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
        darr_append(&a_main, members, memb);
        try_consume_newlines(tokens);
    } while (try_consume(NULL, tokens, TOKEN_COMMA));

    return PARSE_OK;
}


static PARSE_STATUS parse_struct_literal(Uast_struct_literal** struct_lit, Tk_view* tokens, Scope_id scope_id) {
    Token start_token;
    if (!consume_expect(&start_token, tokens, "at start of struct literal", TOKEN_OPEN_CURLY_BRACE)) {
        return PARSE_ERROR;
    }

    Uast_expr_darr members = {0};
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
    
    Uast_expr_darr members = {0};
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
        *result = uast_directive_wrap(uast_directive_new(pos, tk_view_front(*tokens).text, pos));
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
                name_new(parse_state.curr_mod_path, memb_name.text, (Ulang_type_darr) {0}, scope_id)
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
    unwrap(*result);
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
                name_new(parse_state.curr_mod_path, memb_name.text, (Ulang_type_darr) {0}, scope_id)
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

static PARSE_EXPR_STATUS parse_orelse_finish(
    Uast_block** result,
    Tk_view* tokens,
    Uast_expr* lhs,
    Pos pos,
    Scope_id parent
) {
    Scope_id outer = symbol_collection_new(parent, util_literal_name_new());
    Scope_id if_error_scope = symbol_collection_new(outer, util_literal_name_new());

    bool is_err_symbol = false;
    Uast_symbol* err_symbol = NULL;

    Token dummy = {0};
    if (try_consume(&dummy, tokens, TOKEN_OPEN_PAR)) {
        if (tk_view_front(*tokens).type != TOKEN_SYMBOL) {
            msg_parser_expected(tk_view_front(*tokens), "", TOKEN_SYMBOL);
            return PARSE_EXPR_ERROR;
        }
        is_err_symbol = true;
        err_symbol = parse_symbol(tokens, if_error_scope);
        if (!consume_expect(&dummy, tokens, "after name of error symbol", TOKEN_CLOSE_PAR)) {
            return PARSE_EXPR_ERROR;
        }
    }

    Uast_block* if_error = NULL;
    if (PARSE_OK != parse_block(
        &if_error,
        tokens,
        false,
        if_error_scope,
        (Uast_stmt_darr) {0}
    )) {
        return PARSE_EXPR_ERROR;
    }

    Name break_out_of = {0};
    if (PARSE_OK != label_thing(&break_out_of, outer)) {
        return PARSE_EXPR_ERROR;
    }
    assert(break_out_of.base.count > 0);

    Uast_orelse* orelse = uast_orelse_new(
        pos,
        lhs,
        if_error,
        outer,
        break_out_of,
        is_err_symbol,
        err_symbol
    );

    Uast_stmt_darr block_children = {0};
    darr_append(&a_main, &block_children, uast_expr_wrap(uast_orelse_wrap(orelse)));
    *result = uast_block_new(
        orelse->pos,
        block_children,
        orelse->if_error->pos_end,
        outer
    );
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_question_mark_finish(
    Uast_block** result,
    Uast_expr* lhs,
    Pos pos,
    Scope_id parent
) {
    Scope_id outer = symbol_collection_new(parent, util_literal_name_new());

    Name break_out_of = {0};
    if (PARSE_OK != label_thing(&break_out_of, outer)) {
        return PARSE_EXPR_ERROR;
    }

    Uast_question_mark* mark = uast_question_mark_new(pos, lhs, outer, break_out_of);
    Uast_stmt_darr block_children = {0};
    darr_append(&a_main, &block_children, uast_expr_wrap(uast_question_mark_wrap(mark)));
    *result = uast_block_new(
        mark->pos,
        block_children,
        mark->pos,
        outer
    );
    return PARSE_EXPR_OK;
}

static PARSE_EXPR_STATUS parse_right_unary(
    Uast_expr** result,
    Tk_view* tokens,
    Scope_id scope_id
) {
    PARSE_EXPR_STATUS status = parse_high_presidence(result, tokens, scope_id);
    if (status != PARSE_EXPR_OK) {
        return status;
    }

    if (!is_right_unary(tk_view_front(*tokens).type)) {
        return PARSE_EXPR_OK;
    }

    Token oper = consume(tokens);
    static_assert(TOKEN_COUNT == 79, "exhausive handling of token types");
    switch (oper.type) {
        case TOKEN_ORELSE: {
            Uast_block* result_ = NULL;
            status = parse_orelse_finish(&result_, tokens, *result, oper.pos, scope_id);
            if (status != PARSE_EXPR_OK) {
                return status;
            }
            *result = uast_block_wrap(result_);
            return PARSE_EXPR_OK;
        }
        case TOKEN_QUESTION_MARK: {
            Uast_block* result_ = NULL;
            status = parse_question_mark_finish(&result_, *result, oper.pos, scope_id);
            if (status != PARSE_EXPR_OK) {
                return status;
            }
            *result = uast_block_wrap(result_);
            return PARSE_EXPR_OK;
        }
        case TOKEN_NONTYPE:
            fallthrough;
        case TOKEN_SINGLE_PLUS:
            fallthrough;
        case TOKEN_SINGLE_MINUS:
            fallthrough;
        case TOKEN_ASTERISK:
            fallthrough;
        case TOKEN_MODULO:
            fallthrough;
        case TOKEN_SLASH:
            fallthrough;
        case TOKEN_LESS_THAN:
            fallthrough;
        case TOKEN_LESS_OR_EQUAL:
            fallthrough;
        case TOKEN_GREATER_THAN:
            fallthrough;
        case TOKEN_GREATER_OR_EQUAL:
            fallthrough;
        case TOKEN_DOUBLE_EQUAL:
            fallthrough;
        case TOKEN_LOGICAL_NOT_EQUAL:
            fallthrough;
        case TOKEN_BITWISE_AND:
            fallthrough;
        case TOKEN_BITWISE_OR:
            fallthrough;
        case TOKEN_BITWISE_XOR:
            fallthrough;
        case TOKEN_LOGICAL_AND:
            fallthrough;
        case TOKEN_LOGICAL_OR:
            fallthrough;
        case TOKEN_SHIFT_LEFT:
            fallthrough;
        case TOKEN_SHIFT_RIGHT:
            fallthrough;
        case TOKEN_LOGICAL_NOT:
            fallthrough;
        case TOKEN_BITWISE_NOT:
            fallthrough;
        case TOKEN_UNSAFE_CAST:
            fallthrough;
        case TOKEN_STRING_LITERAL:
            fallthrough;
        case TOKEN_INT_LITERAL:
            fallthrough;
        case TOKEN_FLOAT_LITERAL:
            fallthrough;
        case TOKEN_VOID:
            fallthrough;
        case TOKEN_CHAR_LITERAL:
            fallthrough;
        case TOKEN_OPEN_PAR:
            fallthrough;
        case TOKEN_OPEN_CURLY_BRACE:
            fallthrough;
        case TOKEN_OPEN_SQ_BRACKET:
            fallthrough;
        case TOKEN_OPEN_GENERIC:
            fallthrough;
        case TOKEN_CLOSE_PAR:
            fallthrough;
        case TOKEN_CLOSE_CURLY_BRACE:
            fallthrough;
        case TOKEN_CLOSE_SQ_BRACKET:
            fallthrough;
        case TOKEN_CLOSE_GENERIC:
            fallthrough;
        case TOKEN_ENUM:
            fallthrough;
        case TOKEN_SYMBOL:
            fallthrough;
        case TOKEN_DOUBLE_QUOTE:
            fallthrough;
        case TOKEN_NEW_LINE:
            fallthrough;
        case TOKEN_COMMA:
            fallthrough;
        case TOKEN_COLON:
            fallthrough;
        case TOKEN_SINGLE_EQUAL:
            fallthrough;
        case TOKEN_SINGLE_DOT:
            fallthrough;
        case TOKEN_DOUBLE_DOT:
            fallthrough;
        case TOKEN_TRIPLE_DOT:
            fallthrough;
        case TOKEN_EOF:
            fallthrough;
        case TOKEN_ASSIGN_BY_BIN:
            fallthrough;
        case TOKEN_MACRO:
            fallthrough;
        case TOKEN_DEFER:
            fallthrough;
        case TOKEN_DOUBLE_TICK:
            fallthrough;
        case TOKEN_ONE_LINE_BLOCK_START:
            fallthrough;
        case TOKEN_UNDERSCORE:
            fallthrough;
        case TOKEN_FN:
            fallthrough;
        case TOKEN_FOR:
            fallthrough;
        case TOKEN_IF:
            fallthrough;
        case TOKEN_SWITCH:
            fallthrough;
        case TOKEN_CASE:
            fallthrough;
        case TOKEN_DEFAULT:
            fallthrough;
        case TOKEN_ELSE:
            fallthrough;
        case TOKEN_RETURN:
            fallthrough;
        case TOKEN_EXTERN:
            fallthrough;
        case TOKEN_STRUCT:
            fallthrough;
        case TOKEN_LET:
            fallthrough;
        case TOKEN_IN:
            fallthrough;
        case TOKEN_BREAK:
            fallthrough;
        case TOKEN_YIELD:
            fallthrough;
        case TOKEN_CONTINUE:
            fallthrough;
        case TOKEN_RAW_UNION:
            fallthrough;
        case TOKEN_TYPE_DEF:
            fallthrough;
        case TOKEN_GENERIC_TYPE:
            fallthrough;
        case TOKEN_IMPORT:
            fallthrough;
        case TOKEN_DEF:
            fallthrough;
        case TOKEN_SIZEOF:
            fallthrough;
        case TOKEN_COUNTOF:
            fallthrough;
        case TOKEN_USING:
            fallthrough;
        case TOKEN_COMMENT:
            fallthrough;
        case TOKEN_AT_SIGN:
            fallthrough;
        case TOKEN_COUNT:
            msg_todo("", oper.pos);
    }
    unreachable("");
}

WSWITCH_ENUM_IGNORE_START

static PARSE_EXPR_STATUS parse_unary(
    Uast_expr** result,
    Tk_view* tokens,
    bool can_be_tuple, // TODO: remove this parameter?
    Scope_id scope_id
) {
    (void) can_be_tuple;

    if (!is_unary(tk_view_front(*tokens).type)) {
        return parse_right_unary(result, tokens, scope_id);
    }
    Token oper = consume_unary(tokens);

    Uast_expr* child = NULL;
    // this is a placeholder type
    Ulang_type unary_lang_type = ulang_type_new_int_x(tk_view_front(*tokens).pos/*TODO*/, sv("i32"));

    static_assert(TOKEN_COUNT == 79, "exhausive handling of token types (only unary operators need to be handled here");
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
            child = uast_expr_removed_wrap(uast_expr_removed_new(
                tk_view_front(*tokens).pos,
                sv("after unary operator")
            ));
            break;
        default:
            unreachable("");
    }

    static_assert(TOKEN_COUNT == 79, "exhausive handling of token types (only unary operators need to be handled here");
    switch (oper.type) {
        case TOKEN_BITWISE_NOT: {
            Uast_expr_darr args = {0};
            darr_append(&a_main, &args, child);
            *result = uast_function_call_wrap(uast_function_call_new(
                oper.pos,
                args,
                uast_symbol_wrap(uast_symbol_new(oper.pos, name_new(
                    MOD_PATH_RUNTIME,
                    sv("bitwise_not"),
                    (Ulang_type_darr) {0},
                    SCOPE_TOP_LEVEL
                ))),
                false
            ));
        } break;
        case TOKEN_LOGICAL_NOT:
            fallthrough;
        case TOKEN_ASTERISK:
            fallthrough;
        case TOKEN_BITWISE_AND:
            fallthrough;
        case TOKEN_UNSAFE_CAST:
            *result = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                oper.pos,
                child,
                token_type_to_unary_type(oper.type),
                unary_lang_type
            )));
            unwrap(*result);
            break;
        case TOKEN_SINGLE_MINUS: {
            *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(
                oper.pos,
                uast_literal_wrap(uast_int_wrap(uast_int_new(oper.pos, 0))),
                child,
                token_type_to_binary_type(oper.type)
            )));
            unwrap(*result);
            break;
        }
        case TOKEN_COUNTOF:
            fallthrough;
        case TOKEN_SIZEOF:
            *result = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                oper.pos,
                child,
                token_type_to_unary_type(oper.type),
                ulang_type_new_usize(oper.pos)
            )));
            unwrap(*result);
            break;
        default:
            unreachable(FMT, token_print(TOKEN_MODE_LOG, oper));
    }

    return PARSE_EXPR_OK;
}

WSWITCH_ENUM_IGNORE_END

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
        case UAST_IF_ELSE_CHAIN:
            fallthrough;
        case UAST_BLOCK:
            fallthrough;
        case UAST_SWITCH:
            fallthrough;
        case UAST_UNKNOWN:
            fallthrough;
        case UAST_OPERATOR:
            fallthrough;
        case UAST_INDEX:
            fallthrough;
        case UAST_LITERAL:
            fallthrough;
        case UAST_FUNCTION_CALL:
            fallthrough;
        case UAST_STRUCT_LITERAL:
            fallthrough;
        case UAST_ARRAY_LITERAL:
            fallthrough;
        case UAST_TUPLE:
            fallthrough;
        case UAST_DIRECTIVE:
            fallthrough;
        case UAST_ENUM_ACCESS:
            fallthrough;
        case UAST_ENUM_GET_TAG:
            fallthrough;
        case UAST_ORELSE:
            fallthrough;
        case UAST_FN:
            fallthrough;
        case UAST_QUESTION_MARK:
            fallthrough;
        case UAST_UNDERSCORE:
            fallthrough;
        case UAST_EXPR_REMOVED:
            msg_todo("", uast_expr_get_pos(lhs));
            return PARSE_ERROR;
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

static_assert(TOKEN_COUNT == 79, "exhausive handling of token types; only binary operators need to be explicitly handled here");
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
        unwrap(*result);
        return PARSE_EXPR_OK;
    }
    try_consume_newlines(tokens);

    Uast_expr* rhs = NULL;
    PARSE_EXPR_STATUS status = parse_generic_binary(&rhs, tokens, scope_id, bin_idx + 1, depth + 1);
    switch (status) {
        case PARSE_EXPR_OK:
            break;
        case PARSE_EXPR_NONE:
            rhs = uast_expr_removed_wrap(uast_expr_removed_new(
                tk_view_front(*tokens).pos,
                sv("after binary operator")
            ));
            break;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_ERROR;
        default:
            unreachable("");
    }

    *result = uast_operator_wrap(uast_binary_wrap(uast_binary_new(oper.pos, lhs, rhs, binary_type_from_token_type(oper.type))));

    if (!try_peek_1_of_4(&oper, tokens, bin_type_1, bin_type_2, bin_type_3, bin_type_4)) {
        unwrap(*result);
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
    if (bin_idx >= array_count(BIN_IDX_TO_TOKEN_TYPES)) {
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
    Token underscore_tk = {0};
    if (try_consume(&underscore_tk, tokens, TOKEN_UNDERSCORE)) {
        *result = uast_underscore_wrap(uast_underscore_new(underscore_tk.pos));
        return PARSE_EXPR_OK;
    }

    if (tk_view_front(*tokens).type == TOKEN_FN) {
        Parse_state saved_point = parse_state;
        Tk_view saved_tk_view = *tokens;

        Ulang_type lang_type = {0};
        uint32_t prev_error_count = env.error_count;
        (void) prev_error_count;
        if (
            PARSE_OK == parse_lang_type_struct_require_ex(&lang_type, tokens, scope_id, false) &&
            !starts_with_block(*tokens)
        ) {
            *result = uast_fn_wrap(uast_fn_new(
                ulang_type_get_pos(lang_type),
                ulang_type_fn_const_unwrap(lang_type)
            ));
            return PARSE_EXPR_OK;
        }
        assert(
            env.error_count == prev_error_count &&
            "error count should not have been incremented in"
            "parse_lang_type_struct_require_ex when print_error == false"
        );

        *tokens = parse_state_restore(saved_point, saved_tk_view);
        Uast_stmt* fun_def_ = NULL;
        if (PARSE_OK == parse_function_def(&fun_def_, tokens, true, scope_id, (Token) {0})) {
            if (fun_def_->type != UAST_DEF || uast_def_unwrap(fun_def_)->type != UAST_FUNCTION_DEF) {
                msg_todo("", uast_stmt_get_pos(fun_def_));
                return PARSE_EXPR_ERROR;
            }
            Uast_function_def* fun_def = uast_function_def_unwrap(uast_def_unwrap(fun_def_));

            *result = uast_symbol_wrap(uast_symbol_new(fun_def->pos, fun_def->decl->name));
            return PARSE_EXPR_OK;
        }

        unwrap(PARSE_ERROR == parse_lang_type_struct_require_ex(&lang_type, tokens, scope_id, true));
        return PARSE_EXPR_ERROR;
    }

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
        unwrap(*result);
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
            unwrap(*result);
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
    Scope_id old_scope_id_curr_mod_path = parse_state.scope_id_curr_mod_path;

    if (strv_is_equal(MOD_PATH_BUILTIN, file_strip_extension(file_basename(file_path)))) {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN, "file path with basename `builtin.own` is not permitted\n");
        status = false;
        goto error;
    }

    if (file_basename(file_path).count < 1) {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN, "file path with basename length of zero is not permitted\n");
        status = false;
        goto error;
    }

    if (strv_at(file_basename(file_path), 0) == '.') {
        msg(DIAG_FILE_INVALID_NAME, POS_BUILTIN, "file path that starts with `.` is not permitted\n");
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
            status = false;
            goto error;
        }
    }

    Scope_id new_scope = symbol_collection_new(SCOPE_TOP_LEVEL, util_literal_name_new());
    parse_state.scope_id_curr_mod_path = new_scope;

#ifndef NDEBUG
    // TODO: reenable
    //parser_do_tests();
#endif // NDEBUG

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
        darr_append(
            &a_pass,
            &parse_state.using_params,
            uast_using_new(((Pos) {.line = 0, .file_path = sv("std/runtime.own") /* TODO: avoid hardcoding path */}) /* TODO: change this to prelude_alias->pos */, prelude_alias->name, file_strip_extension(file_path))
        );
    }
    if (PARSE_OK != parse_block(block, &tokens, true, new_scope, (Uast_stmt_darr) {0})) {
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
    parse_state.scope_id_curr_mod_path = old_scope_id_curr_mod_path;
    return status;
}

bool parse(void) {
    bool status = true;

    Name alias_name = name_new(MOD_PATH_AUX_ALIASES, MOD_PATH_BUILTIN, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL);
    unwrap(usymbol_add(uast_mod_alias_wrap(uast_mod_alias_new(
        POS_BUILTIN,
        alias_name,
        MOD_PATH_BUILTIN,
        SCOPE_TOP_LEVEL,
        true /* TODO */
    ))));

    Uast_mod_alias* dummy = NULL;
    if (!get_mod_alias_from_path_token(
        &dummy,
        token_new(MOD_ALIAS_TOP_LEVEL.base, TOKEN_SYMBOL),
        POS_BUILTIN,
        file_strip_extension(params.input_file_path),
        false,
        true,
        POS_BUILTIN
    )) {
        status = false;
    }

    while (parse_state.mod_paths_to_parse.info.count > 0) {
        Import_strv curr_mod = darr_pop(&parse_state.mod_paths_to_parse);

        Strv old_mod_path = parse_state.curr_mod_path;
        parse_state.curr_mod_path = curr_mod.curr_mod_path;
        env.mod_path_curr_file = curr_mod.curr_mod_path;
        Name old_mod_alias = parse_state.curr_mod_alias;
        parse_state.curr_mod_alias = curr_mod.curr_mod_alias;

        String file_path = {0};
        string_extend_strv(&a_main, &file_path, curr_mod.mod_path);
        string_extend_cstr(&a_main, &file_path, ".own");
        assert(parse_state.curr_mod_path.count > 0);
        Uast_block* block = NULL;
        if (!parse_file(&block, string_to_strv(file_path), curr_mod.import_pos)) {
            status = false;
            goto loop_end;
        }

        assert(block->scope_id != SCOPE_TOP_LEVEL && "this will cause infinite recursion");
        // TODO: replace block in existing import path instead of making new import path?
        usym_tbl_update(uast_import_path_wrap(uast_import_path_new(
            curr_mod.mod_path_pos,
            block,
            curr_mod.mod_path
        )));

loop_end:
        parse_state.curr_mod_path = old_mod_path;
        env.mod_path_curr_file = old_mod_path;
        parse_state.curr_mod_alias = old_mod_alias;
    }

    return status;
}

static void parser_test_parse_expr(const char* input, int test) {
    (void) input;
    (void) test;
    todo();
    //Env env = {0};
    //Strv file_text = sv(input);
    //env.file_path_to_text = file_text;

    //Token_darr tokens_ = {0};
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

    //Token_darr tokens_ = {0};
    //unwrap(tokenize(&tokens_, & (Strv) {0}));
    //Tk_view tokens = tokens_to_tk_view(tokens_);
    //log_tokens(LOG_DEBUG, tokens);
    //Uast_stmt* result = NULL;
    //switch (parse_stmt(& &result, &tokens, true)) {
    //    case PARSE_EXPR_OK:
    //        log(LOG_DEBUG, "\n"FMT, uast_stmt_print(result));
    //        break;
    //    default:
    //        unwrap(error_count > 0);
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
