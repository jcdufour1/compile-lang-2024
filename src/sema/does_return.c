typedef enum {
    DOES_RTN_POS_EXPECTED_ELSE,
    DOES_RTN_POS_BLOCK_EMPTY,
    DOES_RTN_POS_LAST_DOES_NOT_RETURN,
} DOES_RTN_POS_TYPE;

typedef struct {
    Pos pos;
    DOES_RTN_POS_TYPE type;
} Does_return_pos;

typedef struct {
    Vec_base info;
    Does_return_pos* buf;
} Does_return_pos_darr;

static bool does_return_child_if_is_auto_inserted = false;
static bool does_return_print_notes = false;
static Does_return_pos does_return_print_prev_pos = (Does_return_pos) {0};
static Does_return_pos_darr does_return_print_stack = (Does_return_pos_darr) {0};

static bool does_return_block(Tast_block* block);

static Does_return_pos does_return_pos_new(Pos pos, DOES_RTN_POS_TYPE type) {
    return (Does_return_pos) {.pos = pos, .type = type};
}

static bool does_return_if_else_chain(Tast_if_else_chain* if_else) {
    darr_foreach(idx, Tast_if*, if_stmt, if_else->tasts) {
        if (!does_return_block(if_stmt->body)) {
            does_return_child_if_is_auto_inserted = if_stmt->is_auto_inserted;
            return false;
        }
    }

    return true;
}

static bool does_return_pos_is_equal(Does_return_pos a, Does_return_pos b) {
    return a.type == b.type && pos_is_equal(a.pos, b.pos);
}

static bool does_return_expr(Tast_expr* expr) {
    switch (expr->type) {
        case TAST_BLOCK:
            return does_return_block(tast_block_unwrap(expr));
        case TAST_MODULE_ALIAS:
            todo();
        case TAST_IF_ELSE_CHAIN:
            return does_return_if_else_chain(tast_if_else_chain_unwrap(expr));
        case TAST_ASSIGNMENT:
            todo();
        case TAST_OPERATOR:
            todo();
        case TAST_SYMBOL:
            todo();
        case TAST_MEMBER_ACCESS:
            todo();
        case TAST_INDEX:
            todo();
        case TAST_LITERAL:
            todo();
        case TAST_FUNCTION_CALL:
            if (does_return_print_notes) {
                //msg(
                //    DIAG_NOTE, tast_expr_get_pos(expr),
                //    "last statement of block does not always return\n"
                //);
            }
            return false;
        case TAST_STRUCT_LITERAL:
            todo();
        case TAST_TUPLE:
            todo();
        case TAST_ENUM_CALLEE:
            todo();
        case TAST_ENUM_CASE:
            todo();
        case TAST_ENUM_GET_TAG:
            todo();
        case TAST_ENUM_ACCESS:
            todo();
    }
    unreachable("");
}

static bool does_return_stmt(Tast_stmt* last_stmt_block) {
    switch (last_stmt_block->type) {
        case TAST_DEFER:
            // TODO: warn for useless defer?
            return false;
        case TAST_EXPR:
            log(LOG_DEBUG, FMT"\n", tast_print(last_stmt_block));
            return does_return_expr(tast_expr_unwrap(last_stmt_block));
        case TAST_FOR_WITH_COND:
            todo();
        case TAST_RETURN:
            return true;
        case TAST_ACTUAL_BREAK:
            todo();
            return false; // TODO
        case TAST_YIELD: {
            bool status = does_return_expr(tast_yield_unwrap(last_stmt_block)->yield_expr);
            if (!status && does_return_print_notes) {
            }
            return status;
        }
        case TAST_CONTINUE:
            todo();
            return false; // TODO
        case TAST_DEF:
            todo();
    }
    unreachable("");
}

//static bool does_return_is_trivial_block(Tast_block* block) {
//    for (size_t idx = stmts.info.count - 1; stmts.info.count > 0; idx--) {
//        Tast_stmt* curr_stmt = darr_at(stmts, idx);
//        log(LOG_DEBUG, FMT"\n", tast_print(curr_stmt));
//
//        if (!does_return_is_trivial_stmt(curr_stmt)) {
//            return does_return_stmt(curr_stmt);
//        }
//
//        if (idx < 1) {
//            break;
//        }
//    }
//}

static bool does_return_is_trivial_expr(Tast_expr* expr) {
    switch (expr->type) {
        case TAST_BLOCK:
            return false;
        case TAST_MODULE_ALIAS:
            todo();
        case TAST_IF_ELSE_CHAIN:
            todo();
        case TAST_ASSIGNMENT:
            todo();
        case TAST_OPERATOR:
            todo();
        case TAST_SYMBOL:
            todo();
        case TAST_MEMBER_ACCESS:
            todo();
        case TAST_INDEX:
            todo();
        case TAST_LITERAL:
            todo();
        case TAST_FUNCTION_CALL:
            todo();
        case TAST_STRUCT_LITERAL:
            todo();
        case TAST_TUPLE:
            todo();
        case TAST_ENUM_CALLEE:
            todo();
        case TAST_ENUM_CASE:
            todo();
        case TAST_ENUM_GET_TAG:
            todo();
        case TAST_ENUM_ACCESS:
            todo();
    }
    unreachable("");
}

// TODO: remove *trivial*
static bool does_return_is_trivial_stmt(Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEFER:
            // TODO: warn for unexecuted instruction
            todo();
            return true;
        case TAST_EXPR:
            // TODO: warn for unexecuted instruction
            return does_return_is_trivial_expr(tast_expr_unwrap(stmt));
        case TAST_FOR_WITH_COND:
            todo();
            //return does_return_for_with_cond(tast_for_with_cond_unwrap(curr_stmt));
        case TAST_RETURN:
            return false;
        case TAST_ACTUAL_BREAK:
            // TODO: warn for unexecuted instruction
            todo();
            return true;
        case TAST_YIELD:
            // TODO: warn for unexecuted instruction
            todo();
            return true;
        case TAST_CONTINUE:
            // TODO: warn for unexecuted instruction
            todo();
            return true;
        case TAST_DEF:
            todo();
        default:
            unreachable("");
    }
    unreachable("");
}

bool does_return_stmt_darr(Tast_stmt_darr stmts, Pos pos_parent) {
    bool result = false;

    //static int num = 0;
    //num += 1;

    //params_log_level = LOG_NEVER;
    //if (num > 1) {
    //    params_log_level = LOG_DEBUG;
    //}
    if (does_return_print_notes) {
        breakpoint();
    }
    if (stmts.info.count > 0) {
        //if (num > 1) {
            log(LOG_DEBUG, FMT"\n", tast_print(darr_last(stmts)));
        //}
        result = does_return_stmt(darr_last(stmts));
    }

    if (!result) {
        if (does_return_print_notes) {
            if (stmts.info.count > 0) {
                DOES_RTN_POS_TYPE type = DOES_RTN_POS_LAST_DOES_NOT_RETURN;
                if (does_return_child_if_is_auto_inserted) {
                    type = DOES_RTN_POS_EXPECTED_ELSE;
                }

                Does_return_pos pos = does_return_pos_new(tast_stmt_get_pos(darr_last(stmts)), type);
                if (!does_return_pos_is_equal(does_return_print_prev_pos, pos)) {
                    darr_append(&a_pass, &does_return_print_stack, pos);
                    does_return_print_prev_pos = pos;
                }
            } else {
                DOES_RTN_POS_TYPE type = DOES_RTN_POS_BLOCK_EMPTY;
                if (does_return_child_if_is_auto_inserted) {
                    type = DOES_RTN_POS_EXPECTED_ELSE;
                }

                Does_return_pos pos = does_return_pos_new(pos_parent, type);
                if (!does_return_pos_is_equal(does_return_print_prev_pos, pos)) {
                    darr_append(&a_pass, &does_return_print_stack, pos);
                    does_return_print_prev_pos = pos;
                }
            }
                    
        }
    }

    return result;
}

bool does_return_print_all_notes(Tast_stmt_darr stmts, Pos pos_parent) {
    does_return_print_notes = true;
    does_return_print_prev_pos = does_return_pos_new(POS_BUILTIN, DOES_RTN_POS_LAST_DOES_NOT_RETURN);
    does_return_print_stack = (Does_return_pos_darr) {0};

    bool result = does_return_stmt_darr(stmts, pos_parent);

    log(LOG_DEBUG, FMT"\n", tast_print(darr_last(stmts)));

    if (does_return_print_stack.info.count > 0) {
        darr_pop(&does_return_print_stack);
    }
    while (does_return_print_stack.info.count > 0) {
        Does_return_pos pos = darr_pop(&does_return_print_stack);

        switch (pos.type) {
            case DOES_RTN_POS_EXPECTED_ELSE:
                msg(
                    DIAG_NOTE, pos.pos,
                    "if else chain at end of function must have an else clause\n"
                );
                break;
            case DOES_RTN_POS_BLOCK_EMPTY:
                msg(
                    DIAG_NOTE, pos.pos,
                    "function block does not have a statement that returns\n"
                );
                break;
            case DOES_RTN_POS_LAST_DOES_NOT_RETURN:
                msg(
                    DIAG_NOTE, pos.pos,
                    "last statement of block does not always return\n"
                );
                break;
            default:
                unreachable("");
        }
    }

    does_return_print_notes = false;
    does_return_print_prev_pos = does_return_pos_new(POS_BUILTIN, DOES_RTN_POS_LAST_DOES_NOT_RETURN);
    does_return_print_stack = (Does_return_pos_darr) {0};
    return result;
}

static bool does_return_block(Tast_block* block) {
    return does_return_stmt_darr(block->children, block->pos);
}

