
static bool does_return_block(Tast_block* block);

static bool does_return_stmt(Tast_stmt* last_stmt_block);

static bool does_return_if_else_chain(Tast_if_else_chain* if_else) {
    darr_foreach(idx, Tast_if*, if_stmt, if_else->tasts) {
        if (!does_return_block(if_stmt->body)) {
            return false;
        }
    }

    return true;
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
        case TAST_YIELD:
            return does_return_expr(tast_yield_unwrap(last_stmt_block)->yield_expr);
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

bool does_return_stmt_darr(Tast_stmt_darr stmts) {
    for (size_t idx = stmts.info.count - 1; stmts.info.count > 0; idx--) {
        Tast_stmt* curr_stmt = darr_at(stmts, idx);
        log(LOG_DEBUG, FMT"\n", tast_print(curr_stmt));

        //if (!does_return_is_trivial_stmt(curr_stmt)) {
            return does_return_stmt(curr_stmt);
        //}

        if (idx < 1) {
            break;
        }
    }

    return false;
}

static bool does_return_block(Tast_block* block) {
    return does_return_stmt_darr(block->children);
}

