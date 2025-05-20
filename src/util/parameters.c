#include "parameters.h"
#include "parser_utils.h"

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
static const char* consume_arg(int* argc, char*** argv, const char* msg_if_missing) {
    if (*argc < 1) {
        msg(DIAG_MISSING_COMMAND_LINE_ARG, dummy_pos, "%s\n", msg_if_missing);
        exit(EXIT_CODE_FAIL);
    }
    const char* curr_arg = argv[0][0];
    while (curr_arg[0] && curr_arg[0] == '-') {
        curr_arg++;
    }
    (*argv)++;
    (*argc)--;

    if (curr_arg[0]) {
        return curr_arg;
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

static_assert(DIAG_COUNT == 60, "exhaustive handling of expected fail types");
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
};

// error types are in the same order in expect_fail_str_to_curr_log_level_pair and expect_fail_pair
// TODO: make these names less verbose?
static Expect_fail_str_to_curr_log_level 
expect_fail_str_to_curr_log_level_pair[sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0])] = {0};

bool expect_fail_type_from_strv(size_t* idx_result, DIAG_TYPE* type, Str_view strv) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        if (str_view_is_equal(str_view_from_cstr(expect_fail_pair[idx].str), strv)) {
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
    return str_view_from_cstr(expect_fail_pair[expect_fail_type_get_idx(type)].str);
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
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "compile")) {
        params.compile = true;
        params.input_file_name = consume_arg(argc, argv, "input file path was expected after `compile`");
    } else if (0 == strcmp(curr_opt, "compile-run")) {
        params.compile = true;
        params.run = true;
        params.input_file_name = consume_arg(argc, argv, "input file path was expected after `compile`");
    } else if (0 == strcmp(curr_opt, "test-expected-fail")) {
        params.compile = false;
        params.test_expected_fail = true;

        const char* count_args_cstr = consume_arg(argc, argv, "count expected");
        size_t count_args = SIZE_MAX;
        if (!try_str_view_to_size_t(&count_args, str_view_from_cstr(count_args_cstr))) {
            todo();
        }
        assert(count_args < SIZE_MAX && "count_args unset");

        bool found = false;
        for (size_t idx = 0; idx < count_args; idx++) {
            const char* diag_type_str = consume_arg(
                argc, argv, "expected fail type expected after `test_expected_fail`"
            );

            for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
                if (0 == strcmp(diag_type_str, expect_fail_pair[idx].str)) {
                    found = true;
                    vec_append(&a_main, &params.diag_types, expect_fail_pair[idx].type);
                    break;
                }
            }

            if (!found) {
                log(LOG_FATAL, "invalid expected fail type `%s`\n", diag_type_str);
                exit(EXIT_CODE_FAIL);
            }
            assert(params.diag_types.info.count > 0);
        }

        params.input_file_name = consume_arg(
            argc, argv, "input file path was expected after `test_expected_fail <fail type>`"
        );
    } else {
        log(LOG_FATAL, "invalid option: %s\n", curr_opt);
        exit(EXIT_CODE_FAIL);
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

static void parse_long_option(int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "emit-llvm")) {
        params.emit_llvm = true;
    } else if (0 == strncmp(curr_opt, "backend", strlen("backend"))) {
        Str_view backend = str_view_from_cstr(&curr_opt[strlen("backend")]);
        if (!str_view_try_consume(&backend, '=') || backend.count < 1) {
            log(LOG_FATAL, "expected =<backend> after `backend`");
            exit(EXIT_CODE_FAIL);
        }

        if (str_view_is_equal(backend, str_view_from_cstr("c"))) {
            set_backend(BACKEND_C);
        } else if (str_view_is_equal(backend, str_view_from_cstr("llvm"))) {
            set_backend(BACKEND_LLVM);
        } else {
            log(LOG_FATAL, "backend `"STR_VIEW_FMT"` is not a supported backend\n", str_view_print(backend));
            exit(EXIT_CODE_FAIL);
        }
    } else if (0 == strcmp(curr_opt, "all-errors-fatal")) {
        params.all_errors_fatal = true;
    } else if (0 == strncmp(curr_opt, "error", strlen("error"))) {
        Str_view error = str_view_from_cstr(&curr_opt[strlen("error")]);
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
    } else if (0 == strncmp(curr_opt, "log-level", strlen("log-level"))) {
        Str_view log_level = str_view_from_cstr(&curr_opt[strlen("log-level")]);
        if (!str_view_try_consume(&log_level, '=')) {
            log(LOG_FATAL, "expected =<log level> after `log-level`");
            exit(EXIT_CODE_FAIL);
        }
        if (str_view_is_equal(log_level, str_view_from_cstr("FETAL"))) {
            params_log_level = LOG_FATAL;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("ERROR"))) {
            params_log_level = LOG_ERROR;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("WARNING"))) {
            params_log_level = LOG_WARNING;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("NOTE"))) {
            params_log_level = LOG_NOTE;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("NOTE"))) {
            params_log_level = LOG_NOTE;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("VERBOSE"))) {
            params_log_level = LOG_VERBOSE;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("DEBUG"))) {
            params_log_level = LOG_DEBUG;
        } else if (str_view_is_equal(log_level, str_view_from_cstr("TRACE"))) {
            params_log_level = LOG_TRACE;
        } else {
            log(LOG_FATAL, "log level `"STR_VIEW_FMT"` is not a supported log level\n", str_view_print(log_level));
            exit(EXIT_CODE_FAIL);
        }
    } else {
        log(LOG_FATAL, "invalid option: %s\n", curr_opt);
        exit(EXIT_CODE_FAIL);
    }
}

static void set_params_to_defaults(void) {
    params.emit_llvm = false;
    params.compile = false;

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
}

