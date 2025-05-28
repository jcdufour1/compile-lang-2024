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
    NAME_EMIT_LLVM,
} NAME_MODE;

typedef struct {
    Str_view mod_path;
    Str_view base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Name;

typedef struct {
    Name mod_alias; // TODO: do not use Name for mod_alias; come up with a better system
    Str_view base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Uname;

Name name_new(Str_view mod_path, Str_view base, Ulang_type_vec gen_args, Scope_id scope_id);

Uname uname_new(Name mod_alias, Str_view base, Ulang_type_vec gen_args, Scope_id scope_id);

void extend_name_llvm(String* buf, Name name);

void serialize_str_view(String* buf, Str_view str_view);

Str_view serialize_name_symbol_table(Name name);

Str_view serialize_name(Name name);

Str_view name_print_internal(NAME_MODE mode, bool serialize, Name name);

Str_view uname_print_internal(UNAME_MODE mode, Uname name);

void extend_name_msg(String* buf, Name name);

void extend_uname(UNAME_MODE mode, String* buf, Uname name);

void extend_name(NAME_MODE mode, String* buf, Name name);

void extend_name_log_internal(bool is_msg, String* buf, Name name);

Name name_clone(Name name, Scope_id scope_id);

Uname uname_clone(Uname name, Scope_id scope_id);

#define name_print(mode, name) str_view_print(name_print_internal(mode, false, name))

#define uname_print(mode, name) str_view_print(uname_print_internal(mode, name))

#endif // NAME_H
