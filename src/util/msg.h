#ifndef MSG_H
#define MSG_H

#include <util.h>
#include <vector.h>

#define msg(diag_type, pos, ...) \
    do { \
        msg_internal(__FILE__, __LINE__, diag_type, pos, __VA_ARGS__); \
    } while (0)

typedef struct {
    const char* file;
    int line;
    Pos pos_actual_msg;
    Pos pos_for_sort;
    Strv actual_msg;
    size_t pos_in_defered_msgs;
} Defered_msg;

typedef struct {
    Vec_base info;
    Defered_msg* buf;
} Defered_msg_vec;

void print_all_defered_msgs(void);

// msg* functions and macros print messages that are intended for the user (eg. syntax errors)
void msg_internal(
    const char* file, int line, DIAG_TYPE diag_type,
    Pos pos, const char* format, ...
) __attribute__((format (printf, 5, 6)));

#endif // MSG_H
