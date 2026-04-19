#ifndef DOES_RETURN_H
#define DOES_RETURN_H

// returns true if last stmt always returns
bool does_return_stmt_darr(Tast_stmt_darr stmts, Pos pos_parent);

bool does_return_print_all_notes(Tast_stmt_darr stmts, Pos pos_parent);

#endif // DOES_RETURN_H
