#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "util.h"
#include "newstring.h"
#include "expected_fail_type.h"

typedef struct {
    Vec_base info;
    EXPECT_FAIL_TYPE* buf;
} Expect_fail_type_vec;

typedef enum {
    BACKEND_NONE = 0,
    BACKEND_C,
    BACKEND_LLVM,
} BACKEND;

typedef struct {
    BACKEND backend;
    bool struct_rtn_through_param;
} Backend_info;

typedef struct {
    const char* input_file_name;
    Expect_fail_type_vec expected_fail_types;
    bool compile : 1;
    bool emit_llvm : 1;
    bool test_expected_fail : 1;
    bool all_errors_fatal: 1;
    Backend_info backend_info;
} Parameters;

void parse_args(int argc, char** argv);

bool expect_fail_type_from_strv(EXPECT_FAIL_TYPE* type, Str_view strv);

Str_view expect_fail_type_print_internal(EXPECT_FAIL_TYPE type);

#define expect_fail_type_print(type) str_view_print(expect_fail_type_print_internal(type))

extern Parameters params;

#endif // PARAMETERS_H
