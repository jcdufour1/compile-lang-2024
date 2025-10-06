#include <ast_msg.h>
#include <msg.h>
#include <uast_utils.h>

// TODO: move more functions here

PARSE_STATUS msg_redefinition_of_symbol_internal(const char* file, int line, const Uast_def* new_sym_def) {
    msg_internal(
        file, line, DIAG_REDEFINITION_SYMBOL, uast_def_get_pos(new_sym_def),
        "redefinition of symbol `"FMT"`\n", name_print(NAME_MSG, uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg_internal(
        file, line, DIAG_NOTE, uast_def_get_pos(original_def),
        "`"FMT"` originally defined here\n", name_print(NAME_MSG, uast_def_get_name(original_def))
    );

    return PARSE_ERROR;
}
