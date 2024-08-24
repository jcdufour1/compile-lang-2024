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
#include "parameters.h"
#include "file.h"
#include <stb_ds.h>

int main(int argc, char** argv) {
    parse_args(argc, argv);

    String file_text;
    if (!read_file(&file_text, params.input_file_name)) {
        msg(LOG_FETAL, "dummy", 0, "could not open file %s: errno %d (%s)\n", params.input_file_name, errno, strerror(errno));
        return 1;
    }

    Tokens tokens = tokenize(file_text);

    Node_id root = parse(tokens);

    Node_id item;
    assert(sym_tbl_lookup(&item, str_view_from_cstr("str0")));

    if (params.emit_llvm) {
        emit_llvm_from_tree(root);
    } else {
        todo();
    }

    return 0;
}
