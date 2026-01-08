.PHONY: all setup build gdb test_quick clean

WERROR_ALL ?= 0

# TODO: consider if -Wconversion could be used instead of -Wfloat-conversion
# TODO: decide if -fno-strict-aliasing flag should be kept (if removed, turn on warnings for strict aliasing)
C_WARNINGS = -Werror=incompatible-pointer-types \
			 -Wall -Wextra -Wenum-compare -Wimplicit-fallthrough -Wsign-conversion -Wfloat-conversion -Wswitch-enum \
			 -Wno-missing-braces -Wno-type-limits -Wno-unused-value -Wno-format-zero-length -Wno-unused-function -Wno-address

C_FLAGS_COMMON = ${C_WARNINGS} \
			     -std=c11 -pedantic -g \
			         -I ./third_party/ \
                     -I ${BUILD_DIR} \
			       	 -I src/ \
			       	 -I src/util/ \
			       	 -I src/util/auto_gen/ \
			       	 -I src/token \
			       	 -I src/sema \
			       	 -I src/codegen \
			       	 -I src/lang_type/ \
			       	 -I src/ir \
			       	 -I src/ast_utils/ \
			     -fno-strict-aliasing \
			     -D MIN_LOG_LEVEL=${LOG_LEVEL} \

# TODO: change gnu11 to c11
# TODO: use same sanitize flags for autogen and regular compilation
C_FLAGS_AUTO_GEN= ${C_WARNINGS} \
			     -std=gnu11 -pedantic -g -I ./third_party/ -I src/util/ -I src/util/auto_gen/ \
			     -D MIN_LOG_LEVEL=${LOG_LEVEL} \
			     -fsanitize=address -fno-omit-frame-pointer

BUILD_DIR_DEBUG ?= ./build/debug/
BUILD_DIR_RELEASE ?= ./build/release/

ifndef CC_COMPILER
	ifeq (, $(shell which clang))
		CC_COMPILER = cc
	else
		CC_COMPILER = clang
	endif
endif

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    C_FLAGS = ${C_FLAGS_COMMON}
    C_FLAGS += -fsanitize=undefined -fno-sanitize-recover=undefined \
			   -fsanitize=address -fno-sanitize-recover=address \
			   -fno-omit-frame-pointer
	BUILD_DIR=${BUILD_DIR_DEBUG}
	LOG_LEVEL ?= "LOG_TRACE"
else
    C_FLAGS = ${C_FLAGS_COMMON}
	C_FLAGS += -DNDEBUG -O2 # -flto 
	BUILD_DIR=${BUILD_DIR_RELEASE}
	LOG_LEVEL ?= "LOG_VERBOSE"
endif

ifeq ($(WERROR_ALL), 1)
	C_FLAGS += -Werror -D OWN_WERROR
endif

DEP_UTIL = Makefile src/util/*.h src/util/auto_gen/*.h src/util/auto_gen/auto_gen.c

# TODO: this needs to be done better, because this is error prone
# DEP_COMMON = ${DEP_UTIL} third_party/* src/util/auto_gen/auto_gen* ${BUILD_DIR}/ast_utils/tast.h
DEP_COMMON = ${DEP_UTIL} src/*.h ${BUILD_DIR}/tast.h third_party/*
DEP_COMMON += $(shell find src -type f -name "*.[hc]")

FILE_TO_TEST ?= examples/new_lang/structs.own
ARGS_PROGRAM ?= ${FILE_TO_TEST} --set-log-level VERBOSE

all: build

run: build
	time ${BUILD_DIR}/main ${ARGS_PROGRAM} -lm

# TODO: add -pg flag automatically depending on env variable passed to Makefile? (this will require rebuilding everything if ENV var changes)
# NOTE: for gprof, add "-pg" to release build options, and do not set BUILD to 0
gprof: run
	gprof ${BUILD_DIR}/main gmon.out > report.txt

# NOTE: for gprof2dot, add "-pg" to release build options, and do not set BUILD to 0
gprof2dot: gprof
	gprof2dot report.txt > report_graphvis.txt

# NOTE: for gprof_to_graphvis, add "-pg" to release build options, and do not set BUILD to 0
gprof_to_graphvis: gprof2dot
	dot report_graphvis.txt -T svg > report.svg

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
	mkdir -p ${BUILD_DIR}/unity_build/

# TODO: always run setup before ${BUILD_DIR}/main
build: setup ${BUILD_DIR}/main

# TODO: figure out better way to switch between c and llvm
COMPILER_OUTPUT=test.c

test_quick: run
	./a.out ; echo $$?

# auto_gen and util
${BUILD_DIR}/auto_gen: src/util/auto_gen/auto_gen.c ${DEP_UTIL}
	${CC_COMPILER} ${C_FLAGS_AUTO_GEN} -D IN_AUTOGEN -o ${BUILD_DIR}/auto_gen src/util/params_log_level.c src/util/arena.c src/util/auto_gen/auto_gen.c src/util/newstring.c

${BUILD_DIR}/tast.h: ${BUILD_DIR}/auto_gen
	./${BUILD_DIR}/auto_gen ${BUILD_DIR}

# general
#${BUILD_DIR}/main: ${DEP_COMMON}
	#${CC_COMPILER} ${C_FLAGS} -o ${BUILD_DIR}/main src/unity_build_almost_everything.c src/util/subprocess.c

${BUILD_DIR}/main: ${DEP_COMMON} ${BUILD_DIR}/unity_build/unity_build_token_and_parser.o ${BUILD_DIR}/unity_build/unity_build_ir_and_codegen.o ${BUILD_DIR}/unity_build/unity_build_miscellaneous.o ${BUILD_DIR}/unity_build/unity_build_sema.o
	${CC_COMPILER} ${C_FLAGS} -o ${BUILD_DIR}/main \
		${BUILD_DIR}/unity_build/unity_build_token_and_parser.o \
		${BUILD_DIR}/unity_build/unity_build_ir_and_codegen.o \
		${BUILD_DIR}/unity_build/unity_build_miscellaneous.o \
		${BUILD_DIR}/unity_build/unity_build_sema.o \
		src/util/subprocess.c

${BUILD_DIR}/unity_build/unity_build_token_and_parser.o: ${DEP_COMMON}
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/unity_build/unity_build_token_and_parser.o src/unity_build/unity_build_token_and_parser.c

${BUILD_DIR}/unity_build/unity_build_ir_and_codegen.o: ${DEP_COMMON}
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/unity_build/unity_build_ir_and_codegen.o src/unity_build/unity_build_ir_and_codegen.c

${BUILD_DIR}/unity_build/unity_build_miscellaneous.o: ${DEP_COMMON}
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/unity_build/unity_build_miscellaneous.o src/unity_build/unity_build_miscellaneous.c

${BUILD_DIR}/unity_build/unity_build_sema.o: ${DEP_COMMON}
	${CC_COMPILER} ${C_FLAGS} -c -o ${BUILD_DIR}/unity_build/unity_build_sema.o src/unity_build/unity_build_sema.c

# TODO: implement make clean
# make clean:

