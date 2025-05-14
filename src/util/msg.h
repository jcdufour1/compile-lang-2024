#ifndef MSG_H
#define MSG_H

#include <symbol_table_struct.h>

#define msg(expected_fail_type, pos, ...) \
    do { \
        msg_internal(__FILE__, __LINE__, expected_fail_type, pos, __VA_ARGS__); \
    } while (0)

// msg* functions and macros print messages that are intended for the user (eg. syntax errors)
void msg_internal(
    const char* file, int line, EXPECT_FAIL_TYPE expected_fail_type,
    Pos pos, const char* format, ...
) __attribute__((format (printf, 5, 6)));

#endif // MSG_H
