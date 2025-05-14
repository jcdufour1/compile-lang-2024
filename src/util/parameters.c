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
        msg(LOG_FATAL, EXPECT_FAIL_MISSING_COMMAND_LINE_ARG, dummy_pos, "%s\n", msg_if_missing);
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
    EXPECT_FAIL_TYPE type;
    LOG_LEVEL default_level;
    bool must_be_error;
} Expect_fail_pair;

typedef struct {
    const char* str;
    LOG_LEVEL curr_level;
} Expect_fail_str_to_curr_log_level;

// TODO: uncomment static_assert
static_assert(EXPECT_FAIL_COUNT == 58, "exhaustive handling of expected fail types");
static const Expect_fail_pair expect_fail_pair[] = {
    {"note", EXPECT_FAIL_NOTE, LOG_NOTE, false},
    {"file-built", EXPECT_FAIL_FILE_BUILT, LOG_VERBOSE, false},
    {"missing-command-line-arg", EXPECT_FAIL_MISSING_COMMAND_LINE_ARG, LOG_ERROR, true},
    {"file-could-not-open", EXPECT_FAIL_FILE_COULD_NOT_OPEN, LOG_ERROR, true},
    {"missing-close-double-quote", EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE, LOG_ERROR, true},
    {"no-new-line-after-statement", EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT, LOG_ERROR, true},
    {"missing-close-par", EXPECT_FAIL_MISSING_CLOSE_PAR, LOG_ERROR, true},
    {"missing-close-curly-brace", EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE, LOG_ERROR, true},
    {"invalid-extern-type", EXPECT_FAIL_INVALID_EXTERN_TYPE, LOG_ERROR, true},
    {"invalid-token", EXPECT_FAIL_INVALID_TOKEN, LOG_ERROR, true},
    {"parser-expected", EXPECT_FAIL_PARSER_EXPECTED, LOG_ERROR, true},
    {"redefinition-of-symbol", EXPECT_FAIL_REDEFINITION_SYMBOL, LOG_ERROR, true},
    {"invalid-struct-member-in-literal", EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL, LOG_ERROR, true},
    {"invalid-member-access", EXPECT_FAIL_INVALID_MEMBER_ACCESS, LOG_ERROR, true},
    {"missing-return-statement", EXPECT_FAIL_MISSING_RETURN, LOG_ERROR, true},
    {"invalid-count-fun-args", EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, LOG_ERROR, true},
    {"invalid-function-arg", EXPECT_FAIL_INVALID_FUN_ARG, LOG_ERROR, true},
    {"mismatched-return-type", EXPECT_FAIL_MISMATCHED_RETURN_TYPE, LOG_ERROR, true},
    {"mismatched-yield-type", EXPECT_FAIL_MISMATCHED_YIELD_TYPE, LOG_ERROR, true},
    {"assignment-mismatched-types", EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, LOG_ERROR, true},
    {"unary-mismatched-types", EXPECT_FAIL_UNARY_MISMATCHED_TYPES, LOG_ERROR, true},
    {"binary-mismatched-types", EXPECT_FAIL_BINARY_MISMATCHED_TYPES, LOG_ERROR, true},
    {"expected-expression", EXPECT_FAIL_EXPECTED_EXPRESSION, LOG_ERROR, true},
    {"undefined-symbol", EXPECT_FAIL_UNDEFINED_SYMBOL, LOG_ERROR, true},
    {"undefined-function", EXPECT_FAIL_UNDEFINED_FUNCTION, LOG_ERROR, true},
    {"struct-init-on-raw-union", EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, LOG_ERROR, true},
    {"struct-init-on-sum", EXPECT_FAIL_STRUCT_INIT_ON_SUM, LOG_ERROR, true},
    {"struct-init-on-primitive", EXPECT_FAIL_STRUCT_INIT_ON_PRIMITIVE, LOG_ERROR, true},
    {"undefined-type", EXPECT_FAIL_UNDEFINED_TYPE, LOG_ERROR, true},
    {"missing-close-sq-bracket", EXPECT_FAIL_MISSING_CLOSE_SQ_BRACKET, LOG_ERROR, true},
    {"missing-close-generic", EXPECT_FAIL_MISSING_CLOSE_GENERIC, LOG_ERROR, true},
    {"deref_non_pointer", EXPECT_FAIL_DEREF_NON_POINTER, LOG_ERROR, true},
    {"break-invalid-location", EXPECT_FAIL_BREAK_INVALID_LOCATION, LOG_ERROR, true},
    {"continue-invalid-location", EXPECT_FAIL_CONTINUE_INVALID_LOCATION, LOG_ERROR, true},
    {"mismatched-tuple-count", EXPECT_FAIL_MISMATCHED_TUPLE_COUNT, LOG_ERROR, true},
    {"non-exhaustive-switch", EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH, LOG_ERROR, true},
    {"sum-lit-invalid-arg", EXPECT_FAIL_SUM_LIT_INVALID_ARG, LOG_ERROR, true},
    {"not-yet-implemented", EXPECT_FAIL_NOT_YET_IMPLEMENTED, LOG_ERROR, true},
    {"duplicate-default", EXPECT_FAIL_DUPLICATE_DEFAULT, LOG_ERROR, true},
    {"duplicate-case", EXPECT_FAIL_DUPLICATE_CASE, LOG_ERROR, true},
    {"invalid-count-generic-args", EXPECT_FAIL_INVALID_COUNT_GENERIC_ARGS, LOG_ERROR, true},
    {"missing-yield-statement", EXPECT_FAIL_MISSING_YIELD_STATEMENT, LOG_ERROR, true},
    {"invalid-hex", EXPECT_FAIL_INVALID_HEX, LOG_ERROR, true},
    {"invalid-bin", EXPECT_FAIL_INVALID_BIN, LOG_ERROR, true},
    {"invalid-octal", EXPECT_FAIL_INVALID_OCTAL, LOG_ERROR, true},
    {"invalid-char-lit", EXPECT_FAIL_INVALID_CHAR_LIT, LOG_ERROR, true},
    {"invalid-decimal-lit", EXPECT_FAIL_INVALID_DECIMAL_LIT, LOG_ERROR, true},
    {"missing-close-multiline", EXPECT_FAIL_MISSING_CLOSE_MULTILINE, LOG_ERROR, true},
    {"invalid-count-struct-lit-args", EXPECT_FAIL_INVALID_COUNT_STRUCT_LIT_ARGS, LOG_ERROR, true},
    {"missing-sum-arg", EXPECT_FAIL_MISSING_SUM_ARG, LOG_ERROR, true},
    {"sum-case-too-many-args", EXPECT_FAIL_SUM_CASE_TOO_MANY_ARGS, LOG_ERROR, true},
    {"void-sum-case-has-arg", EXPECT_FAIL_VOID_SUM_CASE_HAS_ARG, LOG_ERROR, true},
    {"invalid-stmt-top-level", EXPECT_FAIL_INVALID_STMT_TOP_LEVEL, LOG_ERROR, true},
    {"invalid-function-callee", EXPECT_FAIL_INVALID_FUNCTION_CALLEE, LOG_ERROR, true},
    {"optional-args-for-variadic-args", EXPECT_FAIL_OPTIONAL_ARGS_FOR_VARIADIC_ARGS, LOG_ERROR, true},
    {"fail-invalid-fail-type", EXPECT_FAIL_INVALID_FAIL_TYPE, LOG_ERROR, false},
    {"no-main-function", EXPECT_FAIL_NO_MAIN/*TODO: rename this to match string*/, LOG_WARNING, false},
    {"struct-like-recursion", EXPECT_FAIL_STRUCT_LIKE_RECURSION, LOG_ERROR, true},
};

// error types are in the same order in expect_fail_str_to_curr_log_level_pair and expect_fail_pair
// TODO: make these names less verbose?
static Expect_fail_str_to_curr_log_level 
expect_fail_str_to_curr_log_level_pair[sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0])] = {0};

bool expect_fail_type_from_strv(size_t* idx_result, EXPECT_FAIL_TYPE* type, Str_view strv) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        if (str_view_is_equal(str_view_from_cstr(expect_fail_pair[idx].str), strv)) {
            *type = expect_fail_pair[idx].type;
            *idx_result = idx;
            return true;
        }
    }
    return false;
}

static size_t expect_fail_type_get_idx(EXPECT_FAIL_TYPE type) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        if (expect_fail_pair[idx].type == type) {
            return idx;
        }
    }
    unreachable("expect_fail_pair does not cover this EXPECT_FAIL_TYPE");
}

Str_view expect_fail_type_print_internal(EXPECT_FAIL_TYPE type) {
    return str_view_from_cstr(expect_fail_pair[expect_fail_type_get_idx(type)].str);
}

static void expect_fail_str_to_curr_log_level_init(void) {
    for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
        expect_fail_str_to_curr_log_level_pair[idx].str = expect_fail_pair[idx].str;
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = expect_fail_pair[idx].default_level;
    }
}

LOG_LEVEL expect_fail_type_to_curr_log_level(EXPECT_FAIL_TYPE type) {
    return expect_fail_str_to_curr_log_level_pair[expect_fail_type_get_idx(type)].curr_level;
}

static void parse_normal_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "compile")) {
        params->compile = true;
        params->input_file_name = consume_arg(argc, argv, "input file path was expected after `compile`");
    } else if (0 == strcmp(curr_opt, "test-expected-fail")) {
        params->compile = false;
        params->test_expected_fail = true;

        const char* count_args_cstr = consume_arg(argc, argv, "count expected");
        size_t count_args = SIZE_MAX;
        if (!try_str_view_to_size_t(&count_args, str_view_from_cstr(count_args_cstr))) {
            todo();
        }
        assert(count_args < SIZE_MAX && "count_args unset");

        bool found = false;
        for (size_t idx = 0; idx < count_args; idx++) {
            const char* expected_fail_type_str = consume_arg(
                argc, argv, "expected fail type expected after `test_expected_fail`"
            );

            for (size_t idx = 0; idx < sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0]); idx++) {
                if (0 == strcmp(expected_fail_type_str, expect_fail_pair[idx].str)) {
                    found = true;
                    vec_append(&a_main, &params->expected_fail_types, expect_fail_pair[idx].type);
                    break;
                }
            }

            if (!found) {
                log(LOG_FATAL, "invalid expected fail type `%s`\n", expected_fail_type_str);
                exit(EXIT_CODE_FAIL);
            }
            assert(params->expected_fail_types.info.count > 0);
        }

        params->input_file_name = consume_arg(
            argc, argv, "input file path was expected after `test_expected_fail <fail type>`"
        );
    } else {
        log(LOG_FATAL, "invalid option: %s\n", curr_opt);
        exit(EXIT_CODE_FAIL);
    }
}

static void parse_long_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "emit-llvm")) {
        params->emit_llvm = true;
    } else if (0 == strncmp(curr_opt, "backend", strlen("backend"))) {
        Str_view backend = str_view_from_cstr(&curr_opt[strlen("backend")]);
        if (!str_view_try_consume(&backend, '=') || backend.count < 1) {
            log(LOG_FATAL, "expected =<backend> after `backend`");
            exit(EXIT_CODE_FAIL);
        }

        if (str_view_is_equal(backend, str_view_from_cstr("c"))) {
            params->backend_info.backend = BACKEND_C;
            params->backend_info.struct_rtn_through_param = false;
        } else if (str_view_is_equal(backend, str_view_from_cstr("llvm"))) {
            params->backend_info.backend = BACKEND_LLVM;
            params->backend_info.struct_rtn_through_param = true;
        } else {
            log(LOG_FATAL, "backend `"STR_VIEW_FMT"` is not a supported backend\n", str_view_print(backend));
            exit(EXIT_CODE_FAIL);
        }
    } else if (0 == strcmp(curr_opt, "all-errors-fatal")) {
        params->all_errors_fatal = true;
    } else if (0 == strncmp(curr_opt, "error", strlen("error"))) {
        Str_view error = str_view_from_cstr(&curr_opt[strlen("error")]);
        if (!str_view_try_consume(&error, '=') || error.count < 1) {
            log(LOG_FATAL, "expected <=error1[,error2,...]> after `error`");
            exit(EXIT_CODE_FAIL);
        }
        EXPECT_FAIL_TYPE type = {0};
        size_t idx = 0;
        if (!expect_fail_type_from_strv(&idx, &type, error)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_INVALID_FAIL_TYPE, POS_BUILTIN,
                "invalid fail type `"STR_VIEW_FMT"`\n", str_view_print(error)
            );
            exit(EXIT_CODE_FAIL);
        }
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = LOG_ERROR;
        params->error_opts_changed = true;
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

static Parameters get_params_with_defaults(void) {
    Parameters params = {0};

    params.emit_llvm = false;
    params.compile = false;

    return params;
}

void parse_args(int argc, char** argv) {
    params = get_params_with_defaults();
    expect_fail_str_to_curr_log_level_init();

    // consume compiler executable name
    consume_arg(&argc, &argv, "internal error");

    while (argc > 0) {
        if (is_short_option(argv)) {
            parse_long_option(&params, &argc, &argv);
        } else if (is_long_option(argv)) {
            parse_long_option(&params, &argc, &argv);
        } else {
            parse_normal_option(&params, &argc, &argv);
        }
    }
}

