#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "newstring.h"
#include <tast.h>
#include "tokens.h"
#include "parameters.h"
#include "file.h"
#include "do_passes.h"

int main(int argc, char** argv) {
    parse_args(argc, argv);
    do_passes(&params);

    return 0;
}
