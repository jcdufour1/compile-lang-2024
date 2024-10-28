#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include "node.h"
#include "tokens.h"
#include "tokenizer.h"
#include "parser.h"
#include "parameters.h"
#include "file.h"
#include "do_passes.h"

int main(int argc, char** argv) {
    parse_args(argc, argv);

    String file_text;
    if (!read_file(&file_text, params.input_file_name)) {
        msg(LOG_FETAL, EXPECT_FAIL_TYPE_NONE, dummy_pos, "could not open file %s: errno %d (%s)\n", params.input_file_name, errno, strerror(errno));
        return 1;
    }

    Tokens tokens = tokenize(file_text, params);

    Node_block* root = parse(tokens);

    do_passes(&root, &params);

    return 0;
}
