
ECHO "building project"

set CC_COMPILER=clang

set WARNINGS=^
    -Wall ^
    -Wextra ^
    -Wno-macro-redefined ^
    -Wno-unused-function ^
    -D _CRT_SECURE_NO_WARNINGS

if "%SHOULD_PRINT_POSIX_MSG%"=="1" (
    set EXTRA_FLAGS=-D PRINT_POSIX_MSG
) else (
    set EXTRA_FLAGS=
)

set BUILD_DIR=build\release

set AUTOGEN_C_FILES=src\util\params_log_level.c src\util\arena.c src\util\auto_gen\auto_gen.c src\util\local_string.c
set AUTOGEN_INCLUDE_PATHS=-I third_party\ -I src\util\ -I src\util\auto_gen\
 
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if %errorlevel% neq 0 exit \b %errorlevel%

%CC_COMPILER% -std=c11 %WARNINGS% %AUTOGEN_C_FILES% -o %BUILD_DIR%\auto_gen.exe %AUTOGEN_INCLUDE_PATHS% -D IN_AUTOGEN 
if %errorlevel% neq 0 exit \b %errorlevel%
%~dp0\%BUILD_DIR%\auto_gen.exe %BUILD_DIR%
if %errorlevel% neq 0 exit \b %errorlevel%
 


set LIBS=-lshlwapi

set MAIN_C_FILES=^
    src\unity_build\unity_build_ir_and_codegen.c ^
    src\unity_build\unity_build_miscellaneous.c ^
    src\unity_build\unity_build_sema.c ^
    src\unity_build\unity_build_token_and_parser.c ^
    src\util\subprocess.c ^
    src\util\winapi_wrappers.c

set MAIN_INCLUDE_PATHS=^
    -I third_party\ ^
    -I %BUILD_DIR% ^
    -I src\ ^
    -I src\util\ ^
    -I src\util\auto_gen\ ^
    -I src\token ^
    -I src\sema ^
    -I src\codegen ^
    -I src\lang_type\ ^
    -I src\ir ^
    -I src\ast_utils\


:: TODO: avoid hardcoding 4 (LOG_LEVEL) if possible
:: TODO: MIN_LOG_LEVEL should be 3 instead of 4?
:: TODO: remove print-posix-parameter, and entirely replace with PRINT_POSIX_MSG

%CC_COMPILER% -std=c11 %WARNINGS% -o %BUILD_DIR%\main.exe %MAIN_INCLUDE_PATHS% %MAIN_C_FILES% %LIBS% -D MIN_LOG_LEVEL=4 %EXTRA_FLAGS%

dir build\release\

:: TODO: this script may not actually exit with nonzero exit code on failure
if %errorlevel% neq 0 exit \b %errorlevel%
