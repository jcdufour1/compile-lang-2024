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
        msg(LOG_FETAL, EXPECT_FAIL_TYPE_NONE, dummy_pos, "%s\n", msg_if_missing);
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

static void parse_normal_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "compile")) {
        params->compile = true;
        params->input_file_name = consume_arg(argc, argv, "input file path was expected after `compile`");
    } else if (0 == strcmp(curr_opt, "test-expected-fail")) {
        params->compile = false;
        params->test_expected_fail = true;

        const char* count_args_cstr = consume_arg(argc, argv, "count expected");
        // TODO: use size_t for count_args_
        int64_t count_args_ = INT64_MAX;
        if (!try_str_view_to_int64_t(&count_args_, str_view_from_cstr(count_args_cstr))) {
            todo();
        }
        assert(count_args_ < INT64_MAX && "count_args_ unset");
        log(LOG_DEBUG, "%s\n", count_args_cstr);
        long count_args = count_args_;
        log(LOG_DEBUG, "%zu\n", count_args);
        for (size_t idx = 0; idx < count_args; idx++) {
            const char* expected_fail_type_str = consume_arg(
                argc, argv, "expected fail type expected after `test_expected_fail`"
            );

            if (0 == strcmp(expected_fail_type_str, "undefined-function")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_UNDEFINED_FUNCTION);
            } else if (0 == strcmp(expected_fail_type_str, "undefined-symbol")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_UNDEFINED_SYMBOL);
            } else if (0 == strcmp(expected_fail_type_str, "expected-expression")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_EXPECTED_EXPRESSION);
            } else if (0 == strcmp(expected_fail_type_str, "binary-mismatched-types")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_BINARY_MISMATCHED_TYPES);
            } else if (0 == strcmp(expected_fail_type_str, "assignment-mismatched-types")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES);
            } else if (0 == strcmp(expected_fail_type_str, "mismatched-return-type")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_MISMATCHED_RETURN_TYPE);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-function-arguments")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_FUN_ARG);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-count-function-arguments")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS);
            } else if (0 == strcmp(expected_fail_type_str, "missing-return-statement")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_MISSING_RETURN_STATEMENT);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-struct-member")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_STRUCT_MEMBER);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-struct-member-in-literal")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_STRUCT_MEMBER_IN_LITERAL);
            } else if (0 == strcmp(expected_fail_type_str, "redefinition-of-symbol")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_REDEFINITION_SYMBOL);
            } else if (0 == strcmp(expected_fail_type_str, "parser-expected")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_PARSER_EXPECTED);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-token")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_TOKEN);
            } else if (0 == strcmp(expected_fail_type_str, "invalid-extern-type")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_INVALID_EXTERN_TYPE);
            } else if (0 == strcmp(expected_fail_type_str, "missing-close-curly-brace")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE);
            } else if (0 == strcmp(expected_fail_type_str, "missing-close-par")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_MISSING_CLOSE_PAR);
            } else if (0 == strcmp(expected_fail_type_str, "no-new-line-after-statement")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT);
            } else if (0 == strcmp(expected_fail_type_str, "missing-close-double-quote")) {
                vec_append(&a_main, &params->expected_fail_types, EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE);
            } else {
                log(LOG_FETAL, "invalid expected fail type `%s`\n", expected_fail_type_str);
                exit(EXIT_CODE_FAIL);
            }
            assert(idx + 1 == params->expected_fail_types.info.count);
            assert(params->expected_fail_types.info.count > 0);
        }

        params->input_file_name = consume_arg(
            argc, argv, "input file path was expected after `test_expected_fail <fail type>`"
        );
    } else {
        log(LOG_FETAL, "invalid option: %s\n", curr_opt);
        exit(EXIT_CODE_FAIL);
    }
}

static void parse_long_option(Parameters* params, int* argc, char*** argv) {
    const char* curr_opt = consume_arg(argc, argv, "arg expected");

    if (0 == strcmp(curr_opt, "emit-llvm")) {
        params->emit_llvm = true;
    } else if (0 == strcmp(curr_opt, "all-errors-fetal")) {
        params->all_errors_fetal = true;
    } else {
        log(LOG_FETAL, "invalid option: %s\n", curr_opt);
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

    // consume compiler executable name
    consume_arg(&argc, &argv, "internal error");

    while (argc > 0) {
        if (is_short_option(argv)) {
            todo();
        } else if (is_long_option(argv)) {
            parse_long_option(&params, &argc, &argv);
        } else {
            parse_normal_option(&params, &argc, &argv);
        }
    }
}

