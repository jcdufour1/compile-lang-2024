#ifndef MSG_H
#define MSG_H

#include <symbol_table_struct.h>

#define msg(log_level, expected_fail_type, file_text, pos, ...) \
    do { \
        msg_internal(__FILE__, __LINE__, log_level, expected_fail_type, file_text, pos, __VA_ARGS__); \
    } while (0)

// msg* functions and macros print messages that are intended for the user (eg. syntax errors)
void msg_internal(
    const char* file, int line, LOG_LEVEL log_level, EXPECT_FAIL_TYPE expected_fail_type,
    File_path_to_text file_text, Pos pos, const char* format, ...
) __attribute__((format (printf, 7, 8)));

#endif // MSG_H
