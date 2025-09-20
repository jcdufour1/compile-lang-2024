#ifndef LLVM_LANG_TYPE_PRINT_H
#define LLVM_LANG_TYPE_PRINT_H

#include <env.h>
#include <newstring.h>
#include <strv.h>
#include <lang_type_mode.h>

#define llvm_lang_type_print(mode, llvm_lang_type) strv_print(llvm_lang_type_print_internal((mode), (llvm_lang_type)))

#define llvm_lang_type_atom_print(mode, atom) strv_print(llvm_lang_type_atom_print_internal(atom, mode))

Strv llvm_lang_type_print_internal(LANG_TYPE_MODE mode, Llvm_lang_type llvm_lang_type);

Strv llvm_lang_type_atom_print_internal(Llvm_lang_type_atom atom, LANG_TYPE_MODE mode);

void extend_serialize_llvm_lang_type_to_string(String* string, Llvm_lang_type llvm_lang_type, bool do_tag);

void extend_llvm_lang_type_atom(String* string, LANG_TYPE_MODE mode, Llvm_lang_type_atom atom);

void extend_llvm_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Llvm_lang_type llvm_lang_type);

#endif // LLVM_LANG_TYPE_PRINT_H
