#include "str_view.h"
#include "string_vec.h"
#include "node_ptr_vec.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include <limits.h>
#include <ctype.h>

// result is rounded up
static int64_t log2_int64_t(int64_t num) {
    int64_t reference = 1;
    for (unsigned int power = 0; power < 64; power++) {
        if (num <= reference) {
            return power;
        }

        reference *= 2;
    }
    unreachable("");
}

//static int64_t bit_width_needed_unsigned(int64_t num) {
//    return MAX(1, log2(num + 1));
//}

static int64_t bit_width_needed_signed(int64_t num) {
    return 1 + log2_int64_t(num + 1);
}

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

static Str_view int64_t_to_str_view(int64_t num) {
    String string = {0};
    string_extend_int64_t(&a_main, &string, num);
    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return str_view;
}

static bool lang_type_is_number_finish(Lang_type lang_type) {
    size_t idx;
    for (idx = 1; idx < lang_type.str.count; idx++) {
        if (!isdigit(lang_type.str.str[idx])) {
            return false;
        }
    }

    return idx > 1;
}

bool lang_type_is_signed(Lang_type lang_type) {
    if (lang_type.str.str[0] != 'i') {
        return false;
    }
    return lang_type_is_number_finish(lang_type);
}

bool lang_type_is_unsigned(Lang_type lang_type) {
    if (lang_type.str.str[0] != 'u') {
        return false;
    }
    return lang_type_is_number_finish(lang_type);
}

bool lang_type_is_number(Lang_type lang_type) {
    return lang_type_is_unsigned(lang_type) || lang_type_is_signed(lang_type);
}

int64_t i_lang_type_to_bit_width(Lang_type lang_type) {
    //assert(lang_type_is_signed(lang_type));
    return str_view_to_int64_t(str_view_slice(lang_type.str, 1, lang_type.str.count - 1));
}

// TODO: put strings in a hash table to avoid allocating duplicate types
static Lang_type bit_width_to_i_lang_type(int64_t bit_width) {
    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_int64_t(&a_main, &string, bit_width);
    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return lang_type_new_from_strv(str_view, 0);
}

Lang_type lang_type_unsigned_to_signed(Lang_type lang_type) {
    assert(lang_type_is_unsigned(lang_type));

    if (lang_type.pointer_depth != 0) {
        todo();
    }

    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_strv(&a_main, &string, str_view_slice(lang_type.str, 1, lang_type.str.count - 1));

    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return lang_type_new_from_strv(str_view, 0);
}

static Node_expr* auto_deref_to_0(const Env* env, Node_expr* expr) {
    while (get_lang_type_expr(expr).pointer_depth > 0) {
        Lang_type init_lang_type = {0};
        expr = unary_new(env, expr, TOKEN_DEREF, init_lang_type);
    }
    return expr;
}

static bool can_be_implicitly_converted(const Env* env, Lang_type dest, Lang_type src, bool implicit_pointer_depth) {
    (void) env;
    //Node* result = NULL;
    //if (symbol_lookup(&result, env, dest.str)) {
    //    log_tree(LOG_DEBUG, result);
    //    unreachable("");
    //}

    if (!implicit_pointer_depth) {
        if (src.pointer_depth != dest.pointer_depth) {
            return false;
        }
    }

    if (!lang_type_is_signed(dest) || !lang_type_is_signed(src)) {
        return lang_type_is_equal(dest, src);
    }
    return i_lang_type_to_bit_width(dest) >= i_lang_type_to_bit_width(src);
}

typedef enum {
    IMPLICIT_CONV_INVALID_TYPES,
    IMPLICIT_CONV_CONVERTED,
    IMPLICIT_CONV_OK,
} IMPLICIT_CONV_STATUS;

static IMPLICIT_CONV_STATUS do_implicit_conversion_if_needed(
    const Env* env,
    Lang_type dest_lang_type,
    Node_expr* src,
    bool inplicit_pointer_depth
) {
    Lang_type src_lang_type = get_lang_type_expr(src);
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );

    if (lang_type_is_equal(dest_lang_type, src_lang_type)) {
        return IMPLICIT_CONV_OK;
    }
    if (!can_be_implicitly_converted(env, dest_lang_type, src_lang_type, inplicit_pointer_depth)) {
        return IMPLICIT_CONV_INVALID_TYPES;
    }

    *get_lang_type_expr_ref(src) = dest_lang_type;
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );
    return IMPLICIT_CONV_CONVERTED;
}

static void msg_undefined_symbol(Str_view file_text, const Node* sym_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_node_name(sym_call))
    );
}

static void try_set_literal_lang_type(Lang_type* lang_type, Node_literal* literal, TOKEN_TYPE token_type);

Str_view literal_name_new(void) {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    char var_name[24];
    sprintf(var_name, "str%zu", count);
    String symbol_name = string_new_from_cstr(&a_main, var_name);
    vec_append(&a_main, &literal_strings, symbol_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

Str_view literal_name_new_prefix(const char* debug_prefix) {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    char var_name[24];
    sprintf(var_name, "%sstr%zu", debug_prefix, count);
    String symbol_name = string_new_from_cstr(&a_main, var_name);
    vec_append(&a_main, &literal_strings, symbol_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

Llvm_register_sym get_storage_location(const Env* env, Str_view sym_name) {
    Node_def* sym_def_;
    if (!symbol_lookup(&sym_def_, env, sym_name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", str_view_print(sym_name));
    }

    switch (sym_def_->type) {
        case NODE_VARIABLE_DEF: {
            Node_variable_def* sym_def = node_unwrap_variable_def(sym_def_);
            return sym_def->storage_location;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

const Node_variable_def* get_normal_symbol_def_from_alloca(const Env* env, const Node* node) {
    Node_def* sym_def;
    if (!symbol_lookup(&sym_def, env, get_node_name(node))) {
        unreachable("node call to undefined variable:"NODE_FMT"\n", node_print(node));
    }
    return node_unwrap_variable_def(sym_def);
}

// TODO: change this to only accept alloca?
const Node_variable_def* get_symbol_def_from_alloca(const Env* env, const Node* alloca) {
    switch (alloca->type) {
        case NODE_ALLOCA:
            return get_normal_symbol_def_from_alloca(env, alloca);
        default:
            unreachable(NODE_FMT"\n", node_print(alloca));
    }
}

Llvm_id get_matching_label_id(const Env* env, Str_view name) {
    Node_def* label_;
    if (!symbol_lookup(&label_, env, name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable("call to undefined label `"STR_VIEW_FMT"`", str_view_print(name));
    }
    Node_label* label = node_unwrap_label(label_);
    return label->llvm_id;
}

Node_assignment* assignment_new(const Env* env, Node* lhs, Node_expr* rhs) {
    Node_assignment* assignment = node_assignment_new(lhs->pos);
    assignment->lhs = lhs;
    assignment->rhs = rhs;

    Lang_type dummy;
    try_set_assignment_operand_types(env, &dummy, assignment);
    return assignment;
}

// TODO: try to deduplicate 2 below functions
Node_literal* util_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* literal = node_literal_new(pos);
    literal->name = literal_name_new();

    switch (token_type) {
        case TOKEN_INT_LITERAL:
            literal->type = NODE_NUMBER;
            node_unwrap_number(literal)->data = str_view_to_int64_t(value);
            break;
        case TOKEN_STRING_LITERAL:
            literal->type = NODE_STRING;
            node_unwrap_string(literal)->data = value;
            break;
        case TOKEN_VOID:
            literal->type = NODE_VOID;
            break;
        case TOKEN_CHAR_LITERAL:
            literal->type = NODE_CHAR;
            node_unwrap_char(literal)->data = str_view_front(value);
            break;
        default:
            unreachable("");
    }

    Lang_type dummy;
    try_set_literal_lang_type(&dummy, literal, token_type);
    return literal;
}

Node_literal* util_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* literal = node_literal_new(pos);
    literal->name = literal_name_new();

    switch (token_type) {
        case TOKEN_INT_LITERAL:
            literal->type = NODE_NUMBER;
            node_unwrap_number(literal)->data = value;
            break;
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_VOID:
            literal->type = NODE_VOID;
            break;
        case TOKEN_CHAR_LITERAL:
            assert(value < INT8_MAX);
            literal->type = NODE_CHAR;
            node_unwrap_char(literal)->data = value;
            break;
        default:
            unreachable("");
    }

    Lang_type dummy;
    try_set_literal_lang_type(&dummy, literal, token_type);
    return literal;
}

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node_expr* symbol_ = node_expr_new(pos);
    symbol_->type = NODE_SYMBOL_UNTYPED;
    Node_symbol_untyped* symbol = node_unwrap_symbol_untyped(symbol_);
    symbol->name = symbol_name;
    return symbol;
}

// TODO: make separate Node_unary_typed and Node_unary_untyped
Node_expr* unary_new(const Env* env, Node_expr* child, TOKEN_TYPE operator_type, Lang_type init_lang_type) {
    Node_expr* operator_ = node_expr_new(node_wrap_expr(child)->pos);
    operator_->type = NODE_OPERATOR;
    Node_operator* operator = node_unwrap_operator(operator_);
    operator->type = NODE_UNARY;
    Node_unary* unary = node_unwrap_unary(operator);
    unary->token_type = operator_type;
    unary->child = child;
    unary->lang_type = init_lang_type;

    Lang_type dummy;
    Node_expr* new_node;
    //symbol_log(LOG_DEBUG, env);
    try(try_set_unary_lang_type(env, &new_node, &dummy, unary));
    return new_node;
}

Node_operator* util_binary_typed_new(const Env* env, Node_expr* lhs, Node_expr* rhs, TOKEN_TYPE operator_type) {
    Node_binary* binary = node_binary_new(node_wrap_expr(lhs)->pos);
    binary->token_type = operator_type;
    binary->lhs = lhs;
    binary->rhs = rhs;

    Lang_type dummy;
    //symbol_log(LOG_DEBUG, env);
    Node_expr* new_node;
    try(try_set_binary_lang_type(env, &new_node, &dummy, binary));

    return node_unwrap_operator(new_node);
}

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

uint64_t sizeof_lang_type(const Env* env, Lang_type lang_type) {
    if (lang_type_is_enum(env, lang_type)) {
        return 4;
    } else if (lang_type_is_equal(lang_type, lang_type_new_from_cstr("u8", 1))) {
        return 8;
    } else if (lang_type_is_equal(lang_type, lang_type_new_from_cstr("i32", 0))) {
        return 4;
    } else {
        unreachable(LANG_TYPE_FMT"\n", lang_type_print(lang_type));
    }
}

static uint64_t sizeof_expr(const Env* env, const Node_expr* expr) {
    (void) env;
    switch (expr->type) {
        case NODE_LITERAL:
            return sizeof_lang_type(env, node_unwrap_literal_const(expr)->lang_type);
        default:
            unreachable("");
    }
}

static uint64_t sizeof_def(const Env* env, const Node_def* def) {
    (void) env;
    switch (def->type) {
        case NODE_VARIABLE_DEF:
            return sizeof_lang_type(env, node_unwrap_variable_def_const(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t sizeof_item(const Env* env, const Node* item) {
    (void) env;
    switch (item->type) {
        case NODE_EXPR:
            return sizeof_expr(env, node_unwrap_expr_const(item));
        case NODE_DEF:
            return sizeof_def(env, node_unwrap_def_const(item));
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Env* env, const Node_struct_literal* struct_literal) {
    const Node_struct_def* struct_def = 
        get_struct_def_const(env, node_wrap_expr_const(node_wrap_struct_literal_const(struct_literal)));
    return sizeof_struct_def_base(env, &struct_def->base);
}

uint64_t sizeof_struct_def_base(const Env* env, const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Node* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_item(env, memb_def);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct_expr(const Env* env, const Node_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case NODE_STRUCT_LITERAL:
            return sizeof_struct_literal(env, node_unwrap_struct_literal_const(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

// TODO: deduplicate these functions to some degree
bool lang_type_is_struct(const Env* env, Lang_type lang_type) {
    Node_def* def = NULL;
    if (!symbol_lookup(&def, env, lang_type.str)) {
        unreachable(LANG_TYPE_FMT"", lang_type_print(lang_type));
    }

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return true;
        case NODE_RAW_UNION_DEF:
            return false;
        case NODE_ENUM_DEF:
            return false;
        case NODE_PRIMITIVE_DEF:
            return false;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(node_wrap_def(def)), lang_type_print(lang_type));
    }
    unreachable("");
}

bool lang_type_is_raw_union(const Env* env, Lang_type lang_type) {
    Node_def* def = NULL;
    try(symbol_lookup(&def, env, lang_type.str));

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return false;
        case NODE_RAW_UNION_DEF:
            return true;
        case NODE_ENUM_DEF:
            return false;
        case NODE_PRIMITIVE_DEF:
            return false;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(node_wrap_def(def)), lang_type_print(lang_type));
    }
    unreachable("");
}

bool lang_type_is_enum(const Env* env, Lang_type lang_type) {
    Node_def* def = NULL;
    try(symbol_lookup(&def, env, lang_type.str));

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return false;
        case NODE_RAW_UNION_DEF:
            return false;
        case NODE_ENUM_DEF:
            return true;
        case NODE_PRIMITIVE_DEF:
            return false;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(node_wrap_def(def)), lang_type_print(lang_type));
    }
    unreachable("");
}

bool lang_type_is_primitive(const Env* env, Lang_type lang_type) {
    Node_def* def = NULL;
    try(symbol_lookup(&def, env, lang_type.str));

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return false;
        case NODE_RAW_UNION_DEF:
            return false;
        case NODE_ENUM_DEF:
            return false;
        case NODE_PRIMITIVE_DEF:
            return true;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(node_wrap_def(def)), lang_type_print(lang_type));
    }
    unreachable("");
}

bool try_get_generic_struct_def(const Env* env, Node_def** def, Node* node) {
    if (node->type == NODE_EXPR) {
        const Node_expr* expr = node_unwrap_expr_const(node);
        switch (expr->type) {
            case NODE_STRUCT_LITERAL: {
                assert(get_lang_type(node).str.count > 0);
                return symbol_lookup(def, env, get_lang_type(node).str);
            }
            case NODE_SYMBOL_TYPED:
                // fallthrough
            case NODE_MEMBER_ACCESS_TYPED: {
                Node_def* var_def;
                assert(get_node_name(node).count > 0);
                if (!symbol_lookup(&var_def, env, get_node_name(node))) {
                    todo();
                    return false;
                }
                assert(get_lang_type_def(var_def).str.count > 0);
                return symbol_lookup(def, env, get_lang_type_def(var_def).str);
            }
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_MEMBER_ACCESS_UNTYPED:
                assert(get_lang_type(node).str.count > 0);
                return symbol_lookup(def, env, get_lang_type(node).str);
            default:
                unreachable(NODE_FMT"\n", node_print(node));
        }
    }

    Node_def* node_def = node_unwrap_def(node);
    switch (node_def->type) {
        case NODE_VARIABLE_DEF: {
            assert(get_lang_type_def(node_def).str.count > 0);
            return symbol_lookup(def, env, get_lang_type_def(node_def).str);
        }
        default:
            unreachable("");
    }
}

bool try_get_struct_def(const Env* env, Node_struct_def** struct_def, Node* node) {
    Node_def* def = NULL;
    if (!try_get_generic_struct_def(env, &def, node)) {
        return false;
    }
    if (def->type != NODE_STRUCT_DEF) {
        return false;
    }

    *struct_def = node_unwrap_struct_def(def);
    return true;
}

static void try_set_literal_lang_type(Lang_type* lang_type, Node_literal* literal, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            literal->lang_type = lang_type_new_from_cstr("u8", 1);
            break;
        case TOKEN_INT_LITERAL: {
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            int64_t bit_width = bit_width_needed_signed(node_unwrap_number(literal)->data);
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            literal->lang_type = lang_type_new_from_strv(string_to_strv(lang_type_str), 0);
            break;
        }
        case TOKEN_VOID:
            literal->lang_type = lang_type_new_from_cstr("void", 0);
            break;
        case TOKEN_CHAR_LITERAL:
            literal->lang_type = lang_type_new_from_cstr("u8", 0);
            break;
        default:
            unreachable("");
    }

    *lang_type = literal->lang_type;
}

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_symbol_untyped* sym_untyped) {
    Pos pos = node_wrap_expr(node_wrap_symbol_untyped(sym_untyped))->pos;

    Node_def* sym_def;
    if (!symbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, node_wrap_expr(node_wrap_symbol_untyped(sym_untyped)));
        return false;
    }

    *lang_type = get_lang_type_def(sym_def);

    Sym_typed_base new_base = {.lang_type = *lang_type, .name = sym_untyped->name};
    if (lang_type_is_struct(env, *lang_type)) {
        Node_struct_sym* sym_typed = node_struct_sym_new(pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_struct_sym(sym_typed));
        return true;
    } else if (lang_type_is_raw_union(env, *lang_type)) {
        Node_raw_union_sym* sym_typed = node_raw_union_sym_new(pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_raw_union_sym(sym_typed));
        return true;
    } else if (lang_type_is_enum(env, *lang_type)) {
        Node_enum_sym* sym_typed = node_enum_sym_new(pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_enum_sym(sym_typed));
        return true;
    } else if (lang_type_is_primitive(env, *lang_type)) {
        Node_primitive_sym* sym_typed = node_primitive_sym_new(pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_primitive_sym(sym_typed));
        return true;
    } else {
        unreachable("uncaught undefined lang_type");
    }

    unreachable("");
}

static int64_t precalulate_number_internal(int64_t lhs_val, int64_t rhs_val, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_SINGLE_PLUS:
            return lhs_val + rhs_val;
        case TOKEN_SINGLE_MINUS:
            return lhs_val - rhs_val;
        case TOKEN_ASTERISK:
            return lhs_val*rhs_val;
        case TOKEN_SLASH:
            return lhs_val/rhs_val;
        case TOKEN_LESS_THAN:
            return lhs_val < rhs_val ? 1 : 0;
        case TOKEN_GREATER_THAN:
            return lhs_val > rhs_val ? 1 : 0;
        case TOKEN_DOUBLE_EQUAL:
            return lhs_val == rhs_val ? 1 : 0;
        case TOKEN_NOT_EQUAL:
            return lhs_val != rhs_val ? 1 : 0;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(token_type));
    }
    unreachable("");
}

static Node_literal* precalulate_number(
    const Node_number* lhs,
    const Node_number* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static Node_literal* precalulate_char(
    const Node_char* lhs,
    const Node_char* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

// returns false if unsuccessful
bool try_set_binary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_binary* operator) {
    Lang_type dummy;
    log_tree(LOG_DEBUG, (Node*)operator->lhs);
    Node_expr* new_lhs;
    if (!try_set_expr_lang_type(env, &new_lhs, &dummy, operator->lhs)) {
        return false;
    }
    assert(new_lhs);
    operator->lhs = new_lhs;

    Node_expr* new_rhs;
    if (!try_set_expr_lang_type(env, &new_rhs, &dummy, operator->rhs)) {
        return false;
    }
    operator->rhs = new_rhs;
    assert(operator->lhs);
    assert(operator->rhs);
    log_tree(LOG_DEBUG, (Node*)operator);

    if (!lang_type_is_equal(get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs))) {
        if (can_be_implicitly_converted(env, get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs), true)) {
            if (operator->rhs->type == NODE_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                node_unwrap_literal(operator->rhs)->lang_type = get_lang_type_expr(operator->lhs);
            } else {
                Node_expr* unary = unary_new(env, operator->rhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->lhs));
                operator->rhs = unary;
            }
        } else if (can_be_implicitly_converted(env, get_lang_type_expr(operator->rhs), get_lang_type_expr(operator->lhs), true)) {
            if (operator->lhs->type == NODE_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                node_unwrap_literal(operator->lhs)->lang_type = get_lang_type_expr(operator->rhs);
            } else {
                Node_expr* unary = unary_new(env, operator->lhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->rhs));
                operator->lhs = unary;
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, node_wrap_expr(node_wrap_operator(node_wrap_binary(operator)))->pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(get_lang_type_expr(operator->lhs)), lang_type_print(get_lang_type_expr(operator->rhs))
            );
            return false;
        }
    }
            
    assert(get_lang_type_expr(operator->lhs).str.count > 0);
    *lang_type = get_lang_type_expr(operator->lhs);
    operator->lang_type = get_lang_type_expr(operator->lhs);

    // precalcuate binary in some situations
    if (operator->lhs->type == NODE_LITERAL && operator->rhs->type == NODE_LITERAL) {
        Node_literal* lhs_lit = node_unwrap_literal(operator->lhs);
        Node_literal* rhs_lit = node_unwrap_literal(operator->rhs);

        if (lhs_lit->type != rhs_lit->type) {
            // TODO: make expected failure test for this
            unreachable("mismatched types");
        }

        Node_literal* literal = NULL;

        switch (lhs_lit->type) {
            case NODE_NUMBER:
                literal = precalulate_number(
                    node_unwrap_number_const(lhs_lit),
                    node_unwrap_number_const(rhs_lit),
                    operator->token_type,
                    node_wrap_expr(node_wrap_operator(node_wrap_binary(operator)))->pos
                );
                break;
            case NODE_CHAR:
                literal = precalulate_char(
                    node_unwrap_char_const(lhs_lit),
                    node_unwrap_char_const(rhs_lit),
                    operator->token_type,
                    node_wrap_expr(node_wrap_operator(node_wrap_binary(operator)))->pos
                );
                break;
            default:
                unreachable("");
        }

        *new_node = node_wrap_literal(literal);
        log_tree(LOG_DEBUG, node_wrap_expr(*new_node));
    } else {
        *new_node = node_wrap_operator(node_wrap_binary(operator));
    }

    assert(get_lang_type_expr(*new_node).str.count > 0);
    assert(*new_node);
    return true;
}

bool try_set_unary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_unary* unary) {
    Pos pos = node_wrap_expr(node_wrap_operator(node_wrap_unary(unary)))->pos;
    assert(lang_type);
    Lang_type init_lang_type;
    Node_expr* new_child;
    if (!try_set_expr_lang_type(env, &new_child, &init_lang_type, unary->child)) {
        return false;
    }
    unary->child = new_child;

    switch (unary->token_type) {
        case TOKEN_NOT:
            unary->lang_type = init_lang_type;
            *lang_type = unary->lang_type;
            break;
        case TOKEN_DEREF:
            unary->lang_type = init_lang_type;
            if (unary->lang_type.pointer_depth <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            unary->lang_type.pointer_depth--;
            *lang_type = unary->lang_type;
            break;
        case TOKEN_REFER:
            unary->lang_type = init_lang_type;
            unary->lang_type.pointer_depth++;
            assert(unary->lang_type.pointer_depth > 0);
            *lang_type = unary->lang_type;
            break;
        case TOKEN_UNSAFE_CAST:
            assert(unary->lang_type.str.count > 0);
            if (unary->lang_type.pointer_depth > 0 && lang_type_is_number(get_lang_type_expr(unary->child))) {
                *lang_type = init_lang_type;
            } else if (lang_type_is_number(unary->lang_type) && get_lang_type_expr(unary->child).pointer_depth > 0) {
                *lang_type = init_lang_type;
            } else if (lang_type_is_number(unary->lang_type) && lang_type_is_number(get_lang_type_expr(unary->child))) {
                *lang_type = init_lang_type;
            } else if (unary->lang_type.pointer_depth > 0 && get_lang_type_expr(unary->child).pointer_depth > 0) {
                *lang_type = init_lang_type;
            } else {
                todo();
            }
            break;
        default:
            unreachable("");
    }

    *new_node = node_wrap_operator(node_wrap_unary(unary));
    return true;
}

// returns false if unsuccessful
bool try_set_operator_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_operator* operator) {
    if (operator->type == NODE_UNARY) {
        return try_set_unary_lang_type(env, new_node, lang_type, node_unwrap_unary(operator));
    } else if (operator->type == NODE_BINARY) {
        return try_set_binary_lang_type(env, new_node, lang_type, node_unwrap_binary(operator));
    } else {
        unreachable("");
    }
}

bool try_set_struct_literal_assignment_types(const Env* env, Node** new_node, Lang_type* lang_type, const Node* lhs, Node_struct_literal* struct_literal) {
    //log(LOG_DEBUG, "------------------------------\n");
    //if (!is_corresponding_to_a_struct(env, lhs)) {
    //    todo(); // non_struct assigned struct literal
    //}
    Node_def* lhs_var_def_;
    try(symbol_lookup(&lhs_var_def_, env, get_node_name(lhs)));
    Node_variable_def* lhs_var_def = node_unwrap_variable_def(lhs_var_def_);
    Node_def* struct_def_;
    try(symbol_lookup(&struct_def_, env, lhs_var_def->lang_type.str));
    switch (struct_def_->type) {
        case NODE_STRUCT_DEF:
            break;
        case NODE_RAW_UNION_DEF:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env->file_text,
                node_wrap_expr(node_wrap_struct_literal(struct_literal))->pos,
                "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        default:
            log(LOG_DEBUG, NODE_FMT"\n", node_print(node_wrap_def(struct_def_)));
            unreachable("");
    }
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    Lang_type new_lang_type = {.str = struct_def->base.name, .pointer_depth = 0};
    struct_literal->lang_type = new_lang_type;
    
    Node_ptr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        //log(LOG_DEBUG, "%zu\n", idx);
        Node* memb_sym_def_ = vec_at(&struct_def->base.members, idx);
        Node_variable_def* memb_sym_def = node_unwrap_variable_def(node_unwrap_def(memb_sym_def_));
        log_tree(LOG_DEBUG, node_wrap_expr_const(node_wrap_struct_literal_const(struct_literal)));
        Node_assignment* assign_memb_sym = node_unwrap_assignment(vec_at(&struct_literal->members, idx));
        Node_symbol_untyped* memb_sym_piece_untyped = node_unwrap_symbol_untyped(node_unwrap_expr(assign_memb_sym->lhs));
        Node_expr* new_rhs = NULL;
        Lang_type dummy;
        if (!try_set_expr_lang_type(env, &new_rhs, &dummy, assign_memb_sym->rhs)) {
            unreachable("");
        }
        Node_literal* assign_memb_sym_rhs = node_unwrap_literal(new_rhs);

        assign_memb_sym_rhs->lang_type = memb_sym_def->lang_type;
        if (!str_view_is_equal(memb_sym_def->name, memb_sym_piece_untyped->name)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL, env->file_text,
                node_wrap_expr(node_wrap_symbol_untyped(memb_sym_piece_untyped))->pos,
                "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
                str_view_print(memb_sym_def->name), str_view_print(memb_sym_piece_untyped->name)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_def(node_wrap_variable_def(lhs_var_def))->pos,
                "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
                str_view_print(lhs_var_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_def(node_wrap_variable_def(memb_sym_def))->pos,
                "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
                str_view_print(memb_sym_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            return false;
        }

        vec_append_safe(&a_main, &new_literal_members, node_wrap_expr(node_wrap_literal(assign_memb_sym_rhs)));
    }

    struct_literal->members = new_literal_members;

    assert(struct_literal->lang_type.str.count > 0);

    *lang_type = new_lang_type;
    *new_node = node_wrap_expr(node_wrap_struct_literal(struct_literal));
    return true;
}

bool try_set_expr_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_expr* node) {
    switch (node->type) {
        case NODE_LITERAL:
            *lang_type = node_unwrap_literal(node)->lang_type;
            *new_node = node;
            return true;
        case NODE_SYMBOL_UNTYPED:
            if (!try_set_symbol_type(env, new_node, lang_type, node_unwrap_symbol_untyped(node))) {
                return false;
            } else {
                assert(*new_node);
            }
            return true;
        case NODE_MEMBER_ACCESS_UNTYPED: {
            Node* new_node_ = NULL;
            if (!try_set_member_access_types(env, &new_node_, lang_type, node_unwrap_member_access_untyped(node))) {
                return false;
            }
            *new_node = node_unwrap_expr(new_node_);
            return true;
        }
        case NODE_MEMBER_ACCESS_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_lang_type(node_unwrap_member_sym_typed(node));
            //*new_node = node;
            //return true;
        case NODE_INDEX_UNTYPED: {
            Node* new_node_ = NULL;
            if (!try_set_index_types(env, &new_node_, lang_type, node_unwrap_index_untyped(node))) {
                return false;
            }
            *new_node = node_unwrap_expr(new_node_);
            return true;
        }
        case NODE_INDEX_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_lang_type(node_unwrap_member_sym_typed(node));
            //*new_node = node;
            //return true;
        case NODE_SYMBOL_TYPED:
            *lang_type = get_lang_type_symbol_typed(node_unwrap_symbol_typed(node));
            *new_node = node;
            return true;
        case NODE_OPERATOR:
            if (!try_set_operator_lang_type(env, new_node, lang_type, node_unwrap_operator(node))) {
                return false;
            }
            assert(*new_node);
            return true;
        case NODE_FUNCTION_CALL:
            return try_set_function_call_types(env, new_node, lang_type, node_unwrap_function_call(node));
        case NODE_STRUCT_LITERAL:
            unreachable("cannot set struct literal type here");
        case NODE_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

bool try_set_def_lang_type(const Env* env, Node_def** new_node, Lang_type* lang_type, Node_def* node) {
    switch (node->type) {
        case NODE_VARIABLE_DEF: {
            Node_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, lang_type, node_unwrap_variable_def(node))) {
                return false;
            }
            *new_node = node_wrap_variable_def(new_def);
            return true;
        }
        case NODE_FUNCTION_DECL:
            // TODO: do this?
            *new_node = node;
            return true;
        case NODE_FUNCTION_DEF:
            *new_node = node;
            return true;
        case NODE_STRUCT_DEF: {
            Node_struct_def* new_def = NULL;
            if (!try_set_struct_def_types(env, &new_def, lang_type, node_unwrap_struct_def(node))) {
                return false;
            }
            *new_node = node_wrap_struct_def(new_def);
            return true;
        }
        case NODE_RAW_UNION_DEF: {
            Node_raw_union_def* new_def = NULL;
            if (!try_set_raw_union_def_types(env, &new_def, lang_type, node_unwrap_raw_union_def(node))) {
                return false;
            }
            *new_node = node_wrap_raw_union_def(new_def);
            return true;
        }
        case NODE_ENUM_DEF: {
            Node_enum_def* new_def = NULL;
            if (!try_set_enum_def_types(env, &new_def, lang_type, node_unwrap_enum_def(node))) {
                return false;
            }
            *new_node = node_wrap_enum_def(new_def);
            return true;
        }
        case NODE_PRIMITIVE_DEF:
            *new_node = node;
            return true;
        case NODE_LABEL:
            *new_node = node;
            return true;
        case NODE_LITERAL_DEF:
            *new_node = node;
            return true;
    }
    unreachable("");
}

bool try_set_assignment_operand_types(const Env* env, Lang_type* lang_type, Node_assignment* assignment) {
    Lang_type lhs_lang_type = {0};
    Lang_type rhs_lang_type = {0};
    Node* new_lhs = NULL;
    if (!try_set_node_lang_type(env, &new_lhs, &lhs_lang_type, assignment->lhs)) { 
        return false;
    }
    assignment->lhs = new_lhs;

    Node_expr* new_rhs;
    if (assignment->rhs->type == NODE_STRUCT_LITERAL) {
        Node* new_rhs_ = NULL;
        if (!try_set_struct_literal_assignment_types(
            env, &new_rhs_, &rhs_lang_type, assignment->lhs, node_unwrap_struct_literal(assignment->rhs)
        )) {
            return false;
        }
        new_rhs = node_unwrap_expr(new_rhs_);
    } else {
        if (!try_set_expr_lang_type(env, &new_rhs, &rhs_lang_type, assignment->rhs)) {
            return false;
        }
    }
    assignment->rhs = new_rhs;

    *lang_type = lhs_lang_type;

    assert(lhs_lang_type.str.count > 0);
    assert(get_lang_type_expr(assignment->rhs).str.count > 0);
    switch (do_implicit_conversion_if_needed(env, lhs_lang_type, assignment->rhs, false)) {
        case IMPLICIT_CONV_INVALID_TYPES: {
            msg(
                LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                node_wrap_expr(assignment->rhs)->pos,
                "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                lang_type_print(rhs_lang_type), lang_type_print(lhs_lang_type)
            );
            return false;
        }
        case IMPLICIT_CONV_OK:
            // fallthrough
        case IMPLICIT_CONV_CONVERTED:
            return true;
    }
    unreachable("");
}

static void msg_invalid_function_arg_internal(
    const char* file,
    int line,
    const Env* env,
    const Node_expr* argument,
    const Node_variable_def* corres_param
) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env->file_text, node_wrap_expr_const(argument)->pos, 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(get_lang_type_expr(argument)), 
        str_view_print(corres_param->name),
        lang_type_print(corres_param->lang_type)
    );
    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_def(node_wrap_variable_def_const(corres_param))->pos,
        "corresponding parameter `"STR_VIEW_FMT"` defined here\n",
        str_view_print(corres_param->name)
    );
}

#define msg_invalid_function_arg(env, argument, corres_param) \
    msg_invalid_function_arg_internal(__FILE__, __LINE__, env, argument, corres_param)

bool try_set_function_call_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_function_call* fun_call) {
    bool status = true;

    Node_def* fun_def;
    if (!symbol_lookup(&fun_def, env, fun_call->name)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env->file_text, node_wrap_expr(node_wrap_function_call(fun_call))->pos,
            "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
        );
        return false;
    }
    Node_function_decl* fun_decl;
    if (fun_def->type == NODE_FUNCTION_DEF) {
        fun_decl = node_unwrap_function_def(fun_def)->declaration;
    } else {
        fun_decl = node_unwrap_function_decl(fun_def);
    }
    Node_lang_type* fun_rtn_type = fun_decl->return_type;
    fun_call->lang_type = fun_rtn_type->lang_type;
    assert(fun_call->lang_type.str.count > 0);
    Node_function_params* params = fun_decl->parameters;
    size_t params_idx = 0;

    size_t min_args;
    size_t max_args;
    if (params->params.info.count < 1) {
        min_args = 0;
        max_args = 0;
    } else if (node_unwrap_variable_def(node_unwrap_def(vec_top(&params->params)))->is_variadic) {
        min_args = params->params.info.count - 1;
        max_args = SIZE_MAX;
    } else {
        min_args = params->params.info.count;
        max_args = params->params.info.count;
    }
    if (fun_call->args.info.count < min_args || fun_call->args.info.count > max_args) {
        String message = {0};
        string_extend_size_t(&print_arena, &message, fun_call->args.info.count);
        string_extend_cstr(&print_arena, &message, " arguments are passed to function `");
        string_extend_strv(&print_arena, &message, fun_call->name);
        string_extend_cstr(&print_arena, &message, "`, but ");
        string_extend_size_t(&print_arena, &message, min_args);
        if (max_args > min_args) {
            string_extend_cstr(&print_arena, &message, " or more");
        }
        string_extend_cstr(&print_arena, &message, " arguments expected\n");
        msg(
            LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env->file_text, node_wrap_expr(node_wrap_function_call(fun_call))->pos,
            STR_VIEW_FMT, str_view_print(string_to_strv(message))
        );

        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_def(fun_def)->pos,
            "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
        );
        return false;
    }

    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Node_variable_def* corres_param = node_unwrap_variable_def(node_unwrap_def(vec_at(&params->params, params_idx)));
        Node_expr** argument = vec_at_ref(&fun_call->args, arg_idx);
        Node_expr* new_arg = NULL;

        if (!corres_param->is_variadic) {
            params_idx++;
        }

        Lang_type dummy = {0};
        Node* new_arg_ = NULL;
        if ((*argument)->type == NODE_STRUCT_LITERAL) {
            todo();
            try(try_set_struct_literal_assignment_types(
                env, &new_arg_, &dummy, node_wrap_def(node_wrap_variable_def(corres_param)), node_unwrap_struct_literal(*argument)
            ));
            new_arg = node_unwrap_expr(new_arg_);
        } else {
            if (!try_set_expr_lang_type(env, &new_arg, &dummy, *argument)) {
                status = false;
                continue;
            }
        }
        log_tree(LOG_DEBUG, node_wrap_expr(*argument));
        assert(new_arg);
        *argument = new_arg;

        if (lang_type_is_equal(corres_param->lang_type, lang_type_new_from_cstr("any", 0))) {
            if (corres_param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                continue;
            } else {
                todo();
            }
        }

        assert(*argument);
        if (!lang_type_is_equal(corres_param->lang_type, get_lang_type_expr(*argument))) {
            if (can_be_implicitly_converted(env, corres_param->lang_type, get_lang_type_expr(*argument), false)) {
                if ((*argument)->type == NODE_LITERAL) {
                    node_unwrap_literal((*argument))->lang_type = corres_param->lang_type;
                } else {
                    msg_invalid_function_arg(env, *argument, corres_param);
                    return false;
                }
            } else {
                msg_invalid_function_arg(env, *argument, corres_param);
                return false;
            }
        }
    }

    *lang_type = fun_rtn_type->lang_type;
    *new_node = node_wrap_function_call(fun_call);
    return status;
}

static void msg_invalid_member(
    const Env* env,
    Struct_def_base base,
    const Node_member_access_untyped* access
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER, env->file_text,
        node_wrap_expr_const(node_wrap_member_access_untyped_const(access))->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(access->member_name), str_view_print(base.name)
    );

    // TODO: add notes for where struct def of callee is defined, etc.
}

bool try_set_member_access_types_finish_generic_struct(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_member_access_untyped* access,
    Struct_def_base def_base,
    Node_expr* new_callee
) {
    Node_variable_def* member_def = NULL;
    if (!try_get_member_def(&member_def, &def_base, access->member_name)) {
        msg_invalid_member(env, def_base, access);
        return false;
    }
    *lang_type = member_def->lang_type;

    Node_member_access_typed* new_access = node_member_access_typed_new(node_wrap_expr(node_wrap_member_access_untyped(access))->pos);
    new_access->lang_type = *lang_type;
    new_access->member_name = access->member_name;
    new_access->callee = new_callee;

    *new_node = node_wrap_expr(node_wrap_member_access_typed(new_access));

    assert(lang_type->str.count > 0);
    return true;
}

bool try_set_member_access_types_finish(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_def* lang_type_def,
    Node_member_access_untyped* access,
    Node_expr* new_callee
) {
    switch (lang_type_def->type) {
        case NODE_STRUCT_DEF: {
            Node_struct_def* struct_def = node_unwrap_struct_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_node, lang_type, access, struct_def->base, new_callee
            );
        }
        case NODE_RAW_UNION_DEF: {
            Node_raw_union_def* raw_union_def = node_unwrap_raw_union_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_node, lang_type, access, raw_union_def->base, new_callee
            );
        }
        case NODE_ENUM_DEF: {
            Node_enum_def* enum_def = node_unwrap_enum_def(lang_type_def);
            Node_variable_def* member_def = NULL;
            if (!try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
                todo();
            }
            *lang_type = member_def->lang_type;
            Node_enum_lit* new_lit = node_enum_lit_new(node_wrap_expr(node_wrap_member_access_untyped(access))->pos);
            new_lit->data = get_member_index(&enum_def->base, access->member_name);
            node_wrap_enum_lit(new_lit)->lang_type = *lang_type;

            *new_node = node_wrap_expr(node_wrap_literal(node_wrap_enum_lit(new_lit)));
            assert(lang_type->str.count > 0);
            return true;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_def(lang_type_def)));
    }

    unreachable("");
}

bool try_set_member_access_types(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_member_access_untyped* access
) {
    Node_expr* new_callee = NULL;
    if (!try_set_expr_lang_type(env, &new_callee, lang_type, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case NODE_SYMBOL_TYPED: {
            Node_symbol_typed* sym = node_unwrap_symbol_typed(new_callee);
            Node_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, get_lang_type_symbol_typed(sym).str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_node, lang_type, lang_type_def, access, new_callee);

        }
        case NODE_MEMBER_ACCESS_TYPED: {
            Node_member_access_typed* sym = node_unwrap_member_access_typed(new_callee);

            Node_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, sym->lang_type.str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_node, lang_type, lang_type_def, access, new_callee);
        }
        default:
            unreachable("");
    }
    unreachable("");
}

bool try_set_index_types(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_index_untyped* index
) {
    Node_expr* new_callee = NULL;
    Node_expr* new_inner_index = NULL;
    if (!try_set_expr_lang_type(env, &new_callee, lang_type, index->callee)) {
        return false;
    }
    if (!try_set_expr_lang_type(env, &new_inner_index, lang_type, index->index)) {
        return false;
    }

    Lang_type new_lang_type = get_lang_type_expr(new_callee);
    if (new_lang_type.pointer_depth < 1) {
        todo();
    }
    new_lang_type.pointer_depth--;

    Node_index_typed* new_index = node_index_typed_new(node_wrap_expr(node_wrap_index_untyped(index))->pos);
    new_index->lang_type = new_lang_type;
    new_index->index = new_inner_index;
    new_index->callee = new_callee;

    *new_node = node_wrap_expr(node_wrap_index_typed(new_index));
    return true;
}

Node_operator* condition_get_default_child(Node_expr* if_cond_child) {
    Node_binary* new_oper = node_binary_new(node_wrap_expr(if_cond_child)->pos);
    new_oper->lhs = node_wrap_literal(
        util_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, node_wrap_expr(if_cond_child)->pos)
    );
    new_oper->rhs = if_cond_child;
    new_oper->token_type = TOKEN_NOT_EQUAL;
    new_oper->lang_type = lang_type_new_from_cstr("i32", 0);

    return node_wrap_binary(new_oper);
}

static bool try_set_condition_types(const Env* env, Lang_type* lang_type, Node_condition* if_cond) {
    Node_expr* new_if_cond_child;
    if (!try_set_operator_lang_type(env, &new_if_cond_child, lang_type, if_cond->child)) {
        return false;
    }

    switch (new_if_cond_child->type) {
        case NODE_OPERATOR:
            if_cond->child = node_unwrap_operator(new_if_cond_child);
            break;
        case NODE_LITERAL:
            if_cond->child = condition_get_default_child(new_if_cond_child);
            break;
        case NODE_FUNCTION_CALL:
            if_cond->child = condition_get_default_child(new_if_cond_child);
            break;
        default:
            unreachable("");
    }

    return true;
}

const Node_function_def* get_parent_function_def_const(const Env* env) {
    if (env->ancesters.info.count < 1) {
        unreachable("");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&env->ancesters, idx);
        if (curr_node->type == NODE_DEF) {
            Node_def* curr_def = node_unwrap_def(curr_node);

            if (curr_def->type == NODE_FUNCTION_DEF) {
                return node_unwrap_function_def(curr_def);
            }
        }

        if (idx < 1) {
            unreachable("");
        }
    }
}

Lang_type get_parent_function_return_type(const Env* env) {
    return get_parent_function_def_const(env)->declaration->return_type->lang_type;
}

bool try_set_struct_base_types(const Env* env, Struct_def_base* base) {
    bool success = true;

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Node* curr = vec_at(&base->members, idx);

        Node* result_dummy = NULL;
        Lang_type lang_type_dummy = {0};
        if (!try_set_node_lang_type(env, &result_dummy, &lang_type_dummy, curr)) {
            success = false;
        }
    }

    return success;
}

bool try_set_enum_def_types(const Env* env, Node_enum_def** new_node, Lang_type* lang_type, Node_enum_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

bool try_set_raw_union_def_types(const Env* env, Node_raw_union_def** new_node, Lang_type* lang_type, Node_raw_union_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

bool try_set_struct_def_types(const Env* env, Node_struct_def** new_node, Lang_type* lang_type, Node_struct_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

bool try_set_variable_def_types(
    const Env* env,
    Node_variable_def** new_node,
    Lang_type* lang_type,
    Node_variable_def* node
) {
    Node_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, node->lang_type.str)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_text,
            node_wrap_def(node_wrap_variable_def(node))->pos,
            "type `"LANG_TYPE_FMT"` is not defined\n", lang_type_print(node->lang_type)
        );
        return false;
    }

    *lang_type = node->lang_type;
    *new_node = node;
    return true;
}

bool try_set_return(const Env* env, Node_return** new_node, Lang_type* lang_type, Node_return* rtn) {
    *new_node = NULL;

    Node_expr* new_rtn_child;
    if (!try_set_expr_lang_type(env, &new_rtn_child, lang_type, rtn->child)) {
        todo();
        return false;
    }
    rtn->child = new_rtn_child;

    Lang_type src_lang_type = get_lang_type_expr(rtn->child);
    Lang_type dest_lang_type = get_parent_function_def_const(env)->declaration->return_type->lang_type;

    if (lang_type_is_equal(dest_lang_type, src_lang_type)) {
        goto success;
    }
    if (can_be_implicitly_converted(env, dest_lang_type, src_lang_type, false)) {
        log_tree(LOG_DEBUG, node_wrap_return(rtn));
        if (rtn->child->type == NODE_LITERAL) {
            node_unwrap_literal(rtn->child)->lang_type = dest_lang_type;
            goto success;
        } else {
            todo();
        }
        Node_expr* unary = unary_new(env, rtn->child, TOKEN_UNSAFE_CAST, dest_lang_type);
        rtn->child = unary;
        goto success;
    }

    const Node_function_def* fun_def = get_parent_function_def_const(env);
    if (rtn->is_auto_inserted) {
        msg(
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env->file_text, node_wrap_return(rtn)->pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            lang_type_print(fun_def->declaration->return_type->lang_type)
        );
    } else {
        msg(
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, node_wrap_return(rtn)->pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(get_lang_type_expr(rtn->child)), 
            lang_type_print(fun_def->declaration->return_type->lang_type)
        );
    }
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_wrap_lang_type(fun_def->declaration->return_type)->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(fun_def->declaration->return_type->lang_type)
    );
    return false;

    unreachable("");

success:
    *new_node = rtn;
    return true;
}

bool try_set_node_lang_type(const Env* env, Node** new_node, Lang_type* lang_type, Node* node) {
    *new_node = node;

    switch (node->type) {
        case NODE_EXPR: {
            Node_expr* new_node_ = NULL;
            if (!try_set_expr_lang_type(env, &new_node_, lang_type, node_unwrap_expr(node))) {
                return false;
            }
            *new_node = node_wrap_expr(new_node_);
            return true;
        }
        case NODE_DEF: {
            Node_def* new_node_ = NULL;
            if (!try_set_def_lang_type(env, &new_node_, lang_type, node_unwrap_def(node))) {
                return false;
            }
            *new_node = node_wrap_def(new_node_);
            return true;
        }
        case NODE_IF:
            return try_set_condition_types(env, lang_type, node_unwrap_if(node)->condition);
        case NODE_FOR_WITH_COND:
            return try_set_condition_types(env, lang_type, node_unwrap_for_with_cond(node)->condition);
        case NODE_ASSIGNMENT:
            return try_set_assignment_operand_types(env, lang_type, node_unwrap_assignment(node));
        case NODE_RETURN: {
            Node_return* new_rtn = NULL;
            if (!try_set_return(env, &new_rtn, lang_type, node_unwrap_return(node))) {
                return false;
            }
            *new_node = node_wrap_return(new_rtn);
            return true;
        }
        case NODE_FOR_RANGE:
            *new_node = node;
            return true;
        case NODE_BREAK:
            *new_node = node;
            return true;
        case NODE_IF_ELSE_CHAIN: {
            Node_if_else_chain* if_else = node_unwrap_if_else_chain(node);
            for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
                Node_if* curr = vec_at(&if_else->nodes, idx);
                if (!try_set_condition_types(env, lang_type, curr->condition)) {
                    return false;
                }
            }
            return true;
        }
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}

