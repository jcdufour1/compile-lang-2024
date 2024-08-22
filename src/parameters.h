#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "util.h"
#include "newstring.h"

typedef struct {
    const char* input_file_name;
    bool compile;
    bool emit_llvm;
} Parameters;

void parse_args(int argc, char** argv);

extern Parameters params;

#endif // PARAMETERS_H
