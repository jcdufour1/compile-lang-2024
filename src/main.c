#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include "node.h"
#include "token.h"
#include "tokens.h"
#include "tokenizer.h"
#include "parser.h"
#include "emit_llvm.h"
#include "symbol_table.h"
#include <stb_ds.h>

typedef struct {
    String input_file_name;
    bool compile;
    bool emit_llvm;
} Parameters;

static bool is_long_option(char** argv) {
    if (strlen(argv[0]) < 2) {
        return false;
    }

    return argv[0][0] == '-' && argv[0][1] == '-';
}

static bool is_short_option(char** argv) {
    return !is_long_option(argv) && argv[0][0] == '-';
}

// this function will exclude - or -- part of arg if present
static const char* consume_arg(int* argc, char*** argv) {
    if (*argc < 1) {
        log(LOG_FETAL, "missing argument\n");
        abort();
    }
    const char* curr_arg = argv[0][0];
    while (curr_arg[0] && curr_arg[0] == '-') {
        curr_arg++;
    }
    (*argv)++;
    (*argc)--;

    if (curr_arg[0]) {
        return curr_arg;
    }

    return consume_arg(argc, argv);
}

static void parse_normal_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv);

    if (0 == strcmp(curr_opt, "compile")) {
        params->compile = true;
        params->input_file_name = string_new_from_cstr(consume_arg(argc, argv));
    } else {
        log(LOG_FETAL, "invalid option: %s\n", curr_opt);
        exit(1);
    }
}

static void parse_long_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv);

    if (0 == strcmp(curr_opt, "emit-llvm")) {
        params->emit_llvm = true;
    } else {
        log(LOG_FETAL, "invalid option: %s\n", curr_opt);
        exit(1);
    }
}

static Parameters get_params_with_defaults(void) {
    Parameters params = {0};

    params.emit_llvm = false;
    params.compile = false;

    return params;
}

static Parameters parse_args(int argc, char** argv) {
    Parameters params = get_params_with_defaults();

    // consume compiler executable name
    consume_arg(&argc, &argv);

    while (argc > 0) {
        if (is_short_option(argv)) {
            todo();
        } else if (is_long_option(argv)) {
            parse_long_option(&params, &argc, &argv);
        } else {
            parse_normal_option(&params, &argc, &argv);
        }
    }

    //params = {.file_name = string_new_from_cstr(argv[1])};
    return params; 
}

static bool read_file(String* file_text, const String* input_file_name) {
    assert(!input_file_name->buf[input_file_name->info.count]);
    memset(file_text, 0, sizeof(*file_text));
    FILE* file = fopen(input_file_name->buf, "rb");
    if (!file) {
        return false;
    }

    size_t buf_size = 2024;
    size_t amount_read;
    // TODO: check for errors?
    do {
        string_reserve(file_text, buf_size);
        amount_read = fread(file_text->buf, 1, buf_size, file);
        file_text->info.count += amount_read;
    } while (amount_read > 0);

    return true;
}

int main(int argc, char** argv) {
    Parameters params = parse_args(argc, argv);

    String file_text;
    if (!read_file(&file_text, &params.input_file_name)) {
        log(LOG_FETAL, "could not open file "STRING_FMT": errno %d (%s)\n", string_print(params.input_file_name), errno, strerror(errno));
        return 1;
    }

    Tokens tokens = tokenize(file_text);

    Node_id root = parse(tokens);

    Node_id item;
    assert(sym_tbl_lookup(&item, str_view_from_cstr("str0")));
    log(LOG_DEBUG, NODE_FMT"\n", node_print(item));

    if (params.emit_llvm) {
        emit_llvm_from_tree(root);
    } else {
        todo();
    }

    return 0;
}
