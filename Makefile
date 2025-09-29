.PHONY: all setup build gdb test_quick clean

CC_COMPILER ?= clang

# TODO: remove -Wundefined-internal?
# TODO: consider if we could use -Wconversion instead of -Wfloat-conversion
C_FLAGS_DEBUG=-Wall -Wextra -Wenum-compare -Wfloat-conversion -Wno-undefined-internal -Wbitfield-constant-conversion -Wno-format-zero-length -Wno-unused-function -Werror=incompatible-pointer-types \
			  -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ -I src/token -I src/sema -I src/codegen -I src/lang_type/ -I src/ir -I src/ast_utils/ \
			  -D MIN_LOG_LEVEL=${LOG_LEVEL} \
			  -fsanitize=address -fno-omit-frame-pointer 
C_FLAGS_RELEASE=-Wall -Wextra -Wno-format-zero-length -Wfloat-conversion -Wbitfield-constant-conversion -Wno-unused-function -Werror=incompatible-pointer-types \
			    -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ -I src/token -I src/sema -I src/codegen -I src/lang_type/ -I src/ir -I src/ast_utils/ \
			    -D MIN_LOG_LEVEL=${LOG_LEVEL} \
			    -DNDEBUG \
				-O2

C_FLAGS_AUTO_GEN=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function \
			     -std=c11 -pedantic -g -I ./third_party/ -I src/util/ \
			     -D MIN_LOG_LEVEL=${LOG_LEVEL} \
			     -fsanitize=address -fno-omit-frame-pointer 

BUILD_DIR_DEBUG ?= ./build/debug/
BUILD_DIR_RELEASE ?= ./build/release/

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    C_FLAGS = ${C_FLAGS_DEBUG}
	BUILD_DIR=${BUILD_DIR_DEBUG}
	LOG_LEVEL ?= "LOG_TRACE"
else
    C_FLAGS = ${C_FLAGS_RELEASE}
	BUILD_DIR=${BUILD_DIR_RELEASE}
	LOG_LEVEL ?= "LOG_INFO"
endif

OBJS=\
	 ${BUILD_DIR}/util/name.o \
	 ${BUILD_DIR}/util/str_and_num_utils.o \
	 ${BUILD_DIR}/main.o \
	 ${BUILD_DIR}/arena.o \
	 ${BUILD_DIR}/ast_utils/uast_print.o \
	 ${BUILD_DIR}/ast_utils/tast_print.o \
	 ${BUILD_DIR}/ast_utils/tast_utils.o \
	 ${BUILD_DIR}/ir/ir_print.o \
	 ${BUILD_DIR}/ir/remove_void_assigns.o \
	 ${BUILD_DIR}/lang_type/lang_type_print.o \
	 ${BUILD_DIR}/lang_type/ir_lang_type_print.o \
	 ${BUILD_DIR}/lang_type/ulang_type_print.o \
	 ${BUILD_DIR}/globals.o \
	 ${BUILD_DIR}/ast_utils/uast_utils.o \
	 ${BUILD_DIR}/ast_utils/symbol_table.o \
	 ${BUILD_DIR}/util/file.o \
	 ${BUILD_DIR}/util/parameters.o \
	 ${BUILD_DIR}/util/operator_type.o \
	 ${BUILD_DIR}/util/params_log_level.o \
	 ${BUILD_DIR}/error_msg.o \
	 ${BUILD_DIR}/lang_type/lang_type_serialize.o \
	 ${BUILD_DIR}/lang_type/ulang_type_serialize.o \
	 ${BUILD_DIR}/lang_type/lang_type_from_ulang_type.o \
	 ${BUILD_DIR}/ast_utils/uast_clone.o \
	 ${BUILD_DIR}/ast_utils/ast_msg.o \
	 ${BUILD_DIR}/ast_utils/symbol_collection_clone.o \
	 ${BUILD_DIR}/sema/uast_expr_to_ulang_type.o \
	 ${BUILD_DIR}/sema/type_checking.o \
	 ${BUILD_DIR}/sema/expand_lang_def.o \
	 ${BUILD_DIR}/sema/resolve_generics.o \
	 ${BUILD_DIR}/sema/generic_sub.o \
	 ${BUILD_DIR}/sema/infer_generic_type.o \
	 ${BUILD_DIR}/sema/check_struct_recursion.o \
	 ${BUILD_DIR}/ast_utils/sizeof.o \
	 ${BUILD_DIR}/token/token.o \
	 ${BUILD_DIR}/token/tokenizer.o \
	 ${BUILD_DIR}/parser.o \
	 ${BUILD_DIR}/ir/add_load_and_store.o \
	 ${BUILD_DIR}/codegen/codegen_common.o \
	 ${BUILD_DIR}/codegen/emit_llvm.o \
	 ${BUILD_DIR}/codegen/emit_c.o \
	 ${BUILD_DIR}/ir/ir_utils.o \
	 ${BUILD_DIR}/ir/ir_graphvis.o \
	 ${BUILD_DIR}/util/subprocess.o

DEP_UTIL = Makefile src/util/*.h src/util/auto_gen.c

# TODO: this needs to be done better, because this is error prone
# DEP_COMMON = ${DEP_UTIL} third_party/* src/util/auto_gen* ${BUILD_DIR}/ast_utils/tast.h
DEP_COMMON = ${DEP_UTIL} src/*.h ${BUILD_DIR}/tast.h third_party/*
DEP_COMMON += $(shell find src -type f -name "*.h")

FILE_TO_TEST ?= examples/new_lang/structs.own
ARGS_PROGRAM ?= compile-run ${FILE_TO_TEST}

all: build

run: build
	time ${BUILD_DIR}/main ${ARGS_PROGRAM}

gdb: build
	gdb --args ${BUILD_DIR}/main ${ARGS_PROGRAM}

setup: 
	mkdir -p ${BUILD_DIR}/
	mkdir -p ${BUILD_DIR}/util/
	mkdir -p ${BUILD_DIR}/sema/
	mkdir -p ${BUILD_DIR}/token/
	mkdir -p ${BUILD_DIR}/codegen/
	mkdir -p ${BUILD_DIR}/lang_type/
	mkdir -p ${BUILD_DIR}/ir/
	mkdir -p ${BUILD_DIR}/ast_utils/

# TODO: always run setup before ${BUILD_DIR}/main
build: setup ${BUILD_DIR}/main

# TODO: figure out better way to switch between c and llvm
COMPILER_OUTPUT=test.c

test_quick: run
	./a.out ; echo $$?

# auto_gen and util
${BUILD_DIR}/auto_gen: src/util/auto_gen.c ${DEP_UTIL}
	${CC_COMPILER} ${C_FLAGS_AUTO_GEN} -o ${BUILD_DIR}/auto_gen src/util/params_log_level.c src/util/arena.c src/util/auto_gen.c

${BUILD_DIR}/tast.h: ${BUILD_DIR}/auto_gen
	./${BUILD_DIR}/auto_gen ${BUILD_DIR}

# general
${BUILD_DIR}/main: ${DEP_COMMON} ${OBJS}
	${CC_COMPILER} ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: ${DEP_COMMON} src/main.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/arena.o: ${DEP_COMMON} src/util/arena.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/arena.o src/util/arena.c

${BUILD_DIR}/sema/uast_expr_to_ulang_type.o: ${DEP_COMMON} src/sema/uast_expr_to_ulang_type.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/uast_expr_to_ulang_type.o src/sema/uast_expr_to_ulang_type.c

${BUILD_DIR}/globals.o: ${DEP_COMMON} src/globals.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/globals.o src/globals.c

${BUILD_DIR}/ast_utils/uast_print.o: ${DEP_COMMON} src/ast_utils/uast_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/uast_print.o src/ast_utils/uast_print.c

${BUILD_DIR}/ast_utils/uast_utils.o: ${DEP_COMMON} src/ast_utils/uast_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/uast_utils.o src/ast_utils/uast_utils.c

${BUILD_DIR}/ast_utils/tast_print.o: ${DEP_COMMON} src/ast_utils/tast_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/tast_print.o src/ast_utils/tast_print.c

${BUILD_DIR}/ir/ir_print.o: ${DEP_COMMON} src/ir/ir_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ir/ir_print.o src/ir/ir_print.c

${BUILD_DIR}/ir/remove_void_assigns.o: ${DEP_COMMON} src/ir/remove_void_assigns.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ir/remove_void_assigns.o src/ir/remove_void_assigns.c

${BUILD_DIR}/lang_type/lang_type_print.o: ${DEP_COMMON} src/lang_type/lang_type_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/lang_type_print.o src/lang_type/lang_type_print.c

${BUILD_DIR}/lang_type/ir_lang_type_print.o: ${DEP_COMMON} src/lang_type/ir_lang_type_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/ir_lang_type_print.o src/lang_type/ir_lang_type_print.c

${BUILD_DIR}/lang_type/ulang_type_print.o: ${DEP_COMMON} src/lang_type/ulang_type_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/ulang_type_print.o src/lang_type/ulang_type_print.c

${BUILD_DIR}/sema/type_checking.o: ${DEP_COMMON} src/sema/type_checking.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/type_checking.o src/sema/type_checking.c

${BUILD_DIR}/sema/resolve_generics.o: ${DEP_COMMON} src/sema/resolve_generics.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/resolve_generics.o src/sema/resolve_generics.c

${BUILD_DIR}/sema/generic_sub.o: ${DEP_COMMON} src/sema/generic_sub.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/generic_sub.o src/sema/generic_sub.c

${BUILD_DIR}/sema/infer_generic_type.o: ${DEP_COMMON} src/sema/infer_generic_type.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/infer_generic_type.o src/sema/infer_generic_type.c

${BUILD_DIR}/sema/check_struct_recursion.o: ${DEP_COMMON} src/sema/check_struct_recursion.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/check_struct_recursion.o src/sema/check_struct_recursion.c

${BUILD_DIR}/sema/expand_lang_def.o: ${DEP_COMMON} src/sema/expand_lang_def.c
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/expand_lang_def.o src/sema/expand_lang_def.c

${BUILD_DIR}/util/file.o: ${DEP_COMMON} src/util/file.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/file.o src/util/file.c

${BUILD_DIR}/util/parameters.o: ${DEP_COMMON} src/util/parameters.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/parameters.o src/util/parameters.c

${BUILD_DIR}/error_msg.o: ${DEP_COMMON} src/error_msg.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/error_msg.o src/error_msg.c

${BUILD_DIR}/ast_utils/sizeof.o: ${DEP_COMMON} src/ast_utils/sizeof.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/sizeof.o src/ast_utils/sizeof.c

${BUILD_DIR}/ast_utils/symbol_table.o: ${DEP_COMMON} src/ast_utils/symbol_table.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/symbol_table.o src/ast_utils/symbol_table.c

${BUILD_DIR}/lang_type/lang_type_serialize.o: ${DEP_COMMON} src/lang_type/lang_type_serialize.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/lang_type_serialize.o src/lang_type/lang_type_serialize.c

${BUILD_DIR}/lang_type/ulang_type_serialize.o: ${DEP_COMMON} src/lang_type/ulang_type_serialize.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/ulang_type_serialize.o src/lang_type/ulang_type_serialize.c

${BUILD_DIR}/lang_type/lang_type_from_ulang_type.o: ${DEP_COMMON} src/lang_type/lang_type_from_ulang_type.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type/lang_type_from_ulang_type.o src/lang_type/lang_type_from_ulang_type.c

${BUILD_DIR}/ast_utils/tast_utils.o: ${DEP_COMMON} src/ast_utils/tast_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/tast_utils.o src/ast_utils/tast_utils.c

${BUILD_DIR}/ast_utils/symbol_collection_clone.o: ${DEP_COMMON} src/ast_utils/symbol_collection_clone.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/symbol_collection_clone.o src/ast_utils/symbol_collection_clone.c

${BUILD_DIR}/ast_utils/ast_msg.o: ${DEP_COMMON} src/ast_utils/ast_msg.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/ast_msg.o src/ast_utils/ast_msg.c

${BUILD_DIR}/ast_utils/uast_clone.o: ${DEP_COMMON} src/ast_utils/uast_clone.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ast_utils/uast_clone.o src/ast_utils/uast_clone.c

${BUILD_DIR}/ir/ir_utils.o: ${DEP_COMMON} src/ir/ir_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ir/ir_utils.o src/ir/ir_utils.c

${BUILD_DIR}/ir/ir_graphvis.o: ${DEP_COMMON} src/ir/ir_graphvis.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ir/ir_graphvis.o src/ir/ir_graphvis.c

${BUILD_DIR}/util/subprocess.o: ${DEP_COMMON} src/util/subprocess.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/subprocess.o src/util/subprocess.c

${BUILD_DIR}/ir/add_load_and_store.o: ${DEP_COMMON} src/ir/add_load_and_store.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ir/add_load_and_store.o src/ir/add_load_and_store.c

${BUILD_DIR}/codegen/codegen_common.o: ${DEP_COMMON} src/codegen/codegen_common.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/codegen/codegen_common.o src/codegen/codegen_common.c

${BUILD_DIR}/codegen/emit_llvm.o: ${DEP_COMMON} src/codegen/emit_llvm.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/codegen/emit_llvm.o src/codegen/emit_llvm.c

${BUILD_DIR}/codegen/emit_c.o: ${DEP_COMMON} src/codegen/emit_c.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/codegen/emit_c.o src/codegen/emit_c.c

${BUILD_DIR}/parser.o: ${DEP_COMMON} src/parser.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/parser.o src/parser.c

${BUILD_DIR}/token/tokenizer.o: ${DEP_COMMON} src/token/tokenizer.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/token/tokenizer.o src/token/tokenizer.c

${BUILD_DIR}/token/token.o: ${DEP_COMMON} src/token/token.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/token/token.o src/token/token.c

${BUILD_DIR}/util/operator_type.o: ${DEP_COMMON} src/util/operator_type.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/operator_type.o src/util/operator_type.c

${BUILD_DIR}/util/name.o: ${DEP_COMMON} src/util/name.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/name.o src/util/name.c

${BUILD_DIR}/util/str_and_num_utils.o: ${DEP_COMMON} src/util/str_and_num_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/str_and_num_utils.o src/util/str_and_num_utils.c

${BUILD_DIR}/util/params_log_level.o: ${DEP_COMMON} src/util/params_log_level.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/params_log_level.o src/util/params_log_level.c

