#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <str_view_vec.h>

Str_view cmd_to_strv(Arena* arena, Str_view_vec cmd);

// returns return code
int subprocess_call(Str_view_vec cmd);

#endif // SUBPROCESS_H
