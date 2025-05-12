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
    clang: Optional[subprocess.CompletedProcess[str]]
    run: Optional[subprocess.CompletedProcess[str]]

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
    with open(expected, "r") as input:
        return input.read()

def get_result_from_process_internal(process: subprocess.CompletedProcess[str], type_str: str) -> str:
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

    print_info("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ")")
    compile_out: subprocess.CompletedProcess[str] = subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if compile_out.returncode != 0:
        return TestResult(compile_out, None, None)

    clang_cmd = ["clang", output_name, "-Wno-incompatible-library-redeclaration", "-Wno-builtin-requires-header", "-o", "test"]
    clang_out: subprocess.CompletedProcess[str]  = subprocess.run(clang_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if clang_out.returncode != 0:
        return TestResult(compile_out, clang_out, None)

    test_cmd = ["./test"]
    run_out: subprocess.CompletedProcess[str] = subprocess.run(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return TestResult(compile_out, clang_out, run_out)


# return true if test was successful
def do_test(file: FileItem, do_debug: bool, expected_output: str, output_name: str, action: Action) -> bool:
    result: TestResult = compile_test(do_debug, output_name, file)

    debug_release_text: str
    if do_debug:
        debug_release_text = "debug"
    else:
        debug_release_text = "release"

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
        sys.exit(1)

    print_success("testing: " + os.path.join(INPUTS_DIR, file.path_base) + " (" + debug_release_text + ") success")
    print()
    return True

def do_debug(files_to_test: list[str], count_threads: int, output_name: str, action: Action) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling debug:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "1"}))
    if process.returncode != 0:
        print_error("compilation of debug failed")
        sys.exit(1)
    print_success("compiling debug: done")

    for file in get_files_to_test(files_to_test):
        do_test(file, do_debug=True, expected_output=get_expected_output(file), output_name=output_name, action=action)
    print_success("testing debug: done")
    print()

def do_release(files_to_test: list[str], count_threads: int, output_name: str, action: Action) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling release:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "0"}))
    if process.returncode != 0:
        print_error("compilation of release failed")
        sys.exit(1)
    print_success("compiling release: done")
    print()

    for file in get_files_to_test(files_to_test):
        do_test(file, do_debug=False, expected_output=get_expected_output(file), output_name=output_name, action=action)
    print_success("testing release: done")
    print()

def append_all_files(list_or_map: list | dict, callback: Callable):
    possible_path: str
    for possible_base in map(to_str, os.listdir(INPUTS_DIR)):
        possible_path = os.path.realpath(os.path.join(INPUTS_DIR, possible_base))
        if os.path.isfile(possible_path):
            callback(list_or_map, possible_path)

def add_to_map(map: dict, path: str):
    map[path] = 0

def parse_args() -> Tuple[list[str], str, Action]:
    action: Action = Action.TEST
    test_output = "" # TODO: be more consistant with test_output variable names
    to_include: dict[str, int] = {}
    has_found_flag = False
    for arg in sys.argv[1:]:
        if arg.startswith("--update"):
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
    return to_include_list, test_output, action

def main() -> None:
    files_to_test: list[str]
    test_output: str
    action: Action
    files_to_test, test_output, action = parse_args()

    count_threads: int
    try:
        count_threads = multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    do_debug(files_to_test, count_threads, test_output, action)
    do_release(files_to_test, count_threads, test_output, action)
    print_success("all tests passed")

if __name__ == '__main__':
    main()
