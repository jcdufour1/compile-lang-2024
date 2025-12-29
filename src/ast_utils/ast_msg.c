#include <ast_msg.h>
#include <msg.h>
#include <uast_utils.h>
#include <did_you_mean.h>

// TODO: move more functions here

void msg_undefined_symbol_internal(const char* file, int line, Name sym_name, Pos sym_pos) {
    msg_internal(
        file, line,
        DIAG_UNDEFINED_SYMBOL, sym_pos,
        "symbol `"FMT"` is not defined"FMT"\n",
        name_print(NAME_MSG, sym_name, false),
        did_you_mean_symbol_print(sym_name)
    );
}

PARSE_STATUS msg_redefinition_of_symbol_internal(const char* file, int line, const Uast_def* new_sym_def) {
    msg_internal(
        file, line, DIAG_REDEFINITION_SYMBOL, uast_def_get_pos(new_sym_def),
        "redefinition of symbol `"FMT"`\n", name_print(NAME_MSG, uast_def_get_name(new_sym_def), false)
    );

    Uast_def* original_def = NULL;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg_internal(
        file, line, DIAG_NOTE, uast_def_get_pos(original_def),
        "`"FMT"` originally defined here\n", name_print(NAME_MSG, uast_def_get_name(original_def), false)
    );

    return PARSE_ERROR;
}

void msg_not_lvalue_internal(const char* file, int line, Pos pos) {
    msg_internal(file, line, DIAG_NOT_LVALUE, pos, "expression is not an lvalue (lvalue is required)\n");
}

void msg_got_type_but_expected_expr_internal(const char* file, int line, Pos pos) {
    msg_internal(file, line, DIAG_GOT_TYPE_BUT_EXPECTED_EXPR, pos, "got type, but expected expression\n");
}

void msg_got_expr_but_expected_type_internal(const char* file, int line, Pos pos) {
    msg_internal(file, line, DIAG_GOT_EXPR_BUT_EXPECTED_TYPE, pos, "got expression, but expected type\n");
}


void msg_struct_literal_assigned_to_non_struct_gen_param_internal(
    const char* file,
    int line,
    Pos pos_struct_lit,
    Pos pos_gen_param
) {
    msg_internal(
        file, line,
        DIAG_INVALID_TYPE,
        pos_struct_lit,
        "struct literal assigned to non-struct generic parameter\n"
    );
    msg_internal(
        file, line,
        DIAG_NOTE,
        pos_gen_param,
        "non-struct generic parameter type defined here\n"
    );
}

void msg_todo_internal(const char* file, int line, Strv feature, Pos pos) {
    msg_internal(
        file, line, DIAG_NOT_YET_IMPLEMENTED, pos, 
        "language feature `"FMT"` not yet implemented (may or may not be implemented in the future)\n",
        strv_print(feature)
    );
}

void msg_soft_todo_internal(const char* file, int line, Strv feature, Pos pos) {
    (void) file;
    (void) line;
    (void) feature;
    (void) pos;

#   ifndef NDEBUG

        msg_internal(
            file, line, DIAG_SOFT_NOT_YET_IMPLEMENTED, pos, 
            "soft todo: language feature `"FMT"` not yet implemented (may or may not be implemented in the future)\n",
            strv_print(feature)
        );

#   endif // NDEBUG
}

