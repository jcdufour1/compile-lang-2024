.PHONY: all setup build gdb

CC_COMPILER ?= clang

C_FLAGS_DEBUG=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function \
			  -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ \
			  -D CURR_LOG_LEVEL=${LOG_LEVEL} \
			  -fsanitize=address -fno-omit-frame-pointer 
C_FLAGS_RELEASE=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function \
			    -std=c11 -pedantic -g -I ./third_party/ -I ${BUILD_DIR} -I src/ -I src/util/ \
			    -D CURR_LOG_LEVEL=${LOG_LEVEL} \
			    -DNDEBUG \
				-O2

C_FLAGS_AUTO_GEN=-Wall -Wextra -Wno-format-zero-length -Wno-unused-function \
			     -std=c11 -pedantic -g -I ./third_party/ -I src/util/ \
			     -D CURR_LOG_LEVEL=${LOG_LEVEL} \
			     -fsanitize=address -fno-omit-frame-pointer 

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    C_FLAGS = ${C_FLAGS_DEBUG}
	BUILD_DIR=./build/debug/
	LOG_LEVEL ?= "LOG_TRACE"
else
    C_FLAGS = ${C_FLAGS_RELEASE}
	BUILD_DIR=./build/release/
	LOG_LEVEL ?= "LOG_VERBOSE"
endif

OBJS=\
	 ${BUILD_DIR}/main.o \
	 ${BUILD_DIR}/arena.o \
	 ${BUILD_DIR}/nodes.o \
	 ${BUILD_DIR}/globals.o \
	 ${BUILD_DIR}/token.o \
	 ${BUILD_DIR}/symbol_table.o \
	 ${BUILD_DIR}/file.o \
	 ${BUILD_DIR}/parameters.o \
	 ${BUILD_DIR}/parser_utils.o \
	 ${BUILD_DIR}/error_msg.o \
	 ${BUILD_DIR}/walk_tree.o \
	 ${BUILD_DIR}/passes/do_passes.o \
	 ${BUILD_DIR}/passes/tokenizer.o \
	 ${BUILD_DIR}/passes/parser.o \
	 ${BUILD_DIR}/passes/for_and_if_to_branch.o \
	 ${BUILD_DIR}/passes/assign_llvm_ids.o \
	 ${BUILD_DIR}/passes/add_load_and_store.o \
	 ${BUILD_DIR}/passes/add_alloca.o \
	 ${BUILD_DIR}/passes/analysis_1.o \
	 ${BUILD_DIR}/passes/change_operators.o \
	 ${BUILD_DIR}/passes/emit_llvm.o

DEP_UTIL = Makefile src/util/*.h
DEP_COMMON = ${DEP_UTIL} src/*.h ${BUILD_DIR}/node.h

FILE_TO_TEST ?= examples/new_lang/structs.own
ARGS_PROGRAM ?= compile ${FILE_TO_TEST} --emit-llvm

all: build

run: build
	time ${BUILD_DIR}/main ${ARGS_PROGRAM}

gdb: build
	gdb --args ${BUILD_DIR}/main ${ARGS_PROGRAM}

build: ${BUILD_DIR}/main

test_quick: run
	${CC_COMPILER} test.ll -o a.out && ./a.out ; echo $$?

# auto_gen and util
${BUILD_DIR}/auto_gen: src/util/auto_gen.c
	${CC_COMPILER} ${C_FLAGS_AUTO_GEN} -o ${BUILD_DIR}/auto_gen src/util/arena.c src/util/auto_gen.c

${BUILD_DIR}/node.h: ${BUILD_DIR}/auto_gen
	./${BUILD_DIR}/auto_gen ${BUILD_DIR}/node.h

# general
${BUILD_DIR}/main: ${DEP_COMMON} ${OBJS}
	${CC_COMPILER} ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: ${DEP_COMMON} src/main.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/arena.o: ${DEP_COMMON} src/util/arena.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/arena.o src/util/arena.c

${BUILD_DIR}/parser_utils.o: ${DEP_COMMON} src/parser_utils.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/parser_utils.o src/parser_utils.c

${BUILD_DIR}/globals.o: ${DEP_COMMON} src/globals.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/globals.o src/globals.c

${BUILD_DIR}/nodes.o: ${DEP_COMMON} src/nodes.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/nodes.o src/nodes.c

${BUILD_DIR}/token.o: ${DEP_COMMON} src/token.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/token.o src/token.c

${BUILD_DIR}/symbol_table.o: ${DEP_COMMON} src/symbol_table.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/symbol_table.o src/symbol_table.c

${BUILD_DIR}/file.o: ${DEP_COMMON} src/file.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/file.o src/file.c

${BUILD_DIR}/parameters.o: ${DEP_COMMON} src/parameters.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/parameters.o src/parameters.c

${BUILD_DIR}/error_msg.o: ${DEP_COMMON} src/error_msg.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/error_msg.o src/error_msg.c

${BUILD_DIR}/walk_tree.o: ${DEP_COMMON} src/walk_tree.c third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/walk_tree.o src/walk_tree.c


# passes
${BUILD_DIR}/passes/do_passes.o: ${DEP_COMMON} src/passes/do_passes.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/do_passes.o src/passes/do_passes.c

${BUILD_DIR}/passes/for_and_if_to_branch.o: ${DEP_COMMON} src/passes/for_and_if_to_branch.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/for_and_if_to_branch.o src/passes/for_and_if_to_branch.c

${BUILD_DIR}/passes/assign_llvm_ids.o: ${DEP_COMMON} src/passes/assign_llvm_ids.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/assign_llvm_ids.o src/passes/assign_llvm_ids.c

${BUILD_DIR}/passes/add_load_and_store.o: ${DEP_COMMON} src/passes/add_load_and_store.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/add_load_and_store.o src/passes/add_load_and_store.c

${BUILD_DIR}/passes/analysis_1.o: ${DEP_COMMON} src/passes/analysis_1.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/analysis_1.o src/passes/analysis_1.c

${BUILD_DIR}/passes/add_alloca.o: ${DEP_COMMON} src/passes/add_alloca.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/add_alloca.o src/passes/add_alloca.c

${BUILD_DIR}/passes/change_operators.o: ${DEP_COMMON} src/passes/change_operators.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/change_operators.o src/passes/change_operators.c

${BUILD_DIR}/passes/emit_llvm.o: ${DEP_COMMON} src/passes/emit_llvm.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/emit_llvm.o src/passes/emit_llvm.c

${BUILD_DIR}/passes/parser.o: ${DEP_COMMON} src/passes/parser.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/parser.o src/passes/parser.c

${BUILD_DIR}/passes/tokenizer.o: ${DEP_COMMON} src/passes/tokenizer.c src/passes/*.h third_party/*
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/passes/tokenizer.o src/passes/tokenizer.c

clean:
	rm -f ${OBJS} ${BUILD_DIR}/main
