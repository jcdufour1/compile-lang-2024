.PHONY: all setup build gdb test_quick clean

CC_COMPILER ?= clang

C_FLAGS_DEBUG=-Wall -Wextra -Wenum-compare -Wno-format-zero-length -Wno-unused-function -Werror=incompatible-pointer-types \
			  -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ -I src/token -I src/sema \
			  -D CURR_LOG_LEVEL=${LOG_LEVEL} \
			  -fsanitize=address -fno-omit-frame-pointer 
C_FLAGS_RELEASE=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function -Werror=incompatible-pointer-types \
			    -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ -I src/token -I src/sema \
			    -D CURR_LOG_LEVEL=${LOG_LEVEL} \
			    -DNDEBUG \
				-O2

C_FLAGS_AUTO_GEN=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function \
			     -std=c11 -pedantic -g -I ./third_party/ -I src/util/ \
			     -D CURR_LOG_LEVEL=${LOG_LEVEL} \
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
	LOG_LEVEL ?= "LOG_VERBOSE"
endif

OBJS=\
	 ${BUILD_DIR}/util/name.o \
	 ${BUILD_DIR}/main.o \
	 ${BUILD_DIR}/arena.o \
	 ${BUILD_DIR}/uast_print.o \
	 ${BUILD_DIR}/tast_print.o \
	 ${BUILD_DIR}/llvm_print.o \
	 ${BUILD_DIR}/lang_type_print.o \
	 ${BUILD_DIR}/ulang_type_print.o \
	 ${BUILD_DIR}/globals.o \
	 ${BUILD_DIR}/uast_utils.o \
	 ${BUILD_DIR}/symbol_table.o \
	 ${BUILD_DIR}/file.o \
	 ${BUILD_DIR}/util/parameters.o \
	 ${BUILD_DIR}/parser_utils.o \
	 ${BUILD_DIR}/error_msg.o \
	 ${BUILD_DIR}/lang_type_serialize.o \
	 ${BUILD_DIR}/ulang_type_serialize.o \
	 ${BUILD_DIR}/lang_type_from_ulang_type.o \
	 ${BUILD_DIR}/uast_clone.o \
	 ${BUILD_DIR}/symbol_collection_clone.o \
	 ${BUILD_DIR}/sema/type_checking.o \
	 ${BUILD_DIR}/sema/expand_lang_def.o \
	 ${BUILD_DIR}/sema/resolve_generics.o \
	 ${BUILD_DIR}/sema/generic_sub.o \
	 ${BUILD_DIR}/sizeof.o \
	 ${BUILD_DIR}/token/token.o \
	 ${BUILD_DIR}/token/tokenizer.o \
	 ${BUILD_DIR}/parser.o \
	 ${BUILD_DIR}/add_load_and_store.o \
	 ${BUILD_DIR}/emit_llvm.o \
	 ${BUILD_DIR}/llvm_utils.o

DEP_UTIL = Makefile src/util/*.h src/util/auto_gen.c

# TODO: this needs to be done better, because this is error prone
# DEP_COMMON = ${DEP_UTIL} third_party/* src/util/auto_gen* ${BUILD_DIR}/tast.h
DEP_COMMON = ${DEP_UTIL} src/*.h ${BUILD_DIR}/tast.h third_party/*
DEP_COMMON += $(shell find src -type f -name "*.h")

FILE_TO_TEST ?= examples/new_lang/structs.own
ARGS_PROGRAM ?= compile ${FILE_TO_TEST} --emit-llvm

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

build: setup ${BUILD_DIR}/main

test_quick: run
	${CC_COMPILER} test.ll -o a.out && ./a.out ; echo $$?

# auto_gen and util
${BUILD_DIR}/auto_gen: src/util/auto_gen.c ${DEP_UTIL}
	${CC_COMPILER} ${C_FLAGS_AUTO_GEN} -o ${BUILD_DIR}/auto_gen src/util/arena.c src/util/auto_gen.c

${BUILD_DIR}/tast.h: ${BUILD_DIR}/auto_gen
	./${BUILD_DIR}/auto_gen ${BUILD_DIR}

# general
${BUILD_DIR}/main: ${DEP_COMMON} ${OBJS}
	${CC_COMPILER} ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: ${DEP_COMMON} src/main.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/arena.o: ${DEP_COMMON} src/util/arena.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/arena.o src/util/arena.c

${BUILD_DIR}/parser_utils.o: ${DEP_COMMON} src/parser_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/parser_utils.o src/parser_utils.c

${BUILD_DIR}/globals.o: ${DEP_COMMON} src/globals.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/globals.o src/globals.c

${BUILD_DIR}/uast_print.o: ${DEP_COMMON} src/uast_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/uast_print.o src/uast_print.c

${BUILD_DIR}/tast_print.o: ${DEP_COMMON} src/tast_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/tast_print.o src/tast_print.c

${BUILD_DIR}/llvm_print.o: ${DEP_COMMON} src/llvm_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/llvm_print.o src/llvm_print.c

${BUILD_DIR}/lang_type_print.o: ${DEP_COMMON} src/lang_type_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type_print.o src/lang_type_print.c

${BUILD_DIR}/ulang_type_print.o: ${DEP_COMMON} src/ulang_type_print.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ulang_type_print.o src/ulang_type_print.c

${BUILD_DIR}/sema/type_checking.o: ${DEP_COMMON} src/sema/type_checking.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/type_checking.o src/sema/type_checking.c

${BUILD_DIR}/sema/resolve_generics.o: ${DEP_COMMON} src/sema/resolve_generics.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/resolve_generics.o src/sema/resolve_generics.c

${BUILD_DIR}/sema/generic_sub.o: ${DEP_COMMON} src/sema/generic_sub.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/generic_sub.o src/sema/generic_sub.c

${BUILD_DIR}/sema/expand_lang_def.o: ${DEP_COMMON} src/sema/expand_lang_def.c
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sema/expand_lang_def.o src/sema/expand_lang_def.c

${BUILD_DIR}/file.o: ${DEP_COMMON} src/file.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/file.o src/file.c

${BUILD_DIR}/util/parameters.o: ${DEP_COMMON} src/util/parameters.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/parameters.o src/util/parameters.c

${BUILD_DIR}/error_msg.o: ${DEP_COMMON} src/error_msg.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/error_msg.o src/error_msg.c

${BUILD_DIR}/sizeof.o: ${DEP_COMMON} src/sizeof.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/sizeof.o src/sizeof.c

${BUILD_DIR}/symbol_table.o: ${DEP_COMMON} src/symbol_table.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/symbol_table.o src/symbol_table.c

${BUILD_DIR}/lang_type_serialize.o: ${DEP_COMMON} src/lang_type_serialize.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type_serialize.o src/lang_type_serialize.c

${BUILD_DIR}/ulang_type_serialize.o: ${DEP_COMMON} src/ulang_type_serialize.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/ulang_type_serialize.o src/ulang_type_serialize.c

${BUILD_DIR}/lang_type_from_ulang_type.o: ${DEP_COMMON} src/lang_type_from_ulang_type.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/lang_type_from_ulang_type.o src/lang_type_from_ulang_type.c

${BUILD_DIR}/uast_utils.o: ${DEP_COMMON} src/uast_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/uast_utils.o src/uast_utils.c

${BUILD_DIR}/symbol_collection_clone.o: ${DEP_COMMON} src/symbol_collection_clone.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/symbol_collection_clone.o src/symbol_collection_clone.c

${BUILD_DIR}/uast_clone.o: ${DEP_COMMON} src/uast_clone.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/uast_clone.o src/uast_clone.c

${BUILD_DIR}/llvm_utils.o: ${DEP_COMMON} src/llvm_utils.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/llvm_utils.o src/llvm_utils.c

${BUILD_DIR}/add_load_and_store.o: ${DEP_COMMON} src/add_load_and_store.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/add_load_and_store.o src/add_load_and_store.c

${BUILD_DIR}/emit_llvm.o: ${DEP_COMMON} src/emit_llvm.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/emit_llvm.o src/emit_llvm.c

${BUILD_DIR}/parser.o: ${DEP_COMMON} src/parser.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/parser.o src/parser.c

${BUILD_DIR}/token/tokenizer.o: ${DEP_COMMON} src/token/tokenizer.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/token/tokenizer.o src/token/tokenizer.c

${BUILD_DIR}/token/token.o: ${DEP_COMMON} src/token/token.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/token/token.o src/token/token.c

${BUILD_DIR}/util/name.o: ${DEP_COMMON} src/util/name.c 
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/util/name.o src/util/name.c

#clean:
#	rm -f ${OBJS} build/*/passes/*
#	rm -f ${OBJS} build/*/*
