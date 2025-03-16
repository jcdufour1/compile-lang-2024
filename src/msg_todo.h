#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <expected_fail_type.h>

#define msg_todo(env, feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, env, feature, pos);



void msg_todo_internal(const char* file, int line, const Env* env, const char* feature, Pos pos) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_NOT_YET_IMPLEMENTED, env->file_text, pos, 
        "language feature `%s` not yet implemented (may or may not be implemented in the future)\n",
        feature
    );
}

// TODO: move this function and macro elsewhere
static void msg_undefined_symbol_internal(const char* file, int line, Str_view file_text, const Uast_stmt* sym_call) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, uast_stmt_get_pos(sym_call),
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(uast_stmt_get_name(sym_call))
    );
}

#define msg_undefined_symbol(file_text, sym_call) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, file_text, sym_call)

#endif // MSG_TODO_H
