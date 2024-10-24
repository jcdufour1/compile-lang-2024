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
    def __init__(self, path: str, expected_success: bool):
        self.path = path
        self.expected_success = expected_success
    path: str
    expected_success: bool

EXAMPLES_DIR = "./examples/new_lang/"
EXPECTED_FAIL_EXAMPLES_DIR = "./tests/expected_failure_examples/"

BUILD_DEBUG_DIR = "./build/debug/"
BUILD_RELEASE_DIR = "./build/release/"
EXE_BASE_NAME = "main"
TEST_OUTPUT = "test.ll"
EXPECTED_SUCCESS_RESULTS_DIR = "./tests/expected_success_results/"

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

def get_files_to_test(files_to_exclude: list[str]) -> list[FileItem]:
    files: list[FileItem] = []
    for possible_file_base in map(to_str, os.listdir(EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path) and possible_file_path not in files_to_exclude:
            files.append(FileItem(possible_file_path, True))
    for possible_file_base in map(to_str, os.listdir(EXPECTED_FAIL_EXAMPLES_DIR)):
        possible_file_path: str = os.path.realpath(os.path.join(EXPECTED_FAIL_EXAMPLES_DIR, possible_file_base))
        if os.path.isfile(possible_file_path) and possible_file_path not in files_to_exclude:
            files.append(FileItem(possible_file_path, False))
    return files

def get_expected_output(file: FileItem) -> str:
    file_expected_base, _ = os.path.splitext(os.path.basename(file.path))
    file_expected = os.path.join(EXPECTED_SUCCESS_RESULTS_DIR, file_expected_base)
    file_expected += ".txt"
    with open(file_expected, "r") as input_file:
        return input_file.read()

# return true if test was successful
def do_test(file: FileItem, do_debug: bool, expected_output: str) -> bool:
    debug_release_text: str
    compile_cmd: list[str]
    if do_debug:
        compile_cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME)]
        debug_release_text = "debug"
    else:
        compile_cmd = [os.path.join(BUILD_RELEASE_DIR, EXE_BASE_NAME)]
        debug_release_text = "release"
    compile_cmd.append("compile")
    compile_cmd.append(file.path)
    compile_cmd.append("--emit-llvm")
    print_info("testing: " + file.path + " (" + debug_release_text + ")")
    process = subprocess.run(compile_cmd)
    if not file.expected_success:
        if process.returncode == 0:
            print_error("testing: compilation of " + file.path + " (" + debug_release_text + ") returned 0 exit code, but failure was expected")
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

    clang_cmd = ["clang", TEST_OUTPUT, "-o", "test"]
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

def do_debug(files_to_exclude: list[str], count_threads: int) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling debug:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "1"}))
    if process.returncode != 0:
        print_error("compilation of debug failed")
        sys.exit(1)
    print_success("compiling debug: done")

    for file in get_files_to_test(files_to_exclude):
        expected_output: str
        if file.expected_success:
            expected_output = get_expected_output(file)
        else:
            expected_output = ""
        do_test(file, do_debug=True, expected_output=expected_output)
    print_success("testing debug: done")
    print()

def do_release(files_to_exclude: list[str], count_threads: int) -> None:
    cmd = ["make", "-j", str(count_threads), "build"]
    print_info("compiling release:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "0"}))
    if process.returncode != 0:
        print_error("compilation of release failed")
        sys.exit(1)
    print_success("compiling release: done")
    print()

    for file in get_files_to_test(files_to_exclude):
        expected_output: str
        if file.expected_success:
            expected_output = get_expected_output(file)
        else:
            expected_output = ""
        do_test(file, do_debug=False, expected_output=expected_output)
    print_success("testing release: done")
    print()

def parse_args() -> list[str]:
    list_files: list[str] = []
    for arg in sys.argv:
        if arg.startswith("--exclude="):
            files_to_exclude = arg[len("--exclude="):]
            for idx, path in enumerate(files_to_exclude.split(",")):
                list_files.append(os.path.realpath(path))
    return list_files

def main() -> None:
    files_to_exclude: list[str] = parse_args()

    count_threads: int
    try:
        count_threads =  multiprocessing.cpu_count()
    except Exception as e:
        print_warning("could not determine number of cpus; assuming 2")
        print_warning(e, file=sys.stderr)
        count_threads = 2

    do_debug(files_to_exclude, count_threads)
    do_release(files_to_exclude, count_threads)
    print_success("all tests passed")

if __name__ == '__main__':
    main()
