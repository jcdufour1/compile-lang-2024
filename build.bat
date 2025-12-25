
ECHO "building project"

set CC_COMPILER=clang

set BUILD_DIR=build/release/

set AUTOGEN_C_FILES=src/util/params_log_level.c src/util/arena.c src/util/auto_gen/auto_gen.c src/util/newstring.c
set AUTOGEN_INCLUDE_PATHS=-I third_party/ -I src/util/ -I src/util/auto_gen/
 
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if %errorlevel% neq 0 exit /b %errorlevel%

%CC_COMPILER% %AUTOGEN_C_FILES% -o %BUILD_DIR%/auto_gen %AUTOGEN_INCLUDE_PATHS% -D IN_AUTOGEN 
if %errorlevel% neq 0 exit /b %errorlevel%
%BUILD_DIR%/auto_gen %BUILD_DIR%
if %errorlevel% neq 0 exit /b %errorlevel%



set LIBS=-lshlwapi

set MAIN_C_FILES=^
    src/util/name.c ^
    src/util/str_and_num_utils.c ^
    src/main.c ^
    src/util/arena.c ^
    src/ast_utils/uast_print.c ^
    src/ast_utils/tast_print.c ^
    src/ast_utils/tast_utils.c ^
    src/lang_type/lang_type_after.c ^
    src/lang_type/ir_lang_type_after.c ^
    src/ir/ir_print.c ^
    src/ir/remove_void_assigns.c ^
    src/ir/check_uninitialized.c ^
    src/ir/construct_cfgs.c ^
    src/lang_type/lang_type_print.c ^
    src/lang_type/ir_lang_type_print.c ^
    src/lang_type/ulang_type_print.c ^
    src/globals.c ^
    src/ast_utils/uast_utils.c ^
    src/ast_utils/symbol_table.c ^
    src/util/file.c ^
    src/util/parameters.c ^
    src/util/operator_type.c ^
    src/util/ir_operator_type.c ^
    src/util/params_log_level.c ^
    src/util/cfg.c ^
    src/util/newstring.c ^
    src/util/winapi_wrappers.c ^
    src/error_msg.c ^
    src/lang_type/ulang_type_serialize.c ^
    src/lang_type/lang_type_from_ulang_type.c ^
    src/lang_type/ulang_type_is_equal.c ^
    src/ast_utils/uast_clone.c ^
    src/ast_utils/did_you_mean.c ^
    src/ast_utils/ast_msg.c ^
    src/ast_utils/symbol_collection_clone.c ^
    src/sema/uast_expr_to_ulang_type.c ^
    src/sema/check_gen_constraints.c ^
    src/sema/type_checking.c ^
    src/sema/expand_lang_def.c ^
    src/sema/expand_using.c ^
    src/sema/check_general_assignment.c ^
    src/sema/resolve_generics.c ^
    src/sema/generic_sub.c ^
    src/sema/infer_generic_type.c ^
    src/sema/check_struct_recursion.c ^
    src/ast_utils/sizeof.c ^
    src/token/token.c ^
    src/token/tokenizer.c ^
    src/parser.c ^
    src/ir/add_load_and_store.c ^
    src/codegen/codegen_common.c ^
    src/codegen/emit_llvm.c ^
    src/codegen/emit_c.c ^
    src/ir/ir_utils.c ^
    src/ir/ir_graphvis.c ^
    src/util/subprocess.c


set MAIN_INCLUDE_PATHS=^
    -I third_party/ ^
    -I %BUILD_DIR% ^
    -I src/ ^
    -I src/util/ ^
    -I src/util/auto_gen/ ^
    -I src/token ^
    -I src/sema ^
    -I src/codegen ^
    -I src/lang_type/ ^
    -I src/ir ^
    -I src/ast_utils/


%CC_COMPILER% -DNDEBUG -O2 -Wno-deprecated-declarations -o %BUILD_DIR%/main %MAIN_INCLUDE_PATHS% %MAIN_C_FILES% %LIBS% 
if %errorlevel% neq 0 exit /b %errorlevel%
