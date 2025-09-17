#ifndef NAME_H
#define NAME_H

#include <ulang_type_forward_decl.h>
#include <newstring.h>

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

typedef struct {
    Strv mod_path;
    Strv base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Name;

// eg. in symbol `io.Optional`, mod_alias == "io" and base == "Optional"
typedef struct {
    Name mod_alias;
    Strv base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Uname;

Name name_new(Strv mod_path, Strv base, Ulang_type_vec gen_args, Scope_id scope_id);

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

Name name_clone(Name name, Scope_id scope_id);

Uname uname_clone(Uname name, Scope_id scope_id);

#define name_print(mode, name) strv_print(name_print_internal(mode, false, name))

#define uname_print(mode, name) strv_print(uname_print_internal(mode, name))

#endif // NAME_H
