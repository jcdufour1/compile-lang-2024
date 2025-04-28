#ifndef NAME_H
#define NAME_H

#include <ulang_type_forward_decl.h>
#include <newstring.h>

typedef struct {
    Str_view mod_path;
    Str_view base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Name;

typedef struct {
    Name mod_alias;
    Str_view base;
    Ulang_type_vec gen_args;
    Scope_id scope_id;
} Uname;

Name name_new(Str_view mod_path, Str_view base, Ulang_type_vec gen_args, Scope_id scope_id);

void extend_name(bool is_llvm, String* buf, Name name);

void extend_name_llvm(String* buf, Name name);

void serialize_str_view(String* buf, Str_view str_view);

Str_view serialize_name_symbol_table(Name name);

Str_view serialize_name(Name name);

Str_view name_print_internal(bool serialize, Name name);

Str_view uname_print_internal(Uname name);

void extend_name_msg(String* buf, Name name);

void extend_uname_msg(String* buf, Uname name);

void extend_uname(String* buf, Uname name);

void extend_name(bool is_llvm, String* buf, Name name);

Name name_clone(Name name, Scope_id scope_id);

#define name_print(name) str_view_print(name_print_internal(false, name))

#define uname_print(name) str_view_print(uname_print_internal(name))

#endif // NAME_H
