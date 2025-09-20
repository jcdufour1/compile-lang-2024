#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <strv_vec.h>

Strv cmd_to_strv(Arena* arena, Strv_vec cmd);

// returns return code
int subprocess_call(Strv_vec cmd);

#endif // SUBPROCESS_H
