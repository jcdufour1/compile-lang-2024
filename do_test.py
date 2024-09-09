import os, subprocess

EXAMPLES_DIR = "./examples/new_lang/"

BUILD_DEBUG_DIR = "./build/debug/"
BUILD_RELEASE_DIR = "./build/release/"
EXE_BASE_NAME = "main"

def to_str(a):
    return str(a)

def get_files_to_test() -> list[str]:
    files: list[str] = []
    for possible_file_base in map(to_str, os.listdir(EXAMPLES_DIR)):
        possible_file: str = os.path.join(EXAMPLES_DIR, possible_file_base)
        if os.path.isfile(possible_file):
            files.append(possible_file)
    return files

def main() -> None:
    cmd = ["make", "-j4", "build"]
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "1"}))
    print("compiling debug:")
    if process.returncode != 0:
        assert(False and "compilation failed")

    cmd = ["make", "-j4", "build"]
    process = subprocess.run(cmd, env=dict(os.environ | {"DEBUG": "0"}))
    print("compiling release:")
    if process.returncode != 0:
        assert(False and "compilation failed")

    for file in get_files_to_test():
        cmd = [os.path.join(BUILD_DEBUG_DIR, EXE_BASE_NAME), "compile"]
        status = subprocess.call(cmd)
        print("testing:", file)
        if status != 0:
            assert(False and "test failed")


if __name__ == '__main__':
    main()
