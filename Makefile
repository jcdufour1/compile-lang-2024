C_FLAGS=-Wall -Wextra -std=c11 -pedantic -g

all: build

run: build
	src/main test.c

build: src/main

src/main: src/main.o src/tokenizer.o
	cc ${C_FLAGS} -o src/main src/main.o src/tokenizer.o 

src/main.o: src/main.c src/*.h
	cc ${C_FLAGS} -c -o src/main.o src/main.c

src/tokenizer.o: src/tokenizer.c src/*.h
	cc ${C_FLAGS} -c -o src/tokenizer.o src/tokenizer.c

