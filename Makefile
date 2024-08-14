C_FLAGS=-Wall -Wextra -std=c11 -pedantic -g

BUILD_DIR=build/debug/
OBJS=${BUILD_DIR}/main.o ${BUILD_DIR}/tokenizer.o ${BUILD_DIR}/parser.o

all: build

setup:
	mkdir -p build/release/
	mkdir -p build/debug/

run: build
	${BUILD_DIR}/main test.c

build: setup ${BUILD_DIR}/main

${BUILD_DIR}/main: ${OBJS}
	cc ${C_FLAGS} -o ${BUILD_DIR}/main ${OBJS}

${BUILD_DIR}/main.o: src/main.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/main.o src/main.c

${BUILD_DIR}/parser.o: src/parser.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/parser.o src/parser.c

${BUILD_DIR}/tokenizer.o: src/tokenizer.c src/*.h
	cc ${C_FLAGS} -c -o ${BUILD_DIR}/tokenizer.o src/tokenizer.c

