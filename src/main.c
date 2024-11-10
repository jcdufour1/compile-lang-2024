#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include "node.h"
#include "tokens.h"
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

    do_passes(file_text, &params);

    return 0;
}
