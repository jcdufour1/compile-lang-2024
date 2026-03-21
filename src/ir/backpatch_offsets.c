// TODO: remove this file
#include <do_passes.h>

//static void backpatch_ir_inline(Ir* ir);
//
//static void backpatch_block(Ir_block* block) {
//    darr_foreach(idx, Ir*, ir, block->children) {
//        backpatch_ir_inline(ir);
//    }
//}
//
//static void backpatch_import_path(Ir_import_path* import) {
//    backpatch_block(import->block);
//}
//
//static void backpatch_literal(Ir_literal* lit) {
//    switch (lit->type) {
//        case IR_INT:
//            return;
//        case IR_FLOAT:
//            todo();
//        case IR_STRING:
//            todo();
//        case IR_VOID:
//            todo();
//        case IR_FUNCTION_NAME:
//            todo();
//    }
//    unreachable("");
//}
//
//static void backpatch_expr(Ir_expr* expr) {
//    switch (expr->type) {
//        case IR_OPERATOR:
//            todo();
//        case IR_LITERAL:
//            backpatch_literal(ir_literal_unwrap(expr));
//            return;
//        case IR_FUNCTION_CALL:
//            todo();
//    }
//    unreachable("");
//}
//
//static void backpatch_function_def(Ir_function_def* def) {
//    String buf = {0};
//
//    //string_extend_f(&a_main/*TODO*/, &buf, "s%zu_"FMT, def->);
//
//    if (!symbol_add(tast_label_wrap(tast_label_new(
//        label->pos,
//        name_new(MOD_PATH_BACKPATCH, string_to_strv(buf), (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL),
//        block_scope
//    )))) {
//        return ir_removed_wrap(ir_removed_new(label->pos));
//    }
//}
//
//static void backpatch_def_out_of_line(Ir_def* def) {
//    switch (def->type) {
//        case IR_FUNCTION_DEF:
//            backpatch_function_def(ir_function_def_unwrap(def));
//            return;
//        case IR_VARIABLE_DEF:
//            todo();
//        case IR_STRUCT_DEF:
//            todo();
//        case IR_PRIMITIVE_DEF:
//            todo();
//        case IR_FUNCTION_DECL:
//            todo();
//        case IR_LABEL:
//            return;
//        case IR_LITERAL_DEF:
//            todo();
//    }
//    unreachable("");
//}
//
//static void backpatch_ir_out_of_line(Ir* ir) {
//    switch (ir->type) {
//        case IR_BLOCK:
//            todo();
//        case IR_EXPR:
//            backpatch_expr(ir_expr_unwrap(ir));
//            return;
//        case IR_LOAD_ELEMENT_PTR:
//            todo();
//        case IR_ARRAY_ACCESS:
//            todo();
//        case IR_FUNCTION_PARAMS:
//            todo();
//        case IR_DEF:
//            backpatch_def_out_of_line(ir_def_unwrap(ir));
//            return;
//        case IR_RETURN:
//            todo();
//        case IR_GOTO:
//            todo();
//        case IR_COND_GOTO:
//            return;
//        case IR_ALLOCA:
//            return;
//        case IR_LOAD_ANOTHER_IR:
//            return;
//        case IR_STORE_ANOTHER_IR:
//            return;
//        case IR_IMPORT_PATH:
//            backpatch_import_path(ir_import_path_unwrap(ir));
//            return;
//        case IR_STRUCT_MEMB_DEF:
//            todo();
//        case IR_REMOVED:
//            todo();
//    }
//    unreachable("");
//}
//
//static void backpatch_ir_inline(Ir* ir) {
//    switch (ir->type) {
//        case IR_BLOCK:
//            todo();
//        case IR_EXPR:
//            todo();
//        case IR_LOAD_ELEMENT_PTR:
//            todo();
//        case IR_ARRAY_ACCESS:
//            todo();
//        case IR_FUNCTION_PARAMS:
//            todo();
//        case IR_DEF:
//            todo();
//        case IR_RETURN:
//            todo();
//        case IR_GOTO:
//            todo();
//        case IR_COND_GOTO:
//            todo();
//        case IR_ALLOCA:
//            todo();
//        case IR_LOAD_ANOTHER_IR:
//            todo();
//        case IR_STORE_ANOTHER_IR:
//            todo();
//        case IR_IMPORT_PATH:
//            todo();
//        case IR_STRUCT_MEMB_DEF:
//            todo();
//        case IR_REMOVED:
//            return;
//    }
//    unreachable("");
//}
//
//void backpatch_offsets(void) {
//    Ir_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
//    Ir* curr = NULL;
//    while (ir_tbl_iter_next(&curr, &iter)) {
//        backpatch_ir_out_of_line(curr);
//    }
//}
