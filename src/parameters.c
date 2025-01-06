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
        msg(LOG_FATAL, EXPECT_FAIL_TYPE_NONE, dummy_file_text, dummy_pos, "%s\n", msg_if_missing);
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
} Expect_fail_pair;

static const Expect_fail_pair expect_fail_pair[] = {
    {"missing-close-double-quote", EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE},
    {"no-new-line-after-statement", EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT},
    {"missing-close-par", EXPECT_FAIL_MISSING_CLOSE_PAR},
    {"missing-close-curly-brace", EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE},
    {"invalid-extern-type", EXPECT_FAIL_INVALID_EXTERN_TYPE},
    {"invalid-token", EXPECT_FAIL_INVALID_TOKEN},
    {"parser-expected", EXPECT_FAIL_PARSER_EXPECTED},
    {"redefinition-of-symbol", EXPECT_FAIL_REDEFINITION_SYMBOL},
    {"invalid-struct-member-in-literal", EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL},
    {"invalid-struct-member", EXPECT_FAIL_INVALID_STRUCT_MEMBER},
    {"invalid-enum-member", EXPECT_FAIL_INVALID_ENUM_MEMBER},
    {"missing-return-statement", EXPECT_FAIL_MISSING_RETURN},
    {"invalid-count-function-arguments", EXPECT_FAIL_INVALID_COUNT_FUN_ARGS},
    {"invalid-function-arguments", EXPECT_FAIL_INVALID_FUN_ARG},
    {"mismatched-return-type", EXPECT_FAIL_MISMATCHED_RETURN_TYPE},
    {"assignment-mismatched-types", EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES},
    {"binary-mismatched-types", EXPECT_FAIL_BINARY_MISMATCHED_TYPES},
    {"expected-expression", EXPECT_FAIL_EXPECTED_EXPRESSION},
    {"undefined-symbol", EXPECT_FAIL_UNDEFINED_SYMBOL},
    {"undefined-function", EXPECT_FAIL_UNDEFINED_FUNCTION},
    {"struct-init-on-raw-union", EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION},
    {"undefined-type", EXPECT_FAIL_UNDEFINED_TYPE},
    {"missing-close-sq-bracket", EXPECT_FAIL_MISSING_CLOSE_SQ_BRACKET},
    {"deref_non_pointer", EXPECT_FAIL_DEREF_NON_POINTER},
    {"break-invalid-location", EXPECT_FAIL_BREAK_INVALID_LOCATION},
    {"continue-invalid-location", EXPECT_FAIL_CONTINUE_INVALID_LOCATION},
    {"mismatched-tuple-count", EXPECT_FAIL_MISMATCHED_TUPLE_COUNT},
    {"non-exhaustive-switch", EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH},
};

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
        log(LOG_DEBUG, "%s\n", count_args_cstr);
        log(LOG_DEBUG, "%zu\n", count_args);

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
    } else if (0 == strcmp(curr_opt, "all-errors-fatal")) {
        params->all_errors_fatal = true;
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

