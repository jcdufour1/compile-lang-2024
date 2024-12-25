#ifndef DO_PASSES_H
#define DO_PASSES_H

#include "util.h"
#include <node.h>
#include "env.h"
#include "parameters.h"

void do_passes(Str_view file_text, const Parameters* params);

#endif // DO_PASSES_H
