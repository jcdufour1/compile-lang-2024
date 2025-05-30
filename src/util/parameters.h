#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <util.h>
#include <newstring.h>
#include <diag_type.h>
#include <str_view_vec.h>

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

typedef enum {
    OPT_LEVEL_O0 = 0,
    OPT_LEVEL_O2,

    // count of opt levels for static_asserts
    OPT_LEVEL_COUNT,
} OPT_LEVEL;

typedef struct {
    Str_view input_file_path;
    Str_view output_file_path;
    Str_view_vec l_flags; // eg. if user passes `-l m -l raylib`, l_flags contains `[sv("m"), sv("raylib")]
    Expect_fail_type_vec diag_types;
    OPT_LEVEL opt_level : 4;
    bool compile : 1;
    bool run : 1;
    bool dump_dot : 1;
    bool emit_llvm : 1;
    bool all_errors_fatal : 1;
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
