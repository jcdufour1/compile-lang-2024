
#ifndef AUTO_GEN_NODE_H
#define AUTO_GEN_NODE_H

#include <auto_gen_util.h>

struct Node_type_;
typedef struct Node_type_ Node_type;

typedef struct {
    Vec_base info;
    Node_type* buf;
} Node_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Node_name;

// eg. in Node_symbol_typed
//  prefix = "Node"
//  base = "symbol_typed"
//  sub_types = [{.prefix = "Node_symbol_typed", .base = "primitive", .sub_types = []}, ...]
typedef struct Node_type_ {
    Node_name name;
    Members members;
    Node_type_vec sub_types;
} Node_type;

static void extend_node_name_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "NODE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_node_name_lower(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "node");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_node_name_first_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Node");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_node_name_upper(String* output, Node_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "NODE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_node_name_lower(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        string_extend_cstr(&gen_a, output, "node");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "node");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_node_name_first_upper(String* output, Node_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "node")) {
        string_extend_cstr(&gen_a, output, "Node");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Node");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Node_name node_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Node_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Node_type node_gen_block(void) {
    Node_type block = {.name = node_name_new("node", "block", false)};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Node_ptr_vec", "children");
    append_member(&block.members, "Symbol_collection", "symbol_collection");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Node_type node_gen_unary(void) {
    Node_type unary = {.name = node_name_new("operator", "unary", false)};

    append_member(&unary.members, "Node_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");
    append_member(&unary.members, "Llvm_id", "llvm_id");

    return unary;
}

static Node_type node_gen_binary(void) {
    Node_type binary = {.name = node_name_new("operator", "binary", false)};

    append_member(&binary.members, "Node_expr*", "lhs");
    append_member(&binary.members, "Node_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");
    append_member(&binary.members, "Llvm_id", "llvm_id");

    return binary;
}

static Node_type node_gen_primitive_sym(void) {
    Node_type primitive = {.name = node_name_new("symbol_typed", "primitive_sym", false)};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Node_type node_gen_enum_sym(void) {
    Node_type lang_enum = {.name = node_name_new("symbol_typed", "enum_sym", false)};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Node_type node_gen_struct_sym(void) {
    Node_type lang_struct = {.name = node_name_new("symbol_typed", "struct_sym", false)};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Node_type node_gen_raw_union_sym(void) {
    Node_type raw_union = {.name = node_name_new("symbol_typed", "raw_union_sym", false)};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Node_type node_gen_operator(void) {
    Node_type operator = {.name = node_name_new("expr", "operator", false)};

    vec_append(&gen_a, &operator.sub_types, node_gen_unary());
    vec_append(&gen_a, &operator.sub_types, node_gen_binary());

    return operator;
}

static Node_type node_gen_symbol_untyped(void) {
    Node_type sym = {.name = node_name_new("expr", "symbol_untyped", false)};

    append_member(&sym.members, "Str_view", "name");

    return sym;
}

static Node_type node_gen_symbol_typed(void) {
    Node_type sym = {.name = node_name_new("expr", "symbol_typed", false)};

    vec_append(&gen_a, &sym.sub_types, node_gen_primitive_sym());
    vec_append(&gen_a, &sym.sub_types, node_gen_struct_sym());
    vec_append(&gen_a, &sym.sub_types, node_gen_raw_union_sym());
    vec_append(&gen_a, &sym.sub_types, node_gen_enum_sym());

    return sym;
}

static Node_type node_gen_member_access_typed(void) {
    Node_type access = {.name = node_name_new("expr", "member_access_typed", false)};

    append_member(&access.members, "Lang_type", "lang_type");
    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");
    append_member(&access.members, "Llvm_id", "llvm_id");

    return access;
}

static Node_type node_gen_member_access_untyped(void) {
    Node_type access = {.name = node_name_new("expr", "member_access_untyped", false)};

    append_member(&access.members, "Str_view", "member_name");
    append_member(&access.members, "Node_expr*", "callee");

    return access;
}

static Node_type node_gen_index_untyped(void) {
    Node_type index = {.name = node_name_new("expr", "index_untyped", false)};

    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");

    return index;
}

static Node_type node_gen_index_typed(void) {
    Node_type index = {.name = node_name_new("expr", "index_typed", false)};

    append_member(&index.members, "Lang_type", "lang_type");
    append_member(&index.members, "Node_expr*", "index");
    append_member(&index.members, "Node_expr*", "callee");
    append_member(&index.members, "Llvm_id", "llvm_id");

    return index;
}

static Node_type node_gen_number(void) {
    Node_type number = {.name = node_name_new("literal", "number", false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Lang_type", "lang_type");

    return number;
}

static Node_type node_gen_string(void) {
    Node_type string = {.name = node_name_new("literal", "string", false)};

    append_member(&string.members, "Str_view", "data");
    append_member(&string.members, "Lang_type", "lang_type");
    append_member(&string.members, "Str_view", "name");

    return string;
}

static Node_type node_gen_char(void) {
    Node_type lang_char = {.name = node_name_new("literal", "char", false)};

    append_member(&lang_char.members, "char", "data");
    append_member(&lang_char.members, "Lang_type", "lang_type");

    return lang_char;
}

static Node_type node_gen_void(void) {
    Node_type lang_void = {.name = node_name_new("literal", "void", false)};

    append_member(&lang_void.members, "int", "dummy");

    return lang_void;
}

static Node_type node_gen_enum_lit(void) {
    Node_type enum_lit = {.name = node_name_new("literal", "enum_lit", false)};

    append_member(&enum_lit.members, "int64_t", "data");
    append_member(&enum_lit.members, "Lang_type", "lang_type");

    return enum_lit;
}

static Node_type node_gen_literal(void) {
    Node_type lit = {.name = node_name_new("expr", "literal", false)};

    vec_append(&gen_a, &lit.sub_types, node_gen_number());
    vec_append(&gen_a, &lit.sub_types, node_gen_string());
    vec_append(&gen_a, &lit.sub_types, node_gen_void());
    vec_append(&gen_a, &lit.sub_types, node_gen_enum_lit());
    vec_append(&gen_a, &lit.sub_types, node_gen_char());

    return lit;
}

static Node_type node_gen_function_call(void) {
    Node_type call = {.name = node_name_new("expr", "function_call", false)};

    append_member(&call.members, "Expr_ptr_vec", "args");
    append_member(&call.members, "Str_view", "name");
    append_member(&call.members, "Llvm_id", "llvm_id");
    append_member(&call.members, "Lang_type", "lang_type");

    return call;
}

static Node_type node_gen_struct_literal(void) {
    Node_type lit = {.name = node_name_new("expr", "struct_literal", false)};

    append_member(&lit.members, "Node_ptr_vec", "members");
    append_member(&lit.members, "Str_view", "name");
    append_member(&lit.members, "Lang_type", "lang_type");
    append_member(&lit.members, "Llvm_id", "llvm_id");

    return lit;
}

static Node_type node_gen_llvm_placeholder(void) {
    Node_type placeholder = {.name = node_name_new("expr", "llvm_placeholder", false)};

    append_member(&placeholder.members, "Lang_type", "lang_type");
    append_member(&placeholder.members, "Llvm_register_sym", "llvm_reg");

    return placeholder;
}

static Node_type node_gen_expr(void) {
    Node_type expr = {.name = node_name_new("node", "expr", false)};

    vec_append(&gen_a, &expr.sub_types, node_gen_operator());
    vec_append(&gen_a, &expr.sub_types, node_gen_symbol_untyped());
    vec_append(&gen_a, &expr.sub_types, node_gen_symbol_typed());
    vec_append(&gen_a, &expr.sub_types, node_gen_member_access_untyped());
    vec_append(&gen_a, &expr.sub_types, node_gen_member_access_typed());
    vec_append(&gen_a, &expr.sub_types, node_gen_index_untyped());
    vec_append(&gen_a, &expr.sub_types, node_gen_index_typed());
    vec_append(&gen_a, &expr.sub_types, node_gen_literal());
    vec_append(&gen_a, &expr.sub_types, node_gen_function_call());
    vec_append(&gen_a, &expr.sub_types, node_gen_struct_literal());
    vec_append(&gen_a, &expr.sub_types, node_gen_llvm_placeholder());

    return expr;
}

static Node_type node_gen_struct_def(void) {
    Node_type def = {.name = node_name_new("def", "struct_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Node_type node_gen_raw_union_def(void) {
    Node_type def = {.name = node_name_new("def", "raw_union_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Node_type node_gen_function_decl(void) {
    Node_type def = {.name = node_name_new("def", "function_decl", false)};

    append_member(&def.members, "Node_function_params*", "parameters");
    append_member(&def.members, "Node_lang_type*", "return_type");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Node_type node_gen_function_def(void) {
    Node_type def = {.name = node_name_new("def", "function_def", false)};

    append_member(&def.members, "Node_function_decl*", "declaration");
    append_member(&def.members, "Node_block*", "body");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Node_type node_gen_variable_def(void) {
    Node_type def = {.name = node_name_new("def", "variable_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic"); // TODO: : 1
    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Node_type node_gen_enum_def(void) {
    Node_type def = {.name = node_name_new("def", "enum_def", false)};

    append_member(&def.members, "Struct_def_base", "base");

    return def;
}

static Node_type node_gen_primitive_def(void) {
    Node_type def = {.name = node_name_new("def", "primitive_def", false)};

    append_member(&def.members, "Lang_type", "lang_type");

    return def;
}

static Node_type node_gen_label(void) {
    Node_type def = {.name = node_name_new("def", "label", false)};

    append_member(&def.members, "Llvm_id", "llvm_id");
    append_member(&def.members, "Str_view", "name");

    return def;
}

static Node_type node_gen_string_def(void) {
    Node_type def = {.name = node_name_new("literal_def", "string_def", false)};

    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Str_view", "data");

    return def;
}

static Node_type node_gen_struct_lit_def(void) {
    Node_type def = {.name = node_name_new("literal_def", "struct_lit_def", false)};

    append_member(&def.members, "Node_ptr_vec", "members");
    append_member(&def.members, "Str_view", "name");
    append_member(&def.members, "Lang_type", "lang_type");
    append_member(&def.members, "Llvm_id", "llvm_id");

    return def;
}

static Node_type node_gen_literal_def(void) {
    Node_type def = {.name = node_name_new("def", "literal_def", false)};

    vec_append(&gen_a, &def.sub_types, node_gen_string_def());
    vec_append(&gen_a, &def.sub_types, node_gen_struct_lit_def());

    return def;
}

static Node_type node_gen_def(void) {
    Node_type def = {.name = node_name_new("node", "def", false)};

    vec_append(&gen_a, &def.sub_types, node_gen_function_def());
    vec_append(&gen_a, &def.sub_types, node_gen_variable_def());
    vec_append(&gen_a, &def.sub_types, node_gen_struct_def());
    vec_append(&gen_a, &def.sub_types, node_gen_raw_union_def());
    vec_append(&gen_a, &def.sub_types, node_gen_enum_def());
    vec_append(&gen_a, &def.sub_types, node_gen_primitive_def());
    vec_append(&gen_a, &def.sub_types, node_gen_function_decl());
    vec_append(&gen_a, &def.sub_types, node_gen_label());
    vec_append(&gen_a, &def.sub_types, node_gen_literal_def());

    return def;
}

static Node_type node_gen_load_element_ptr(void) {
    Node_type load = {.name = node_name_new("node", "load_element_ptr", false)};

    append_member(&load.members, "Lang_type", "lang_type");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Llvm_register_sym", "struct_index");
    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Str_view", "name");
    append_member(&load.members, "bool", "is_from_struct");

    return load;
}

static Node_type node_gen_function_params(void) {
    Node_type params = {.name = node_name_new("node", "function_params", false)};

    append_member(&params.members, "Node_var_def_vec", "params");
    append_member(&params.members, "Llvm_id", "llvm_id");

    return params;
}

static Node_type node_gen_lang_type(void) {
    Node_type lang_type = {.name = node_name_new("node", "lang_type", false)};

    append_member(&lang_type.members, "Lang_type", "lang_type");

    return lang_type;
}

static Node_type node_gen_for_lower_bound(void) {
    Node_type bound = {.name = node_name_new("node", "for_lower_bound", false)};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Node_type node_gen_for_upper_bound(void) {
    Node_type bound = {.name = node_name_new("node", "for_upper_bound", false)};

    append_member(&bound.members, "Node_expr*", "child");

    return bound;
}

static Node_type node_gen_condition(void) {
    Node_type bound = {.name = node_name_new("node", "condition", false)};

    append_member(&bound.members, "Node_operator*", "child");

    return bound;
}

static Node_type node_gen_for_range(void) {
    Node_type range = {.name = node_name_new("node", "for_range", false)};

    append_member(&range.members, "Node_variable_def*", "var_def");
    append_member(&range.members, "Node_for_lower_bound*", "lower_bound");
    append_member(&range.members, "Node_for_upper_bound*", "upper_bound");
    append_member(&range.members, "Node_block*", "body");

    return range;
}

static Node_type node_gen_for_with_cond(void) {
    Node_type for_cond = {.name = node_name_new("node", "for_with_cond", false)};

    append_member(&for_cond.members, "Node_condition*", "condition");
    append_member(&for_cond.members, "Node_block*", "body");

    return for_cond;
}

static Node_type node_gen_break(void) {
    Node_type lang_break = {.name = node_name_new("node", "break", false)};

    append_member(&lang_break.members, "Node*", "child");

    return lang_break;
}

static Node_type node_gen_continue(void) {
    Node_type lang_cont = {.name = node_name_new("node", "continue", false)};

    append_member(&lang_cont.members, "Node*", "child");

    return lang_cont;
}

static Node_type node_gen_assignment(void) {
    Node_type assign = {.name = node_name_new("node", "assignment", false)};

    append_member(&assign.members, "Node*", "lhs");
    append_member(&assign.members, "Node_expr*", "rhs");

    return assign;
}

static Node_type node_gen_if(void) {
    Node_type lang_if = {.name = node_name_new("node", "if", false)};

    append_member(&lang_if.members, "Node_condition*", "condition");
    append_member(&lang_if.members, "Node_block*", "body");

    return lang_if;
}

static Node_type node_gen_return(void) {
    Node_type rtn = {.name = node_name_new("node", "return", false)};

    append_member(&rtn.members, "Node_expr*", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted"); // TODO: use : 1 size?

    return rtn;
}

static Node_type node_gen_goto(void) {
    Node_type lang_goto = {.name = node_name_new("node", "goto", false)};

    append_member(&lang_goto.members, "Str_view", "name");
    append_member(&lang_goto.members, "Llvm_id", "llvm_id");

    return lang_goto;
}

static Node_type node_gen_cond_goto(void) {
    Node_type cond_goto = {.name = node_name_new("node", "cond_goto", false)};

    append_member(&cond_goto.members, "Llvm_register_sym", "node_src");
    append_member(&cond_goto.members, "Str_view", "if_true");
    append_member(&cond_goto.members, "Str_view", "if_false");
    append_member(&cond_goto.members, "Llvm_id", "llvm_id");

    return cond_goto;
}

static Node_type node_gen_alloca(void) {
    Node_type lang_alloca = {.name = node_name_new("node", "alloca", false)};

    append_member(&lang_alloca.members, "Llvm_id", "llvm_id");
    append_member(&lang_alloca.members, "Lang_type", "lang_type");
    append_member(&lang_alloca.members, "Str_view", "name");

    return lang_alloca;
}

static Node_type node_gen_load_another_node(void) {
    Node_type load = {.name = node_name_new("node", "load_another_node", false)};

    append_member(&load.members, "Llvm_register_sym", "node_src");
    append_member(&load.members, "Llvm_id", "llvm_id");
    append_member(&load.members, "Lang_type", "lang_type");

    return load;
}

static Node_type node_gen_store_another_node(void) {
    Node_type store = {.name = node_name_new("node", "store_another_node", false)};

    append_member(&store.members, "Llvm_register_sym", "node_src");
    append_member(&store.members, "Llvm_register_sym", "node_dest");
    append_member(&store.members, "Llvm_id", "llvm_id");
    append_member(&store.members, "Lang_type", "lang_type");

    return store;
}

static Node_type node_gen_if_else_chain(void) {
    Node_type chain = {.name = node_name_new("node", "if_else_chain", false)};

    append_member(&chain.members, "Node_if_ptr_vec", "nodes");

    return chain;
}

static Node_type node_gen_node(void) {
    Node_type node = {.name = node_name_new("node", "", true)};

    vec_append(&gen_a, &node.sub_types, node_gen_block());
    vec_append(&gen_a, &node.sub_types, node_gen_expr());
    vec_append(&gen_a, &node.sub_types, node_gen_load_element_ptr());
    vec_append(&gen_a, &node.sub_types, node_gen_function_params());
    vec_append(&gen_a, &node.sub_types, node_gen_lang_type());
    vec_append(&gen_a, &node.sub_types, node_gen_for_lower_bound());
    vec_append(&gen_a, &node.sub_types, node_gen_for_upper_bound());
    vec_append(&gen_a, &node.sub_types, node_gen_def());
    vec_append(&gen_a, &node.sub_types, node_gen_condition());
    vec_append(&gen_a, &node.sub_types, node_gen_for_range());
    vec_append(&gen_a, &node.sub_types, node_gen_for_with_cond());
    vec_append(&gen_a, &node.sub_types, node_gen_break());
    vec_append(&gen_a, &node.sub_types, node_gen_continue());
    vec_append(&gen_a, &node.sub_types, node_gen_assignment());
    vec_append(&gen_a, &node.sub_types, node_gen_if());
    vec_append(&gen_a, &node.sub_types, node_gen_return());
    vec_append(&gen_a, &node.sub_types, node_gen_goto());
    vec_append(&gen_a, &node.sub_types, node_gen_cond_goto());
    vec_append(&gen_a, &node.sub_types, node_gen_alloca());
    vec_append(&gen_a, &node.sub_types, node_gen_load_another_node());
    vec_append(&gen_a, &node.sub_types, node_gen_store_another_node());
    vec_append(&gen_a, &node.sub_types, node_gen_if_else_chain());

    return node;
}

static void node_gen_node_forward_decl(Node_type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        node_gen_node_forward_decl(vec_at(&node.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void node_gen_node_struct_as(String* output, Node_type node) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_node_name_first_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Node_type curr = vec_at(&node.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_node_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_node_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_node_name_first_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void node_gen_node_struct_enum(String* output, Node_type node) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Node_type curr = vec_at(&node.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_node_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void node_gen_node_struct(Node_type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        node_gen_node_struct(vec_at(&node.sub_types, idx));
    }

    if (node.sub_types.info.count > 0) {
        node_gen_node_struct_as(&output, node);
        node_gen_node_struct_enum(&output, node);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (node.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_node_name_first_upper(&as_member_type, node.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_node_name_upper(&enum_member_type, node.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < node.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&node.members, idx));
    }

    if (node.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = str_view_from_cstr("Pos"), .name = str_view_from_cstr("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void node_gen_unwrap_internal(Node_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        node_gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node_unwrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node) {\n");

    //    try(node->type == upper); 
    string_extend_cstr(&gen_a, &function, "    try(node->type == ");
    extend_node_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &node->as.");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void node_gen_wrap_internal(Node_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        node_gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Node_##lower* node_unwrap_##lower(Node* node) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node_wrap_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node) {\n");

    //    return &node->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) node;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void node_gen_node_unwrap(Node_type node) {
    node_gen_unwrap_internal(node, false);
    node_gen_unwrap_internal(node, true);
}

void node_gen_node_wrap(Node_type node) {
    node_gen_wrap_internal(node, false);
    node_gen_wrap_internal(node, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Node)
static void node_gen_print_forward_decl(Node_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        node_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(node) str_view_print(");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(node))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* node);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void node_gen_new_internal(Node_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        node_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_node_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_node_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(");
    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    } else {
        string_extend_cstr(&gen_a, &function, "Pos pos");
    }
    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        extend_parent_node_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_node = ");
        extend_parent_node_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_node->type = ");
        extend_node_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    node_unwrap_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "(base_node)->pos = pos;\n");
        }

        string_extend_cstr(&gen_a, &function, "    return node_unwrap_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(base_node);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void node_gen_get_pos_internal(Node_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        node_gen_get_pos_internal(vec_at(&type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    node_get_pos(const Node* node)");
    } else {
        string_extend_cstr(&gen_a, &function, "    node_get_pos_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "(const ");
        extend_node_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* node)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return node->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (node->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Node_type curr = vec_at(&type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_node_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return node_get_pos_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "(node_unwrap_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const(node));\n");

                string_extend_cstr(&gen_a, &function, "        break;\n");
            }

            string_extend_cstr(&gen_a, &function, "    }\n");
        }

        string_extend_cstr(&gen_a, &function, "}\n");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_node_new_forward_decl(Node_type node) {
    node_gen_new_internal(node, false);
}

static void gen_node_new_define(Node_type node) {
    node_gen_new_internal(node, true);
}

static void gen_node_get_pos_forward_decl(Node_type node) {
    node_gen_get_pos_internal(node, false);
}

static void gen_node_get_pos_define(Node_type node) {
    node_gen_get_pos_internal(node, true);
}

static void gen_all_nodes(const char* file_path) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("/* autogenerated */\n");
    gen_gen("#include <node_hand_written.h>\n");
    gen_gen("#include <expr_ptr_vec.h>\n");
    gen_gen("#include <token.h>\n");
    gen_gen("#ifndef NODE_H\n");
    gen_gen("#define NODE_H\n");

    Node_type node = node_gen_node();

    node_gen_node_forward_decl(node);
    node_gen_node_struct(node);

    node_gen_node_unwrap(node);
    node_gen_node_wrap(node);

    gen_gen("%s\n", "static inline Node* node_new(void) {");
    gen_gen("%s\n", "    Node* new_node = arena_alloc(&a_main, sizeof(*new_node));");
    gen_gen("%s\n", "    return new_node;");
    gen_gen("%s\n", "}");

    gen_node_new_forward_decl(node);
    node_gen_print_forward_decl(node);
    gen_node_new_define(node);

    gen_node_get_pos_forward_decl(node);
    gen_node_get_pos_define(node);

    gen_gen("#endif // NODE_H\n");

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_NODE_H
