import os, subprocess, sys, pathlib, difflib, multiprocessing

class Changes:
    REMOVED = "\033[37;41m" #]
    ADDED = "\033[30;42m" #]
    TO_NORMAL = "\033[0m" #]

class StatusColors:
    RED = "\033[31;40m" #]
    GREEN = "\033[32;40m" #]
    BLUE = "\033[34;40m" #]
    TO_NORMAL = "\033[0m" #]

class FileItem:
    def __init__(self, path: str, expected_success: bool, expected_fail_str: list[str] | None):
        self.path = path
        self.expected_success = expected_success
        self.expected_fail_str = expected_fail_str
    path: str
    expected_success: bool
    expected_fail_str: list[str] | None

EXAMPLES_DIR = "./examples/new_lang/"
EXPECTED_FAIL_EXAMPLES_DIR = "./tests/expected_failure_examples/"

BUILD_DEBUG_DIR = "./build/debug/"
BUILD_RELEASE_DIR = "./build/release/"
EXE_BASE_NAME = "main"
EXPECTED_SUCCESS_RESULTS_DIR = "./tests/expected_success_results/"

EXPECT_FAIL_FILE_PATH_TO_TYPE: dict[str, list[str]] = {
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_symbol.own": ["undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_function.own": ["undefined-symbol", "missing-return-statement"],
    EXPECTED_FAIL_EXAMPLES_DIR + "expected_expression.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "incomplete_var_def.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "binary_mismatched_types.own": ["not-yet-implemented"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_fun_arg.own": ["invalid-function-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_count_fun_args.own": ["invalid-count-fun-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_count_fun_args_2.own": ["invalid-count-fun-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_count_fun_args_3.own": ["invalid-count-fun-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "missing_return.own": ["missing-return-statement", "missing-return-statement"],
    EXPECTED_FAIL_EXAMPLES_DIR + "var_redef.own": ["redefinition-of-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_struct_memb.own": ["invalid-member-access"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_struct_memb_in_literal.own": ["invalid-struct-member-in-literal"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_literal_assign_to_struct.own": ["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_literal_assign.own": ["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_operation_assign.own": ["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "mismatched_return_type.own": ["mismatched-return-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "trailing_comma_fun_arg.own": ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "missing_comma_fun_arg.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "triple_equals.own": ["invalid-token"],
    EXPECTED_FAIL_EXAMPLES_DIR + "extern_c_no_close_par_after_c.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "extern_c_no_open_par_before_c.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "extern_c_no_c_after_extern.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "extern_c_empty_str_extern_type.own": ["invalid-extern-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "extern_c_no_fn.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "for_no_double_dot.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "for_no_double_dot_2.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "var_redef_2.own": ["redefinition-of-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "var_def_no_colon.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "var_def_no_name.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "function_param_type_missing.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "function_param_type_missing_2.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "fun_params_missing_open_par.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "fun_params_missing_close_par.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "structs_missing_open_brace.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "structs_missing_memb_name.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "structs_missing_memb_type.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "structs_missing_close_brace.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_lit_no_close_brace.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "assign_no_rhs.own": ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "missing_close_par.own": ["missing-close-par"],
    EXPECTED_FAIL_EXAMPLES_DIR + "missing_close_par_2.own": ["expected-expression", "missing-close-par"],
    EXPECTED_FAIL_EXAMPLES_DIR + "two_undef_symbols.own": ["undefined-symbol", "undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "two_parse_errors.own": ["expected-expression", "no-new-line-after-statement"],
    EXPECTED_FAIL_EXAMPLES_DIR + "function_main_no_close_brace.own": ["missing-close-curly-brace"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_literal_no_delim.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "not_is_first_op_and_lhs_lower_pres_than_rhs.own": ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "unclosed_string_literal.own": ["missing-close-double-quote"],
    EXPECTED_FAIL_EXAMPLES_DIR + "union_init_like_a_struct.own": ["struct-init-on-raw-union"],
    EXPECTED_FAIL_EXAMPLES_DIR + "if_cond_undef_function.own": ["undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undefined_types.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_type_in_struct_def_member.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_type_in_raw_union_def_member.own": 2*["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "index_no_closing_sq_bracket.own": ["missing-close-sq-bracket"],
    EXPECTED_FAIL_EXAMPLES_DIR + "failure_to_deref_function_arg.own": 2*["invalid-function-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "failure_deref_lhs_of_assignment.own": ["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "deref_non_pointer.own": ["deref_non_pointer"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_enum_subtype_access.own": ["invalid-member-access"],
    EXPECTED_FAIL_EXAMPLES_DIR + "mismatched_literal_types_to_binary.own": ["not-yet-implemented"],
    EXPECTED_FAIL_EXAMPLES_DIR + "semantic_error_in_return_expr.own": ["undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "parse_error_in_if_condition.own": ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_literal_assign_parse_errors.own": [
        "expected-expression",
        "expected-expression",
        "parser-expected"
    ],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_literal_missing_dot.own": [
        "undefined-symbol",
        "undefined-symbol"
    ],
    EXPECTED_FAIL_EXAMPLES_DIR + "break_invalid_location.own": ["break-invalid-location", "continue-invalid-location"],

    EXPECTED_FAIL_EXAMPLES_DIR + "tuple_mismatched_counts.own": 2*["assignment-mismatched-types"] + 1*["mismatched-tuple-count"],
    # TODO: commented out line below should be uncommented (above line is temporary)
    #EXPECTED_FAIL_EXAMPLES_DIR + "tuple_mismatched_counts.own": 2*["mismatched-tuple-count"] + 2*["assignment-mismatched-types"],

    EXPECTED_FAIL_EXAMPLES_DIR + "tuple_invalid_subtype.own": 2*["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_literal_invalid_types.own": 2*["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "non_exhaustive_switch.own": ["non-exhaustive-switch"],
    EXPECTED_FAIL_EXAMPLES_DIR + "enum_def_line_extra_tokens.own": 3*["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_lit_invalid_arg.own": 2*["sum-lit-invalid-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_lit_assigned_to_sum.own": ["struct-init-on-sum"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_assignment_to_sum.own": ["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "logical_not_on_void_type.own": 2*["unary-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_switch_invalid_case.own": ["binary-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "type_token_struct_wrong_order.own": 2*["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "type_token_no_sum_after.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_case_var_is_redef.own": ["redefinition-of-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "signed_assigned_to_unsigned.own": 2*["assignment-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "deref_literal.own": ["deref_non_pointer"],
    EXPECTED_FAIL_EXAMPLES_DIR + "assign_struct_literal_to_int.own": 2*["struct-init-on-primitive"],
    EXPECTED_FAIL_EXAMPLES_DIR + "duplicate_default.own": ["duplicate-default"],
    EXPECTED_FAIL_EXAMPLES_DIR + "duplicate_case.own": ["duplicate-case", "duplicate-case"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_arg_with_void_varient.own": ["missing-sum-arg"] + ["invalid-count-fun-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_case_invalid_cond_defered_symbols.own": ["binary-mismatched-types"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_symbol_in_generic.own": ["undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_count_generic_args.own": 4*["invalid-count-generic-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undef_member_in_generic_sum.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "switch_undef_operand.own": ["undefined-symbol"],
    EXPECTED_FAIL_EXAMPLES_DIR + "no_generic_args_when_required.own": ["invalid-count-generic-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undefined_type_in_function_param.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "undefined_generic_type_in_fun_param.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "member_access_on_primitive.own": ["invalid-member-access"],
    EXPECTED_FAIL_EXAMPLES_DIR + "missing_yield.own": 2*["missing-yield-statement"],
    EXPECTED_FAIL_EXAMPLES_DIR + "mismatched_yield_type.own": ["mismatched-yield-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_hex.own": ["invalid-hex"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_hex_2.own": ["invalid-hex"] + ["invalid-decimal-lit"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_octal.own": ["invalid-octal"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_octal_2.own": ["invalid-octal"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_bin.own": ["invalid-bin"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_bin_2.own": ["invalid-bin"],
    EXPECTED_FAIL_EXAMPLES_DIR + "multiline_comment_open_unmatched.own": ["missing-close-multiline"],
    EXPECTED_FAIL_EXAMPLES_DIR + "multiline_comment_close_unmatched.own": ["missing-close-multiline"],
    EXPECTED_FAIL_EXAMPLES_DIR + "nested_generic_mismatch_fun_arg.own": ["invalid-function-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "optional_76.own": ["undefined-type"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_non_designated_init_too_few_elems.own": ["invalid-count-struct-lit-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "struct_non_designated_init_too_many_elems.own": ["invalid-count-struct-lit-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_case_too_many_args.own": ["sum-case-too-many-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "void_sum_case_has_arg.own": ["void-sum-case-has-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_callee_too_many_args.own": ["missing-sum-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "void_sum_callee_has_arg.own": ["invalid-count-fun-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "stray_expr_top_level.own": ["stray-expr-top-level"],
    EXPECTED_FAIL_EXAMPLES_DIR + "stray_expr_top_level.own": ["invalid-stmt-top-level"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_stmt_top_level.own": 5*["invalid-stmt-top-level"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_par_with_enum.own": ["invalid-function-callee"],
    EXPECTED_FAIL_EXAMPLES_DIR + "string_literal_as_fun_callee.own": ["invalid-function-callee"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_case_empty_par_on_void_case.own": ["missing-sum-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "sum_callee_empty_par_on_void_case.own": ["missing-sum-arg"],
    EXPECTED_FAIL_EXAMPLES_DIR + "optional_args_for_variadic_args.own": ["optional-args-for-variadic-args"],
    EXPECTED_FAIL_EXAMPLES_DIR + "included_module_has_parse_error.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "import_invalid_syntax.own": 5*["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "def_invalid_syntax.own": ["parser-expected"] + ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "expected_expr_at_end_of_file.own": ["expected-expression"],
    EXPECTED_FAIL_EXAMPLES_DIR + "empty_file.own": [],
    EXPECTED_FAIL_EXAMPLES_DIR + "switch_missing_open_curly_brace.own": ["parser-expected"],
    EXPECTED_FAIL_EXAMPLES_DIR + "char_lit_too_large.own": ["invalid-char-lit"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_char_lit_2.own": 4*["invalid-char-lit"],
    EXPECTED_FAIL_EXAMPLES_DIR + "invalid_decimal_lit.own": 3*["invalid-decimal-lit"],
}

def to_str(a):
    return str(a)

def print_error(*base, **kargs) -> None:
    print(StatusColors.RED, *base, StatusColors.TO_NORMAL, file=sys.stderr, sep = "", **kargs)

def print_warning(*base, **kargs) -> None:
    print(StatusColors.RED, *base, StatusColors.TO_NORMAL, file=sys.stderr, sep = "", **kargs)

def print_success(*base, **kargs) -> None:
    print(StatusColors.GREEN, *base, StatusColors.TO_NORMAL, file=sys.stderr, sep = "", **kargs)

def print_info(*base, **kargs) -> None:
    print(StatusColors.BLUE, *base, StatusColors.TO_NORMAL, file=sys.stderr, sep = "", **kargs)

def get_files_to_test(files_to_test: list[str]) -> list[FileItem]:
    files: list[FileItem] = []
    for possible_file_base in map(to_str, os.listdir(EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path) and possible_file_path in files_to_test:
            files.append(FileItem(possible_file_path, True, None))
    for possible_file_base in map(to_str, os.listdir(EXPECTED_FAIL_EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXPECTED_FAIL_EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path) and possible_file_path in files_to_test:
            files.append(FileItem(possible_file_path, False, EXPECT_FAIL_FILE_PATH_TO_TYPE[os.path.join(EXPECTED_FAIL_EXAMPLES_DIR, possible_file_base)]))
    return files

def get_expected_output(file: FileItem) -> str:
    file_expected_base, _ = os.path.splitext(os.path.basename(file.path))
    file_expected = os.path.join(EXPECTED_SUCCESS_RESULTS_DIR, file_expected_base)
    file_expected += ".txt"
    with open(file_expected, "r") as input_file:
        return input_file.read()

# return true if test was successful
def do_test(file: FileItem, do_debug: bool, expected_output: str, output_name: str) -> bool:
    debug_release_text: str
    compile_cmd: list[str]
    if do_debug:
        compile_cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME)]
        debug_release_text = "debug"
    else:
        compile_cmd = [os.path.join(BUILD_RELEASE_DIR, EXE_BASE_NAME)]
        debug_release_text = "release"

    if output_name == "test.c":
        compile_cmd.append("--backend=c")
    elif output_name == "test.ll":
        compile_cmd.append("--backend=llvm")
    else:
        assert(False and "not implemented")

    if file.expected_success:
        compile_cmd.append("compile")
        compile_cmd.append(file.path)
        compile_cmd.append("--emit-llvm")
    else:
        assert(file.expected_fail_str != None)
        compile_cmd.append("test-expected-fail")
        compile_cmd.append(str(len(file.expected_fail_str)))
        compile_cmd.extend(file.expected_fail_str)
        compile_cmd.append(file.path)

    print_info("testing: " + file.path + " (" + debug_release_text + ")")
    process = subprocess.run(compile_cmd)
    if not file.expected_success:
        if process.returncode != 2:
            print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") returned " + str(process.returncode) + " exit code, but expected failure (exit code 2) was expected")
            sys.exit(1)
            return
        else:
            print_success("testing: " + file.path + " (" + debug_release_text + ") expected fail success")
            print()
            return

    if process.returncode != 0:
        print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") fail")
        sys.exit(1)
        return
    if process.returncode != 0:
        print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") fail")
        sys.exit(1)

    clang_cmd = ["clang", output_name, "-o", "test"]
    process = subprocess.run(clang_cmd)
    if process.returncode != 0:
        print_error("testing: clang " + file.path + " (" + debug_release_text + ") fail")
        sys.exit(1)

    test_cmd = ["./test"]
    process = subprocess.run(test_cmd, stdout=subprocess.PIPE, text=True)
    if process.returncode != 0:
        print_error(
            "testing: test " + file.path + " (" + debug_release_text +
             ") fail (returned exit code " + str(process.returncode) + ")"
        )
        sys.exit(1)
    if str(process.stdout) != expected_output:
        stdout_color: str = ""
        expected_color: str = ""
        print_error("test fail:" + file.path + " (" + debug_release_text + ")")
        diff = difflib.SequenceMatcher(None, expected_output, str(process.stdout))
        for tag, expected_start, expected_end, stdout_start, stdout_end, in diff.get_opcodes():
            if tag == 'insert':
                stdout_color += Changes.ADDED + \
                                str(process.stdout)[stdout_start:stdout_end] + \
                                Changes.TO_NORMAL
            elif tag == 'equal':
                expected_color += str(process.stdout)[stdout_start:stdout_end]
                stdout_color += str(process.stdout)[stdout_start:stdout_end]
            elif tag == 'replace':
                expected_color += Changes.REMOVED + \
                                  expected_output[expected_start:expected_end] + \
                                  Changes.TO_NORMAL
                stdout_color += Changes.ADDED + \
                                str(process.stdout)[stdout_start:stdout_end] + \
                                Changes.TO_NORMAL
            elif tag == 'delete':
                expected_color += Changes.REMOVED + \
                                  str(expected_output)[expected_start:expected_end] + \
                                  Changes.TO_NORMAL
            else:
                print_error("tag unregonized:" + tag)
                assert False
        print_info("expected output:")
        print(expected_color)
        print_info("actual process output:")
        print(stdout_color)
        sys.exit(1)

    print_success("testing: " + file.path + " (" + debug_release_text + ") success")
    print()

def do_debug(files_to_test: list[str], count_threads: int, output_name: str) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling debug:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "1"}))
    if process.returncode != 0:
        print_error("compilation of debug failed")
        sys.exit(1)
    print_success("compiling debug: done")

    for file in get_files_to_test(files_to_test):
        expected_output: str
        if file.expected_success:
            expected_output = get_expected_output(file)
        else:
            expected_output = ""
        do_test(file, do_debug=True, expected_output=expected_output, output_name=output_name)
    print_success("testing debug: done")
    print()

def do_release(files_to_test: list[str], count_threads: int, output_name: str) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling release:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "0"}))
    if process.returncode != 0:
        print_error("compilation of release failed")
        sys.exit(1)
    print_success("compiling release: done")
    print()

    for file in get_files_to_test(files_to_test):
        expected_output: str
        if file.expected_success:
            expected_output = get_expected_output(file)
        else:
            expected_output = ""
        do_test(file, do_debug=False, expected_output=expected_output, output_name=output_name)
    print_success("testing release: done")
    print()

def append_all_files(list_or_map, callback):
    for possible_file_base in map(to_str, os.listdir(EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path):
            callback(list_or_map, possible_file_path)
    for possible_file_base in map(to_str, os.listdir(EXPECTED_FAIL_EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXPECTED_FAIL_EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path):
            callback(list_or_map, possible_file_path)

def add_to_map(map, path):
    map[path] = 0

def parse_args() -> (list[str], str):
    test_output = "" # TODO: be more consistant with test_output variable names
    to_include: dict[str, int] = {}
    has_found_flag = False
    for arg in sys.argv[1:]:
        if arg.startswith("--output-file="):
            test_output = arg.split("--output-file=")[1]
        elif arg.startswith("--exclude="):
            to_exclude_raw = arg[len("--exclude="):]
            for idx, path in enumerate(to_exclude_raw.split(",")):
                if not has_found_flag:
                    append_all_files(to_include, add_to_map)
                    has_found_flag = True
                if os.path.realpath(path) in to_include:
                    del to_include[os.path.realpath(path)]
        elif arg.startswith("--include="):
            has_found_flag = True
            to_include_raw = arg[len("--include="):]
            for idx, path in enumerate(to_include_raw.split(",")):
                if not os.path.realpath(path) in to_include:
                    to_include[os.path.realpath(path)] = 0
        else:
            print_error("invalid option ", arg)
            sys.exit(1)

    if not has_found_flag:
        append_all_files(to_include, add_to_map)

    to_include_list: list[str] = []
    for path in to_include:
        to_include_list.append(path)
    return to_include_list, test_output

def main() -> None:
    files_to_test: list[str]
    output_file: str
    files_to_test, test_output = parse_args()

    count_threads: int
    try:
        count_threads = multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    do_debug(files_to_test, count_threads, test_output)
    do_release(files_to_test, count_threads, test_output)
    print_success("all tests passed")

if __name__ == '__main__':
    main()
