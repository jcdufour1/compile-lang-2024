#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <str_view_vec.h>

// returns return code
int subprocess_call(Str_view_vec cmd);

#endif // SUBPROCESS_H
