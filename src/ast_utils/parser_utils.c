#include <strv.h>
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
#include <errno.h>


static bool lang_type_atom_is_number_finish(Lang_type_atom atom, bool allow_decimal) {
    (void) atom;
    size_t idx = 0;
    bool decimal_enc = false;
    for (idx = 1; idx < atom.str.base.count; idx++) {
        if (strv_at(atom.str.base, idx) == '.') {
            if (!allow_decimal || decimal_enc) {
                return false;
            }
            decimal_enc = true;
        } else if (!isdigit(strv_at(atom.str.base, idx))) {
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
    if (strv_at(atom.str.base, 0) != 'i') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, false);
}

// TODO: get rid of this function?
bool lang_type_atom_is_unsigned(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (strv_at(atom.str.base, 0) != 'u') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, false);
}

bool lang_type_atom_is_float(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (strv_at(atom.str.base, 0) != 'f') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, true);
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
        case LANG_TYPE_FLOAT:
            return true;
        case LANG_TYPE_OPAQUE:
            return false;
    }
    unreachable("");
}

// for general use
bool lang_type_primitive_is_number(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            return false;
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_FLOAT:
            return true;
        case LANG_TYPE_OPAQUE:
            return false;
    }
    unreachable("");
}

// for general use
bool lang_type_is_number(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_is_number(lang_type_primitive_const_unwrap(lang_type));
}

bool lang_type_is_signed(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_SIGNED_INT;
}

bool lang_type_is_unsigned(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_UNSIGNED_INT;
}

int64_t i_lang_type_atom_to_bit_width(const Lang_type_atom atom) {
    //assert(lang_type_atom_is_signed(lang_type));
    return strv_to_int64_t( POS_BUILTIN, strv_slice(atom.str.base, 1, atom.str.base.count - 1));
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
    string_extend_strv(&a_main, &string, strv_slice(lang_type.str.base, 1, lang_type.str.base.count - 1));
    return lang_type_atom_new(name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0), 0);
}

Tast_assignment* util_assignment_new(Uast_expr* lhs, Uast_expr* rhs) {
    Uast_assignment* assignment = uast_assignment_new(uast_expr_get_pos(lhs), lhs, rhs);

    Tast_assignment* new_assign = NULL;
    try_set_assignment_types( &new_assign, assignment);
    return new_assign;
}

// will print error on failure
bool util_try_uast_literal_new_from_strv(Uast_literal** new_lit, const Strv value, TOKEN_TYPE token_type, Pos pos) {
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            int64_t raw = 0;
            if (!try_strv_to_int64_t(&raw,  pos, value)) {
                return false;
            }
            Uast_int* literal = uast_int_new(pos, raw);
            *new_lit = uast_int_wrap(literal);
            break;
        }
        case TOKEN_FLOAT_LITERAL: {
            double raw = 0;
            if (!try_strv_to_double(&raw,  pos, value)) {
                return false;
            }
            Uast_float* literal = uast_float_new(pos, raw);
            *new_lit = uast_float_wrap(literal);
            break;
        }
        case TOKEN_STRING_LITERAL: {
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
            char raw = '\0';
            if (!try_strv_to_char(&raw, pos, value)) {
                return false;
            }
            Uast_char* lang_char = uast_char_new(pos, raw);
            *new_lit = uast_char_wrap(lang_char);
            break;
        }
        default:
            unreachable("");
    }

    assert(*new_lit);
    return true;
}

Uast_literal* util_uast_literal_new_from_strv(const Strv value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* lit = NULL;
    unwrap(util_try_uast_literal_new_from_strv(&lit,  value, token_type, pos));
    return lit;
}

Uast_literal* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Uast_literal* new_literal = NULL;

    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Uast_int* literal = uast_int_new(pos, value);
            literal->data = value;
            new_literal = uast_int_wrap(literal);
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
    return new_literal;
}

Uast_literal* util_uast_literal_new_from_double(double value, Pos pos) {
    Uast_literal* lit = uast_float_wrap(uast_float_new(pos, value));
    return lit;
}

Tast_literal* util_tast_literal_new_from_double(double value, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_double(value, pos));
}

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_int64_t(value, token_type, pos));
}

Tast_operator* util_binary_typed_new(Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operator_type) {
    Uast_binary* binary = uast_binary_new(uast_expr_get_pos(lhs), lhs, rhs, token_type_to_binary_type(operator_type));

    Tast_expr* new_tast;
    unwrap(try_set_binary_types(&new_tast, binary));

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
        uint64_t curr_size = sizeof_lang_type(vec_at(&base.members, idx)->lang_type);
        if (curr_size > size_result) {
            size_result = curr_size;
            result = idx;
        }
    }

    return result;
}

size_t ir_struct_def_base_get_idx_largest_member(Ir_struct_def_base base) {
    size_t result = 0;
    uint64_t size_result = 0;

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        uint64_t curr_size = sizeof_llvm_lang_type(vec_at(&base.members, idx)->lang_type);
        if (curr_size > size_result) {
            size_result = curr_size;
            result = idx;
        }
    }

    return result;
}

