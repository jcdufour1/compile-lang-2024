#include <ast_msg.h>
#include <msg.h>
#include <uast_utils.h>

// TODO: move more functions here

void msg_undefined_symbol_internal(const char* file, int line, Name sym_name, Pos sym_pos) {
    msg_internal(
        file, line,
        DIAG_UNDEFINED_SYMBOL, sym_pos,
        "symbol `"FMT"` is not defined\n", name_print(NAME_MSG, sym_name)
    );
}

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

void msg_not_lvalue_internal(const char* file, int line, Pos pos) {
    msg_internal(file, line, DIAG_NOT_LVALUE, pos, "expression is not an lvalue (lvalue is required)\n");
}
