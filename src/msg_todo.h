#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <expected_fail_type.h>

void msg_todo(const Env* env, const char* feature, Pos pos) {
    msg(
        LOG_ERROR, EXPECT_FAIL_NOT_YET_IMPLEMENTED, env->file_text, pos, 
        "language feature `%s` not yet implemented (may or may not be implemented in the future)\n",
        feature
    );
}

#endif // MSG_TODO_H
