#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <util.h>
#include <newstring.h>
#include <diag_type.h>
#include <strv_vec.h>

typedef enum {
    ARCH_X86_64,
} TARGET_ARCH;

typedef enum {
    VENDOR_UNKNOWN,
    VENDOR_PC,
} TARGET_VENDOR;

typedef enum {
    OS_LINUX,
    OS_WINDOWS,
} TARGET_OS;

typedef enum {
    ABI_GNU,
} TARGET_ABI;

typedef struct {
    TARGET_ARCH arch;
    TARGET_VENDOR vendor;
    TARGET_OS os;
    TARGET_ABI abi;
} Target_triplet;

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

typedef enum {
    STOP_AFTER_NONE = 0,
    STOP_AFTER_IR,
    STOP_AFTER_BACKEND_IR,
    STOP_AFTER_UPPER_S,
    STOP_AFTER_OBJ,
    STOP_AFTER_BIN,
    STOP_AFTER_RUN,

    // count for static asserts
    STOP_AFTER_COUNT,
} STOP_AFTER;

// PARAMETERS_COUNT should be set to the number of members in Parameters
#define PARAMETERS_COUNT 32
typedef struct {
    Target_triplet target_triplet;
    uint32_t sizeof_usize; 
    uint32_t sizeof_ptr_non_fn; 
    char usize_size_ux[8]; // eg. "u64"
    Strv build_dir;
    Strv input_file_path;
    Strv output_file_path;
    Strv path_c_compiler;
    Strv_vec l_flags; // eg. if user passes `-l m -l raylib`, l_flags contains `[sv("m"), sv("raylib")]
    Strv_vec static_libs;
    Strv_vec dynamic_libs;
    Strv_vec c_input_files;
    Strv_vec object_files;
    Strv_vec lower_s_files;
    Strv_vec upper_s_files;
    Strv_vec run_args;
    Expect_fail_type_vec diag_types;
    OPT_LEVEL opt_level : 4;
    STOP_AFTER stop_after : 4;
    bool compile_own : 1;
    bool dump_dot : 1;
    bool all_errors_fatal : 1;
    bool error_opts_changed : 1;
    bool do_prelude : 1;
    bool print_immediately : 1;
    bool is_path_c_compiler : 1;
    bool is_output_file_path : 1;
    Backend_info backend_info;
    uint32_t max_errors;
    int argc;
    char** argv;
    Pos pos_lower_o;
} Parameters;

#define stop_after_print(stop_after) strv_print(stop_after_print_internal(stop_after))

Strv stop_after_print_internal(STOP_AFTER stop_after);

bool target_triplet_is_equal(Target_triplet a, Target_triplet b);

Target_triplet get_default_target_triplet(void);

bool is_compiling(void);

void parse_args(int argc, char** argv);

bool expect_fail_type_from_strv(size_t* idx, DIAG_TYPE* type, Strv strv);

LOG_LEVEL expect_fail_type_to_curr_log_level(DIAG_TYPE type);

Strv expect_fail_type_print_internal(DIAG_TYPE type);

bool expect_fail_type_is_type_inference_error(DIAG_TYPE type);

#define expect_fail_type_print(type) strv_print(expect_fail_type_print_internal(type))

extern Parameters params;

#endif // PARAMETERS_H
