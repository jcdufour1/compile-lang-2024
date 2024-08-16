C_FLAGS=-Wall -Wextra -std=c11 -pedantic -g

BUILD_DIR=build/debug/
OBJS=\
	 ${BUILD_DIR}/main.o \
	 ${BUILD_DIR}/tokenizer.o \
	 ${BUILD_DIR}/parser.o \
	 ${BUILD_DIR}/globals.o \
	 ${BUILD_DIR}/token.o \
	 ${BUILD_DIR}/nodes.o

all: build

setup:
	mkdir -p build/release/
	mkdir -p build/debug/

run: build
	${BUILD_DIR}/main examples/test.c

build: setup ${BUILD_DIR}/main

${BUILD_DIR}/main: ${OBJS}
	cc ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: src/main.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/parser.o: src/parser.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/parser.o src/parser.c

${BUILD_DIR}/tokenizer.o: src/tokenizer.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/tokenizer.o src/tokenizer.c

${BUILD_DIR}/globals.o: src/globals.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/globals.o src/globals.c

${BUILD_DIR}/nodes.o: src/nodes.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/nodes.o src/nodes.c

${BUILD_DIR}/token.o: src/token.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/token.o src/token.c


clean:
	rm -f ${OBJS} ${BUILD_DIR}/main
