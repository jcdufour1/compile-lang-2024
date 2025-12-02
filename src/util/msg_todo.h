#ifndef MSG_TODO_H
#define MSG_TODO_H

#include <diag_type.h>
#include <util.h>
#include <strv.h>
#include <msg.h>

// TODO: msg_todo should accept only Strv for simplicity?
#define msg_todo(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, sv(feature), pos);

#define msg_soft_todo(feature, pos) \
    msg_soft_todo_internal(__FILE__, __LINE__, sv(feature), pos);

#define msg_todo_strv(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, feature, pos);

static inline void msg_todo_internal(const char* file, int line, Strv feature, Pos pos) {
    msg_internal(
        file, line, DIAG_NOT_YET_IMPLEMENTED, pos, 
        "language feature `"FMT"` not yet implemented (may or may not be implemented in the future)\n",
        strv_print(feature)
    );
}

static inline void msg_soft_todo_internal(const char* file, int line, Strv feature, Pos pos) {
#   ifndef NDEBUG

        msg_internal(
            file, line, DIAG_SOFT_NOT_YET_IMPLEMENTED, pos, 
            "soft todo: language feature `"FMT"` not yet implemented (may or may not be implemented in the future)\n",
            strv_print(feature)
        );

#   endif // NDEBUG
}

#endif // MSG_TODO_H
