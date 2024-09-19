#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include "node.h"
#include "tokens.h"
#include "tokenizer.h"
#include "parser.h"
#include "emit_llvm.h"
#include "parameters.h"
#include "file.h"
#include "passes/do_passes.h"

// we do not want to detect leaks (for now) since this compiler is a short running program
const char* __asan_default_options() { return "detect_leaks=0"; }

int main(int argc, char** argv) {
    parse_args(argc, argv);

    String file_text;
    if (!read_file(&file_text, params.input_file_name)) {
        msg(LOG_FETAL, "dummy", 0, "could not open file %s: errno %d (%s)\n", params.input_file_name, errno, strerror(errno));
        return 1;
    }

    Tokens tokens = tokenize(file_text, params);

    Node* root = parse(tokens);

    do_passes(&root);

    if (params.emit_llvm) {
        emit_llvm_from_tree(root);
    } else {
        todo();
    }

    arena_log_free_nodes();
    //log(LOG_VERBOSE, "arena size total capacity: %zu KB\n", arena_get_total_capacity() / 1024);

    return 0;
}
