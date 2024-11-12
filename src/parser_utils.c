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

bool is_i_lang_type(Lang_type lang_type) {
    if (lang_type.str.str[0] != 'i') {
        return false;
    }

    size_t idx;
    for (idx = 1; idx < lang_type.str.count; idx++) {
        if (!isdigit(lang_type.str.str[idx])) {
            return false;
        }
    }

    return idx > 1;
}

int64_t i_lang_type_to_bit_width(Lang_type lang_type) {
    assert(is_i_lang_type(lang_type));
    return str_view_to_int64_t(str_view_slice(lang_type.str, 1, lang_type.str.count - 1));
}

// TODO: put strings in a hash table to avoid allocating duplicate types
static Lang_type bit_width_to_i_lang_type(int64_t bit_width) {
    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_int64_t(&a_main, &string, bit_width);
    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return lang_type_from_strv(str_view, 0);
}

static Node_expr* auto_deref_to_0(const Env* env, Node_expr* expr) {
    while (get_lang_type_expr(expr).pointer_depth > 0) {
        Lang_type init_lang_type = {0};
        expr = unary_new(env, expr, TOKEN_DEREF, init_lang_type);
    }
    return expr;
}

static bool can_be_implicitly_converted(Lang_type dest, Lang_type src) {
    if (!is_i_lang_type(dest) || !is_i_lang_type(src)) {
        return lang_type_is_equal(dest, src);
    }
    return i_lang_type_to_bit_width(dest) >= i_lang_type_to_bit_width(src);
}

typedef enum {
    IMPLICIT_CONV_INVALID_TYPES,
    IMPLICIT_CONV_CONVERTED,
    IMPLICIT_CONV_OK,
} IMPLICIT_CONV_STATUS;

static IMPLICIT_CONV_STATUS do_implicit_conversion_if_needed(Lang_type dest_lang_type, Node_expr* src) {
    Lang_type src_lang_type = get_lang_type_expr(src);
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );

    if (lang_type_is_equal(dest_lang_type, src_lang_type)) {
        return IMPLICIT_CONV_OK;
    }
    if (!can_be_implicitly_converted(dest_lang_type, src_lang_type)) {
        return IMPLICIT_CONV_INVALID_TYPES;
    }

    *get_lang_type_expr_ref(src) = dest_lang_type;
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );
    return IMPLICIT_CONV_CONVERTED;
}

static void msg_undefined_symbol(const Node* sym_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_node_name(sym_call))
    );
}

static void try_set_literal_lang_type(Lang_type* lang_type, Node_e_literal* literal, TOKEN_TYPE token_type);

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

Node_e_llvm_register_sym* get_storage_location(const Env* env, Str_view sym_name) {
    Node* sym_def_;
    if (!symbol_lookup(&sym_def_, env, sym_name)) {
        symbol_log(LOG_DEBUG, env);
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", str_view_print(sym_name));
    }
    Node_variable_def* sym_def = node_unwrap_variable_def(sym_def_);
    if (!sym_def->storage_location) {
        unreachable("no storage location associated with symbol definition");
    }
    return sym_def->storage_location;
}

const Node_variable_def* get_normal_symbol_def_from_alloca(const Env* env, const Node* node) {
    Node* sym_def;
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
    Node* label_;
    if (!symbol_lookup(&label_, env, name)) {
        unreachable("call to undefined label");
    }
    Node_label* label = node_unwrap_label(label_);
    return label->llvm_id;
}

Node_assignment* assignment_new(const Env* env, Node* lhs, Node_expr* rhs) {
    Node_assignment* assignment = node_unwrap_assignment(node_new(lhs->pos, NODE_ASSIGNMENT));
    assignment->lhs = lhs;
    assignment->rhs = rhs;

    Lang_type dummy;
    try_set_assignment_operand_types(env, &dummy, assignment);
    return assignment;
}

Node_e_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node_expr* literal_ = node_unwrap_expr(node_new(pos, NODE_EXPR));
    literal_->type = NODE_E_LITERAL;
    Node_e_literal* literal = node_unwrap_e_literal(literal_);
    literal->name = literal_name_new();

    switch (token_type) {
        case TOKEN_INT_LITERAL:
            literal->type = NODE_LIT_NUMBER;
            node_unwrap_lit_number(literal)->data = str_view_to_int64_t(value);
            break;
        case TOKEN_STRING_LITERAL:
            literal->type = NODE_LIT_STRING;
            node_unwrap_lit_string(literal)->data = value;
            break;
        case TOKEN_VOID:
            literal->type = NODE_LIT_VOID;
            break;
        default:
            unreachable("");
    }

    Lang_type dummy;
    try_set_literal_lang_type(&dummy, literal, token_type);
    return literal;
}

Node_e_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node_expr* symbol_ = node_unwrap_expr(node_new(pos, NODE_EXPR));
    symbol_->type = NODE_E_SYMBOL_UNTYPED;
    Node_e_symbol_untyped* symbol = node_unwrap_e_symbol_untyped(symbol_);
    symbol->name = symbol_name;
    return symbol;
}

// TODO: make separate Node_unary_typed and Node_unary_untyped
Node_expr* unary_new(const Env* env, Node_expr* child, TOKEN_TYPE operator_type, Lang_type init_lang_type) {
    Node_expr* operator_ = node_unwrap_expr(node_new(node_wrap_expr(child)->pos, NODE_EXPR));
    operator_->type = NODE_E_OPERATOR;
    Node_e_operator* operator = node_unwrap_e_operator(operator_);
    operator->type = NODE_OP_UNARY;
    Node_op_unary* unary = node_unwrap_op_unary(operator);
    unary->token_type = operator_type;
    unary->child = child;
    unary->lang_type = init_lang_type;

    Lang_type dummy;
    Node_expr* new_node;
    symbol_log(LOG_DEBUG, env);
    try(try_set_unary_lang_type(env, &new_node, &dummy, unary));
    return new_node;
}

// TODO: make Node_untyped_binary
Node_expr* binary_new(const Env* env, Node_expr* lhs, Node_expr* rhs, TOKEN_TYPE operator_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node_expr* operator_ = node_unwrap_expr(node_new(node_wrap_expr(lhs)->pos, NODE_EXPR));
    operator_->type = NODE_E_OPERATOR;
    Node_e_operator* operator = node_unwrap_e_operator(operator_);
    operator->type = NODE_OP_BINARY;
    Node_op_binary* binary = node_unwrap_op_binary(operator);
    binary->token_type = operator_type;
    binary->lhs = lhs;
    binary->rhs = rhs;

    Lang_type dummy;
    symbol_log(LOG_DEBUG, env);
    Node_expr* new_node;
    try(try_set_binary_lang_type(env, &new_node, &dummy, binary));
    return new_node;
}

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

uint64_t sizeof_lang_type(Lang_type lang_type) {
    if (lang_type_is_equal(lang_type, lang_type_from_cstr("ptr", 0))) {
        return 8;
    } else if (lang_type_is_equal(lang_type, lang_type_from_cstr("i32", 0))) {
        return 4;
    } else {
        unreachable("");
    }
}

static uint64_t sizeof_expr(const Env* env, const Node_expr* expr) {
    (void) env;
    switch (expr->type) {
        case NODE_E_LITERAL:
            return sizeof_lang_type(node_unwrap_e_literal_const(expr)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t sizeof_item(const Env* env, const Node* item) {
    (void) env;
    switch (item->type) {
        case NODE_EXPR:
            return sizeof_expr(env, node_unwrap_expr_const(item));
        case NODE_VARIABLE_DEF:
            return sizeof_lang_type(node_unwrap_variable_def_const(item)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Env* env, const Node_e_struct_literal* struct_literal) {
    const Node_struct_def* struct_def = 
        get_struct_def_const(env, node_wrap_expr_const(node_wrap_e_struct_literal_const(struct_literal)));
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
        case NODE_E_STRUCT_LITERAL:
            return sizeof_struct_literal(env, node_unwrap_e_struct_literal_const(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

uint64_t sizeof_struct(const Env* env, const Node* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case NODE_STRUCT_DEF:
            return sizeof_struct_def_base(env, &node_unwrap_struct_def_const(struct_literal_or_def)->base);
        default:
            node_printf(struct_literal_or_def);
            todo();
    }
    unreachable("");
}

bool lang_type_is_struct(const Env* env, Lang_type lang_type) {
    Node* def = NULL;
    if (!symbol_lookup(&def, env, lang_type.str)) {
        return false;
    }

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return true;
        case NODE_RAW_UNION_DEF:
            return false;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(def), lang_type_print(lang_type));
    }
    unreachable("");
}

bool lang_type_is_raw_union(const Env* env, Lang_type lang_type) {
    Node* def = NULL;
    if (!symbol_lookup(&def, env, lang_type.str)) {
        return false;
    }

    switch (def->type) {
        case NODE_STRUCT_DEF:
            return false;
        case NODE_RAW_UNION_DEF:
            return true;
        default:
            unreachable(NODE_FMT"    "LANG_TYPE_FMT"\n", node_print(def), lang_type_print(lang_type));
    }
    unreachable("");
}

bool is_corresponding_to_a_struct(const Env* env, const Node* node) {
    Node* var_def;
    Node* struct_def;

    if (node->type == NODE_EXPR) {
        const Node_expr* expr = node_unwrap_expr_const(node);
        switch (expr->type) {
            case NODE_E_STRUCT_LITERAL:
                return true;
            case NODE_E_SYMBOL_TYPED:
                assert(get_node_name(node).count > 0);
                if (!symbol_lookup(&var_def, env, get_node_name(node))) {
                    todo();
                    return false;
                }
                if (!symbol_lookup(&struct_def, env, get_lang_type(var_def).str)) {
                    return false;
                }
                return true;
            case NODE_E_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_E_LITERAL:
                unreachable("");
            case NODE_E_MEMBER_SYM_TYPED:
                return true;
            default:
                unreachable("");
        }
    }

    switch (node->type) {
        case NODE_VARIABLE_DEF:
            // fallthrough
        case NODE_STORE_ANOTHER_NODE:
            // fallthrough
        case NODE_MEMBER_SYM_PIECE_TYPED:
            // fallthrough
            assert(get_node_name(node).count > 0);
            if (!symbol_lookup(&var_def, env, get_node_name(node))) {
                todo();
                return false;
            }
            if (!symbol_lookup(&struct_def, env, get_lang_type(var_def).str)) {
                return false;
            }
            return true;
        default:
            log_tree(LOG_FETAL, node);
            todo();
    }
}

bool try_get_struct_def(const Env* env, Node_struct_def** struct_def, Node* node) {
    if (node->type == NODE_EXPR) {
        const Node_expr* expr = node_unwrap_expr_const(node);
        switch (expr->type) {
            case NODE_E_STRUCT_LITERAL: {
                assert(get_lang_type(node).str.count > 0);
                Node* struct_def_;
                if (!symbol_lookup(&struct_def_, env, get_lang_type(node).str)) {
                    return false;
                }
                *struct_def = node_unwrap_struct_def(struct_def_);
                return true;
            }
            case NODE_E_SYMBOL_TYPED:
                // fallthrough
            case NODE_E_MEMBER_SYM_TYPED: {
                Node* var_def;
                assert(get_node_name(node).count > 0);
                if (!symbol_lookup(&var_def, env, get_node_name(node))) {
                    todo();
                    return false;
                }
                assert(get_lang_type(var_def).str.count > 0);
                Node* struct_def_;
                if (!symbol_lookup(&struct_def_, env, get_lang_type(var_def).str)) {
                    todo();
                    return false;
                }
                *struct_def = node_unwrap_struct_def(struct_def_);
                return true;
            }
            case NODE_E_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            default:
                unreachable("");
        }
    }

    switch (node->type) {
        case NODE_VARIABLE_DEF: {
            assert(get_lang_type(node).str.count > 0);
            Node* struct_def_;
            if (!symbol_lookup(&struct_def_, env, get_lang_type(node).str)) {
                return false;
            }
            *struct_def = node_unwrap_struct_def(struct_def_);
            return true;
        }
        default:
            unreachable("");
    }
}

static void try_set_literal_lang_type(Lang_type* lang_type, Node_e_literal* literal, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            literal->lang_type = lang_type_from_cstr("u8", 1);
            break;
        case TOKEN_INT_LITERAL: {
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            int64_t bit_width = bit_width_needed_signed(node_unwrap_lit_number(literal)->data);
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            literal->lang_type = lang_type_from_strv(string_to_strv(lang_type_str), 0);
            break;
        }
        case TOKEN_VOID:
            literal->lang_type = lang_type_from_cstr("void", 0);
            break;
        default:
            unreachable("");
    }

    *lang_type = literal->lang_type;
}

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_e_symbol_untyped* sym_untyped) {
    Node* sym_def;
    if (!symbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(node_wrap_expr(node_wrap_e_symbol_untyped(sym_untyped)));
        return false;
    }
    Node_e_symbol_untyped temp = *sym_untyped;
    node_wrap_e_symbol_untyped(sym_untyped)->type = NODE_E_SYMBOL_TYPED;
    Node_e_symbol_typed* sym_typed = node_unwrap_e_symbol_typed(node_wrap_e_symbol_untyped(sym_untyped));
    sym_typed->name = temp.name;
    sym_typed->lang_type = node_unwrap_variable_def_const(sym_def)->lang_type;
    *lang_type = sym_typed->lang_type;
    *new_node = node_wrap_e_symbol_typed(sym_typed);
    assert(*new_node);
    return true;
}

// returns false if unsuccessful
bool try_set_binary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_op_binary* operator) {
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

    if (!lang_type_is_equal(get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs))) {
        if (can_be_implicitly_converted(get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs))) {
            if (operator->rhs->type == NODE_E_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                node_unwrap_e_literal(operator->rhs)->lang_type = get_lang_type_expr(operator->lhs);
            } else {
                Node_expr* unary = unary_new(env, operator->rhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->lhs));
                operator->rhs = unary;
            }
        } else if (can_be_implicitly_converted(get_lang_type_expr(operator->rhs), get_lang_type_expr(operator->lhs))) {
            if (operator->lhs->type == NODE_E_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                node_unwrap_e_literal(operator->lhs)->lang_type = get_lang_type_expr(operator->rhs);
            } else {
                Node_expr* unary = unary_new(env, operator->lhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->rhs));
                operator->lhs = unary;
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, node_wrap_expr(node_wrap_e_operator(node_wrap_op_binary(operator)))->pos,
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
    if (operator->lhs->type == NODE_E_LITERAL && operator->rhs->type == NODE_E_LITERAL) {
        int64_t lhs_val = node_auto_unwrap_lit_number(operator->lhs)->data;
        int64_t rhs_val = node_auto_unwrap_lit_number(operator->rhs)->data;

        int64_t result_val;
        switch (operator->token_type) {
            case TOKEN_SINGLE_PLUS:
                result_val = lhs_val + rhs_val;
                break;
            case TOKEN_SINGLE_MINUS:
                result_val = lhs_val - rhs_val;
                break;
            case TOKEN_ASTERISK:
                result_val = lhs_val*rhs_val;
                break;
            case TOKEN_SLASH:
                result_val = lhs_val/rhs_val;
                break;
            case TOKEN_LESS_THAN:
                result_val = lhs_val < rhs_val ? 1 : 0;
                break;
            case TOKEN_GREATER_THAN:
                result_val = lhs_val > rhs_val ? 1 : 0;
                break;
            case TOKEN_DOUBLE_EQUAL:
                result_val = lhs_val == rhs_val ? 1 : 0;
                break;
            case TOKEN_NOT_EQUAL:
                result_val = lhs_val != rhs_val ? 1 : 0;
                break;
            default:
                unreachable(TOKEN_TYPE_FMT"\n", token_type_print(operator->token_type));
        }

        Str_view result_strv = int64_t_to_str_view(result_val);
        Node_e_literal* literal = literal_new(result_strv, TOKEN_INT_LITERAL, node_wrap_expr(node_wrap_e_operator(node_wrap_op_binary(operator)))->pos);
        *new_node = node_wrap_e_literal(literal);
        log_tree(LOG_DEBUG, node_wrap_expr(*new_node));
    } else {
        *new_node = node_wrap_e_operator(node_wrap_op_binary(operator));
    }

    assert(get_lang_type_expr(*new_node).str.count > 0);
    assert(*new_node);
    return true;
}

bool try_set_unary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_op_unary* unary) {
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
            assert(unary->lang_type.pointer_depth > 0);
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
            if (!is_i_lang_type(unary->lang_type) || !is_i_lang_type(get_lang_type_expr(unary->child))) {
                todo();
            }
            *lang_type = init_lang_type;
            break;
        default:
            unreachable("");
    }

    *new_node = node_wrap_e_operator(node_wrap_op_unary(unary));
    return true;
}

// returns false if unsuccessful
bool try_set_operator_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_e_operator* operator) {
    if (operator->type == NODE_OP_UNARY) {
        return try_set_unary_lang_type(env, new_node, lang_type, node_unwrap_op_unary(operator));
    } else if (operator->type == NODE_OP_BINARY) {
        return try_set_binary_lang_type(env, new_node, lang_type, node_unwrap_op_binary(operator));
    } else {
        unreachable("");
    }
}

bool try_set_struct_literal_assignment_types(const Env* env, Node** new_node, Lang_type* lang_type, const Node* lhs, Node_e_struct_literal* struct_literal) {
    log(LOG_DEBUG, "------------------------------\n");
    if (!is_corresponding_to_a_struct(env, lhs)) {
        todo(); // non_struct assigned struct literal
    }
    Node* lhs_var_def_;
    try(symbol_lookup(&lhs_var_def_, env, get_node_name(lhs)));
    Node_variable_def* lhs_var_def = node_unwrap_variable_def(lhs_var_def_);
    Node* struct_def_;
    try(symbol_lookup(&struct_def_, env, lhs_var_def->lang_type.str));
    switch (struct_def_->type) {
        case NODE_STRUCT_DEF:
            break;
        case NODE_RAW_UNION_DEF:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION,
                node_wrap_expr(node_wrap_e_struct_literal(struct_literal))->pos,
                "struct literal cannot be assigned to union\n"
            );
            return false;
        default:
            unreachable("");
    }
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    Lang_type new_lang_type = {.str = struct_def->base.name, .pointer_depth = 0};
    struct_literal->lang_type = new_lang_type;
    
    Node_ptr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        log(LOG_DEBUG, "%zu\n", idx);
        Node* memb_sym_def_ = vec_at(&struct_def->base.members, idx);
        Node_variable_def* memb_sym_def = node_unwrap_variable_def(memb_sym_def_);
        log_tree(LOG_DEBUG, node_wrap_expr_const(node_wrap_e_struct_literal_const(struct_literal)));
        Node_assignment* assign_memb_sym = node_unwrap_assignment(vec_at(&struct_literal->members, idx));
        Node_e_symbol_untyped* memb_sym_piece_untyped = node_unwrap_e_symbol_untyped(node_unwrap_expr(assign_memb_sym->lhs));
        Node_e_literal* assign_memb_sym_rhs = node_unwrap_e_literal(assign_memb_sym->rhs);

        assign_memb_sym_rhs->lang_type = memb_sym_def->lang_type;
        if (!str_view_is_equal(memb_sym_def->name, memb_sym_piece_untyped->name)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL, node_wrap_expr(node_wrap_e_symbol_untyped(memb_sym_piece_untyped))->pos,
                "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
                str_view_print(memb_sym_def->name), str_view_print(memb_sym_piece_untyped->name)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap_variable_def(lhs_var_def)->pos,
                "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
                str_view_print(lhs_var_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap_variable_def(memb_sym_def)->pos,
                "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
                str_view_print(memb_sym_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            return false;
        }

        vec_append_safe(&a_main, &new_literal_members, node_wrap_expr(node_wrap_e_literal(assign_memb_sym_rhs)));
    }

    struct_literal->members = new_literal_members;

    assert(struct_literal->lang_type.str.count > 0);

    *lang_type = new_lang_type;
    *new_node = node_wrap_expr(node_wrap_e_struct_literal(struct_literal));
    return true;
}

bool try_set_expr_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_expr* node) {
    switch (node->type) {
        case NODE_E_LITERAL:
            *lang_type = node_unwrap_e_literal(node)->lang_type;
            *new_node = node;
            return true;
        case NODE_E_SYMBOL_UNTYPED:
            if (!try_set_symbol_type(env, new_node, lang_type, node_unwrap_e_symbol_untyped(node))) {
                return false;
            } else {
                assert(*new_node);
            }
            return true;
        case NODE_E_MEMBER_SYM_UNTYPED: {
            Node* new_node_ = NULL;
            if (!try_set_member_symbol_types(env, &new_node_, lang_type, node_unwrap_e_member_sym_untyped(node))) {
                return false;
            }
            *new_node = node_unwrap_expr(new_node_);
            return true;
        }
        case NODE_E_MEMBER_SYM_TYPED:
            *lang_type = get_member_sym_piece_final_lang_type(node_unwrap_e_member_sym_typed(node));
            *new_node = node;
            return true;
        case NODE_E_SYMBOL_TYPED:
            *lang_type = node_unwrap_e_symbol_typed(node)->lang_type;
            *new_node = node;
            return true;
        case NODE_E_OPERATOR:
            if (!try_set_operator_lang_type(env, new_node, lang_type, node_unwrap_e_operator(node))) {
                return false;
            }
            assert(*new_node);
            return true;
        case NODE_E_FUNCTION_CALL:
            return try_set_function_call_types(env, new_node, lang_type, node_unwrap_e_function_call(node));
        case NODE_E_STRUCT_LITERAL:
            unreachable("cannot set struct literal type here");
        default:
            unreachable("");
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
    log_tree(LOG_DEBUG, node_wrap_assignment(assignment));

    Node_expr* new_rhs;
    if (assignment->rhs->type == NODE_E_STRUCT_LITERAL) {
        Node* new_rhs_ = NULL;
        if (!try_set_struct_literal_assignment_types(
            env, &new_rhs_, &rhs_lang_type, assignment->lhs, node_unwrap_e_struct_literal(assignment->rhs)
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
    switch (do_implicit_conversion_if_needed(lhs_lang_type, assignment->rhs)) {
        case IMPLICIT_CONV_INVALID_TYPES: {
            msg(
                LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, node_wrap_assignment(assignment)->pos,
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

bool try_set_function_call_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_e_function_call* fun_call) {
    bool status = true;

    Node* fun_def;
    if (!symbol_lookup(&fun_def, env, fun_call->name)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, node_wrap_expr(node_wrap_e_function_call(fun_call))->pos,
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
    } else if (node_unwrap_variable_def(vec_top(&params->params))->is_variadic) {
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
            LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, node_wrap_expr(node_wrap_e_function_call(fun_call))->pos,
            STR_VIEW_FMT, str_view_print(string_to_strv(message))
        );

        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, fun_def->pos,
            "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
        );
        return false;
    }

    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Node_variable_def* corres_param = node_unwrap_variable_def(vec_at(&params->params, params_idx));
        Node_expr** argument = vec_at_ref(&fun_call->args, arg_idx);
        Node_expr* new_arg = NULL;

        if (!corres_param->is_variadic) {
            params_idx++;
        }

        Lang_type dummy = {0};
        Node* new_arg_ = NULL;
        if ((*argument)->type == NODE_E_STRUCT_LITERAL) {
            todo();
            try(try_set_struct_literal_assignment_types(
                env, &new_arg_, &dummy, node_wrap_variable_def(corres_param), node_unwrap_e_struct_literal(*argument)
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

        if (lang_type_is_equal(corres_param->lang_type, lang_type_from_cstr("any", 0))) {
            if (corres_param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                continue;
            } else {
                todo();
            }
        }

        assert(*argument);
        if (!lang_type_is_equal(corres_param->lang_type, get_lang_type_expr(*argument))) {
            if (can_be_implicitly_converted(corres_param->lang_type, get_lang_type_expr(*argument))) {
                if ((*argument)->type == NODE_E_LITERAL) {
                    node_unwrap_e_literal((*argument))->lang_type = corres_param->lang_type;
                } else {
                    *argument = unary_new(env, *argument, TOKEN_UNSAFE_CAST, corres_param->lang_type);
                }
            } else {
                msg(
                    LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, node_wrap_expr(*argument)->pos, 
                    "argument is of type `"LANG_TYPE_FMT"`, "
                    "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
                    lang_type_print(get_lang_type_expr(*argument)), 
                    str_view_print(corres_param->name),
                    lang_type_print(corres_param->lang_type)
                );
                msg(
                    LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap_variable_def(corres_param)->pos,
                    "corresponding parameter `"STR_VIEW_FMT"` defined here\n",
                    str_view_print(corres_param->name)
                );
                return false;
            }
        }
    }

    *lang_type = fun_rtn_type->lang_type;
    *new_node = node_wrap_e_function_call(fun_call);
    return status;
}

bool try_set_member_symbol_types(const Env* env, Node** new_node, Lang_type* lang_type, Node_e_member_sym_untyped* memb_sym_untyped) {
    Node_variable_def* curr_memb_def = NULL;
    Node* struct_def_;
    Node* struct_var;
    if (!symbol_lookup(&struct_var, env, memb_sym_untyped->name)) {
        msg_undefined_symbol(node_wrap_expr(node_wrap_e_member_sym_untyped(memb_sym_untyped)));
        *new_node = node_wrap_expr(node_wrap_e_member_sym_untyped(memb_sym_untyped));
        return false;
    }
    if (!symbol_lookup(&struct_def_, env, node_unwrap_variable_def(struct_var)->lang_type.str)) {
        todo(); // this should possibly never happen
    }

    const Struct_def_base* def_base = NULL;
    switch (struct_def_->type) {
        case NODE_STRUCT_DEF:
            def_base = &node_unwrap_struct_def(struct_def_)->base;
            break;
        case NODE_RAW_UNION_DEF:
            def_base = &node_unwrap_raw_union_def(struct_def_)->base;
            break;
        default:
            unreachable("");
    }

    Node_expr* memb_sym_typed_ = node_unwrap_expr(node_new(node_wrap_expr(node_wrap_e_member_sym_untyped(memb_sym_untyped))->pos, NODE_EXPR));
    memb_sym_typed_->type = NODE_E_MEMBER_SYM_TYPED;
    Node_e_member_sym_typed* memb_sym_typed = node_unwrap_e_member_sym_typed(memb_sym_typed_);

    memb_sym_typed->name = memb_sym_untyped->name;
    assert(memb_sym_typed->name.count > 0);
    bool is_struct = true;
    const Struct_def_base* prev_def_base = def_base;
    for (size_t idx = 0; idx < memb_sym_untyped->children.info.count; idx++) {
        Node* memb_sym_piece_untyped_ = vec_at(&memb_sym_untyped->children, idx);
        Node_member_sym_piece_untyped* memb_sym_piece_untyped = 
            node_unwrap_member_sym_piece_untyped(memb_sym_piece_untyped_);
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, def_base, node_wrap_member_sym_piece_untyped(memb_sym_piece_untyped))) {
            //msg_invalid_struct_member(env, memb_sym_typed, memb_sym_piece_untyped);
            msg(
                LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER, node_wrap_member_sym_piece_untyped(memb_sym_piece_untyped)->pos,
                "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
                str_view_print(memb_sym_piece_untyped->name), str_view_print(memb_sym_typed->name)
            );
            Node* memb_sym_def_;
            try(symbol_lookup(&memb_sym_def_, env, memb_sym_typed->name));
            const Node_variable_def* memb_sym_def = node_unwrap_variable_def_const(memb_sym_def_);
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap_variable_def_const(memb_sym_def)->pos,
                "`"STR_VIEW_FMT"` defined here as type `"LANG_TYPE_FMT"`\n",
                str_view_print(memb_sym_def->name),
                lang_type_print(memb_sym_def->lang_type)
            );
            Node* struct_def;
            try(symbol_lookup(&struct_def, env, memb_sym_def->lang_type.str));
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, struct_def->pos,
                "struct `"LANG_TYPE_FMT"` defined here\n", 
                lang_type_print(memb_sym_def->lang_type)
            );
            *new_node = node_wrap_expr(node_wrap_e_member_sym_untyped(memb_sym_untyped));
            return false;
        }

        Node_struct_def* struct_def_ = NULL;
        if (try_get_struct_def(env, &struct_def_, node_wrap_variable_def(curr_memb_def))) {
            def_base = &struct_def_->base;
        } else {
            is_struct = false;
        }

        Node_member_sym_piece_typed* memb_sym_piece_typed = 
            node_unwrap_member_sym_piece_typed(node_new(
                node_wrap_expr(node_wrap_e_member_sym_untyped(memb_sym_untyped))->pos, NODE_MEMBER_SYM_PIECE_TYPED
            ));
        memb_sym_piece_typed->name = curr_memb_def->name;
        memb_sym_piece_typed->lang_type = curr_memb_def->lang_type;
        *lang_type = memb_sym_piece_typed->lang_type;
        vec_append_safe(&a_main, &memb_sym_typed->children, node_wrap_member_sym_piece_typed(memb_sym_piece_typed));
        memb_sym_piece_typed->struct_index = get_member_index(prev_def_base, memb_sym_piece_typed);

        prev_def_base = def_base;
    }

    *new_node = node_wrap_expr(node_wrap_e_member_sym_typed(memb_sym_typed));
    return true;
}

INLINE Node_expr* condition_get_default_child(const Env* env, Node_expr* if_cond_child) {
    return binary_new(
        env,
        if_cond_child,
        node_wrap_e_literal(literal_new(str_view_from_cstr("0"), TOKEN_INT_LITERAL, node_wrap_expr(if_cond_child)->pos)),
        TOKEN_NOT_EQUAL
    );
}

static bool try_set_condition_types(const Env* env, Lang_type* lang_type, Node_condition* if_cond) {
    Node_expr* new_if_cond_child;
    try(try_set_expr_lang_type(env, &new_if_cond_child, lang_type, if_cond->child));
    if_cond->child = new_if_cond_child;

    switch (if_cond->child->type) {
        case NODE_E_OPERATOR:
            break;
        case NODE_E_SYMBOL_TYPED:
            if_cond->child = condition_get_default_child(env, if_cond->child);
            break;
        case NODE_E_FUNCTION_CALL:
            if_cond->child = condition_get_default_child(env, if_cond->child);
            break;
        case NODE_E_LITERAL:
            if_cond->child = condition_get_default_child(env, if_cond->child);
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
        if (curr_node->type == NODE_FUNCTION_DEF) {
            return node_unwrap_function_def(curr_node);
        }

        if (idx < 1) {
            unreachable("");
        }
    }
}

Lang_type get_parent_function_return_type(const Env* env) {
    return get_parent_function_def_const(env)->declaration->return_type->lang_type;
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
        case NODE_IF:
            return try_set_condition_types(env, lang_type, node_unwrap_if(node)->condition);
        case NODE_FOR_WITH_COND:
            return try_set_condition_types(env, lang_type, node_unwrap_for_with_cond(node)->condition);
        case NODE_ASSIGNMENT:
            return try_set_assignment_operand_types(env, lang_type, node_unwrap_assignment(node));
        case NODE_VARIABLE_DEF:
            *lang_type = node_unwrap_variable_def(node)->lang_type;
            return true;
        case NODE_RETURN: {
            Node_return* rtn_statement = node_unwrap_return(node);
            Node_expr* new_rtn_child;
            if (!try_set_expr_lang_type(env, &new_rtn_child, lang_type, rtn_statement->child)) {
                todo();
                return false;
            }
            rtn_statement->child = new_rtn_child;

            Lang_type src_lang_type = get_lang_type_expr(rtn_statement->child);
            Lang_type dest_lang_type = get_parent_function_def_const(env)->declaration->return_type->lang_type;
            log(
                LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
                lang_type_print(dest_lang_type)
            );

            if (lang_type_is_equal(dest_lang_type, src_lang_type)) {
                return true;
            }
            if (can_be_implicitly_converted(dest_lang_type, src_lang_type)) {
                log_tree(LOG_DEBUG, node_wrap_return(rtn_statement));
                if (rtn_statement->child->type == NODE_E_LITERAL) {
                    node_unwrap_e_literal(rtn_statement->child)->lang_type = dest_lang_type;
                    return true;
                }
                Node_expr* unary = unary_new(env, rtn_statement->child, TOKEN_UNSAFE_CAST, dest_lang_type);
                rtn_statement->child = unary;
                return true;
            }

            const Node_function_def* fun_def = get_parent_function_def_const(env);
            if (rtn_statement->auto_inserted) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, node_wrap_return(rtn_statement)->pos,
                    "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
                    lang_type_print(fun_def->declaration->return_type->lang_type)
                );
            } else {
                msg(
                    LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, node_wrap_return(rtn_statement)->pos,
                    "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
                    lang_type_print(get_lang_type_expr(rtn_statement->child)), 
                    lang_type_print(fun_def->declaration->return_type->lang_type)
                );
            }
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap_lang_type(fun_def->declaration->return_type)->pos,
                "function return type `"LANG_TYPE_FMT"` defined here\n",
                lang_type_print(fun_def->declaration->return_type->lang_type)
            );
            return false;
            //log(
            //    LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
            //    lang_type_print(dest_lang_type)
            //);
            unreachable("");
        }
        case NODE_FUNCTION_DECL:
            // TODO: do this?
            *new_node = node;
            return true;
        case NODE_FUNCTION_DEF:
            *new_node = node;
            return true;
        case NODE_STRUCT_DEF:
            *new_node = node;
            return true;
        case NODE_RAW_UNION_DEF:
            *new_node = node;
            return true;
        case NODE_FOR_RANGE:
            *new_node = node;
            return true;
        case NODE_BREAK:
            *new_node = node;
            return true;
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}

