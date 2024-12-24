#include <str_view.h>
#include <string_vec.h>
#include <node_ptr_vec.h>
#include <node.h>
#include <nodes.h>
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

Str_view util_literal_name_new(void) {
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

Str_view util_literal_name_new_prefix(const char* debug_prefix) {
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
            Llvm* result = NULL;
            if (!alloca_lookup(&result, env, sym_def->name)) {
                unreachable(STR_VIEW_FMT"\n", str_view_print(sym_def->name));
            }
            return (Llvm_register_sym) {
                .lang_type = llvm_get_lang_type(result),
                .llvm = result
            };
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

Llvm_id get_matching_label_id(const Env* env, Str_view name) {
    Llvm* label_;
    if (!alloca_lookup(&label_, env, name)) {
        symbol_log(LOG_DEBUG, env);
    }
    Llvm_label* label = llvm_unwrap_label(llvm_unwrap_def(label_));
    return label->llvm_id;
}

Node_assignment* util_assignment_new(Env* env, Node* lhs, Node_expr* rhs) {
    Node_assignment* assignment = node_assignment_new(node_get_pos(lhs));
    assignment->lhs = lhs;
    assignment->rhs = rhs;

    try_set_assignment_types(env, assignment);
    return assignment;
}

// TODO: try to deduplicate 2 below functions
Node_literal* util_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* new_literal = NULL;
    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Node_number* literal = node_number_new(pos);
            literal->data = str_view_to_int64_t(value);
            new_literal = node_wrap_number(literal);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            Node_string* literal = node_string_new(pos);
            literal->data = value;
            literal->name = util_literal_name_new();
            new_literal = node_wrap_string(literal);
            break;
        }
        case TOKEN_VOID: {
            Node_void* literal = node_void_new(pos);
            new_literal = node_wrap_void(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            Node_char* literal = node_char_new(pos);
            literal->data = str_view_front(value);
            new_literal = node_wrap_char(literal);
            break;
        }
        default:
            unreachable("");
    }

    assert(new_literal);

    try_set_literal_types(new_literal, token_type);
    return new_literal;
}

Node_literal* util_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* new_literal = NULL;

    switch (token_type) {
        case TOKEN_INT_LITERAL: {
            Node_number* literal = node_number_new(pos);
            literal->data = value;
            new_literal = node_wrap_number(literal);
            break;
        }
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_VOID: {
            Node_void* literal = node_void_new(pos);
            new_literal = node_wrap_void(literal);
            break;
        }
        case TOKEN_CHAR_LITERAL: {
            Node_char* literal = node_char_new(pos);
            assert(value < INT8_MAX);
            literal->data = value;
            new_literal = node_wrap_char(literal);
            break;
        }
        default:
            unreachable("");
    }

    assert(new_literal);

    try_set_literal_types(new_literal, token_type);
    return new_literal;
}

Node_symbol_untyped* util_symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node_symbol_untyped* symbol = node_symbol_untyped_new(pos);
    symbol->name = symbol_name;
    return symbol;
}

// TODO: make separate Node_unary_typed and Node_unary_untyped
Node_expr* util_unary_new(Env* env, Node_expr* child, TOKEN_TYPE operator_type, Lang_type init_lang_type) {
    Node_unary* unary = node_unary_new(node_get_pos_expr(child));
    unary->token_type = operator_type;
    unary->child = child;
    unary->lang_type = init_lang_type;

    Node_expr* new_node;
    //symbol_log(LOG_DEBUG, env);
    try(try_set_unary_types(env, &new_node, unary));
    return new_node;
}

Node_operator* util_binary_typed_new(Env* env, Node_expr* lhs, Node_expr* rhs, TOKEN_TYPE operator_type) {
    Node_binary* binary = node_binary_new(node_get_pos_expr(lhs));
    binary->token_type = operator_type;
    binary->lhs = lhs;
    binary->rhs = rhs;

    Node_expr* new_node;
    try(try_set_binary_types(env, &new_node, binary));

    return node_unwrap_operator(new_node);
}

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

Node_operator* condition_get_default_child(Node_expr* if_cond_child) {
    Node_binary* new_oper = node_binary_new(node_get_pos_expr(if_cond_child));
    new_oper->lhs = node_wrap_literal(
        util_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, node_get_pos_expr(if_cond_child))
    );
    new_oper->rhs = if_cond_child;
    new_oper->token_type = TOKEN_NOT_EQUAL;
    new_oper->lang_type = lang_type_new_from_cstr("i32", 0);

    return node_wrap_binary(new_oper);
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
            return sizeof_lang_type(env, node_get_lang_type_literal(node_unwrap_literal_const(expr)));
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

static uint64_t llvm_sizeof_expr(const Env* env, const Llvm_expr* expr) {
    (void) env;
    switch (expr->type) {
        case NODE_LITERAL:
            return sizeof_lang_type(env, llvm_get_lang_type_literal(llvm_unwrap_literal_const(expr)));
        default:
            unreachable("");
    }
}

static uint64_t llvm_sizeof_def(const Env* env, const Llvm_def* def) {
    (void) env;
    switch (def->type) {
        case NODE_VARIABLE_DEF:
            return sizeof_lang_type(env, llvm_unwrap_variable_def_const(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_item(const Env* env, const Llvm* item) {
    (void) env;
    switch (item->type) {
        case NODE_EXPR:
            return llvm_sizeof_expr(env, llvm_unwrap_expr_const(item));
        case NODE_DEF:
            return llvm_sizeof_def(env, llvm_unwrap_def_const(item));
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_struct_literal(const Env* env, const Llvm_struct_literal* struct_literal) {
    const Node_struct_def* struct_def = 
        llvm_get_struct_def_const(env, llvm_wrap_expr_const(llvm_wrap_struct_literal_const(struct_literal)));
    return sizeof_struct_def_base(env, &struct_def->base);
}

uint64_t llvm_sizeof_struct_def_base(const Env* env, const Struct_def_base* base) {
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

uint64_t llvm_sizeof_struct_expr(const Env* env, const Llvm_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case NODE_STRUCT_LITERAL:
            return llvm_sizeof_struct_literal(env, llvm_unwrap_struct_literal_const(struct_literal_or_def));
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
                assert(node_get_lang_type(node).str.count > 0);
                return symbol_lookup(def, env, node_get_lang_type(node).str);
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
                assert(node_get_lang_type_def(var_def).str.count > 0);
                return symbol_lookup(def, env, node_get_lang_type_def(var_def).str);
            }
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_MEMBER_ACCESS_UNTYPED:
                assert(node_get_lang_type(node).str.count > 0);
                return symbol_lookup(def, env, node_get_lang_type(node).str);
            default:
                unreachable(NODE_FMT"\n", node_print(node));
        }
    }

    Node_def* node_def = node_unwrap_def(node);
    switch (node_def->type) {
        case NODE_VARIABLE_DEF: {
            assert(node_get_lang_type_def(node_def).str.count > 0);
            return symbol_lookup(def, env, node_get_lang_type_def(node_def).str);
        }
        default:
            unreachable("");
    }
}

bool llvm_try_get_generic_struct_def(const Env* env, Node_def** def, Llvm* llvm) {
    if (llvm->type == LLVM_EXPR) {
        const Llvm_expr* expr = llvm_unwrap_expr_const(llvm);
        switch (expr->type) {
            case LLVM_STRUCT_LITERAL: {
                assert(llvm_get_lang_type(llvm).str.count > 0);
                return symbol_lookup(def, env, llvm_get_lang_type(llvm).str);
            }
            case LLVM_SYMBOL_TYPED:
                // fallthrough
            case LLVM_MEMBER_ACCESS_TYPED: {
                Node_def* var_def;
                assert(llvm_get_node_name(llvm).count > 0);
                if (!symbol_lookup(&var_def, env, llvm_get_node_name(llvm))) {
                    todo();
                    return false;
                }
                assert(node_get_lang_type_def(var_def).str.count > 0);
                return symbol_lookup(def, env, node_get_lang_type_def(var_def).str);
            }
            case LLVM_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case LLVM_MEMBER_ACCESS_UNTYPED:
                assert(llvm_get_lang_type(llvm).str.count > 0);
                return symbol_lookup(def, env, llvm_get_lang_type(llvm).str);
            default:
                unreachable(LLVM_FMT"\n", llvm_print(llvm));
        }
    }

    Llvm_def* llvm_def = llvm_unwrap_def(llvm);
    switch (llvm_def->type) {
        case LLVM_VARIABLE_DEF: {
            assert(llvm_get_lang_type_def(llvm_def).str.count > 0);
            return symbol_lookup(def, env, llvm_get_lang_type_def(llvm_def).str);
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

bool llvm_try_get_struct_def(const Env* env, Node_struct_def** struct_def, Llvm* node) {
    Node_def* def = NULL;
    if (!llvm_try_get_generic_struct_def(env, &def, node)) {
        return false;
    }
    if (def->type != NODE_STRUCT_DEF) {
        return false;
    }

    *struct_def = node_unwrap_struct_def(def);
    return true;
}

