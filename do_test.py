import os, subprocess, sys, pathlib

EXAMPLES_DIR = "./examples/new_lang/"

BUILD_DEBUG_DIR = "./build/debug/"
BUILD_RELEASE_DIR = "./build/release/"
EXE_BASE_NAME = "main"
TEST_OUTPUT = "test.ll"
EXPECTED_DIR = "./tests/expected/"

def to_str(a):
    return str(a)

def get_files_to_test() -> list[str]:
    files: list[str] = []
    for possible_file_base in map(to_str, os.listdir(EXAMPLES_DIR)):
        possible_file: str = os.path.join(EXAMPLES_DIR, possible_file_base)
        if os.path.isfile(possible_file):
            files.append(possible_file)
    return files

def get_expected_output(file: str) -> str:
    file_expected_base, _ = os.path.splitext(os.path.basename(file))
    file_expected = os.path.join(EXPECTED_DIR, file_expected_base)
    file_expected += ".txt"
    with open(file_expected, "r") as input_file:
        return input_file.read()

# return true if test was successful
def do_test(file: str, do_debug: bool, expected_output: str) -> bool:
    debug_release_text: str
    compile_cmd: list[str]
    if do_debug:
        compile_cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME)]
        debug_release_text = "debug"
    else:
        compile_cmd = [os.path.join(BUILD_RELEASE_DIR, EXE_BASE_NAME)]
        debug_release_text = "release"
    compile_cmd.append("compile")
    compile_cmd.append(file)
    compile_cmd.append("--emit-llvm")
    print("testing: " + file + " (" + debug_release_text + ")")
    process = subprocess.run(compile_cmd)
    if process.returncode != 0:
        print("testing: compilation of " + file + " (" + debug_release_text + ") fail")
        return False

    clang_cmd = ["clang", TEST_OUTPUT, "-o", "test"]
    process = subprocess.run(clang_cmd)
    if process.returncode != 0:
        print("testing: clang " + file + " (" + debug_release_text + ") fail")
        return False

    test_cmd = ["./test"]
    process = subprocess.run(test_cmd, stdout=subprocess.PIPE, text=True)
    print(process.stdout)
    if process.returncode != 0:
        print("testing: test " + file + " (" + debug_release_text + ") fail (returned exit code " + str(process.returncode))
        return False
    if str(process.stdout) != expected_output:
        print("testing: test " + file + " (" + debug_release_text + ") fail (stdout did not match expected)")
        print("stdout:")
        print(str(process.stdout))
        print("expected:")
        print(expected_output)
        return False

    print("testing: " + file + " (" + debug_release_text + ") success")
    print()
    return True

def main() -> None:
    debug_fail_count: int = 0
    release_fail_count: int = 0

    cmd = ["make", "-j4", "build"]
    print("compiling debug:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "1"}))
    if process.returncode != 0:
        print("compilation of debug failed", file=sys.stderr)
        sys.exit(1)
    print("compiling debug: done")
    print()

    cmd = ["make", "-j4", "build"]
    print("compiling release:")
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "0"}))
    if process.returncode != 0:
        print("compilation of release failed", file=sys.stderr)
        sys.exit(1)
    print("compiling release: done")
    print()

    for file in get_files_to_test():
        if not do_test(file, True, get_expected_output(file)):
            debug_fail_count += 1
        if not do_test(file, False, get_expected_output(file)):
            release_fail_count += 1

    if debug_fail_count == 0 and release_fail_count == 0:
        print("all tests passed")
        sys.exit(0)

    print(str(debug_fail_count) + " debug tests failed")
    print(str(release_fail_count) + " release tests failed")
    sys.exit(1)


if __name__ == '__main__':
    main()
