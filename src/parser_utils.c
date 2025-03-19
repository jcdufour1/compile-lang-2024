#include <str_view.h>
#include <newstring.h>
#include <string_vec.h>
#include <uast.h>
#include <tast.h>
#include <tasts.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <limits.h>
#include <ctype.h>
#include <token_type_to_operator_type.h>
#include <sizeof.h>
#include <symbol_log.h>
#include <lang_type_get_pos.h>

bool try_str_view_octal_after_0_to_int64_t(int64_t* result, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view.str[idx];

        *result *= 8;
        *result += curr_char - '0';
    }

    return idx > 0;
}

bool try_str_view_hex_after_0x_to_int64_t(int64_t* result, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view.str[idx];
        int increment = 0;
        if (isdigit(curr_char)) {
            increment = curr_char - '0';
        } else if (curr_char >= 'a' && curr_char <= 'f') {
            increment = (curr_char - 'a') + 10;
        } else if (curr_char >= 'A' && curr_char <= 'F') {
            increment = (curr_char - 'A') + 10;
        } else {
            assert(!ishex(curr_char));
            break;
        }

        *result *= 16;
        *result += increment;
    }

    return idx > 0;
}

bool try_str_view_to_int64_t(int64_t* result, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view.str[idx];
        log(LOG_DEBUG, "idx: %zu char: %c\n", idx, curr_char);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char == '0' && idx == 0 && str_view.count > 1 && isdigit(str_view_at(str_view, 1))) {
            return try_str_view_octal_after_0_to_int64_t(result, str_view_slice(str_view, 1, str_view.count - 1));
        }

        if (isalpha(curr_char)) {
            if (curr_char != 'x' || idx != 1) {
                todo();
            }
            return try_str_view_hex_after_0x_to_int64_t(result, str_view_slice(str_view, 2, str_view.count - 2));
        }

        if (!isdigit(curr_char)) {
            break;
        }

        *result *= 10;
        *result += curr_char - '0';
    }

    return idx > 0;
}

bool try_str_view_to_size_t(size_t* result, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view.str[idx];
        if (!isdigit(curr_char)) {
            break;
        }

        *result *= 10;
        *result += curr_char - '0';
    }

    if (idx < 1) {
        return false;
    }
    return true;
}

int64_t str_view_to_int64_t(Str_view str_view) {
    int64_t result = INT64_MAX;

    if (!try_str_view_to_int64_t(&result, str_view)) {
        unreachable(STR_VIEW_FMT, str_view_print(str_view));
    }
    return result;
}

static bool lang_type_atom_is_number_finish(Lang_type_atom atom) {
    size_t idx;
    for (idx = 1; idx < atom.str.count; idx++) {
        if (!isdigit(atom.str.str[idx])) {
            return false;
        }
    }

    return idx > 1;
}

// TODO: get rid of this function?
bool lang_type_atom_is_signed(Lang_type_atom atom) {
    if (atom.str.count < 1) {
        return false;
    }
    if (atom.str.str[0] != 'i') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom);
}

// TODO: get rid of this function?
bool lang_type_atom_is_unsigned(Lang_type_atom atom) {
    if (atom.str.count < 1) {
        return false;
    }
    if (atom.str.str[0] != 'u') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom);
}

bool lang_type_atom_is_number(Lang_type_atom atom) {
    return lang_type_atom_is_unsigned(atom) || lang_type_atom_is_signed(atom);
}

// only for unsafe_cast and similar cases
bool lang_type_is_number_like(Lang_type lang_type) {
    if (lang_type_get_pointer_depth(lang_type) > 0) {
        return true;
    }
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    switch (lang_type_primitive_const_unwrap(lang_type).type) {
        case LANG_TYPE_CHAR:
            return true;
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_ANY:
            return false;
    }
    unreachable("");
}

// for general use
bool lang_type_is_number(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }

    switch (lang_type_primitive_const_unwrap(lang_type).type) {
        case LANG_TYPE_CHAR:
            return false;
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_ANY:
            return false;
    }
    unreachable("");
}

bool lang_type_is_signed(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }

    switch (lang_type_primitive_const_unwrap(lang_type).type) {
        case LANG_TYPE_CHAR:
            return false;
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return false;
        case LANG_TYPE_ANY:
            return false;
    }
    unreachable("");
}

bool lang_type_is_unsigned(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }

    switch (lang_type_primitive_const_unwrap(lang_type).type) {
        case LANG_TYPE_CHAR:
            return false;
        case LANG_TYPE_SIGNED_INT:
            return false;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_ANY:
            return false;
    }
    unreachable("");
}

int64_t i_lang_type_atom_to_bit_width(Lang_type_atom atom) {
    //assert(lang_type_atom_is_signed(lang_type));
    return str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1));
}

// TODO: put strings in a hash table to avoid allocating duplicate types
Lang_type_atom lang_type_atom_unsigned_to_signed(Lang_type_atom lang_type) {
    assert(lang_type_atom_is_unsigned(lang_type));

    if (lang_type.pointer_depth != 0) {
        todo();
    }

    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_strv(&a_main, &string, str_view_slice(lang_type.str, 1, lang_type.str.count - 1));
    return lang_type_atom_new(string_to_strv(string), 0);
}

Str_view util_literal_name_new_prefix(const char* debug_prefix) {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    String var_name = {0};
    string_extend_cstr(&a_main, &var_name, debug_prefix);
    string_extend_cstr(&a_main, &var_name, "str");
    string_extend_size_t(&a_main, &var_name, count);
    vec_append(&a_main, &literal_strings, var_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

Str_view util_literal_name_new(void) {
    return util_literal_name_new_prefix("");
}

Str_view get_storage_location(Env* env, Str_view sym_name) {
    Tast_def* sym_def_;
    if (!symbol_lookup(&sym_def_, env, sym_name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", str_view_print(sym_name));
    }

    switch (sym_def_->type) {
        case TAST_VARIABLE_DEF: {
            Tast_variable_def* sym_def = tast_variable_def_unwrap(sym_def_);
            Llvm* result = NULL;
            if (!alloca_lookup(&result, env, sym_def->name)) {
                unreachable(STR_VIEW_FMT"\n", str_view_print(sym_def->name));
            }
            return llvm_tast_get_name(result);
        }
        default:
            unreachable(TAST_FMT, tast_def_print(sym_def_));
    }
    unreachable("");
}

Llvm_id get_matching_label_id(Env* env, Str_view name) {
    Llvm* label_;
    if (!alloca_lookup(&label_, env, name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable(STR_VIEW_FMT"\n", str_view_print(name));
    }
    Llvm_label* label = llvm_label_unwrap(llvm_def_unwrap(label_));
    return label->llvm_id;
}

Tast_assignment* util_assignment_new(Env* env, Uast_expr* lhs, Uast_expr* rhs) {
    Uast_assignment* assignment = uast_assignment_new(uast_expr_get_pos(lhs), lhs, rhs);

    Tast_assignment* new_assign = NULL;
    try_set_assignment_types(env, &new_assign, assignment);
    return new_assign;
}

// TODO: try to deduplicate 2 below functions
Tast_literal* util_tast_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_strv(value, token_type, pos));
}

// TODO: try to deduplicate 2 below functions
Uast_literal* util_uast_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* new_literal = NULL;
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Uast_number* literal = uast_number_new(pos, str_view_to_int64_t(value));
            new_literal = uast_number_wrap(literal);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            // TODO: figure out if literal name can be eliminated for uast_string
            Uast_string* literal = uast_string_new(pos, value, util_literal_name_new());
            new_literal = uast_string_wrap(literal);
            break;
        }
        case TOKEN_VOID: {
            Uast_void* literal = uast_void_new(pos);
            new_literal = uast_void_wrap(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            Uast_char* literal = uast_char_new(pos, str_view_front(value));
            if (value.count > 1) {
                todo();
            }
            new_literal = uast_char_wrap(literal);
            break;
        }
        default:
            unreachable("");
    }

    assert(new_literal);

    return new_literal;
}

Uast_literal* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* new_literal = NULL;

    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Uast_number* literal = uast_number_new(pos, value);
            literal->data = value;
            new_literal = uast_number_wrap(literal);
            break;
        }
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_VOID: {
            Uast_void* literal = uast_void_new(pos);
            new_literal = uast_void_wrap(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            assert(value < INT8_MAX);
            Uast_char* literal = uast_char_new(pos, value);
            new_literal = uast_char_wrap(literal);
            break;
        }
        default:
            unreachable("");
    }

    assert(new_literal);

    try_set_literal_types(new_literal);
    return new_literal;
}

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_int64_t(value, token_type, pos));
}

Tast_operator* util_binary_typed_new(Env* env, Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operator_type) {
    Uast_binary* binary = uast_binary_new(uast_expr_get_pos(lhs), lhs, rhs, token_type_to_binary_type(operator_type));

    Tast_expr* new_tast;
    unwrap(try_set_binary_types(env, &new_tast, binary));

    return tast_operator_unwrap(new_tast);
}

Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child) {
    Tast_binary* binary = tast_binary_new(
        tast_expr_get_pos(if_cond_child),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, tast_expr_get_pos(if_cond_child))
        ),
        if_cond_child,
        BINARY_NOT_EQUAL,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_get_pos(tast_expr_get_lang_type(if_cond_child)), 32, 0)))
    );

    return tast_binary_wrap(binary);
}

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child) {
    Uast_binary* binary = uast_binary_new(
        uast_expr_get_pos(if_cond_child),
        uast_literal_wrap(
            util_uast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, uast_expr_get_pos(if_cond_child))
        ),
        if_cond_child,
        BINARY_NOT_EQUAL
    );

    return uast_binary_wrap(binary);
}

size_t struct_def_base_get_idx_largest_member(Env* env, Struct_def_base base) {
    assert(base.members.info.count > 0);

    size_t result = 0;
    uint64_t size_result = 0;

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        uint64_t curr_size = sizeof_lang_type(env, vec_at(&base.members, idx)->lang_type);
        if (curr_size > size_result) {
            size_result = curr_size;
            result = idx;
        }
    }

    return result;
}

