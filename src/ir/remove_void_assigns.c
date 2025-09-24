
static Arena* a_old = NULL;
static Arena* a_new = NULL;

static void rm_void_block(Ir_block* block) {
    //Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);

    size_t removed_cnt = 0;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Ir* curr_ = vec_at(&block->children, idx);
        if (curr->type != IR_DEF) {
            continue;
        }
        Ir_def* curr = if_def_unwrap(curr_);
        if (curr->type != IR_VARIABLE_DEF) {
            continue;
        }
        Ir_variable_def* var = ir_variable_def_unwrap(curr);
        if (var->lang_type.type == LLVM_LANG_TYPE_VOID) {
            todo();
        }
    }

    if (removed_cnt > 0) {
        block->children.count--;
        todo();
    }
}

void remove_void_assigns(Ir_block* block, Arena* a_old_, Arena* a_new_) {
    a_old = a_old_;
    a_new = a_new_;

    rm_void_block(block);

    todo();
}

