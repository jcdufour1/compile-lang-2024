#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "util.h"
#include "newstring.h"
#include "diag_type.h"

typedef struct {
    Vec_base info;
    DIAG_TYPE* buf;
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
    Expect_fail_type_vec diag_types;
    bool compile : 1;
    bool emit_llvm : 1;
    bool test_expected_fail : 1;
    bool all_errors_fatal: 1;
    bool error_opts_changed : 1;
    Backend_info backend_info;
} Parameters;

void parse_args(int argc, char** argv);

bool expect_fail_type_from_strv(size_t* idx, DIAG_TYPE* type, Str_view strv);

LOG_LEVEL expect_fail_type_to_curr_log_level(DIAG_TYPE type);

Str_view expect_fail_type_print_internal(DIAG_TYPE type);

#define expect_fail_type_print(type) str_view_print(expect_fail_type_print_internal(type))

extern Parameters params;

#endif // PARAMETERS_H
