C_FLAGS=-Wall -Wextra -std=c11 -pedantic -g

all: build

run: build
	./src/main test.c

build: main

main: src/main.c
	cc ${C_FLAGS} -o src/main src/main.c 


