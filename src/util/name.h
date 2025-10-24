#ifndef NAME_H
#define NAME_H

#include <ulang_type_forward_decl.h>
#include <newstring.h>
#include <attrs.h>

typedef enum {
    UNAME_MSG,
    UNAME_LOG,
} UNAME_MODE;

typedef enum {
    NAME_MSG,
    NAME_LOG,
    NAME_EMIT_C,
    NAME_EMIT_IR,
} NAME_MODE;

// TODO: nodes should store Name_id (which would contain size_t) instead of Name?
//   lookup table should store actual name struct associated with Name_id
typedef struct {
    Strv mod_path;
    Strv base;
    Ulang_type_vec gen_args; // TODO: use Ulang_type_view instead of Ulang_type_vec?
    Scope_id scope_id;
    Attrs attrs;
} Name;

// TODO: nodes should store Uname_id (which would contain size_t) instead of Uname?
//   lookup table should store actual name struct associated with Uname_id

// eg. in symbol `io.Optional`, mod_alias == "io" and base == "Optional"
typedef struct {
    Name mod_alias;
    Strv base;
    Ulang_type_vec gen_args; // TODO: use Ulang_type_view instead of Ulang_type_vec?
    Scope_id scope_id;
} Uname;

Name name_new(Strv mod_path, Strv base, Ulang_type_vec gen_args, Scope_id scope_id, Attrs attrs);

Uname uname_new(Name mod_alias, Strv base, Ulang_type_vec gen_args, Scope_id scope_id);

Uname name_to_uname(Name name);

void extend_name_ir(String* buf, Name name);

void serialize_strv(String* buf, Strv strv);

Strv serialize_name_symbol_table(Name name);

Strv serialize_name(Name name);

Strv name_print_internal(NAME_MODE mode, bool serialize, Name name);

Strv uname_print_internal(UNAME_MODE mode, Uname name);

void extend_name_msg(String* buf, Name name);

void extend_uname(UNAME_MODE mode, String* buf, Uname name);

void extend_name(NAME_MODE mode, String* buf, Name name);

void extend_name_log_internal(bool is_msg, String* buf, Name name);

Name name_clone(Name name, bool use_new_scope, Scope_id new_scope);

Uname uname_clone(Uname name, bool use_new_scope, Scope_id scope_id);

#define name_print(mode, name) strv_print(name_print_internal(mode, false, name))

#define uname_print(mode, name) strv_print(uname_print_internal(mode, name))

bool name_is_equal(Name a, Name b);

bool uname_is_equal(Uname a, Uname b);

#endif // NAME_H
