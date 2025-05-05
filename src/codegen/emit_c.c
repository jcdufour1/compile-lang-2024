#include <llvm.h>
#include <util.h>
#include <symbol_iter.h>
#include <msg.h>
#include <errno.h>

void emit_c_from_tree(const Llvm_block* root) {
    String struct_defs = {0};
    String output = {0};
    String literals = {0};

    Alloca_iter iter = all_tbl_iter_new(0);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        todo();
        //emit_c_sometimes(&struct_defs, &output, &literals, curr);
    }

    todo();
    //emit_c_block(&struct_defs, &output, &literals, root);

    FILE* file = fopen("test.c", "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < struct_defs.info.count; idx++) {
        if (EOF == fputc(vec_at(&struct_defs, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < output.info.count; idx++) {
        if (EOF == fputc(vec_at(&output, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < literals.info.count; idx++) {
        if (EOF == fputc(vec_at(&literals, idx), file)) {
            todo();
        }
    }

    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "file %s built\n",
        params.input_file_name
    );

    fclose(file);
}
