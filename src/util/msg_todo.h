#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <expected_fail_type.h>

// TODO: remove this forward declaration
__attribute__((format (printf, 6, 7)))
void msg_internal(
    const char* file, int line, LOG_LEVEL log_level, EXPECT_FAIL_TYPE msg_expect_fail_type,
    Pos pos, const char* format, ...
);

#define msg_todo(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__,  feature, pos);

static inline void msg_todo_internal(const char* file, int line, const char* feature, Pos pos) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_NOT_YET_IMPLEMENTED, pos, 
        "language feature `%s` not yet implemented (may or may not be implemented in the future)\n",
        feature
    );
}

#endif // MSG_TODO_H
