#ifndef LLVM_HAND_WRITTEN_H
#define LLVM_HAND_WRITTEN_H

#include <lang_type_struct.h>
#include <vector.h>

struct Llvm_;
typedef struct Llvm_ Llvm;

struct Llvm_variable_def_;
typedef struct Llvm_variable_def_ Llvm_variable_def;

struct Llvm_if_;
typedef struct Llvm_if_ Llvm_if;

struct Llvm_expr_;
typedef struct Llvm_expr_ Llvm_expr;

//typedef struct {
//    Lang_type lang_type;
//    Llvm* llvm;
//} Llvm_register_sym;
//
typedef struct {
    Vec_base info;
    Llvm** buf;
} Llvm_ptr_vec;

typedef struct {
    Vec_base info;
    Llvm_variable_def** buf;
} Llvm_var_def_vec;

typedef struct {
    Vec_base info;
    Llvm_if** buf;
} Llvm_if_ptr_vec;

typedef struct {
    Vec_base info;
    Llvm_expr** buf;
} Llvm_expr_ptr_vec;

#endif // LLVM_HAND_WRITTEN_H
