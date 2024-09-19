#include "error_msg.h"
#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"

void msg_redefinition_of_symbol(const Node* new_sym_def) {
    assert(new_sym_def->line_num > 0);
    assert(new_sym_def->file_path && strlen(new_sym_def->file_path) > 0);

    msg(
        LOG_ERROR, new_sym_def->file_path, new_sym_def->line_num,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(new_sym_def->name)
    );

    Node* original_def;
    try(sym_tbl_lookup(&original_def, new_sym_def->name));
    assert(original_def->line_num > 0);
    assert(original_def->file_path && strlen(original_def->file_path) > 0);
    msg(
        LOG_NOTE, original_def->file_path, original_def->line_num,
        STR_VIEW_FMT " originally defined here\n", str_view_print(original_def->name)
    );
}
