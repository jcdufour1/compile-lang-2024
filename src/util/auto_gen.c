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

// eg. in Node_symbol_typed
//  prefix = "Node"
//  base = "symbol_typed"
//  sub_types = [{.prefix = "Node_symbol_typed", .base = "primitive", .sub_types = []}, ...]
typedef struct Type_ {
    Str_view name;
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

static void extend_node_name_upper(String* output, Str_view name) {
    if (name.count < 1) {
        string_extend_cstr(&gen_a, output, "NODE");
    } else {
        string_extend_cstr(&gen_a, output, "NODE_");
        extend_strv_upper(output, name);
    }
}

static void extend_node_name_lower(String* output, Str_view name) {
    if (name.count < 1) {
        string_extend_cstr(&gen_a, output, "node");
    } else {
        string_extend_cstr(&gen_a, output, "node_");
        extend_strv_lower(output, name);
    }
}

static void extend_node_name_first_upper(String* output, Str_view name) {
    if (name.count < 1) {
        string_extend_cstr(&gen_a, output, "Node");
    } else {
        string_extend_cstr(&gen_a, output, "Node_");
        extend_strv_lower(output, name);
    }
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = str_view_from_cstr(type),
        .name = str_view_from_cstr(name)
    };
    vec_append(&gen_a, members, member);
}

static Type gen_block(void) {
    Type block = {.name = str_view_from_cstr("block")};

    append_member(&block.members, "bool", "is_variadic");
    append_member(&block.members, "Node_ptr_vec", "children");
    append_member(&block.members, "Symbol_table", "symbol_table");
    append_member(&block.members, "Pos", "pos_end");

    return block;
}

static Type gen_node(void) {
    Type node = {.name = str_view_from_cstr("")};

    vec_append(&gen_a, &node.sub_types, gen_block());
    
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

    gen_gen("#endif // NODE_H");
}

