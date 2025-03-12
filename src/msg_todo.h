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

#endif // MSG_TODO_H
