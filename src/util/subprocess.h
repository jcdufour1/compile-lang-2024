#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <strv_darr.h>

Strv cmd_to_strv(Arena* arena, Strv_darr cmd);

// returns return code
int subprocess_call(Strv_darr cmd);

#endif // SUBPROCESS_H
