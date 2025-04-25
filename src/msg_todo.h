#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <expected_fail_type.h>

#define msg_todo(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__,  feature, pos);



void msg_todo_internal(const char* file, int line, const char* feature, Pos pos) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_NOT_YET_IMPLEMENTED, env.file_path_to_text, pos, 
        "language feature `%s` not yet implemented (may or may not be implemented in the future)\n",
        feature
    );
}

// TODO: move this function and macro elsewhere
static void msg_undefined_symbol_internal(const char* file, int line, File_path_to_text file_text, const Uast_symbol* sym_call) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", name_print(sym_call->name)
    );
}

#define msg_undefined_symbol(file_text, sym_call) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, file_text, sym_call)

#endif // MSG_TODO_H
