#include <do_passes.h>
#include <tast.h>
#include <tasts.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>

Tast_block* analysis_1(Uast_block* root) {
    Tast_block* new_block = NULL;
    if (!try_set_block_types( &new_block, root, false, true)) {
        new_block = NULL;
    }
    return new_block;
}
