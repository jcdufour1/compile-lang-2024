#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <expected_fail_type.h>

// TODO: remove this forward declaration
__attribute__((format (printf, 5, 6)))
void msg_internal(
    const char* file, int line, EXPECT_FAIL_TYPE msg_expect_fail_type,
    Pos pos, const char* format, ...
);

#define msg_todo(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, str_view_from_cstr(feature), pos);

#define msg_todo_strv(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, feature, pos);

static inline void msg_todo_internal(const char* file, int line, Str_view feature, Pos pos) {
    msg_internal(
        file, line, EXPECT_FAIL_NOT_YET_IMPLEMENTED, pos, 
        "language feature `"STR_VIEW_FMT"` not yet implemented (may or may not be implemented in the future)\n",
        str_view_print(feature)
    );
}

#endif // MSG_TODO_H
