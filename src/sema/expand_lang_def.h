#ifndef EXPAND_LANG_DEF
#define EXPAND_LANG_DEF

typedef enum {
    EXPAND_NAME_ERROR,
    EXPAND_NAME_NORMAL, // only changes name
    EXPAND_NAME_NEW_EXPR, // new_expr must be substituted where the name was
    EXPAND_NAME_NEW_ULANG_TYPE, // new_expr must be substituted where the name was
} EXPAND_NAME_STATUS;

typedef enum {
    EXPAND_EXPR_ERROR,
    EXPAND_EXPR_NEW_EXPR, // new_expr must be substituted where the name was
    EXPAND_EXPR_NEW_ULANG_TYPE, // new_expr must be substituted where the name was
} EXPAND_EXPR_STATUS;

void expand_def_def(Uast_def* def);

void expand_def_block(Uast_block* block);

EXPAND_NAME_STATUS expand_def_uname(Ulang_type* new_lang_type, Uast_expr** new_expr, Uname* name, Pos pos, Pos dest_pos);

void expand_def_expr_vec(Uast_expr_vec* exprs);

void expand_def_expr_not_ulang_type(Uast_expr** new_expr, Uast_expr* expr);

EXPAND_EXPR_STATUS expand_def_expr(Ulang_type* new_lang_type, Uast_expr** new_expr, Uast_expr* expr);

void expand_def_generic_param_vec(Uast_generic_param_vec* params);

void expand_def_variable_def_vec(Uast_variable_def_vec* defs);

EXPAND_NAME_STATUS expand_def_symbol(Ulang_type* new_lang_type, Uast_expr** new_expr, Uast_symbol* sym);

void expand_def_ulang_type(Ulang_type* lang_type, Pos dest_pos);

Ulang_type_regular expand_def_ulang_type_regular(
    Ulang_type_regular lang_type,
    Pos dest_pos
);

void expand_def_function_def(Uast_function_def* def);

void expand_def_switch(Uast_switch* lang_switch);

Uast_stmt* expand_def_stmt(Uast_stmt* stmt);

void expand_def_if_else_chain(Uast_if_else_chain* if_else);

void expand_def_operator(Uast_operator* oper);

void expand_def_defer(Uast_defer* lang_defer);

EXPAND_NAME_STATUS expand_def_name(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Name* name,
    Pos dest_pos
);

#endif // EXPAND_LANG_DEF
