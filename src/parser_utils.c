#include <str_view.h>
#include <newstring.h>
#include <string_vec.h>
#include <uast.h>
#include <tast.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <limits.h>
#include <ctype.h>
#include <token_type_to_operator_type.h>
#include <sizeof.h>
#include <symbol_log.h>
#include <lang_type_get_pos.h>
#include <name.h>

static bool isdigit_no_underscore(char prev, char curr) {
    (void) prev;
    return isdigit(curr);
}

bool try_str_view_octal_after_0_to_int64_t(int64_t* result, const Pos pos, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view_at(str_view, idx);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char < '0' || curr_char > '7') {
            msg(LOG_ERROR, EXPECT_FAIL_INVALID_OCTAL, env.file_path_to_text, pos, "invalid octal literal\n");
            return false;
        }

        *result *= 8;
        *result += curr_char - '0';
    }

    assert(idx > 0);
    return true;
}

bool try_str_view_hex_after_0x_to_int64_t(int64_t* result, const Pos pos, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view_at(str_view, idx);
        if (curr_char == '_') {
            continue;
        }

        int increment = 0;
        if (isdigit(curr_char)) {
            increment = curr_char - '0';
        } else if (curr_char >= 'a' && curr_char <= 'f') {
            increment = (curr_char - 'a') + 10;
        } else if (curr_char >= 'A' && curr_char <= 'F') {
            increment = (curr_char - 'A') + 10;
        } else {
            msg(LOG_ERROR, EXPECT_FAIL_INVALID_HEX, env.file_path_to_text, pos, "invalid hex literal\n");
            return false;
        }

        *result *= 16;
        *result += increment;
    }

    assert(idx > 0);
    return true;
}

bool try_str_view_bin_after_0b_to_int64_t(int64_t* result, const Pos pos, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view_at(str_view, idx);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char != '0' && curr_char != '1') {
            msg(LOG_ERROR, EXPECT_FAIL_INVALID_BIN, env.file_path_to_text, pos, "invalid bin literal\n");
            return false;
        }

        *result *= 2;
        *result += curr_char - '0';
    }

    assert(idx > 0);
    return true;
}

bool try_str_view_to_int64_t(int64_t* result, const Pos pos, Str_view str_view) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < str_view.count; idx++) {
        char curr_char = str_view_at(str_view, idx);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char == '0' && idx == 0 && str_view.count > 1 && isdigit(str_view_at(str_view, 1))) {
            return try_str_view_octal_after_0_to_int64_t(result,  pos, str_view_slice(str_view, 1, str_view.count - 1));
        }

        if (isalpha(curr_char)) {
            if (idx != 1) {
                todo();
            }

            if (curr_char == 'x') {
                return try_str_view_hex_after_0x_to_int64_t(
                    result,
                    
                    pos,
                    str_view_slice(str_view, 2, str_view.count - 2)
                );
            }

            if (curr_char == 'b') {
                return try_str_view_bin_after_0b_to_int64_t(
                    result,
                    
                    pos,
                    str_view_slice(str_view, 2, str_view.count - 2)
                );
            }

            todo();
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
        char curr_char = str_view_at(str_view, idx);
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

bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore) {
    assert(!ignore_underscore && "not implemented");
    Str_view num = str_view_consume_while(str_view, isdigit_no_underscore);
    return try_str_view_to_size_t(result, num);
}

int64_t str_view_to_int64_t(const Pos pos, Str_view str_view) {
    int64_t result = INT64_MAX;

    if (!try_str_view_to_int64_t(&result,  pos, str_view)) {
        unreachable(STR_VIEW_FMT, str_view_print(str_view));
    }
    return result;
}

static bool lang_type_atom_is_number_finish(Lang_type_atom atom) {
    (void) atom;
    size_t idx = 0;
    for (idx = 1; idx < atom.str.base.count; idx++) {
        if (!isdigit(str_view_at(atom.str.base, idx))) {
            return false;
        }
    }

    return idx > 1;
}

// TODO: get rid of this function?
bool lang_type_atom_is_signed(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (str_view_at(atom.str.base, 0) != 'i') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom);
}

// TODO: get rid of this function?
bool lang_type_atom_is_unsigned(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (str_view_at(atom.str.base, 0) != 'u') {
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

int64_t i_lang_type_atom_to_bit_width(const Lang_type_atom atom) {
    //assert(lang_type_atom_is_signed(lang_type));
    return str_view_to_int64_t( POS_BUILTIN, str_view_slice(atom.str.base, 1, atom.str.base.count - 1));
}

// TODO: put strings in a hash table to avoid allocating duplicate types
Lang_type_atom lang_type_atom_unsigned_to_signed(Lang_type_atom lang_type) {
    // TODO: remove or change this function
    assert(lang_type_atom_is_unsigned(lang_type));

    if (lang_type.pointer_depth != 0) {
        todo();
    }

    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_strv(&a_main, &string, str_view_slice(lang_type.str.base, 1, lang_type.str.base.count - 1));
    return lang_type_atom_new(name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0), 0);
}

Str_view util_literal_str_view_new_internal(const char* file, int line, const char* debug_prefix) {
    (void) file;
    (void) line;
    static String_vec literal_strings = {0};
    static size_t count = 0;

    String var_name = {0};
    string_extend_cstr(&a_main, &var_name, "str");

    // TODO: use better solution for debugging
    //string_extend_cstr(&a_main, &var_name, "file____");
    //string_extend_strv(&a_main, &var_name, serialize_name(name_new(str_view_from_cstr(file), (Str_view) {0}, (Ulang_type_vec) {0}, 0)));
    //string_extend_cstr(&a_main, &var_name, "_");
    //string_extend_size_t(&a_main, &var_name, line);
    //string_extend_cstr(&a_main, &var_name, "_");

    string_extend_cstr(&a_main, &var_name, debug_prefix);
    string_extend_size_t(&a_main, &var_name, count);
    vec_append(&a_main, &literal_strings, var_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

// TODO: return Name instead of Str_view
// accept mod_path as parameter
Str_view util_literal_name_new_prefix_internal(const char* file, int line, const char* debug_prefix) {
    return util_literal_str_view_new_internal(file, line, debug_prefix);
}

Name util_literal_name_new_prefix_internal_2(const char* file, int line, const char* debug_prefix, Str_view mod_path) {
    return name_new(mod_path, util_literal_str_view_new_internal(file, line, debug_prefix), (Ulang_type_vec) {0}, SCOPE_BUILTIN);
}

// TODO: inline this function
Name get_storage_location(Name sym_name) {
    Tast_def* sym_def_;
    if (!symbol_lookup(&sym_def_, sym_name)) {
        symbol_log(LOG_DEBUG, sym_name.scope_id);
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", name_print(sym_name));
    }

    switch (sym_def_->type) {
        case TAST_VARIABLE_DEF: {
            Tast_variable_def* sym_def = tast_variable_def_unwrap(sym_def_);
            Llvm* result = NULL;
            if (!alloca_lookup(&result,  sym_def->name)) {
                unreachable("");
            }
            return llvm_tast_get_name(result);
        }
        default:
            unreachable(TAST_FMT, tast_def_print(sym_def_));
    }
    unreachable("");
}

Tast_assignment* util_assignment_new(Uast_expr* lhs, Uast_expr* rhs) {
    Uast_assignment* assignment = uast_assignment_new(uast_expr_get_pos(lhs), lhs, rhs);

    Tast_assignment* new_assign = NULL;
    try_set_assignment_types( &new_assign, assignment);
    return new_assign;
}

Tast_literal* util_tast_literal_new_from_strv(const Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* lit = NULL;
    unwrap(util_try_uast_literal_new_from_strv(&lit,  value, token_type, pos));
    return try_set_literal_types( lit);
}

// will print error on failure
bool util_try_uast_literal_new_from_strv(Uast_literal** new_lit, const Str_view value, TOKEN_TYPE token_type, Pos pos) {
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            int64_t raw = 0;
            if (!try_str_view_to_int64_t(&raw,  pos, value)) {
                return false;
            }
            Uast_number* literal = uast_number_new(pos, raw);
            *new_lit = uast_number_wrap(literal);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            // TODO: figure out if literal name can be eliminated for uast_string
            Uast_string* string = uast_string_new(pos, value);
            *new_lit = uast_string_wrap(string);
            break;
        }
        case TOKEN_VOID: {
            Uast_void* lang_void = uast_void_new(pos);
            *new_lit = uast_void_wrap(lang_void);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            Uast_char* lang_char = uast_char_new(pos, str_view_front(value));
            if (value.count > 1) {
                todo();
            }
            *new_lit = uast_char_wrap(lang_char);
            break;
        }
        default:
            unreachable("");
    }

    assert(*new_lit);
    return true;
}

Uast_literal* util_uast_literal_new_from_strv(const Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* lit = NULL;
    unwrap(util_try_uast_literal_new_from_strv(&lit,  value, token_type, pos));
    return lit;
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

    try_set_literal_types( new_literal);
    return new_literal;
}

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    return try_set_literal_types( util_uast_literal_new_from_int64_t( value, token_type, pos));
}

Tast_operator* util_binary_typed_new(Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operator_type) {
    Uast_binary* binary = uast_binary_new(uast_expr_get_pos(lhs), lhs, rhs, token_type_to_binary_type(operator_type));

    Tast_expr* new_tast;
    unwrap(try_set_binary_types( &new_tast, binary));

    return tast_operator_unwrap(new_tast);
}

Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child) {
    Tast_binary* binary = tast_binary_new(
        tast_expr_get_pos(if_cond_child),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t( 0, TOKEN_INT_LITERAL, tast_expr_get_pos(if_cond_child))
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
            util_uast_literal_new_from_int64_t( 0, TOKEN_INT_LITERAL, uast_expr_get_pos(if_cond_child))
        ),
        if_cond_child,
        BINARY_NOT_EQUAL
    );

    return uast_binary_wrap(binary);
}

size_t struct_def_base_get_idx_largest_member(Struct_def_base base) {
    size_t result = 0;
    uint64_t size_result = 0;

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        uint64_t curr_size = sizeof_lang_type( vec_at(&base.members, idx)->lang_type);
        if (curr_size > size_result) {
            size_result = curr_size;
            result = idx;
        }
    }

    return result;
}

