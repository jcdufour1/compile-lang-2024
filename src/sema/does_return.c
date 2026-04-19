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
            does_return_child_if_is_auto_inserted = if_stmt->is_auto_inserted_else;
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
            return false;
        case TAST_IF_ELSE_CHAIN:
            return does_return_if_else_chain(tast_if_else_chain_unwrap(expr));
        case TAST_ASSIGNMENT:
            return false;
        case TAST_OPERATOR:
            return false;
        case TAST_SYMBOL:
            return false;
        case TAST_MEMBER_ACCESS:
            return false;
        case TAST_INDEX:
            return false;
        case TAST_LITERAL:
            return false;
        case TAST_FUNCTION_CALL:
            return false;
        case TAST_STRUCT_LITERAL:
            return false;
        case TAST_TUPLE:
            return false;
        case TAST_ENUM_CALLEE:
            return false;
        case TAST_ENUM_CASE:
            return false;
        case TAST_ENUM_GET_TAG:
            return false;
        case TAST_ENUM_ACCESS:
            return false;
    }
    unreachable("");
}

static bool does_return_stmt(Tast_stmt* last_stmt_block) {
    switch (last_stmt_block->type) {
        case TAST_DEFER:
            return false;
        case TAST_EXPR:
            return does_return_expr(tast_expr_unwrap(last_stmt_block));
        case TAST_FOR_WITH_COND:
            return false;
        case TAST_RETURN:
            return true;
        case TAST_ACTUAL_BREAK:
            return false;
        case TAST_YIELD:
            return does_return_expr(tast_yield_unwrap(last_stmt_block)->yield_expr);
        case TAST_CONTINUE:
            return false;
        case TAST_DEF:
            return false;
    }
    unreachable("");
}

bool does_return_stmt_darr(Tast_stmt_darr stmts, bool is_auto_inserted) {
    bool result = false;

    if (stmts.info.count > 0) {
        result = does_return_stmt(darr_last(stmts));
    }

    if (!result && !is_auto_inserted && does_return_print_notes) {
        DOES_RTN_POS_TYPE type = DOES_RTN_POS_BLOCK_EMPTY;
        if (stmts.info.count > 0) {
            type = DOES_RTN_POS_LAST_DOES_NOT_RETURN;
        }

        if (does_return_child_if_is_auto_inserted) {
            type = DOES_RTN_POS_EXPECTED_ELSE;
        }

        Does_return_pos pos = does_return_pos_new(tast_stmt_get_pos(darr_last(stmts)), type);
        if (pos_is_equal(does_return_print_prev_pos.pos, pos.pos)) {
            if (does_return_child_if_is_auto_inserted) {
                darr_last_ref(&does_return_print_stack)->type = DOES_RTN_POS_EXPECTED_ELSE;
            }
        } else {
            darr_append(&a_pass, &does_return_print_stack, pos);
            does_return_print_prev_pos = pos;
        }

        does_return_child_if_is_auto_inserted = false;
    }

    return result;
}

bool does_return_print_all_notes(Tast_stmt_darr stmts, bool is_auto_inserted) {
    does_return_print_notes = true;
    does_return_print_prev_pos = does_return_pos_new(POS_BUILTIN, DOES_RTN_POS_LAST_DOES_NOT_RETURN);
    does_return_print_stack = (Does_return_pos_darr) {0};

    bool result = does_return_stmt_darr(stmts, is_auto_inserted);

    log(LOG_DEBUG, FMT"\n", tast_print(darr_last(stmts)));

    bool is_first = true;
    while (does_return_print_stack.info.count > 0) {
        Does_return_pos pos = darr_pop(&does_return_print_stack);
        DIAG_TYPE diag_type = DIAG_NOTE;

        switch (pos.type) {
            case DOES_RTN_POS_EXPECTED_ELSE:
                if (is_first) {
                    diag_type = DIAG_MISSING_RETURN_IN_FUN;
                }
                msg(
                    diag_type, pos.pos,
                    "`if` or `else if` at end of function must have an else clause that returns\n"
                );
                break;
            case DOES_RTN_POS_BLOCK_EMPTY:
                if (is_first) {
                    diag_type = DIAG_MISSING_RETURN_IN_FUN;
                }
                msg(
                    diag_type, pos.pos,
                    "function block does not have a statement that returns\n"
                );
                break;
            case DOES_RTN_POS_LAST_DOES_NOT_RETURN:
                if (is_first) {
                    diag_type = DIAG_MISSING_RETURN_IN_FUN;
                }
                msg(
                    diag_type, pos.pos,
                    "last statement of block does not always return\n"
                );
                break;
            default:
                unreachable("");
        }

        is_first = false;
    }

    does_return_child_if_is_auto_inserted = false;
    does_return_print_notes = false;
    does_return_print_prev_pos = does_return_pos_new(POS_BUILTIN, DOES_RTN_POS_LAST_DOES_NOT_RETURN);
    does_return_print_stack = (Does_return_pos_darr) {0};
    return result;
}

static bool does_return_block(Tast_block* block) {
    return does_return_stmt_darr(block->children, block->is_auto_inserted);
}

