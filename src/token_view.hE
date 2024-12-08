#ifndef TOKEN_VIEW_H
#define TOKEN_VIEW_H

#include <stddef.h>
#include "token.h"
#include "assert.h"

typedef struct {
    const Token* tokens;
    size_t count;
} Tk_view;

#define log_tokens(log_level, token_view) \
    do { \
        log((log_level), "tokens:\n"); \
        for (size_t idx = 0; idx < (token_view).count; idx++) { \
            log((log_level), TOKEN_FMT"\n", token_print((token_view).tokens[idx])); \
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
static inline bool get_idx_token(size_t* idx_matching, Tk_view tokens, size_t start, TOKEN_TYPE type_to_match) {
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

static inline Token tk_view_at(Tk_view token_view, size_t idx) {
    assert(token_view.count > idx);
    return token_view.tokens[idx];
}

static inline Token tk_view_front(Tk_view token_view) {
    return tk_view_at(token_view, 0);
}

static inline Tk_view tk_view_consume_count(Tk_view* token_view, size_t count) {
    if (token_view->count < count) {
        unreachable("out of bounds");
    }
    Tk_view result = {.tokens = token_view->tokens, .count = count};
    token_view->tokens += count;
    token_view->count -= count;
    return result;
}

static inline Tk_view tk_view_consume_count_at_most(Tk_view* token_view, size_t count) {
    return tk_view_consume_count(token_view, MIN(token_view->count, count));
}

static inline Token tk_view_consume(Tk_view* token_view) {
    return tk_view_front(tk_view_consume_count(token_view, 1));
}

static inline bool tk_view_try_consume(Token* result, Tk_view* tokens, TOKEN_TYPE expected) {
    if (tokens->count < 1 || tk_view_front(*tokens).type != expected) {
        return false;
    }
    
    Token token = tk_view_consume(tokens);
    if (result) {
        *result = token;
    }
    assert(result);
    return true;
}

static inline bool tk_view_try_consume_symbol(Token* result, Tk_view* tokens, const char* cstr) {
    if (!str_view_cstr_is_equal(tk_view_front(*tokens).text, cstr)) {
        return false;
    }
    return tk_view_try_consume(result, tokens, TOKEN_SYMBOL);
}

static inline Tk_view tk_view_consume_while(
    Tk_view* tk_view,
    bool (*should_continue)(const Token* /* previous token */, const Token* /* current token */)
) {
    Tk_view new_tk_view;
    for (size_t idx = 0; tk_view->count > idx; idx++) {
        const Token* prev_token = idx > 0 ? (&tk_view->tokens[idx]) : (NULL);
        if (!should_continue(prev_token, &tk_view->tokens[idx])) {
            new_tk_view.tokens = tk_view->tokens;
            new_tk_view.count = idx;
            tk_view->tokens += idx;
            tk_view->count -= idx;
            return new_tk_view;
        }
    }
    todo();
}

static inline Tk_view tk_view_consume_until_common(Tk_view* token_view, TOKEN_TYPE delim, bool or_all_fallback) {
    Tk_view new_token_view;
    size_t idx = 0;
    for (; idx < token_view->count; idx++) {
        if (token_view->tokens[idx].type == delim) {
            break;
        }
    }

    if (!or_all_fallback && (idx >= token_view->count)) {
        unreachable("delim not found");
    }

    new_token_view.tokens = token_view->tokens;
    new_token_view.count = idx;
    token_view->tokens += idx;
    token_view->count -= idx;
    return new_token_view;
}

// delim itself is left in the input token_view
static inline Tk_view tk_view_consume_until(Tk_view* token_view, TOKEN_TYPE delim) {
    return tk_view_consume_until_common(token_view, delim, false);
}

// delim itself is left in the input token_view
// consume all if delim not found
static inline Tk_view tk_view_consume_until_or_all(Tk_view* token_view, TOKEN_TYPE delim) {
    return tk_view_consume_until_common(token_view, delim, true);
}

static inline Tk_view tk_view_consume_until_matching_delim_common(
    Tk_view* token_view,
    TOKEN_TYPE delim,
    bool matching_opening_included,
    bool or_all_fallback
) {
    size_t idx_matching;
    if (!get_idx_matching_token(&idx_matching, *token_view, matching_opening_included, delim)) {
        if (!or_all_fallback) {
            unreachable("");
        }
        return tk_view_consume_count(token_view, token_view->count);
    }
    Tk_view result = tk_view_consume_count(token_view, idx_matching);
    return result;
}

// delim itself is left in the input token_view
static inline Tk_view tk_view_consume_until_matching_delim(
    Tk_view* token_view,
    TOKEN_TYPE delim,
    bool matching_opening_included // if true, matching opening ( in tokens
    ) {
    return tk_view_consume_until_matching_delim_common(token_view, delim, matching_opening_included, false);
}

// considers matching ()
// delim itself is left in the input token_view
// consume all if delim not found
static inline Tk_view tk_view_consume_until_matching_delim_or_all(
    Tk_view* token_view,
    TOKEN_TYPE delim,
    bool matching_opening_included // if true, matching opening ( in tokens
    ) {
    return tk_view_consume_until_matching_delim_common(token_view, delim, matching_opening_included, true);
}

static inline Tk_view tk_view_consume_until_matching_type_delims_or_all(
    Tk_view* token_view,
    TOKEN_TYPE delim1,
    TOKEN_TYPE delim2,
    bool matching_opening_included // if true, matching opening ( in tokens
) {
    Tk_view option_1_tokens = *token_view;
    Tk_view option_2_tokens = *token_view;
    Tk_view option_1 = tk_view_consume_until_matching_delim_common(&option_1_tokens, delim1, matching_opening_included, true);
    Tk_view option_2 = tk_view_consume_until_matching_delim_common(&option_2_tokens, delim2, matching_opening_included, true);
    if (option_1.count < option_2.count) {
        *token_view = option_1_tokens;
        return option_1;
    }
    *token_view = option_2_tokens;
    return option_2;
}

static inline bool tk_view_try_find(
    size_t* first_occcurance,
    Tk_view token_view,
    TOKEN_TYPE token_type
) {
    for (size_t idx = 0; idx < token_view.count; idx++) {
        if (token_view.tokens[idx].type == token_type) {
            *first_occcurance = idx;
            return true;
        }
    }
    return false;
}

static inline Str_view tk_view_print_internal(Arena* arena, Tk_view token_view) {
    String buf = {0};
    vec_reset(&buf);

    for (size_t idx = 0; idx < token_view.count; idx++) {
        string_extend_strv(&print_arena, &buf, token_print_internal(arena, token_view.tokens[idx], false));
        string_extend_cstr(&print_arena, &buf, ";    ");
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

#define TK_VIEW_FMT STR_VIEW_FMT

#define tk_view_print(token_view) str_view_print(tk_view_print_internal(token_view))

#endif // TOKEN_VIEW_H
