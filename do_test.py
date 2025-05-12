import os, subprocess, sys, pathlib, difflib, multiprocessing
from typing import Callable, Tuple

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
            files.append(FileItem(possible_path, True, None))
    return files

def get_expected_output(file: FileItem) -> str:
    expect_base, _ = os.path.splitext(os.path.basename(file.path))
    expected = os.path.join(RESULTS_DIR, expect_base)
    expected += ".txt"
    with open(expected, "r") as input:
        return input.read()

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
        compile_cmd.append("test-expected-fail")
        if file.expected_fail_str is None:
            raise AssertionError()
        compile_cmd.append(str(len(file.expected_fail_str)))
        compile_cmd.extend(file.expected_fail_str)
        compile_cmd.append(file.path)

    print_info("testing: " + file.path + " (" + debug_release_text + ")")
    process: subprocess.CompletedProcess[str]
    process = subprocess.run(compile_cmd, text=True)
    if not file.expected_success:
        if process.returncode != 2:
            print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") returned " + str(process.returncode) + " exit code, but expected failure (exit code 2) was expected")
            sys.exit(1)
            return
        else:
            print_success("testing: " + file.path + " (" + debug_release_text + ") expected fail success")
            print()
            return True

    if process.returncode != 0:
        print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") fail")
        sys.exit(1)
        return
    if process.returncode != 0:
        print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") fail")
        sys.exit(1)

    clang_cmd = ["clang", output_name, "-o", "test"]
    process = subprocess.run(clang_cmd, text=True)
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
    return True

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

def append_all_files(list_or_map: list | dict, callback: Callable):
    possible_path: str
    for possible_base in map(to_str, os.listdir(INPUTS_DIR)):
        possible_path = os.path.realpath(os.path.join(INPUTS_DIR, possible_base))
        if os.path.isfile(possible_path):
            callback(list_or_map, possible_path)

def add_to_map(map: dict, path: str):
    map[path] = 0

def parse_args() -> Tuple[list[str], str]:
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
