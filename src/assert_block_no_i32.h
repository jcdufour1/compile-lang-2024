#ifndef ASSERT_BLOCK_NO_I32_H
#define ASSERT_BLOCK_NO_I32_H

extern const Uast_block* old_block;

static inline void assert_block_no_i32(void) {
    Uast_def* dummy = NULL;
    unwrap(usym_tbl_lookup(&dummy, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(dummy)->lang_type).atom.str, "i32")) {
        unreachable("%p\n", (void*)dummy);
    }
    unwrap(usym_tbl_lookup(&dummy, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(dummy)->lang_type).atom.str, "i32")) {
        todo();
    }
}

static inline void assert_block_no_original_for(const Uast_def* def, const Uast_block* new_block) {
    for (size_t idx = 0; idx < new_block->children.info.count; idx++) {
        const Uast_stmt* curr__ = vec_at(&new_block->children, idx);
        if (curr__->type != UAST_DEF) {
            continue;
        }
        const Uast_def* curr_ = uast_def_const_unwrap(curr__);
        //if (curr_->type != UAST_VARIABLE_DEF) {
            //continue;
        //}
        //const Uast_variable_def* curr = uast_variable_def_const_unwrap(curr_);

        unwrap(def != curr_);
    }
}

static inline void assert_block_no_original(const Uast_block* new_block) {
    Uast_def* def = NULL;
    unwrap(usym_tbl_lookup(&def, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(def)->lang_type).atom.str, "i32")) {
        assert_block_no_original_for(def, new_block);
        unreachable("%p\n", (void*)def);
    }
    unwrap(usym_tbl_lookup(&def, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(def)->lang_type).atom.str, "i32")) {
        assert_block_no_original_for(def, new_block);
        todo();
    }

}

static inline void assert_block_not_match(const Uast_variable_def* new_def) {
    Uast_def* def = NULL;
    unwrap(usym_tbl_lookup(&def, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(def)->lang_type).atom.str, "i32")) {
        unreachable("%p\n", (void*)def);
    }
    unwrap(usym_tbl_lookup(&def, &old_block->symbol_collection.usymbol_table, str_view_from_cstr("dummy")));
    if (str_view_cstr_is_equal(ulang_type_regular_const_unwrap(uast_variable_def_unwrap(def)->lang_type).atom.str, "i32")) {
        todo();
    }

    unwrap((void*)def != (void*)new_def);
}

#endif // ASSERT_BLOCK_NO_I32_H
