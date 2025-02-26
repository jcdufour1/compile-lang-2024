#ifndef UAST_CLONE_H
#define UAST_CLONE_H

#include <uast.h>

Uast_number* uast_number_clone(const Uast_number* lit);

Uast_literal* uast_literal_clone(const Uast_literal* lit);

Uast_expr* uast_expr_clone(const Uast_expr* expr);

#endif // UAST_CLONE_H
