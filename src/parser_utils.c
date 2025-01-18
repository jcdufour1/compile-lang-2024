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

bool try_str_view_to_int64_t(int64_t* result, Str_view str_view) {
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

bool lang_type_atom_is_signed(Lang_type_atom atom) {
    if (atom.str.str[0] != 'i') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom);
}

bool lang_type_atom_is_unsigned(Lang_type_atom atom) {
    if (atom.str.str[0] != 'u') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom);
}

bool lang_type_atom_is_number(Lang_type_atom atom) {
    return lang_type_atom_is_unsigned(atom) || lang_type_atom_is_signed(atom);
}

int64_t i_lang_type_atom_to_bit_width(Lang_type_atom atom) {
    //assert(lang_type_atom_is_signed(lang_type));
    return str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1));
}

// TODO: put strings in a hash table to avoid allocating duplicate types
Lang_type_atom lang_type_unsigned_to_signed(Lang_type_atom lang_type) {
    assert(lang_type_is_unsigned(lang_type));

    if (lang_type.pointer_depth != 0) {
        todo();
    }

    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_strv(&a_main, &string, str_view_slice(lang_type.str, 1, lang_type.str.count - 1));

    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return lang_type_atom_new(str_view, 0);
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

Str_view get_storage_location(const Env* env, Str_view sym_name) {
    Tast_def* sym_def_;
    if (!symbol_lookup(&sym_def_, env, sym_name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", str_view_print(sym_name));
    }

    switch (sym_def_->type) {
        case UAST_VARIABLE_DEF: {
            Tast_variable_def* sym_def = tast_unwrap_variable_def(sym_def_);
            Llvm* result = NULL;
            if (!alloca_lookup(&result, env, sym_def->name)) {
                unreachable(STR_VIEW_FMT"\n", str_view_print(sym_def->name));
            }
            return llvm_get_tast_name(result);
        }
        default:
            unreachable("");
    }
    unreachable("");
}

const Tast_variable_def* get_normal_symbol_def_from_alloca(const Env* env, const Tast* tast) {
    Tast_def* sym_def;
    if (!symbol_lookup(&sym_def, env, get_tast_name(tast))) {
        unreachable("tast call to undefined variable:"TAST_FMT"\n", tast_print(tast));
    }
    return tast_unwrap_variable_def(sym_def);
}

Llvm_id get_matching_label_id(const Env* env, Str_view name) {
    Llvm* label_;
    if (!alloca_lookup(&label_, env, name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable(STR_VIEW_FMT"\n", str_view_print(name));
    }
    Llvm_label* label = llvm_unwrap_label(llvm_unwrap_def(label_));
    return label->llvm_id;
}

Tast_assignment* util_assignment_new(Env* env, Uast_stmt* lhs, Uast_expr* rhs) {
    Uast_assignment* assignment = uast_assignment_new(uast_get_pos_stmt(lhs), lhs, rhs);

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
            new_literal = uast_wrap_number(literal);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            Uast_string* literal = uast_string_new(pos, value, util_literal_name_new());
            new_literal = uast_wrap_string(literal);
            break;
        }
        case TOKEN_VOID: {
            Uast_void* literal = uast_void_new(pos);
            new_literal = uast_wrap_void(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            Uast_char* literal = uast_char_new(pos, str_view_front(value));
            if (value.count > 1) {
                todo();
            }
            new_literal = uast_wrap_char(literal);
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
            new_literal = uast_wrap_number(literal);
            break;
        }
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_VOID: {
            Uast_void* literal = uast_void_new(pos);
            new_literal = uast_wrap_void(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            assert(value < INT8_MAX);
            Uast_char* literal = uast_char_new(pos, value);
            new_literal = uast_wrap_char(literal);
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
    Uast_binary* binary = uast_binary_new(uast_get_pos_expr(lhs), lhs, rhs, operator_type);

    Tast_expr* new_tast;
    try(try_set_binary_types(env, &new_tast, binary));

    return tast_unwrap_operator(new_tast);
}

const Tast* get_lang_type_from_sym_definition(const Tast* sym_def) {
    (void) sym_def;
    todo();
}

Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child) {
    Tast_binary* binary = tast_binary_new(
        tast_get_pos_expr(if_cond_child),
        tast_wrap_literal(
            util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, tast_get_pos_expr(if_cond_child))
        ),
        if_cond_child,
        TOKEN_NOT_EQUAL,
        lang_type_wrap_primitive_const(lang_type_primitive_new(lang_type_atom_new_from_cstr("i32", 0)))
    );

    return tast_wrap_binary(binary);
}

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child) {
    Uast_binary* binary = uast_binary_new(
        uast_get_pos_expr(if_cond_child),
        uast_wrap_literal(
            util_uast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, uast_get_pos_expr(if_cond_child))
        ),
        if_cond_child,
        TOKEN_NOT_EQUAL
    );

    return uast_wrap_binary(binary);
}

uint64_t sizeof_primitive(Lang_type_primitive primitive) {
    // TODO: make more generalized system for different bit widths, etc.
    if (lang_type_atom_is_equal(primitive.atom, lang_type_atom_new_from_cstr("u8", 1))) {
        return 8;
    } else if (lang_type_atom_is_equal(primitive.atom, lang_type_atom_new_from_cstr("i32", 0))) {
        return 4;
    } else if (lang_type_atom_is_equal(primitive.atom, lang_type_atom_new_from_cstr("i64", 0))) {
        return 8;
    } else {
        unreachable(LANG_TYPE_FMT"\n", lang_type_atom_print(primitive.atom));
    }
}

uint64_t sizeof_lang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_ENUM:
            return 4;
        case LANG_TYPE_PRIMITIVE:
            return sizeof_primitive(lang_type_unwrap_primitive_const(lang_type));
        default:
            todo();
    }
}

static uint64_t sizeof_expr(const Env* env, const Tast_expr* expr) {
    (void) env;
    switch (expr->type) {
        case TAST_LITERAL:
            return sizeof_lang_type(tast_get_lang_type_literal(tast_unwrap_literal_const(expr)));
        default:
            unreachable("");
    }
}

static uint64_t sizeof_def(const Env* env, const Tast_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(tast_unwrap_variable_def_const(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t sizeof_stmt(const Env* env, const Tast_stmt* stmt) {
    (void) env;
    switch (stmt->type) {
        case TAST_EXPR:
            return sizeof_expr(env, tast_unwrap_expr_const(stmt));
        case TAST_DEF:
            return sizeof_def(env, tast_unwrap_def_const(stmt));
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Env* env, const Tast_struct_literal* struct_literal) {
    const Tast_struct_def* struct_def = 
        get_struct_def_const(env, tast_wrap_expr_const(tast_wrap_struct_literal_const(struct_literal)));
    return sizeof_struct_def_base(&struct_def->base);
}

uint64_t sizeof_struct_def_base(const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct_expr(const Env* env, const Tast_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case TAST_STRUCT_LITERAL:
            return sizeof_struct_literal(env, tast_unwrap_struct_literal_const(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t llvm_sizeof_expr(const Env* env, const Llvm_expr* expr) {
    (void) env;
    switch (expr->type) {
        case LLVM_LITERAL:
            return sizeof_lang_type(llvm_get_lang_type_literal(llvm_unwrap_literal_const(expr)));
        default:
            unreachable("");
    }
}

static uint64_t llvm_sizeof_def(const Env* env, const Llvm_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(llvm_unwrap_variable_def_const(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_item(const Env* env, const Llvm* item) {
    (void) env;
    switch (item->type) {
        case TAST_EXPR:
            return llvm_sizeof_expr(env, llvm_unwrap_expr_const(item));
        case TAST_DEF:
            return llvm_sizeof_def(env, llvm_unwrap_def_const(item));
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_struct_def_base(const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

size_t struct_def_base_get_idx_largest_member(Struct_def_base base) {
    assert(base.members.info.count > 0);

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

bool try_get_generic_struct_def(const Env* env, Tast_def** def, Tast_stmt* stmt) {
    if (stmt->type == TAST_EXPR) {
        const Tast_expr* expr = tast_unwrap_expr_const(stmt);
        switch (expr->type) {
            case TAST_STRUCT_LITERAL: {
                assert(lang_type_get_str(tast_get_lang_type_stmt(stmt)).count > 0);
                return symbol_lookup(def, env, lang_type_get_str(tast_get_lang_type_stmt(stmt)));
            }
            case TAST_SYMBOL_TYPED:
                // fallthrough
            case TAST_MEMBER_ACCESS_TYPED: {
                Tast_def* var_def;
                assert(get_tast_name_stmt(stmt).count > 0);
                if (!symbol_lookup(&var_def, env, get_tast_name_stmt(stmt))) {
                    todo();
                    return false;
                }
                assert(lang_type_get_str((tast_get_lang_type_def(var_def))).count > 0);
                return symbol_lookup(def, env, lang_type_get_str((tast_get_lang_type_def(var_def))));
            }
            default:
                unreachable(TAST_FMT"\n", tast_stmt_print(stmt));
        }
    }

    Tast_def* tast_def = tast_unwrap_def(stmt);
    switch (tast_def->type) {
        case TAST_VARIABLE_DEF: {
            assert(lang_type_get_str((tast_get_lang_type_def(tast_def))).count > 0);
            return symbol_lookup(def, env, lang_type_get_str((tast_get_lang_type_def(tast_def))));
        }
        default:
            unreachable("");
    }
}

Tast_def* llvm_get_generic_struct_def(const Env* env, Llvm* llvm) {
    Tast_def* def = NULL;

    if (llvm->type == LLVM_EXPR) {
        const Llvm_expr* expr = llvm_unwrap_expr_const(llvm);
        switch (expr->type) {
            case LLVM_SYMBOL_TYPED: {
                Tast_def* var_def;
                assert(llvm_get_tast_name(llvm).count > 0);
                try(symbol_lookup(&var_def, env, llvm_get_tast_name(llvm)));
                assert(lang_type_get_str((tast_get_lang_type_def(var_def))).count > 0);
                try(symbol_lookup(&def, env, lang_type_get_str((tast_get_lang_type_def(var_def)))));
                return def;
            }
            default:
                unreachable(LLVM_FMT"\n", llvm_print(llvm));
        }
    }

    Llvm_def* llvm_def = llvm_unwrap_def(llvm);
    switch (llvm_def->type) {
        case LLVM_VARIABLE_DEF: {
            assert(lang_type_get_str(llvm_get_lang_type_def(llvm_def)).count > 0);
            try(symbol_lookup(&def, env, lang_type_get_str(llvm_get_lang_type_def(llvm_def))));
            return def;
        }
        default:
            unreachable("");
    }
}

bool try_get_struct_def(const Env* env, Tast_struct_def** struct_def, Tast_stmt* stmt) {
    Tast_def* def = NULL;
    if (!try_get_generic_struct_def(env, &def, stmt)) {
        return false;
    }
    if (def->type != TAST_STRUCT_DEF) {
        return false;
    }

    *struct_def = tast_unwrap_struct_def(def);
    return true;
}

bool llvm_try_get_struct_def(const Env* env, Tast_struct_def** struct_def, Llvm* tast) {
    Tast_def* def = llvm_get_generic_struct_def(env, tast);
    if (def->type != TAST_STRUCT_DEF) {
        return false;
    }

    *struct_def = tast_unwrap_struct_def(def);
    return true;
}

