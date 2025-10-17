import os, subprocess, sys, pathlib, difflib, multiprocessing
from typing import Callable, Tuple, Optional
from dataclasses import dataclass
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


@dataclass
class FileItem:
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

INPUTS_DIR = os.path.join("tests", "inputs")
RESULTS_DIR = os.path.join("tests", "results")

BUILD_DEBUG_DIR = os.path.join("build", "debug")
BUILD_RELEASE_DIR = os.path.join("build", "release")

EXE_BASE_NAME = "main"

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

def get_files_to_test(files_to_test: list[str]) -> list[FileItem]:
    # TODO: print error for invalid path in files_to_test
    files: list[FileItem] = []
    possible_path: str
    for possible_base in map(to_str, list_files_recursively(INPUTS_DIR)):
        possible_path = os.path.realpath(possible_base)
        actual_base = possible_path[len(os.path.realpath(INPUTS_DIR)) + 1:]
        if os.path.isfile(possible_path) and possible_path in files_to_test:
            files.append(FileItem(actual_base))
    return files

def get_expected_output(file: FileItem, action: Action) -> str:
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

# TODO: try to avoid using do_debug for both function name and parameter name
def compile_test(do_debug: bool, output_name: str, file: FileItem, debug_release_text: str, path_c_compiler: Optional[str]) -> TestResult:
    compile_cmd: list[str]
    if do_debug:
        compile_cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME)]
    else:
        compile_cmd = [os.path.join(BUILD_RELEASE_DIR, EXE_BASE_NAME)]

    if output_name == "test.c":
        compile_cmd.append("--backend=c")
    elif output_name == "test.ll":
        compile_cmd.append("--backend=llvm")
    else:
        assert(False and "not implemented")

    compile_cmd.append(os.path.join(INPUTS_DIR, file.path_base))
    compile_cmd.append("-lm")
    compile_cmd.append("--set-log-level=INFO")
    compile_cmd.append("-o")
    compile_cmd.append("test")
    compile_cmd.append("--error=no-main-function")
    if path_c_compiler is not None:
        compile_cmd.append("--path-c-compiler=" + path_c_compiler)
    compile_cmd.append("--run")

    # TODO: print when --verbose flag
    #print_info("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
    return TestResult(subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))

def do_tests(do_debug: bool, params: Parameters):
    success = True

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

    # TODO: run multiple tests at once
    for file in get_files_to_test(params.files_to_test):
        if not test_file(file, do_debug, get_expected_output(file, params.action), params.test_output, params.action, debug_release_text, params.path_c_compiler, params.do_color):
            if not params.keep_going:
                sys.exit(1)
            success = False
    if not success:
        sys.exit(1)


def normalize(string: str) -> str:
    return string.replace("\r", "")

# return true if test was successful
# TODO: this should accept Parameter
def test_file(
    file: FileItem,
    do_debug: bool,
    expected_output: str,
    output_name: str,
    action: Action,
    debug_release_text: str,
    path_c_compiler: Optional[str],
    do_color: bool
) -> bool:
    result: TestResult = compile_test(do_debug, output_name, file, debug_release_text, path_c_compiler)

    process_result: str = get_result_from_test_result(result)
    if action == Action.UPDATE:
        path = os.path.join(RESULTS_DIR, file.path_base)
        dir = path[:path.rfind('/')]
        os.makedirs(dir, exist_ok = True)
        with open(path, "w") as newResult:
            newResult.write(process_result)
        return True
    elif action == Action.TEST:
        pass
    else:
        raise AssertionError("uncovered case")

    if normalize(process_result) != normalize(expected_output):
        actual_color: str = ""
        expected_color: str = ""
        print_error("test fail:" + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
        diff = difflib.SequenceMatcher(None, expected_output, process_result)
        for tag, expected_start, expected_end, stdout_start, stdout_end, in diff.get_opcodes():
            if tag == 'insert':
                if do_color:
                    actual_color += Changes.ADDED + \
                                    process_result[stdout_start:stdout_end] + \
                                    Changes.TO_NORMAL
                else:
                    actual_color = process_result[stdout_start:stdout_end]
            elif tag == 'equal':
                expected_color += process_result[stdout_start:stdout_end]
                actual_color += process_result[stdout_start:stdout_end]
            elif tag == 'replace':
                if do_color:
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
                if do_color:
                    expected_color += Changes.REMOVED + \
                                      expected_output[expected_start:expected_end] + \
                                      Changes.TO_NORMAL
                else:
                    expected_color += expected_output[expected_start:expected_end]
                                      
            else:
                print_error("tag unregonized:" + tag)
                assert False
        print_info("expected output:")
        if do_color:
            print(expected_color)
        else:
            print(expected_output)
        print_info("actual output:")
        if do_color:
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

def add_to_map(map: dict, path: str):
    map[path] = 0

def parse_args() -> Parameters:
    action: Action = Action.TEST
    test_output = "test.c" # TODO: be more consistant with test_output variable names
    to_include: dict[str, int] = {}
    keep_going: bool = True
    has_found_flag = False
    path_c_compiler: Optional[str] = None
    do_color: bool = True
    for arg in sys.argv[1:]:
        if arg.startswith("--keep-going"):
            keep_going = True
        elif arg.startswith("--stop-on-error"):
            keep_going = False
        elif arg.startswith("--update"):
            action = Action.UPDATE
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
    for path in to_include:
        to_include_list.append(path)

    count_threads: int
    try:
        count_threads = multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    return Parameters(to_include_list, test_output, action, keep_going, path_c_compiler, do_color, count_threads)

def main() -> None:
    # TODO: make a class with params because there are many parameters being passed around now
    params: Parameters = parse_args()

    # TODO: when --update is used, only one of debug or release should be ran (to save time)
    do_tests(True, params)
    do_tests(False, params)
    print_success("all tests passed")

if __name__ == '__main__':
    main()
