.PHONY: all setup build valgrind gdb

C_FLAGS=-Wall -Wextra -std=c11 -pedantic -g -I ./third_party/

BUILD_DIR=build/debug/
OBJS=\
	 ${BUILD_DIR}/main.o \
	 ${BUILD_DIR}/tokenizer.o \
	 ${BUILD_DIR}/parser.o \
	 ${BUILD_DIR}/globals.o \
	 ${BUILD_DIR}/token.o \
	 ${BUILD_DIR}/emit_llvm.o \
	 ${BUILD_DIR}/hash_table.o \
	 ${BUILD_DIR}/file.o \
	 ${BUILD_DIR}/parameters.o \
	 ${BUILD_DIR}/passes/do_passes.o \
	 ${BUILD_DIR}/nodes.o

ARGS_PROGRAM=compile examples/test.c --emit-llvm

all: build

run: build
	${BUILD_DIR}/main ${ARGS_PROGRAM}

valgrind: build
	valgrind ${BUILD_DIR}/main ${ARGS_PROGRAM}

gdb: build
	gdb --args ${BUILD_DIR}/main ${ARGS_PROGRAM}

build: ${BUILD_DIR}/main

# general
${BUILD_DIR}/main: ${OBJS}
	cc ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: src/main.c src/*.h src/passes/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/parser.o: src/parser.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/parser.o src/parser.c

${BUILD_DIR}/tokenizer.o: src/tokenizer.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/tokenizer.o src/tokenizer.c

${BUILD_DIR}/globals.o: src/globals.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/globals.o src/globals.c

${BUILD_DIR}/nodes.o: src/nodes.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/nodes.o src/nodes.c

${BUILD_DIR}/token.o: src/token.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/token.o src/token.c

${BUILD_DIR}/emit_llvm.o: src/emit_llvm.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/emit_llvm.o src/emit_llvm.c

${BUILD_DIR}/hash_table.o: src/hash_table.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/hash_table.o src/hash_table.c

${BUILD_DIR}/file.o: src/file.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/file.o src/file.c

${BUILD_DIR}/parameters.o: src/parameters.c src/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/parameters.o src/parameters.c


# passes
${BUILD_DIR}/passes/do_passes.o: src/passes/do_passes.c src/*.h src/passes/*.h third_party/*
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/passes/do_passes.o src/passes/do_passes.c


clean:
	rm -f ${OBJS} ${BUILD_DIR}/main
