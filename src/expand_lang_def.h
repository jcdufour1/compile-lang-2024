#ifndef EXPAND_LANG_DEF
#define EXPAND_LANG_DEF

bool expand_def_block(Uast_block* block);

bool expand_def_def(Uast_def* def);

bool expand_def_block(Uast_block* block);

bool expand_def_uname(Uname* name);

bool expand_def_expr_vec(Uast_expr_vec* exprs);

bool expand_def_expr(Uast_expr* expr);

bool expand_def_generic_param_vec(Uast_generic_param_vec* params);

bool expand_def_variable_def_vec(Uast_variable_def_vec* defs);

bool expand_def_symbol(Uast_symbol* sym);

#endif // EXPAND_LANG_DEF
