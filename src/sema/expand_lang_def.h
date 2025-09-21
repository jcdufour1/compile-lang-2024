#ifndef EXPAND_LANG_DEF
#define EXPAND_LANG_DEF

typedef enum {
    EXPAND_NAME_ERROR,
    EXPAND_NAME_NORMAL, // only changes name
    EXPAND_NAME_NEW_EXPR, // new_expr must be substituted where the name was
} EXPAND_NAME_STATUS;

bool expand_def_block(Uast_block* block);

bool expand_def_def(Uast_def* def);

bool expand_def_block(Uast_block* block);

EXPAND_NAME_STATUS expand_def_uname(Uast_expr** new_expr, Uname* name, Pos pos, Pos dest_pos);

bool expand_def_expr_vec(Uast_expr_vec* exprs);

bool expand_def_expr(Uast_expr** new_expr, Uast_expr* expr);

bool expand_def_generic_param_vec(Uast_generic_param_vec* params);

bool expand_def_variable_def_vec(Uast_variable_def_vec* defs);

EXPAND_NAME_STATUS expand_def_symbol(Uast_expr** new_expr, Uast_symbol* sym);

bool expand_def_ulang_type(Ulang_type* lang_type, Pos dest_pos);

bool expand_def_ulang_type_regular(
    Ulang_type_regular* new_lang_type,
    Ulang_type_regular lang_type,
    Pos dest_pos
);

bool expand_def_function_def(Uast_function_def* def);

bool expand_def_switch(Uast_switch* lang_switch);

bool expand_def_stmt(Uast_stmt** new_stmt, Uast_stmt* stmt);

bool expand_def_if_else_chain(Uast_if_else_chain* if_else);

bool expand_def_operator(Uast_operator* oper);

#endif // EXPAND_LANG_DEF
