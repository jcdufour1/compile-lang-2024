#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include "token.h"
#include "tokens.h"
#include "tokenizer.h"

typedef struct {
    String file_name;
} Parameters;

Parameters parse_args(int argc, char** argv) {
    if (argc != 2) {
        todo();
    }

    Parameters params = {.file_name = String_new_from_cstr(argv[1])};
    return params; 
}

int read_file(String* file_text, const String* input_file_name) {
    assert(!input_file_name->buf[input_file_name->count]);
    String_init(file_text);
    FILE* file = fopen(input_file_name->buf, "rb");
    if (!file) {
        return 0;
    }

    size_t buf_size = 2024;
    size_t amount_read;
    // TODO: check for errors?
    do {
        String_reserve(file_text, buf_size);
        amount_read = fread(file_text->buf, 1, buf_size, file);
        file_text->count += amount_read;
    } while (amount_read > 0);

    return 1;
}

int main(int argc, char** argv) {
    Parameters params = parse_args(argc, argv);

    String file_text;
    if (!read_file(&file_text, &params.file_name)) {
        log(LOG_FETAL, "could not open file "STRING_FMT": errno %d (%s)", String_print(params.file_name), errno, strerror(errno));
        return 1;
    }

    Tokens tokens = tokenize(file_text);
    todo();
    // parse
    //
    // llvm thing

    return 0;
}
