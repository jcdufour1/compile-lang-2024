#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <newstring.h>
#include <arena.h>
#include <log_internal.h>
#include <string_vec.h>
#include <util.h>

// TODO: actually use newline to end statement depending on last token of line of line
// TODO: expected failure case for invalid type in extern "c" function
// TODO: char literal with escape
// TODO: lang_type subtypes for number, etc.
//  lang_type_number should also have subtypes for signed, unsigned, etc.
//
//

FILE* global_output = NULL;
Arena gen_a = {0};

typedef struct {
    Str_view name;
    Str_view type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

struct Type_;
typedef struct Type_ Type;

typedef struct {
    Vec_base info;
    Type* buf;
} Type_vec;

typedef struct {
    Str_view parent;
    Str_view base;
} Node_name;

// eg. in Node_symbol_typed
//  prefix = "Node"
//  base = "symbol_typed"
//  sub_types = [{.prefix = "Node_symbol_typed", .base = "primitive", .sub_types = []}, ...]
typedef struct Type_ {
    Node_name name;
    Members members;
    Type_vec sub_types;
} Type;

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...)
__attribute__((format (printf, 4, 5)));

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...) {
    assert(output);

    va_list args;
    va_start(args, format);

    vfprintf(output, format, args);
    fprintf(output, "    /* %s:%d: */\n", file, line);

    va_end(args);
}

#define gen_gen(...) \
    gen_gen_internal(global_output, __FILE__, __LINE__, __VA_ARGS__)

static void extend_strv_upper(String* output, Str_view name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, toupper(str_view_at(name, idx)));
    }
}

static void extend_strv_lower(String* output, Str_view name) {
    for (size_t idx = 0; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(str_view_at(name, idx)));
    }
}

static void extend_strv_first_upper(String* output, Str_view name) {
    if (name.count < 1) {
        return;
    }
    vec_append(&gen_a, output, toupper(str_view_front(name)));
    for (size_t idx = 1; idx < name.count; idx++) {
        vec_append(&gen_a, output, tolower(str_view_at(name, idx)));
    }
}

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
    todo();
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

static Node_name node_name_new(const char* parent, const char* base) {
    return (Node_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base)};
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Type gen_block(void) {
    Type block = {.name = node_name_new("node", "block")};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Node_ptr_vec", "children");
    append_member(&block.members, "Symbol_table", "symbol_table");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Type gen_unary(void) {
    Type unary = {.name = node_name_new("operator", "unary")};

    append_member(&unary.members, "Node_expr*", "child");
    append_member(&unary.members, "TOKEN_TYPE", "token_type");
    append_member(&unary.members, "Lang_type", "lang_type");
    append_member(&unary.members, "Llvm_id", "llvm_id");

    return unary;
}

static Type gen_binary(void) {
    Type binary = {.name = node_name_new("operator", "binary")};

    append_member(&binary.members, "Node_expr*", "lhs");
    append_member(&binary.members, "Node_expr*", "rhs");
    append_member(&binary.members, "TOKEN_TYPE", "token_type");
    append_member(&binary.members, "Lang_type", "lang_type");
    append_member(&binary.members, "Llvm_id", "llvm_id");

    return binary;
}

static Type gen_primitive_sym(void) {
    Type primitive = {.name = node_name_new("symbol_typed", "primitive_sym")};

    append_member(&primitive.members, "Sym_typed_base", "base");

    return primitive;
}

static Type gen_enum_sym(void) {
    Type lang_enum = {.name = node_name_new("symbol_typed", "enum_sym")};

    append_member(&lang_enum.members, "Sym_typed_base", "base");

    return lang_enum;
}

static Type gen_struct_sym(void) {
    Type lang_struct = {.name = node_name_new("symbol_typed", "struct_sym")};

    append_member(&lang_struct.members, "Sym_typed_base", "base");

    return lang_struct;
}

static Type gen_raw_union_sym(void) {
    Type raw_union = {.name = node_name_new("symbol_typed", "raw_union_sym")};

    append_member(&raw_union.members, "Sym_typed_base", "base");

    return raw_union;
}

static Type gen_operator(void) {
    Type operator = {.name = node_name_new("expr", "operator")};

    vec_append(&gen_a, &operator.sub_types, gen_unary());
    vec_append(&gen_a, &operator.sub_types, gen_binary());

    return operator;
}

static Type gen_symbol_typed(void) {
    Type sym = {.name = node_name_new("expr", "symbol_typed")};

    vec_append(&gen_a, &sym.sub_types, gen_primitive_sym());
    vec_append(&gen_a, &sym.sub_types, gen_struct_sym());
    vec_append(&gen_a, &sym.sub_types, gen_raw_union_sym());
    vec_append(&gen_a, &sym.sub_types, gen_enum_sym());

    return sym;
}

static Type gen_expr(void) {
    Type expr = {.name = node_name_new("node", "expr")};

    vec_append(&gen_a, &expr.sub_types, gen_operator());
    vec_append(&gen_a, &expr.sub_types, gen_symbol_typed());

    return expr;
}

static Type gen_node(void) {
    Type node = {.name = node_name_new("node", "")};

    vec_append(&gen_a, &node.sub_types, gen_block());
    vec_append(&gen_a, &node.sub_types, gen_expr());
    
    append_member(&node.members, "Pos", "pos");

    return node;
}

static void gen_node_forward_decl(Type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        gen_node_forward_decl(vec_at(&node.sub_types, idx));
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

static void extend_struct_member(String* output, Member member) {
    string_extend_cstr(&gen_a, output, "    ");
    string_extend_strv(&gen_a, output, member.type);
    string_extend_cstr(&gen_a, output, " ");
    string_extend_strv(&gen_a, output, member.name);
    string_extend_cstr(&gen_a, output, ";\n");
}

static void gen_node_struct_as(String* output, Type node) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_node_name_first_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Type curr = vec_at(&node.sub_types, idx);
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

static void gen_node_struct_enum(String* output, Type node) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        Type curr = vec_at(&node.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_node_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_node_name_upper(output, node.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void gen_node_struct(Type node) {
    String output = {0};

    for (size_t idx = 0; idx < node.sub_types.info.count; idx++) {
        gen_node_struct(vec_at(&node.sub_types, idx));
    }

    if (node.sub_types.info.count > 0) {
        gen_node_struct_as(&output, node);
        gen_node_struct_enum(&output, node);
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

    string_extend_cstr(&gen_a, &output, "}");
    extend_node_name_first_upper(&output, node.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void gen_unwrap_internal(Type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_unwrap_internal(vec_at(&type.sub_types, idx), is_const);
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

static void gen_wrap_internal(Type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_wrap_internal(vec_at(&type.sub_types, idx), is_const);
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

void gen_node_unwrap(Type node) {
    gen_unwrap_internal(node, false);
    gen_unwrap_internal(node, true);
}

void gen_node_wrap(Type node) {
    gen_wrap_internal(node, false);
    gen_wrap_internal(node, true);
}

int main(int argc, char** argv) {
    assert(argc == 2 && "output file path should be provided");

    global_output = fopen(argv[1], "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    gen_gen("/* autogenerated */");
    gen_gen("#include <node_hand_written.h>");
    gen_gen("#include <expr_ptr_vec.h>");
    gen_gen("#include <token.h>");
    gen_gen("#ifndef NODE_H");
    gen_gen("#define NODE_H");

    Type node = gen_node();

    gen_node_forward_decl(node);
    gen_node_struct(node);

    gen_node_unwrap(node);
    gen_node_wrap(node);

    gen_gen("#endif // NODE_H");
}

