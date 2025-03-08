#ifndef UAST_CLONE_H
#define UAST_CLONE_H

#include <uast.h>

Uast_number* uast_number_clone(const Uast_number* lit);

Uast_literal* uast_literal_clone(const Uast_literal* lit);

Uast_expr* uast_expr_clone(const Uast_expr* expr);

Uast_stmt* uast_stmt_clone(const Uast_stmt* stmt);

Uast_variable_def* uast_variable_def_clone(const Uast_variable_def* def);

#endif // UAST_CLONE_H
