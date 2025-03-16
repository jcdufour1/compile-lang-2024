#ifndef UAST_CLONE_H
#define UAST_CLONE_H

#include <uast.h>

Uast_number* uast_number_clone(const Uast_number* lit);

Uast_literal* uast_literal_clone(const Uast_literal* lit);

Uast_expr* uast_expr_clone(const Uast_expr* expr);

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt);

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def);

Uast_param* uast_param_clone(const Uast_param* param);

Uast_block* uast_block_clone(const Uast_block* block);

Uast_def* uast_def_clone(const Uast_def* def);

Uast_case* uast_case_clone(const Uast_case* lang_case);

Uast_condition* uast_condition_clone(const Uast_condition* cond);

Uast_if* uast_if_clone(const Uast_if* lang_if);

#endif // UAST_CLONE_H
