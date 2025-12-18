#ifndef AUTO_GEN_UTIL_H
#define AUTO_GEN_UTIL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <local_string.h>
#include <arena.h>
#include <log_internal.h>
#include <string_vec.h>
#include <util.h>

FILE* global_output = NULL;
Arena a_gen = {0};

#define close_file(file) \
    do { \
        if (file) { \
            fprintf(file, "\n"); \
            fclose(file); \
        } \
        (file) = NULL; \
    } while(0)

typedef struct {
    Strv type_name;
    Strv normal_prefix;
    Strv internal_prefix;
    Strv key_fn_get_name;
    Strv symbol_table_name;
    Strv ancesters_type;
    Strv print_fn;
    bool do_primitives;
    bool env_thing;
} Symbol_tbl_type;

typedef struct {
    Vec_base info;
    Symbol_tbl_type* buf;
} Sym_tbl_type_vec;

typedef struct {
    Strv name;
    Strv type;
} Member;

typedef struct {
    Vec_base info;
    Member* buf;
} Members;

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...)
__attribute__((format (printf, 4, 5)));

static void gen_gen_internal(FILE* output, const char* file, int line, const char* format, ...) {
    (void) file;
    (void) line;
    unwrap(output);

    va_list args;
    va_start(args, format);

    fprintf(output, "\n/* %s:%d: */\n", file, line);
    vfprintf(output, format, args);

    va_end(args);
}

#define gen_gen(...) \
    gen_gen_internal(global_output, __FILE__, __LINE__, __VA_ARGS__)

static void extend_strv_first_upper(String* output, Strv name) {
    if (name.count < 1) {
        return;
    }
    vec_append(&a_gen, output, toupper(strv_first(name)));
    for (size_t idx = 1; idx < name.count; idx++) {
        vec_append(&a_gen, output, tolower(strv_at(name, idx)));
    }
}

static void extend_struct_member(String* output, Member member) {
    string_extend_cstr(&a_gen, output, "    ");
    string_extend_strv(&a_gen, output, member.type);
    string_extend_cstr(&a_gen, output, " ");
    string_extend_strv(&a_gen, output, member.name);
    string_extend_cstr(&a_gen, output, ";\n");
}

static void append_member(Members* members, const char* type, const char* name) {
    Member member = {
        .type = sv(type),
        .name = sv(name)
    };
    vec_append(&a_gen, members, member);
}

static Strv loc_print_internal(const char* file, int line) {
    String buf = {0};
    string_extend_f(&a_gen, &buf, "/* %s:%d */", file, line);
    return string_to_strv(buf);
}

#define loc_print() strv_print(loc_print_internal(file, line))


//
// Uast_type
//

struct Uast_type_;
typedef struct Uast_type_ Uast_type;

typedef struct {
    Vec_base info;
    Uast_type* buf;
} Uast_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent; // TODO: remove?
    Strv base;
    Strv type; // eg. uast, tast, ir
} Uast_name;

typedef struct Uast_type_ {
    Uast_name name;
    Members members;
    Uast_type_vec sub_types;
} Uast_type;

static void uast_get_type_vec_internal(Uast_type_vec* type_vec, Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_get_type_vec_internal(type_vec, vec_at(uast.sub_types, idx));
    }

    vec_append(&a_gen, type_vec, uast);
}

static Uast_type_vec uast_get_type_vec(Uast_type uast) {
    Uast_type_vec type_vec = {0};
    uast_get_type_vec_internal(&type_vec, uast);
    return type_vec;
}

static void extend_uast_name_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    string_extend_upper(&a_gen, output, name.type);
    if (name.base.count > 0) {
        string_extend_cstr(&a_gen, output, "_");
        string_extend_upper(&a_gen, output, name.base);
    }
}

static Strv uast_upper_print_internal(Uast_name name) {
    String buf = {0};
    extend_uast_name_upper(&buf, name);
    return string_to_strv(buf);
}

#define uast_upper_print(name) \
    strv_print(uast_upper_print_internal(name))

static void extend_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    string_extend_lower(&a_gen, output, name.type);
    if (name.base.count > 0) {
        string_extend_cstr(&a_gen, output, "_");
        string_extend_lower(&a_gen, output, name.base);
    }
}

static Strv uast_lower_print_internal(Uast_name name) {
    String buf = {0};
    extend_uast_name_lower(&buf, name);
    return string_to_strv(buf);
}

#define uast_lower_print(name) \
    strv_print(uast_lower_print_internal(name))

static void extend_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    extend_strv_first_upper(output, name.type);
    if (name.base.count > 0) {
        string_extend_cstr(&a_gen, output, "_");
        string_extend_lower(&a_gen, output, name.base);
    }
}

static Strv uast_first_upper_print_internal(Uast_name name) {
    String buf = {0};
    extend_uast_name_first_upper(&buf, name);
    return string_to_strv(buf);
}

#define uast_first_upper_print(name) \
    strv_print(uast_first_upper_print_internal(name))

static void extend_parent_uast_name_upper(String* output, Uast_name name) {
    todo();
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        unreachable("");
    } else {
        string_extend_cstr(&a_gen, output, "UAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&a_gen, output, "_");
        string_extend_upper(&a_gen, output, name.base);
    }
}

static void extend_parent_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, name.type)) {
        string_extend_lower(&a_gen, output, name.type);
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_lower(&a_gen, output, name.type);
    string_extend_cstr(&a_gen, output, "_");
    string_extend_lower(&a_gen, output, name.parent);
}

static void extend_parent_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, name.type)) {
        extend_strv_first_upper(output, name.type);
        return;
    }

    unwrap(name.base.count > 0);

    extend_strv_first_upper(output, name.type);
    string_extend_cstr(&a_gen, output, "_");
    string_extend_lower(&a_gen, output, name.parent);
}

static void uast_gen_uast_forward_decl(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_forward_decl(vec_at(uast.sub_types, idx));
    }

    string_extend_cstr(&a_gen, &output, "struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&a_gen, &output, "_;\n");

    string_extend_cstr(&a_gen, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&a_gen, &output, "_ ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&a_gen, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static Uast_name uast_name_new(const char* parent, const char* base, bool is_topmost, const char* type /* eg. uast, tast*/) {
    // TODO: assert that cstrs passed here are all lowercase?
    return (Uast_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost, .type = sv(type)};
}

static void uast_gen_uast_struct_as(String* output, Uast_type uast) {
    string_extend_cstr(&a_gen, output, "typedef union ");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&a_gen, output, "_as");
    string_extend_cstr(&a_gen, output, "_ ");
    string_extend_cstr(&a_gen, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&a_gen, output, "    ");
        extend_uast_name_first_upper(output, curr.name);
        string_extend_cstr(&a_gen, output, " ");
        extend_uast_name_lower(output, curr.name);
        string_extend_cstr(&a_gen, output, ";\n");
    }

    string_extend_cstr(&a_gen, output, "}");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&a_gen, output, "_as");
    string_extend_cstr(&a_gen, output, ";\n");

}

static void uast_gen_uast_struct_enum(String* output, Uast_type uast) {
    string_extend_cstr(&a_gen, output, "typedef enum ");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&a_gen, output, "_TYPE");
    string_extend_cstr(&a_gen, output, "_ ");
    string_extend_cstr(&a_gen, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&a_gen, output, "    ");
        extend_uast_name_upper(output, curr.name);
        string_extend_cstr(&a_gen, output, ",\n");
    }

    string_extend_cstr(&a_gen, output, "}");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&a_gen, output, "_TYPE");
    string_extend_cstr(&a_gen, output, ";\n");

}

static void uast_gen_uast_struct(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_struct(vec_at(uast.sub_types, idx));
    }

    if (uast.sub_types.info.count > 0) {
        uast_gen_uast_struct_as(&output, uast);
        uast_gen_uast_struct_enum(&output, uast);
    }

    string_extend_cstr(&a_gen, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&a_gen, &output, "_ ");
    string_extend_cstr(&a_gen, &output, "{\n");

    if (uast.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_uast_name_first_upper(&as_member_type, uast.name);
        string_extend_cstr(&a_gen, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&a_gen, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_uast_name_upper(&enum_member_type, uast.name);
        string_extend_cstr(&a_gen, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&a_gen, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < uast.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(uast.members, idx));
    }

    string_extend_f(&a_gen, &output, "#  ifndef NDEBUG\n");
    extend_struct_member(&output, (Member) {.name = sv("loc"), .type = sv("Loc")});
    string_extend_f(&a_gen, &output, "#  endif // NDEBUG\n");

    if (uast.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&a_gen, &output, "}");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&a_gen, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

#endif // AUTO_GEN_UTIL_H
