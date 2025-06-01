#include <parameters.h>
#include <parser_utils.h>
#include <file.h>

Str_view stop_after_print_internal(STOP_AFTER stop_after) {
    switch (stop_after) {
        case STOP_AFTER_NONE:
            return sv("none");
        case STOP_AFTER_GEN_IR:
            return sv("gen_ir");
        case STOP_AFTER_EMIT_BACKEND_IR:
            return sv("emit_backend_ir");
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

// TODO: remove this function?
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
static Str_view consume_arg(int* argc, char*** argv, const char* msg_if_missing) {
    if (*argc < 1) {
        msg(DIAG_MISSING_COMMAND_LINE_ARG, POS_BUILTIN, "%s\n", msg_if_missing);
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

    return consume_arg(argc, argv, "stray - or -- is not permitted");
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

static_assert(DIAG_COUNT == 61, "exhaustive handling of expected fail types");
static const Expect_fail_pair expect_fail_pair[] = {
    {"note", DIAG_NOTE, LOG_NOTE, false},
    {"file-built", DIAG_FILE_BUILT, LOG_VERBOSE, false},
    {"missing-command-line-arg", DIAG_MISSING_COMMAND_LINE_ARG, LOG_ERROR, true},
    {"file-could-not-open", DIAG_FILE_COULD_NOT_OPEN, LOG_ERROR, true},
    {"missing-close-double-quote", DIAG_MISSING_CLOSE_DOUBLE_QUOTE, LOG_ERROR, true},
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
    {"enum-case-too-many-args", DIAG_ENUM_CASE_TOO_MANY_ARGS, LOG_ERROR, true},
    {"void-enum-case-has-arg", DIAG_VOID_ENUM_CASE_HAS_ARG, LOG_ERROR, true},
    {"invalid-stmt-top-level", DIAG_INVALID_STMT_TOP_LEVEL, LOG_ERROR, true},
    {"invalid-function-callee", DIAG_INVALID_FUNCTION_CALLEE, LOG_ERROR, true},
    {"optional-args-for-variadic-args", DIAG_OPTIONAL_ARGS_FOR_VARIADIC_ARGS, LOG_ERROR, true},
    {"fail-invalid-fail-type", DIAG_INVALID_FAIL_TYPE, LOG_ERROR, false},
    {"no-main-function", DIAG_NO_MAIN/*TODO: rename this to match string*/, LOG_WARNING, false},
    {"struct-like-recursion", DIAG_STRUCT_LIKE_RECURSION, LOG_ERROR, true},
    {"child-process-failure", DIAG_CHILD_PROCESS_FAILURE, LOG_FATAL, true},
    {"no-input-files", DIAG_NO_INPUT_FILES, LOG_FATAL, true},
};

// error types are in the same order in expect_fail_str_to_curr_log_level_pair and expect_fail_pair
// TODO: make these names less verbose?
static Expect_fail_str_to_curr_log_level 
expect_fail_str_to_curr_log_level_pair[sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0])] = {0};

bool expect_fail_type_from_strv(size_t* idx_result, DIAG_TYPE* type, Str_view strv) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        if (str_view_is_equal(sv(expect_fail_pair[idx].str), strv)) {
            *type = expect_fail_pair[idx].type;
            *idx_result = idx;
            return true;
        }
    }
    return false;
}

static size_t expect_fail_type_get_idx(DIAG_TYPE type) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        if (expect_fail_pair[idx].type == type) {
            return idx;
        }
    }
    unreachable("expect_fail_pair does not cover this DIAG_TYPE");
}

Str_view expect_fail_type_print_internal(DIAG_TYPE type) {
    return sv(expect_fail_pair[expect_fail_type_get_idx(type)].str);
}

static void expect_fail_str_to_curr_log_level_init(void) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        expect_fail_str_to_curr_log_level_pair[idx].str = expect_fail_pair[idx].str;
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = expect_fail_pair[idx].default_level;
    }
}

LOG_LEVEL expect_fail_type_to_curr_log_level(DIAG_TYPE type) {
    return expect_fail_str_to_curr_log_level_pair[expect_fail_type_get_idx(type)].curr_level;
}

static void parse_normal_option(int* argc, char*** argv) {
    Str_view curr_opt = consume_arg(argc, argv, "arg expected");

    static_assert(
        PARAMETERS_COUNT == 17,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );

    if (str_view_is_equal(curr_opt, sv("dump-dot"))) {
        params.dump_dot = true;
        params.input_file_path = consume_arg(argc, argv, "input file path was expected after `compile-run`");
    } else {
        static_assert(FILE_TYPE_COUNT == 6, "exhaustive handling of file types");
        switch (get_file_type(curr_opt)) {
            case FILE_TYPE_OWN:
                if (is_compiling()) {
                    msg_todo("multiple .own files specified on the command line", POS_BUILTIN);
                    exit(EXIT_CODE_FAIL);
                }
                params.compile_own = true;
                if (params.stop_after == STOP_AFTER_NONE) {
                    params.stop_after = STOP_AFTER_BIN;
                }
                params.input_file_path = curr_opt;
                break;
            case FILE_TYPE_STATIC_LIB:
                if (is_compiling()) {
                    // TODO
                    todo();
                }
                vec_append(&a_main, &params.static_libs, curr_opt);
                break;
            case FILE_TYPE_DYNAMIC_LIB:
                if (is_compiling()) {
                    // TODO
                    todo();
                }
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
            default:
                unreachable("");
        }
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

// TODO: allow `-ooutput` as well as `-o output`
static void parse_long_option(int* argc, char*** argv) {
    Str_view curr_opt = consume_arg(argc, argv, "arg expected");

    static_assert(
        PARAMETERS_COUNT == 17,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );

    if (str_view_is_equal(curr_opt, sv("emit-llvm"))) {
        params.emit_llvm = true;
    } else if (str_view_is_equal(curr_opt, sv("l"))) {
        vec_append(&a_main, &params.l_flags, consume_arg(argc, argv, "library name was expected after `-l`"));
    } else if (str_view_starts_with(curr_opt, sv("backend"))) {
        Str_view backend = curr_opt;
        str_view_consume_count(&backend, sv("backend").count);
        if (!str_view_try_consume(&backend, '=') || backend.count < 1) {
            log(LOG_FATAL, "expected =<backend> after `backend`");
            exit(EXIT_CODE_FAIL);
        }

        if (str_view_is_equal(backend, sv("c"))) {
            set_backend(BACKEND_C);
        } else if (str_view_is_equal(backend, sv("llvm"))) {
            set_backend(BACKEND_LLVM);
        } else {
            log(LOG_FATAL, "backend `"STR_VIEW_FMT"` is not a supported backend\n", str_view_print(backend));
            exit(EXIT_CODE_FAIL);
        }
    } else if (str_view_is_equal(curr_opt, sv("all-errors-fatal"))) {
        params.all_errors_fatal = true;
    } else if (str_view_is_equal(curr_opt, sv("S"))) {
        params.stop_after = STOP_AFTER_LOWER_S;
    } else if (str_view_is_equal(curr_opt, sv("c"))) {
        params.stop_after = STOP_AFTER_OBJ;
    } else if (str_view_is_equal(curr_opt, sv("run"))) {
        static_assert(
            PARAMETERS_COUNT == 17,
            "exhausive handling of params for if statement below "
            "(not all parameters are explicitly handled)"
        );
        // TODO: make enum for dump_lower_s, compile, etc.
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
        // TODO: args after `--run` should be passed to the program being compiled and run
    } else if (str_view_is_equal(curr_opt, sv("o"))) {
        params.output_file_path = consume_arg(argc, argv, "output file path was expected after `-o`");
    } else if (str_view_is_equal(curr_opt, sv("O0"))) {
        params.opt_level = OPT_LEVEL_O0;
    } else if (str_view_is_equal(curr_opt, sv("O2"))) {
        params.opt_level = OPT_LEVEL_O2;
    } else if (str_view_starts_with(curr_opt, sv("error"))) {
        Str_view error = curr_opt;
        str_view_consume_count(&error, sv("error").count);
        if (!str_view_try_consume(&error, '=') || error.count < 1) {
            log(LOG_FATAL, "expected <=error1[,error2,...]> after `error`");
            exit(EXIT_CODE_FAIL);
        }

        DIAG_TYPE type = {0};
        size_t idx = 0;
        if (!expect_fail_type_from_strv(&idx, &type, error)) {
            msg(
                DIAG_INVALID_FAIL_TYPE, POS_BUILTIN,
                "invalid fail type `"STR_VIEW_FMT"`\n", str_view_print(error)
            );
            exit(EXIT_CODE_FAIL);
        }
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = LOG_ERROR;
        params.error_opts_changed = true;
    } else if (str_view_starts_with(curr_opt, sv("log-level"))) {
        Str_view log_level = curr_opt;
        str_view_consume_count(&log_level, sv("log-level").count);
        if (!str_view_try_consume(&log_level, '=')) {
            log(LOG_FATAL, "expected =<log level> after `log-level`");
            exit(EXIT_CODE_FAIL);
        }

        if (str_view_is_equal(log_level, sv("FETAL"))) {
            params_log_level = LOG_FATAL;
        } else if (str_view_is_equal(log_level, sv("ERROR"))) {
            params_log_level = LOG_ERROR;
        } else if (str_view_is_equal(log_level, sv("WARNING"))) {
            params_log_level = LOG_WARNING;
        } else if (str_view_is_equal(log_level, sv("NOTE"))) {
            params_log_level = LOG_NOTE;
        } else if (str_view_is_equal(log_level, sv("VERBOSE"))) {
            params_log_level = LOG_VERBOSE;
        } else if (str_view_is_equal(log_level, sv("DEBUG"))) {
            params_log_level = LOG_DEBUG;
        } else if (str_view_is_equal(log_level, sv("TRACE"))) {
            params_log_level = LOG_TRACE;
        } else {
            log(LOG_FATAL, "log level `"STR_VIEW_FMT"` is not a supported log level\n", str_view_print(log_level));
            exit(EXIT_CODE_FAIL);
        }
    } else {
        log(LOG_FATAL, "invalid option: "STR_VIEW_FMT"\n", str_view_print(curr_opt));
        exit(EXIT_CODE_FAIL);
    }
}

static void set_params_to_defaults(void) {
    params.opt_level = OPT_LEVEL_O0;
    params.emit_llvm = false;
    params.output_file_path = sv("a.out");

    set_backend(BACKEND_C);
}

void parse_args(int argc, char** argv) {
    set_params_to_defaults();
    expect_fail_str_to_curr_log_level_init();

    // consume compiler executable name
    consume_arg(&argc, &argv, "internal error");

    while (argc > 0) {
        if (is_short_option(argv)) {
            parse_long_option(&argc, &argv);
        } else if (is_long_option(argv)) {
            parse_long_option(&argc, &argv);
        } else {
            parse_normal_option(&argc, &argv);
        }
    }

    static_assert(
        PARAMETERS_COUNT == 17,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    if (
        !params.compile_own &&
        params.static_libs.info.count == 0 &&
        params.c_input_files.info.count == 0 &&
        params.object_files.info.count == 0 &&
        params.lower_s_files.info.count == 0
    ) {
        msg(DIAG_NO_INPUT_FILES, POS_BUILTIN, "no input files were provided\n");
        exit(EXIT_CODE_FAIL);
    }
}

