#include <parameters.h>
#include <file.h>
#include <msg.h>
#include <msg_todo.h>
#include <util.h>

static Strv compiler_exe_name;

static void print_usage(void);

typedef struct {
    TARGET_ARCH arch;
    const char* arch_cstr;
    unsigned int sizeof_usize;
    unsigned int sizeof_ptr_non_fn;
} Arch_row;
static Arch_row arch_table[] = {
    {ARCH_X86_64, "x86_64", 64, 64},
};

static struct {
    TARGET_VENDOR vendor;
    const char* vendor_cstr;
} vendor_table[] = {
    {VENDOR_UNKNOWN, "unknown"},
    {VENDOR_PC, "pc"},
};

static struct {
    TARGET_OS os;
    const char* os_cstr;
} os_table[] = {
    {OS_LINUX, "linux"},
    {OS_WINDOWS, "windows"},
};

static struct {
    TARGET_ABI abi;
    const char* abi_cstr;
} abi_table[] = {
    {ABI_GNU, "gnu"},
};

static_assert(array_count(arch_table) == 1, "exhausive handling of architectures");
static TARGET_ARCH get_default_arch(void) {
#   if defined(__x86_64__) || defined(_M_X64)
        return ARCH_X86_64;
#   else
        // TODO: return ARCH_UNKNOWN?
#       error "unsupported architecture"
#   endif
}

static TARGET_VENDOR get_default_vendor(void) {
    return VENDOR_UNKNOWN; // TODO
}

static_assert(array_count(os_table) == 2, "exhausive handling of operating systems");
static TARGET_OS get_default_os(void) {
    // TODO: add ifdef for windows
#   ifdef __linux__
        return OS_LINUX;
#   else
        // TODO: return OS_UNKNOWN?
#       error "unsupported operating system"
#   endif
}

static_assert(array_count(abi_table) == 1, "exhausive handling of abis");
static TARGET_ABI get_default_abi(void) {
#   ifdef __GLIBC__
        return ABI_GNU;
#   else
        // TODO: return ABI_UNKNOWN?
#       error "unsupported abi"
#   endif
}

static Arch_row get_arch_row_from_arch(TARGET_ARCH arch) {
    for (size_t idx = 0; idx < array_count(arch_table); idx++) {
        if (array_at(arch_table, idx).arch == arch) {
            return array_at(arch_table, idx);
        }
    }
    unreachable("");
}

Strv strv_from_target_arch(TARGET_ARCH arch) {
    return sv(get_arch_row_from_arch(arch).arch_cstr);
}

Strv strv_from_target_vendor(TARGET_VENDOR vendor) {
    for (size_t idx = 0; idx < array_count(vendor_table); idx++) {
        if (array_at(vendor_table, idx).vendor == vendor) {
            return sv(array_at(vendor_table, idx).vendor_cstr);
        }
    }
    unreachable("");
}

Strv strv_from_target_os(TARGET_OS os) {
    for (size_t idx = 0; idx < array_count(os_table); idx++) {
        if (array_at(os_table, idx).os == os) {
            return sv(array_at(os_table, idx).os_cstr);
        }
    }
    unreachable("");
}

Strv strv_from_target_abi(TARGET_ABI abi) {
    for (size_t idx = 0; idx < array_count(abi_table); idx++) {
        if (array_at(abi_table, idx).abi == abi) {
            return sv(array_at(abi_table, idx).abi_cstr);
        }
    }
    unreachable("");
}

static bool try_target_arch_from_strv(TARGET_ARCH* arch, Strv strv) {
    for (size_t idx = 0; idx < array_count(arch_table); idx++) {
        if (strv_is_equal(sv(array_at(arch_table, idx).arch_cstr), strv)) {
            *arch = array_at(arch_table, idx).arch;
            return true;
        }
    }
    return false;
}

static bool try_target_vendor_from_strv(TARGET_VENDOR* vendor, Strv strv) {
    for (size_t idx = 0; idx < array_count(vendor_table); idx++) {
        if (strv_is_equal(sv(array_at(vendor_table, idx).vendor_cstr), strv)) {
            *vendor = array_at(vendor_table, idx).vendor;
            return true;
        }
    }
    return false;
}

static bool try_target_os_from_strv(TARGET_OS* os, Strv strv) {
    for (size_t idx = 0; idx < array_count(os_table); idx++) {
        if (strv_is_equal(sv(array_at(os_table, idx).os_cstr), strv)) {
            *os = array_at(os_table, idx).os;
            return true;
        }
    }
    return false;
}

static bool try_target_abi_from_strv(TARGET_ABI* abi, Strv strv) {
    for (size_t idx = 0; idx < array_count(abi_table); idx++) {
        if (strv_is_equal(sv(array_at(abi_table, idx).abi_cstr), strv)) {
            *abi = array_at(abi_table, idx).abi;
            return true;
        }
    }
    return false;
}

bool target_triplet_is_equal(Target_triplet a, Target_triplet b) {
    return a.arch == b.arch && a.vendor == b.vendor && a.os == b.os && a.abi == b.abi;
}

Strv target_triplet_print_internal(Target_triplet triplet) {
    String buf = {0};

    string_extend_strv(&a_print, &buf, strv_from_target_arch(triplet.arch));
    string_extend_cstr(&a_print, &buf, "-");
    string_extend_strv(&a_print, &buf, strv_from_target_os(triplet.os));
    string_extend_cstr(&a_print, &buf, "-");
    string_extend_strv(&a_print, &buf, strv_from_target_abi(triplet.abi));

    return string_to_strv(buf);
}

#define target_triplet_print(triplet) strv_print(target_triplet_print_internal(triplet))

Target_triplet get_default_target_triplet(void) {
    return (Target_triplet) {
        .arch = get_default_arch(),
        .vendor = get_default_vendor(),
        .os = get_default_os(),
        .abi = get_default_abi(),
    };
}

Strv stop_after_print_internal(STOP_AFTER stop_after) {
    switch (stop_after) {
        case STOP_AFTER_NONE:
            return sv("none");
        case STOP_AFTER_GEN_IR:
            return sv("gen_ir");
        case STOP_AFTER_GEN_BACKEND_IR:
            return sv("gen_backend_ir");
        case STOP_AFTER_LOWER_S:
            return sv("lower_s");
        case STOP_AFTER_OBJ:
            return sv("obj");
        case STOP_AFTER_BIN:
            return sv("bin");
        case STOP_AFTER_RUN:
            return sv("run");
        case STOP_AFTER_COUNT:
            unreachable("");
    }
    unreachable("");
}

bool is_compiling(void) {
    static_assert(
        STOP_AFTER_COUNT == 7,
        "exhausive handling of stop after states (not all states are explicitly handled)"
    );
    return params.stop_after == STOP_AFTER_RUN || params.stop_after == STOP_AFTER_BIN;
}

static void stop_after_set_if_none(STOP_AFTER set_to) {
    if (params.stop_after == STOP_AFTER_NONE) {
        params.stop_after = set_to;
    }
}

static bool is_long_option(char** argv) {
    if (strlen(argv[0]) < 2) {
        return false;
    }

    return argv[0][0] == '-' && argv[0][1] == '-';
}

static bool is_short_option(char** argv) {
    return !is_long_option(argv) && argv[0][0] == '-';
}

// this function will exclude - or -- part of arg if present
static Strv consume_arg(int* argc, char*** argv, Strv msg_if_missing) {
    if (*argc < 1) {
        msg(DIAG_MISSING_COMMAND_LINE_ARG, POS_BUILTIN, FMT"\n", strv_print(msg_if_missing));
        exit(EXIT_CODE_FAIL);
    }
    const char* curr_arg = argv[0][0];
    while (curr_arg[0] && curr_arg[0] == '-') {
        curr_arg++;
    }
    (*argv)++;
    (*argc)--;

    if (curr_arg[0]) {
        return sv(curr_arg);
    }

    return consume_arg(argc, argv, sv("stray - or -- is not permitted"));
}

typedef struct {
    const char* str;
    DIAG_TYPE type;
    LOG_LEVEL default_level;
    bool must_be_error;
} Expect_fail_pair;

typedef struct {
    const char* str;
    LOG_LEVEL curr_level;
} Expect_fail_str_to_curr_log_level;

static_assert(DIAG_COUNT == 86, "exhaustive handling of expected fail types");
static const Expect_fail_pair expect_fail_pair[] = {
    {"info", DIAG_INFO, LOG_INFO, false},
    {"note", DIAG_NOTE, LOG_NOTE, false},
    {"file-built", DIAG_FILE_BUILT, LOG_VERBOSE, false},
    {"missing-command-line-arg", DIAG_MISSING_COMMAND_LINE_ARG, LOG_ERROR, true},
    {"file-could-not-open", DIAG_FILE_COULD_NOT_OPEN, LOG_ERROR, true},
    {"file-could-not-read", DIAG_FILE_COULD_NOT_READ, LOG_ERROR, true},
    {"diag-enum-non-void-case-no-par-on-assign", DIAG_ENUM_NON_VOID_CASE_NO_PAR_ON_ASSIGN, LOG_ERROR, true},
    {"diag-function-param-not-specified", DIAG_FUNCTION_PARAM_NOT_SPECIFIED, LOG_ERROR, true},
    {"missing-close-double-quote", DIAG_MISSING_CLOSE_DOUBLE_QUOTE, LOG_ERROR, true},
    {"missing-close-single-quote", DIAG_MISSING_CLOSE_SINGLE_QUOTE, LOG_ERROR, true},
    {"no-new-line-after-statement", DIAG_NO_NEW_LINE_AFTER_STATEMENT, LOG_ERROR, true},
    {"missing-close-par", DIAG_MISSING_CLOSE_PAR, LOG_ERROR, true},
    {"missing-close-curly-brace", DIAG_MISSING_CLOSE_CURLY_BRACE, LOG_ERROR, true},
    {"invalid-extern-type", DIAG_INVALID_EXTERN_TYPE, LOG_ERROR, true},
    {"invalid-token", DIAG_INVALID_TOKEN, LOG_ERROR, true},
    {"parser-expected", DIAG_PARSER_EXPECTED, LOG_ERROR, true},
    {"redefinition-of-symbol", DIAG_REDEFINITION_SYMBOL, LOG_ERROR, true},
    {"invalid-struct-member-in-literal", DIAG_INVALID_MEMBER_IN_LITERAL, LOG_ERROR, true},
    {"invalid-member-access", DIAG_INVALID_MEMBER_ACCESS, LOG_ERROR, true},
    {"missing-return-statement", DIAG_MISSING_RETURN, LOG_ERROR, true},
    {"invalid-count-fun-args", DIAG_INVALID_COUNT_FUN_ARGS, LOG_ERROR, true},
    {"invalid-function-arg", DIAG_INVALID_FUN_ARG, LOG_ERROR, true},
    {"mismatched-return-type", DIAG_MISMATCHED_RETURN_TYPE, LOG_ERROR, true},
    {"mismatched-yield-type", DIAG_MISMATCHED_YIELD_TYPE, LOG_ERROR, true},
    {"diag-mismatched-closing-curly-brace", DIAG_MISMATCHED_CLOSING_CURLY_BRACE, LOG_ERROR, true},
    {"assignment-mismatched-types", DIAG_ASSIGNMENT_MISMATCHED_TYPES, LOG_ERROR, true},
    {"unary-mismatched-types", DIAG_UNARY_MISMATCHED_TYPES, LOG_ERROR, true},
    {"binary-mismatched-types", DIAG_BINARY_MISMATCHED_TYPES, LOG_ERROR, true},
    {"expected-expression", DIAG_EXPECTED_EXPRESSION, LOG_ERROR, true},
    {"undefined-symbol", DIAG_UNDEFINED_SYMBOL, LOG_ERROR, true},
    {"undefined-function", DIAG_UNDEFINED_FUNCTION, LOG_ERROR, true},
    {"struct-init-on-raw-union", DIAG_STRUCT_INIT_ON_RAW_UNION, LOG_ERROR, true},
    {"struct-init-on-enum", DIAG_STRUCT_INIT_ON_ENUM, LOG_ERROR, true},
    {"struct-init-on-primitive", DIAG_STRUCT_INIT_ON_PRIMITIVE, LOG_ERROR, true},
    {"undefined-type", DIAG_UNDEFINED_TYPE, LOG_ERROR, true},
    {"missing-close-sq-bracket", DIAG_MISSING_CLOSE_SQ_BRACKET, LOG_ERROR, true},
    {"missing-close-generic", DIAG_MISSING_CLOSE_GENERIC, LOG_ERROR, true},
    {"deref_non_pointer", DIAG_DEREF_NON_POINTER, LOG_ERROR, true},
    {"break-invalid-location", DIAG_BREAK_INVALID_LOCATION, LOG_ERROR, true},
    {"continue-invalid-location", DIAG_CONTINUE_INVALID_LOCATION, LOG_ERROR, true},
    {"mismatched-tuple-count", DIAG_MISMATCHED_TUPLE_COUNT, LOG_ERROR, true},
    {"non-exhaustive-switch", DIAG_NON_EXHAUSTIVE_SWITCH, LOG_ERROR, true},
    {"enum-lit-invalid-arg", DIAG_ENUM_LIT_INVALID_ARG, LOG_ERROR, true},
    {"not-yet-implemented", DIAG_NOT_YET_IMPLEMENTED, LOG_ERROR, true},
    {"duplicate-default", DIAG_DUPLICATE_DEFAULT, LOG_ERROR, true},
    {"duplicate-case", DIAG_DUPLICATE_CASE, LOG_ERROR, true},
    {"invalid-count-generic-args", DIAG_INVALID_COUNT_GENERIC_ARGS, LOG_ERROR, true},
    {"missing-yield-statement", DIAG_MISSING_YIELD_STATEMENT, LOG_ERROR, true},
    {"invalid-hex", DIAG_INVALID_HEX, LOG_ERROR, true},
    {"invalid-bin", DIAG_INVALID_BIN, LOG_ERROR, true},
    {"invalid-octal", DIAG_INVALID_OCTAL, LOG_ERROR, true},
    {"invalid-char-lit", DIAG_INVALID_CHAR_LIT, LOG_ERROR, true},
    {"invalid-decimal-lit", DIAG_INVALID_DECIMAL_LIT, LOG_ERROR, true},
    {"invalid-float-lit", DIAG_INVALID_FLOAT_LIT, LOG_ERROR, true},
    {"missing-close-multiline", DIAG_MISSING_CLOSE_MULTILINE, LOG_ERROR, true},
    {"invalid-count-struct-lit-args", DIAG_INVALID_COUNT_STRUCT_LIT_ARGS, LOG_ERROR, true},
    {"missing-enum-arg", DIAG_MISSING_ENUM_ARG, LOG_ERROR, true},
    {"enum-case-too-mopaque-args", DIAG_ENUM_CASE_TOO_MOPAQUE_ARGS, LOG_ERROR, true},
    {"void-enum-case-has-arg", DIAG_VOID_ENUM_CASE_HAS_ARG, LOG_ERROR, true},
    {"invalid-stmt-top-level", DIAG_INVALID_STMT_TOP_LEVEL, LOG_ERROR, true},
    {"invalid-function-callee", DIAG_INVALID_FUNCTION_CALLEE, LOG_ERROR, true},
    {"optional-args-for-variadic-args", DIAG_OPTIONAL_ARGS_FOR_VARIADIC_ARGS, LOG_ERROR, true},
    {"fail-invalid-fail-type", DIAG_INVALID_FAIL_TYPE, LOG_ERROR, false},
    {"no-main-function", DIAG_NO_MAIN_FUNCTION, LOG_WARNING, false},
    {"struct-like-recursion", DIAG_STRUCT_LIKE_RECURSION, LOG_ERROR, true},
    {"child-process-failure", DIAG_CHILD_PROCESS_FAILURE, LOG_FATAL, true},
    {"no-input-files", DIAG_NO_INPUT_FILES, LOG_FATAL, true},
    {"return-in-defer", DIAG_RETURN_IN_DEFER, LOG_ERROR, true},
    {"break-out-of-defer", DIAG_BREAK_OUT_OF_DEFER, LOG_ERROR, true},
    {"continue-out-of-defer", DIAG_CONTINUE_OUT_OF_DEFER, LOG_ERROR, true},
    {"assignment-to-void", DIAG_ASSIGNMENT_TO_VOID, LOG_ERROR, true},
    {"if-else-in-if-let", DIAG_IF_ELSE_IN_IF_LET, LOG_ERROR, true},
    {"unknown-on-non-enum-type", DIAG_UNKNOWN_ON_NON_ENUM_TYPE, LOG_ERROR, true},
    {"invalid-label-pos", DIAG_INVALID_LABEL_POS, LOG_ERROR, true},
    {"invalid-countof", DIAG_INVALID_COUNTOF, LOG_ERROR, true},
    {"redef-struct-base-member", DIAG_REDEF_STRUCT_BASE_MEMBER, LOG_ERROR, true},
    {"switch-no-cases", DIAG_SWITCH_NO_CASES, LOG_ERROR, true},
    {"wrong-gen-type", DIAG_WRONG_GEN_TYPE, LOG_ERROR, true},
    {"using-on-non-struct-or-mod-alias", DIAG_USING_ON_NON_STRUCT_OR_MOD_ALIAS, LOG_ERROR, true},
    {"file-invalid-name", DIAG_FILE_INVALID_NAME, LOG_ERROR, true},
    {"gen-infer-more-than-64-wide", DIAG_GEN_INFER_MORE_THAN_64_WIDE, LOG_WARNING, false},
    {"if-should-be-if-let", DIAG_IF_SHOULD_BE_IF_LET, LOG_ERROR, true},
    {"unsupported-target-triplet", DIAG_UNSUPPORTED_TARGET_TRIPLET, LOG_ERROR, true},
    {"invalid-literal-prefix", DIAG_INVALID_LITERAL_PREFIX, LOG_ERROR, true},
    {"def-recursion", DIAG_DEF_RECURSION, LOG_ERROR, true},
    {"diag-not-lvalue", DIAG_NOT_LVALUE, LOG_ERROR, true},
};

// error types are in the same order in expect_fail_str_to_curr_log_level_pair and expect_fail_pair
// TODO: make these names less verbose?
static Expect_fail_str_to_curr_log_level 
expect_fail_str_to_curr_log_level_pair[sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0])] = {0};

bool expect_fail_type_from_strv(size_t* idx_result, DIAG_TYPE* type, Strv strv) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        if (strv_is_equal(sv(array_at(expect_fail_pair, idx).str), strv)) {
            *type = expect_fail_pair[idx].type;
            *idx_result = idx;
            return true;
        }
    }
    return false;
}

static size_t expect_fail_type_get_idx(DIAG_TYPE type) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        if (expect_fail_pair[idx].type == type) {
            return idx;
        }
    }
    unreachable("expect_fail_pair does not cover this DIAG_TYPE");
}

Strv expect_fail_type_print_internal(DIAG_TYPE type) {
    return sv(expect_fail_pair[expect_fail_type_get_idx(type)].str);
}

static void expect_fail_str_to_curr_log_level_init(void) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        expect_fail_str_to_curr_log_level_pair[idx].str = expect_fail_pair[idx].str;
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = expect_fail_pair[idx].default_level;
    }
}

LOG_LEVEL expect_fail_type_to_curr_log_level(DIAG_TYPE type) {
    return array_at(expect_fail_str_to_curr_log_level_pair, expect_fail_type_get_idx(type)).curr_level;
}

static void parse_file_option(int* argc, char*** argv) {
    Strv curr_opt = consume_arg(argc, argv, sv("arg expected"));

    static_assert(
        PARAMETERS_COUNT == 24,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    static_assert(FILE_TYPE_COUNT == 7, "exhaustive handling of file types");
    switch (get_file_type(curr_opt)) {
        case FILE_TYPE_OWN:
            if (is_compiling()) {
                msg_todo("multiple .own files specified on the command line", POS_BUILTIN);
                exit(EXIT_CODE_FAIL);
            }
            stop_after_set_if_none(STOP_AFTER_BIN);
            params.compile_own = true;
            params.input_file_path = curr_opt;
            break;
        case FILE_TYPE_STATIC_LIB:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.static_libs, curr_opt);
            break;
        case FILE_TYPE_DYNAMIC_LIB:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.dynamic_libs, curr_opt);
            break;
        case FILE_TYPE_C:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.c_input_files, curr_opt);
            break;
        case FILE_TYPE_OBJECT:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.object_files, curr_opt);
            break;
        case FILE_TYPE_LOWER_S:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.lower_s_files, curr_opt);
            break;
        case FILE_TYPE_UPPER_S:
            stop_after_set_if_none(STOP_AFTER_BIN);
            vec_append(&a_main, &params.upper_s_files, curr_opt);
            break;
        default:
            unreachable("");
    }
}

static void set_backend(BACKEND backend) {
    switch (backend) {
        case BACKEND_C:
            params.backend_info.backend = BACKEND_C;
            params.backend_info.struct_rtn_through_param = false;
            return;
        case BACKEND_LLVM:
            params.backend_info.backend = BACKEND_LLVM;
            params.backend_info.struct_rtn_through_param = true;
            return;
        case BACKEND_NONE:
            unreachable("");
    }
    unreachable("");
}

typedef void(*Long_option_action)(Strv curr_opt);

typedef struct {
    const char* text;
    const char* description;
    Long_option_action action;
    bool arg_expected; // TODO: instead of bool, use enum to specify whether `=` is used, etc.
} Long_option_pair;

static void long_option_help(Strv curr_opt) {
    (void) curr_opt;
    print_usage();
    exit(EXIT_CODE_SUCCESS);
}

static void long_option_l(Strv curr_opt) {
    vec_append(&a_main, &params.l_flags, curr_opt);
}

static void long_option_backend(Strv curr_opt) {
    Strv backend = curr_opt;
    if (!strv_try_consume(&backend, '=') || backend.count < 1) {
        log(LOG_FATAL, "expected =<backend> after `backend`");
        exit(EXIT_CODE_FAIL);
    }

    if (strv_is_equal(backend, sv("c"))) {
        set_backend(BACKEND_C);
    } else if (strv_is_equal(backend, sv("llvm"))) {
        set_backend(BACKEND_LLVM);
    } else {
        log(LOG_FATAL, "backend `"FMT"` is not a supported backend\n", strv_print(backend));
        exit(EXIT_CODE_FAIL);
    }
}

static void long_option_all_errors_fetal(Strv curr_opt) {
    (void) curr_opt;
    params.all_errors_fatal = true;
}

static void long_option_dump_backend_ir(Strv curr_opt) {
    (void) curr_opt;
    params.stop_after = STOP_AFTER_GEN_BACKEND_IR;
}

static void long_option_dump_ir(Strv curr_opt) {
    (void) curr_opt;
    params.stop_after = STOP_AFTER_GEN_IR;
}

static void long_option_upper_s(Strv curr_opt) {
    (void) curr_opt;
    params.stop_after = STOP_AFTER_LOWER_S;
}

static void long_option_upper_c(Strv curr_opt) {
    (void) curr_opt;
    params.stop_after = STOP_AFTER_OBJ;
}

static void long_option_dump_dot(Strv curr_opt) {
    msg_todo("dump_dot", POS_BUILTIN);
    (void) curr_opt;
    //params.stop_after = STOP_AFTER_GEN_IR;
    //params.dump_dot = true;
}

static void long_option_run(Strv curr_opt) {
    (void) curr_opt;
    static_assert(
        PARAMETERS_COUNT == 24,
        "exhausive handling of params for if statement below "
        "(not all parameters are explicitly handled)"
    );
    if (params.stop_after == STOP_AFTER_NONE) {
        log(LOG_FATAL, "file to be compiled must be specified prior to `--run` argument\n");
        exit(EXIT_CODE_FAIL);
    }
    if (!is_compiling()) {
        log(
            LOG_FATAL,
            "`--run` option cannot be used when generating intermediate files (eg. .s files)\n"
        );
        exit(EXIT_CODE_FAIL);
    }
    params.stop_after = STOP_AFTER_RUN;
}

static void long_option_lower_o(Strv curr_opt) {
    params.is_output_file_path = true;
    params.output_file_path = curr_opt;
}

static void long_option_upper_o0(Strv curr_opt) {
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_O0;
}

static void long_option_upper_o2(Strv curr_opt) {
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_O2;
}

static void long_option_error(Strv curr_opt) {
    Strv error = curr_opt;
    if (!strv_try_consume(&error, '=') || error.count < 1) {
        log(LOG_FATAL, "expected =<error1[,error2,...]> after `error`\n");
        exit(EXIT_CODE_FAIL);
    }

    DIAG_TYPE type = {0};
    size_t idx = 0;
    if (!expect_fail_type_from_strv(&idx, &type, error)) {
        msg(
            DIAG_INVALID_FAIL_TYPE, POS_BUILTIN,
            "invalid fail type `"FMT"`\n", strv_print(error)
        );
        exit(EXIT_CODE_FAIL);
    }
    array_at_ref(expect_fail_str_to_curr_log_level_pair, idx)->curr_level = LOG_ERROR;
    params.error_opts_changed = true;
}

static void long_option_path_c_compiler(Strv curr_opt) {
    Strv cc = curr_opt;
    if (!strv_try_consume(&cc, '=') || cc.count < 1) {
        log(LOG_FATAL, "expected =<c_compiler_path> after `path_c_compiler`\n");
        exit(EXIT_CODE_FAIL);
    }

    params.path_c_compiler = cc;
    params.is_path_c_compiler = true;
}

static void long_option_target_triplet(Strv curr_opt) {
    Strv cc = curr_opt;
    if (!strv_try_consume(&cc, '=') || cc.count < 1) {
        log(LOG_FATAL, "expected =<target-triplet> after `target-triplet`\n");
        exit(EXIT_CODE_FAIL);
    }

    Strv temp = {0};

    if (!strv_try_consume_until(&temp, &cc, '-')) {
        log(LOG_FATAL, "architecture was not specified in target-triplet\n");
        exit(EXIT_CODE_FAIL);
    }
    strv_consume(&cc);
    if (!try_target_arch_from_strv(&params.target_triplet.arch, temp)) {
        log(LOG_FATAL, "unsupported architecture `"FMT"`\n", strv_print(temp));
        exit(EXIT_CODE_FAIL);
    }

    if (!strv_try_consume_until(&temp, &cc, '-')) {
        log(LOG_FATAL, "operating system was not specified in target-triplet\n");
        exit(EXIT_CODE_FAIL);
    }
    strv_consume(&cc);
    if (!try_target_vendor_from_strv(&params.target_triplet.vendor, temp)) {
        log(LOG_FATAL, "unsupported vendor `"FMT"`\n", strv_print(temp));
        exit(EXIT_CODE_FAIL);
    }

    if (!strv_try_consume_until(&temp, &cc, '-')) {
        log(LOG_FATAL, "operating system was not specified in target-triplet\n");
        exit(EXIT_CODE_FAIL);
    }
    strv_consume(&cc);
    if (!try_target_os_from_strv(&params.target_triplet.os, temp)) {
        log(LOG_FATAL, "unsupported operating system `"FMT"`\n", strv_print(temp));
        exit(EXIT_CODE_FAIL);
    }

    Strv dummy = {0};
    if (strv_try_consume_until(&dummy, &cc, '-')) {
        log(LOG_FATAL, "target triplet has too many sub-strings\n");
        exit(EXIT_CODE_FAIL);
    }
    temp = cc;
    if (temp.count < 1) {
        log(LOG_FATAL, "abi (application binary interface, a.k.a. environment type) was not specified in target-triplet\n");
        exit(EXIT_CODE_FAIL);
    }
    if (!try_target_abi_from_strv(&params.target_triplet.abi, temp)) {
        log(LOG_FATAL, "unsupported abi (application binary interface, a.k.a. environment type) `"FMT"`\n", strv_print(temp));
        exit(EXIT_CODE_FAIL);
    }
}

static void long_option_no_prelude(Strv curr_opt) {
    (void) curr_opt;
    params.do_prelude = false;
}

static void long_option_log_level(Strv curr_opt) {
    Strv log_level = curr_opt;
    if (!strv_try_consume(&log_level, '=')) {
        log(LOG_FATAL, "expected =<log level> after `log-level`");
        exit(EXIT_CODE_FAIL);
    }

    if (strv_is_equal(log_level, sv("FETAL"))) {
        params_log_level = LOG_FATAL;
    } else if (strv_is_equal(log_level, sv("ERROR"))) {
        params_log_level = LOG_ERROR;
    } else if (strv_is_equal(log_level, sv("WARNING"))) {
        params_log_level = LOG_WARNING;
    } else if (strv_is_equal(log_level, sv("NOTE"))) {
        params_log_level = LOG_NOTE;
    } else if (strv_is_equal(log_level, sv("INFO"))) {
        params_log_level = LOG_INFO;
    } else if (strv_is_equal(log_level, sv("VERBOSE"))) {
        params_log_level = LOG_VERBOSE;
    } else if (strv_is_equal(log_level, sv("DEBUG"))) {
        params_log_level = LOG_DEBUG;
    } else if (strv_is_equal(log_level, sv("TRACE"))) {
        params_log_level = LOG_TRACE;
    } else {
        log(LOG_FATAL, "log level `"FMT"` is not a supported log level\n", strv_print(log_level));
        exit(EXIT_CODE_FAIL);
    }
}

static_assert(
    PARAMETERS_COUNT == 24,
    "exhausive handling of params (not all parameters are explicitly handled)"
);
Long_option_pair long_options[] = {
    {"help", "display usage", long_option_help, false},
    {"l", "library name to link", long_option_l, true},
    {"backend", "c or llvm", long_option_backend, true},
    {"all-errors-fetal", "stop immediately after an error occurs", long_option_all_errors_fetal, false},
    {"dump-ir", "stop compiling after IR file(s) have been generated", long_option_dump_ir, false},
    {"dump-backend-ir", "stop compiling after .c file(s) or .ll file(s) have been generated", long_option_dump_backend_ir, false},
    {"S", "stop compiling after assembly file(s) have been generated", long_option_upper_s, false},
    {"c", "stop compiling after object file(s) have been generated", long_option_upper_c, false},
    {"dump-dot", "stop compiling after IR file(s) have been generated, and dump .dot file(s)", long_option_dump_dot, false},
    {"o", "output file path", long_option_lower_o, true},
    {"O0", "disable most optimizations", long_option_upper_o0, false},
    {"O2", "enable optimizations", long_option_upper_o2, false},
    {"error", "TODO", long_option_error, true},
    {
        "target-triplet",
        "=ARCH-VENDOR-OS-ABI    (eg. \"target-triplet=x86_64-unknown-linux-gnu\"",
        long_option_target_triplet,
        true
    },
    {
        "path-c-compiler",
        "specify the c compiler to use to compile program",
        long_option_path_c_compiler,
        true
    },
    {"no-prelude", "disable the prelude (std::prelude)", long_option_no_prelude, false},
    {
        "set-log-level",
        "=OPT where OPT is "
          "\"FETAL\", \"ERROR\", \"WARNING\", \"NOTE\", \"INFO\", \"VERBOSE\", \"DEBUG\", or \"TRACE\" ("
          "eg. \"set-log-level=NOTE\" will suppress messages that are less important than \"NOTE\")",
        long_option_log_level,
        true
    },

    // run should be at the bottom for now
    // TODO: consider moving run elsewhere, because run is not a regular option
    {"run", "compile and run the program (NOTE: arguments after `--run` are passed to the program, and are not interpreted as build options)", long_option_run, false},
};

static void parse_long_option(int* argc, char*** argv) {
    Strv curr_opt = consume_arg(argc, argv, sv("arg expected"));

    for (size_t idx = 0; idx < array_count(long_options); idx++) {
        Long_option_pair curr = array_at(long_options, idx);
        if (strv_starts_with(curr_opt, sv(curr.text))) {
            strv_consume_count(&curr_opt, sv(curr.text).count);
            if (curr.arg_expected && curr_opt.count < 1) {
                String buf = {0};
                string_extend_strv(&a_print, &buf, sv("argument expected after `"));
                string_extend_strv(&a_print, &buf, sv(curr.text));
                string_extend_strv(&a_print, &buf, sv("`"));
                curr.action(consume_arg(argc, argv, string_to_strv(buf)));
            } else {
                curr.action(curr_opt);
            }
            return;
        }
    }

    log(LOG_FATAL, "invalid option: "FMT"\n", strv_print(curr_opt));
    exit(EXIT_CODE_FAIL);
}

static_assert(
    PARAMETERS_COUNT == 24,
    "exhausive handling of params (not all parameters are explicitly handled)"
);
static void set_params_to_defaults(void) {
    set_backend(BACKEND_C);
    params.do_prelude = true;
    params.target_triplet = get_default_target_triplet();
}

static void print_usage(void) {
    log(LOG_DEBUG, "%d\n", params_log_level);
    msg(DIAG_INFO, POS_BUILTIN, "usage:\n");
    msg(DIAG_INFO, POS_BUILTIN, "    "FMT" <files> [options] [--run [subprocess arguments]]\n", strv_print(compiler_exe_name));
    msg(DIAG_INFO, POS_BUILTIN, "\n");
    msg(DIAG_INFO, POS_BUILTIN, "options:\n");
    for (size_t idx = 0; idx < array_count(long_options); idx++) {
        Long_option_pair curr = array_at(long_options, idx);
        msg(DIAG_INFO, POS_BUILTIN, "    -"FMT"\n", strv_print(sv(curr.text)));
        msg(DIAG_INFO, POS_BUILTIN, "        "FMT"\n", strv_print(sv(curr.description)));
        msg(DIAG_INFO, POS_BUILTIN, "\n");
    }
}

void parse_args(int argc, char** argv) {
    set_params_to_defaults();
    expect_fail_str_to_curr_log_level_init();

#   ifndef NDEBUG
        for (size_t last = 0; last < array_count(long_options); last++) {
            for (size_t first = 0; first < last; first++) {
                if (first == last) {
                    continue;
                }

                Strv first_str = sv(array_at(long_options, first).text);
                Strv last_str = sv(array_at(long_options, last).text);
                if (strv_starts_with(first_str, last_str) || strv_starts_with(last_str, first_str)) {
                    log(LOG_FATAL, "options with indices %zu and %zu have overlapping names\n", first, last);
                    exit(EXIT_CODE_FAIL);
                }
            }
        }
#   endif // NDEBUG

    // consume compiler executable name
    compiler_exe_name = consume_arg(&argc, &argv, sv("internal error"));

    while (argc > 0) {
        if (is_short_option(argv) || is_long_option(argv)) {
            parse_long_option(&argc, &argv);
        } else {
            parse_file_option(&argc, &argv);
        }
    }

    static_assert(
        PARAMETERS_COUNT == 24,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    if (
        !params.compile_own &&
        params.static_libs.info.count == 0 &&
        params.c_input_files.info.count == 0 &&
        params.object_files.info.count == 0 &&
        params.lower_s_files.info.count == 0 &&
        params.upper_s_files.info.count == 0
    ) {
        log(LOG_DEBUG, "%d\n", params_log_level);
        print_usage();
        msg(DIAG_NO_INPUT_FILES, POS_BUILTIN, "no input files were provided\n");
        exit(EXIT_CODE_FAIL);
    }

    if (params.compile_own && (params.stop_after == STOP_AFTER_BIN || params.stop_after == STOP_AFTER_RUN)) {
        vec_append(&a_main, &params.c_input_files, sv("std/util.c"));
    }

    // set default output file path
    if (params.output_file_path.count < 1) {
        static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop after states");
        switch (params.stop_after) {
            case STOP_AFTER_NONE:
                unreachable("");
                // TODO: rename STOP_AFTER_GEN_IR to STOP_AFTER_IR
            case STOP_AFTER_GEN_IR:
                if (params.dump_dot) {
                    params.output_file_path = sv("test.dot");
                } else {
                    params.output_file_path = sv("test.ownir");
                }
                break;
                // TODO: rename STOP_AFTER_GEN_BACKEND_IR to STOP_AFTER_BACKEND_IR
            case STOP_AFTER_GEN_BACKEND_IR:
                params.output_file_path = sv("test.c");
                break;
                // TODO: rename STOP_AFTER_LOWER_S to STOP_AFTER_UPPER_S
            case STOP_AFTER_LOWER_S:
                params.output_file_path = sv("test.s");
                break;
            case STOP_AFTER_OBJ:
                params.output_file_path = sv("test.o");
                break;
            case STOP_AFTER_BIN:
                // fallthrough
            case STOP_AFTER_RUN:
                params.output_file_path = sv("a.out");
                break;
            case STOP_AFTER_COUNT:
                unreachable("");
            default:
                unreachable("");
        }
    } else {
        assert(params.is_output_file_path);
        size_t count_files_compile = 
            params.c_input_files.info.count +
            params.lower_s_files.info.count +
            params.upper_s_files.info.count;
        if (params.compile_own) {
            count_files_compile++;
        }
        if (
            params.stop_after != STOP_AFTER_RUN &&
            params.stop_after != STOP_AFTER_BIN &&
            count_files_compile > 1
        ) {
            log(LOG_FATAL, "-o <file path> option cannot be used when compiling multiple files to an intermediate step\n");
            exit(EXIT_CODE_FAIL);
        }
    }

    Arch_row arch_row = get_arch_row_from_arch(params.target_triplet.arch);
    params.sizeof_usize = arch_row.sizeof_usize;
    params.sizeof_ptr_non_fn = arch_row.sizeof_ptr_non_fn;
    unwrap(
        (size_t)snprintf(params.usize_size_ux, array_count(params.usize_size_ux), "u%u", params.sizeof_usize) <
        array_count(params.usize_size_ux) &&
        "the buffer (params.usize_size_ux) is too small"
    );
    params.sizeof_usize = arch_row.sizeof_usize;
}

