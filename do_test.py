import os, subprocess, sys, pathlib, difflib, multiprocessing
from typing import Callable, Tuple, Optional
from dataclasses import dataclass
from concurrent.futures import ProcessPoolExecutor
from enum import Enum

class Changes:
    REMOVED = "\033[37;41m" #]
    ADDED = "\033[30;42m" #]
    TO_NORMAL = "\033[0m" #]

class StatusColors:
    RED = "\033[31;40m" #]
    GREEN = "\033[32;40m" #]
    BLUE = "\033[34;40m" #]
    TO_NORMAL = "\033[0m" #]

class Action(Enum):
    TEST = 1
    UPDATE = 2

# FileNormal will be compiled, and test output will be checked
@dataclass
class FileNormal:
    path_base: str

# FileExample will only be compiled
@dataclass
class FileExample:
    path_base: str

@dataclass
class TestResult:
    compile: subprocess.CompletedProcess[str]

@dataclass
class Parameters:
    files_to_test: list[str]
    test_output: str
    action: Action
    keep_going: bool
    path_c_compiler: Optional[str]
    do_color: bool
    count_threads: int
    do_debug_internal: bool
    do_release_internal: bool

EXAMPLES_DIR = "examples"
INPUTS_DIR = os.path.join("tests", "inputs")
RESULTS_DIR = os.path.join("tests", "results")

BUILD_DEBUG_DIR = os.path.join("build", "debug")
BUILD_RELEASE_DIR = os.path.join("build", "release")

EXE_BASE_NAME = "main"

def remove_extension(file_path: str) -> str:
    return file_path[:file_path.rfind(".")]

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

def list_files_recursively(dir: str) -> list[str]:
    result: list[str] = []
    for root, _, files in os.walk(dir):
        for file_path in files:
            result.append(os.path.join(root, file_path))
    return result

def get_files_to_test(files_to_test: list[str]) -> list[FileNormal | FileExample]:
    # TODO: print error for invalid path in files_to_test
    files: list[FileNormal | FileExample] = []
    possible_path: str

    for possible_base in map(to_str, list_files_recursively(INPUTS_DIR)):
        possible_path = os.path.realpath(possible_base)
        if os.path.isfile(possible_path) and possible_path in files_to_test:
            files.append(FileNormal(possible_path[len(os.path.realpath(INPUTS_DIR)) + 1:]))

    for possible_base in map(to_str, list_files_recursively(EXAMPLES_DIR)):
        possible_path = os.path.realpath(possible_base)
        if os.path.isfile(possible_path) and possible_path in files_to_test:
            files.append(FileExample(possible_path[len(os.path.realpath(EXAMPLES_DIR)) + 1:]))

    return files

def get_expected_output(file: FileNormal, action: Action) -> str:
    if action == Action.TEST:
        expect_base: str = file.path_base
        expected: str = os.path.join(RESULTS_DIR, expect_base)
        try:
            with open(expected, "r") as input:
                return input.read()
        except FileNotFoundError as e:
            print_warning(
                "result file not found for " +
                os.path.join(INPUTS_DIR, expect_base) +
                "; if this test input generates correct results, use --update --include=" +
                os.path.join(INPUTS_DIR, expect_base)
            )
            print(e)
            return ""
    elif action == Action.UPDATE:
        return ""
    else:
        raise NotImplementedError


def get_result_from_process_internal(process: subprocess.CompletedProcess[str], type_str: str) -> str:
    result: str = ""
    result += type_str + "::" + "stdout " + str(str(process.stdout).count("\n")) + "\n"
    result += str(process.stdout) + "\n"
    result += type_str + "::" + "stderr " + str(str(process.stderr).count("\n")) + "\n"
    result += str(process.stderr) + "\n"
    result += type_str + "::" + "return_code " + str(process.returncode) + "\n\n"
    return result

def get_result_from_test_result(process: TestResult) -> str:
    return get_result_from_process_internal(process.compile, "compile")

def compile_and_run_test(do_debug: bool, output_name: str, file: FileNormal | FileExample, debug_release_text: str, path_c_compiler: Optional[str]) -> TestResult:
    compile_cmd: list[str]
    if do_debug:
        compile_cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME)]
    else:
        compile_cmd = [os.path.join(BUILD_RELEASE_DIR, EXE_BASE_NAME)]

    if output_name == "test.c":
        compile_cmd.append("--backend")
        compile_cmd.append("c")
    elif output_name == "test.ll":
        compile_cmd.append("--backend")
        compile_cmd.append("llvm")
    else:
        assert(False and "not implemented")

    if isinstance(file, FileNormal):
        compile_cmd.append(os.path.join(INPUTS_DIR, file.path_base))
    elif isinstance(file, FileExample):
        compile_cmd.append(os.path.join(EXAMPLES_DIR, file.path_base))
    else:
        raise NotImplementedError
    compile_cmd.append("-lm")
    if do_debug:
        compile_cmd.append("--set-log-level")
        compile_cmd.append("INFO")
    if isinstance(file, FileNormal):
        compile_cmd.append("-o")
        compile_cmd.append(os.path.basename(remove_extension(file.path_base)))
    elif isinstance(file, FileExample):
        compile_cmd.append("-c")
        compile_cmd.append("-o")
        compile_cmd.append(os.path.basename(remove_extension(file.path_base)) + ".o")
    else:
        raise NotImplementedError
    compile_cmd.append("--error")
    compile_cmd.append("no-main-function")
    if path_c_compiler is not None:
        compile_cmd.append("--path-c-compiler")
        compile_cmd.append(path_c_compiler)
    if isinstance(file, FileNormal):
        compile_cmd.append("--run")
    elif isinstance(file, FileExample):
        pass
    else:
        raise NotImplementedError

    # TODO: print when --verbose flag
    #print_info("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
    return TestResult(subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))

def do_regular_test(file: Tuple[FileNormal | FileExample, bool, str, Parameters]) -> bool:
    if isinstance(file[0], FileNormal):
        if test_file(file[0], file[1], file[2], file[3]):
            return True
        else:
            if not file[3].keep_going:
                sys.exit(1)
            return False
    elif isinstance(file[0], FileExample):
        result: TestResult = compile_and_run_test(file[1], file[3].test_output, file[0], file[2], file[3].path_c_compiler)
        if result.compile.returncode == 0:
            return True
            #success_count += 1
        else:
            print_error("example compilation fail:" + os.path.join(EXAMPLES_DIR, file[0].path_base) + " (" + file[2] + ")")
            print_error("compile error:")
            print(get_result_from_test_result(result))
            if not file[3].keep_going:
                sys.exit(1)
            return False
    else:
        raise NotImplementedError

def do_tests(do_debug: bool, params: Parameters):
    success = True
    success_count = 0
    fail_count = 0

    debug_env: str
    debug_release_text: str
    if do_debug:
        debug_release_text = "debug"
        debug_env = "1"
    else:
        debug_release_text = "release"
        debug_env = "0"

    cmd = ["make", "-j", str(params.count_threads), "build"]
    print_info("compiling " + debug_release_text + " :")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": debug_env}))
    if process.returncode != 0:
        print_error("compilation of " + debug_release_text + " failed")
        sys.exit(1)
    print_success("compiling " + debug_release_text + " : done")
    print()

    # TODO: having bool, str, and Parameters in every element of regular_files may not be ideal
    regular_files: list[tuple[FileNormal | FileExample, bool, str, Parameters]] = []

    for file in get_files_to_test(params.files_to_test):
        regular_files.append((file, do_debug, debug_release_text, params))

    with ProcessPoolExecutor(max_workers = params.count_threads) as executor:
        futures = executor.map(do_regular_test, regular_files)
        for future in futures:
            if future:
                success_count += 1
            else:
                fail_count += 1
                success = False

    print("testing " + debug_release_text + " done")
    print("    tests total:  " + str(fail_count + success_count))
    print("    tests failed: " + str(fail_count))
    print("    tests passed: " + str(success_count))

    if not success:
        sys.exit(1)


def normalize(string: str) -> str:
    return string.replace("\r", "")

# return true if test was successful
def test_file(file: FileNormal, do_debug: bool, debug_release_text: str, params: Parameters) -> bool:
    result: TestResult = compile_and_run_test(do_debug, params.test_output, file, debug_release_text, params.path_c_compiler)
    process_result: str = get_result_from_test_result(result)
    expected_output: str = get_expected_output(file, params.action)

    if params.action == Action.UPDATE:
        path = os.path.join(RESULTS_DIR, file.path_base)
        dir = path[:path.rfind('/')]
        os.makedirs(dir, exist_ok = True)
        with open(path, "w") as newResult:
            newResult.write(process_result)
        return True
    elif params.action == Action.TEST:
        pass
    else:
        raise NotImplementedError

    if normalize(process_result) != normalize(expected_output):
        actual_color: str = ""
        expected_color: str = ""
        print_error("test fail:" + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
        diff = difflib.SequenceMatcher(None, expected_output, process_result)
        for tag, expected_start, expected_end, stdout_start, stdout_end, in diff.get_opcodes():
            if tag == 'insert':
                if params.do_color:
                    actual_color += Changes.ADDED + \
                                    process_result[stdout_start:stdout_end] + \
                                    Changes.TO_NORMAL
                else:
                    actual_color = process_result[stdout_start:stdout_end]
            elif tag == 'equal':
                expected_color += process_result[stdout_start:stdout_end]
                actual_color += process_result[stdout_start:stdout_end]
            elif tag == 'replace':
                if params.do_color:
                    expected_color += Changes.REMOVED + \
                                      expected_output[expected_start:expected_end] + \
                                      Changes.TO_NORMAL
                    actual_color += Changes.ADDED + \
                                    process_result[stdout_start:stdout_end] + \
                                    Changes.TO_NORMAL
                else:
                    expected_color = expected_output[expected_start:expected_end]
                    actual_color += process_result[stdout_start:stdout_end]
            elif tag == 'delete':
                if params.do_color:
                    expected_color += Changes.REMOVED + \
                                      expected_output[expected_start:expected_end] + \
                                      Changes.TO_NORMAL
                else:
                    expected_color += expected_output[expected_start:expected_end]
                                      
            else:
                print_error("tag unregonized:" + tag)
                assert False
        print_info("expected output:")
        if params.do_color:
            print(expected_color)
        else:
            print(expected_output)
        print_info("actual output:")
        if params.do_color:
            print(actual_color)
        else:
            print(process_result)
        return False

    # TODO: print when --verbose flag
    #print_success("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ") success")
    #print()
    return True

def append_all_files(list_or_map: list | dict, callback: Callable):
    possible_path: str
    for possible_base in list_files_recursively(INPUTS_DIR):
        callback(list_or_map, os.path.realpath(possible_base))
    for possible_base in list_files_recursively(EXAMPLES_DIR):
        callback(list_or_map, os.path.realpath(possible_base))

def add_to_map(map: dict, path: str):
    map[path] = 0

def parse_args() -> Parameters:
    action: Action = Action.TEST
    test_output = "test.c" # TODO: be more consistant with test_output variable names
    to_include: dict[str, int] = {}
    keep_going: bool = True
    has_found_flag = False
    path_c_compiler: Optional[str] = None
    do_color:bool = True
    do_release: Optional[bool] = True
    do_debug: Optional[bool] = True
    for arg in sys.argv[1:]:
        if arg.startswith("--keep-going"):
            keep_going = True
        elif arg.startswith("--stop-on-error"):
            keep_going = False
        elif arg.startswith("--update"):
            action = Action.UPDATE
        elif arg.startswith("--release-only"):
             do_release = True
             do_debug = False
        elif arg.startswith("--debug-only"):
             do_release = False
             do_debug = True
        elif arg.startswith("--debug-and-release"):
             do_debug = True
             do_release = True
        elif arg.startswith("--test"):
            action = Action.TEST
        elif arg.startswith("--output-file="):
            test_output = arg[len("--output-file="):]
        elif arg.startswith("--no-color"):
            do_color = False
        elif arg.startswith("--path-c-compiler="):
            path_c_compiler = arg[len("--path-c-compiler="):]
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
    to_include_list.extend(to_include)

    count_threads: int
    try:
        count_threads = multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    do_debug_actual: bool = True
    if action == Action.UPDATE:
        do_debug_actual = False
    if do_debug is not None:
        do_debug_actual = do_debug

    do_release_actual: bool = True
    if do_release is not None:
        do_release_actual = do_release

    return Parameters(
        to_include_list,
        test_output,
        action,
        keep_going,
        path_c_compiler,
        do_color,
        count_threads,
        do_debug_actual,
        do_release_actual
    )



# TODO: rename this function
# WARNING: readme_ex_lines will contain lines past actual file contents 
#   (do_something needs to check for ``` delimiter internally)
def do_something(readme_ex_lines: list[str], actual_ex_lines, example: str) -> None:
    actual_is_too_small: bool = False

    idx_readme: int = 0
    while not readme_ex_lines[idx_readme].startswith("```"):
        if idx_readme >= len(actual_ex_lines):
            actual_is_too_small = True

        if not actual_is_too_small and (readme_ex_lines[idx_readme] != actual_ex_lines[idx_readme]):
            print_error("line " + str(idx_readme + 1) + " of \"" + example + "\" is different in the README than in the actual file")
            sys.exit(1)
        idx_readme += 1
        if idx_readme >= len(readme_ex_lines):
            print_error("could not find \"```\" delimiter for \"" + example + "\" in README")
            sys.exit(1)

    if actual_is_too_small or idx_readme != len(actual_ex_lines):
        print_error("\"" + example + "\" is " + str(idx_readme) + " lines long in the README, but " + str(len(actual_ex_lines)) + " lines long in the actual file")
        sys.exit(1)

def main() -> None:
    params: Parameters = parse_args()

    if params.do_debug_internal:
        do_tests(True, params)
    if params.do_release_internal:
        do_tests(False, params)

    examples_in_readme: list[str] = ["examples/optional.own", "examples/defer.own"]
    for example in examples_in_readme:
        with open("README.md", "r") as file:
            lines = file.readlines() 
            for idx, line in enumerate(lines):
                if not example in line:
                    continue
                if not lines[idx + 1].startswith("```c"):
                    print_error("\"```c\" is expected on line immediately after the line that contains \"" + example + "\"")
                idx_exam = idx + 2
                with open(example, "r") as actual_example_file:
                    actual_ex_lines = actual_example_file.readlines()
                    do_something(lines[idx_exam:], actual_ex_lines, example)

    print_success("all tests passed")

if __name__ == '__main__':
    main()
