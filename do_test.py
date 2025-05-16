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
    compile: subprocess.CompletedProcess[bytes]
    clang: Optional[subprocess.CompletedProcess[bytes]]
    run: Optional[subprocess.CompletedProcess[bytes]]

INPUTS_DIR = "./tests2/inputs/"
RESULTS_DIR = "./tests2/results/"

BUILD_DEBUG_DIR = "./build/debug/"
BUILD_RELEASE_DIR = "./build/release/"
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

def get_files_to_test(files_to_test: list[str]) -> list[FileItem]:
    files: list[FileItem] = []
    possible_path: str
    for possible_base in map(to_str, os.listdir(INPUTS_DIR)):
        possible_path = os.path.realpath(os.path.join(INPUTS_DIR, possible_base))
        if os.path.isfile(possible_path) and possible_path in files_to_test:
            files.append(FileItem(possible_base))
    return files

def get_expected_output(file: FileItem) -> str:
    expect_base: str = file.path_base
    expected: str = os.path.join(RESULTS_DIR, expect_base)
    try:
        with open(expected, "r") as input:
            return input.read()
    except FileNotFoundError as e:
        print_warning(
            "result file not found for " +
            os.path.join(INPUTS_DIR, expect_base) +
            "; if this test input generates correct results, use --test --include=" +
            os.path.join(INPUTS_DIR, expect_base)
        )
        print(e)
        return ""


def get_result_from_process_internal(process: subprocess.CompletedProcess[bytes], type_str: str) -> str:
    result: str = ""
    result += type_str + "::" + "stdout " + str(str(process.stdout).count("\n")) + "\n"
    result += str(process.stdout) + "\n"
    result += type_str + "::" + "stderr " + str(str(process.stderr).count("\n")) + "\n"
    result += str(process.stderr) + "\n"
    result += type_str + "::" + "return_code " + str(process.returncode) + "\n\n"
    return result

def get_result_from_test_result(process: TestResult) -> str:
    result: str = get_result_from_process_internal(process.compile, "compile")
    if process.clang is not None:
        result += get_result_from_process_internal(process.clang, "clang")
    if process.run is not None:
        result += get_result_from_process_internal(process.run, "run")
    return result

# TODO: try to avoid using do_debug for both function name and parameter name
def compile_test(do_debug: bool, output_name: str, file: FileItem) -> TestResult:
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

    compile_cmd.append("compile")
    compile_cmd.append(os.path.join(INPUTS_DIR, file.path_base))
    compile_cmd.append("--emit-llvm")
    compile_cmd.append("--log-level=NOTE")

    print_info("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
    compile_out: subprocess.CompletedProcess[bytes] = subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if compile_out.returncode != 0:
        return TestResult(compile_out, None, None)

    clang_cmd = ["clang", output_name, "-Wno-override-module", "-Wno-incompatible-library-redeclaration", "-Wno-builtin-requires-header", "-o", "test"]
    clang_out: subprocess.CompletedProcess[bytes]  = subprocess.run(clang_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if clang_out.returncode != 0:
        return TestResult(compile_out, clang_out, None)

    test_cmd = ["./test"]
    run_out: subprocess.CompletedProcess[bytes] = subprocess.run(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return TestResult(compile_out, clang_out, run_out)


def do_tests(files_to_test: list[str], do_debug: bool, output_name: str, action: Action, count_threads: int, keep_going: bool):
    success = True

    debug_env: str
    debug_release_text: str
    if do_debug:
        debug_release_text = "debug"
        debug_env = "1"
    else:
        debug_release_text = "release"
        debug_env = "0"

    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling " + debug_release_text + " :")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": debug_env}))
    if process.returncode != 0:
        print_error("compilation of " + debug_release_text + " failed")
        sys.exit(1)
    print_success("compiling " + debug_release_text + " : done")
    print()

    for file in get_files_to_test(files_to_test):
        if not test_file(file, True, get_expected_output(file), output_name, action, debug_release_text):
            if not keep_going:
                sys.exit(1)
            success = False
    if not success:
        sys.exit(1)



# return true if test was successful
def test_file(file: FileItem, do_debug: bool, expected_output: str, output_name: str, action: Action, debug_release_text: str) -> bool:
    result: TestResult = compile_test(do_debug, output_name, file)

    process_result: str = get_result_from_test_result(result)
    if action == Action.UPDATE:
        with open(os.path.join(RESULTS_DIR, file.path_base), "w") as newResult:
            newResult.write(process_result)
        return True
    elif action == Action.TEST:
        pass
    else:
        raise AssertionError("uncovered case")

    if process_result != expected_output:
        stdout_color: str = ""
        expected_color: str = ""
        print_error("test fail:" + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
        diff = difflib.SequenceMatcher(None, expected_output, process_result)
        for tag, expected_start, expected_end, stdout_start, stdout_end, in diff.get_opcodes():
            if tag == 'insert':
                stdout_color += Changes.ADDED + \
                                process_result[stdout_start:stdout_end] + \
                                Changes.TO_NORMAL
            elif tag == 'equal':
                expected_color += process_result[stdout_start:stdout_end]
                stdout_color += process_result[stdout_start:stdout_end]
            elif tag == 'replace':
                expected_color += Changes.REMOVED + \
                                  expected_output[expected_start:expected_end] + \
                                  Changes.TO_NORMAL
                stdout_color += Changes.ADDED + \
                                process_result[stdout_start:stdout_end] + \
                                Changes.TO_NORMAL
            elif tag == 'delete':
                expected_color += Changes.REMOVED + \
                                  expected_output[expected_start:expected_end] + \
                                  Changes.TO_NORMAL
            else:
                print_error("tag unregonized:" + tag)
                assert False
        print_info("expected output:")
        print(expected_color)
        print_info("actual output:")
        print(stdout_color)
        return False

    print_success("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ") success")
    print()
    return True

def append_all_files(list_or_map: list | dict, callback: Callable):
    possible_path: str
    for possible_base in map(to_str, os.listdir(INPUTS_DIR)):
        possible_path = os.path.realpath(os.path.join(INPUTS_DIR, possible_base))
        if os.path.isfile(possible_path):
            callback(list_or_map, possible_path)

def add_to_map(map: dict, path: str):
    map[path] = 0

def parse_args() -> Tuple[list[str], str, Action, bool]:
    action: Action = Action.TEST
    test_output = "test.c" # TODO: be more consistant with test_output variable names
    to_include: dict[str, int] = {}
    keep_going: bool = True
    has_found_flag = False
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
    return to_include_list, test_output, action, keep_going

def main() -> None:
    files_to_test: list[str]
    test_output: str
    action: Action
    keep_going: bool
    files_to_test, test_output, action, keep_going = parse_args()

    count_threads: int
    try:
        count_threads = multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    do_tests(files_to_test, True, test_output, action, count_threads, keep_going)
    do_tests(files_to_test, False, test_output, action, count_threads, keep_going)
    print_success("all tests passed")

if __name__ == '__main__':
    main()
