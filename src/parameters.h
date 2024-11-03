#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "util.h"
#include "newstring.h"
#include "expected_fail_type.h"

typedef struct {
    const char* input_file_name;
    EXPECT_FAIL_TYPE expected_fail_type;
    bool compile : 1;
    bool emit_llvm : 1;
    bool test_expected_fail : 1;
    bool all_errors_fetal: 1;
} Parameters;

void parse_args(int argc, char** argv);

extern Parameters params;

#endif // PARAMETERS_H
