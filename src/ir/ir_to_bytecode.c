#include <ir_to_bytecode.h>
#include <uint8_t_darr.h>
#include <bytecode.h>
#include <pos_util.h>
#include <offsetof.h>

// TODO: move this definition?
Bytecode bytecode;

uint64_t bytecode_stack_offset_ = 0;
uint64_t bytecode_locals_offset = 0;
uint64_t bytecode_locals_pos = 0;
uint64_t bytecode_locals_pos_at_max_alloc = 0;
uint64_t bytecode_space_locals_alloca_size = 0; // TODO: rename
static bool bytecode_is_backpatching = false;
static Ir_variable_def_darr curr_fun_args = {0};
static Name bytecode_fun_name = {0};
static Pos bytecode_fun_pos = {0};

static Name symbol_name_to_int_name(Name sym_name) {
    assert(sym_name.base.count > 0);
    return name_new(
        MOD_PATH_BACKPATCH_STACK_SIZE,
        serialize_name(sym_name),
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
}

static Name function_name_to_loc(Name sym_name) {
    assert(sym_name.base.count > 0);
    return name_new(
        MOD_PATH_FUNCTION_TO_LOC,
        serialize_name(sym_name),
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
}

static Name fun_name_to_space_locals_alloc_size(Name sym_name) {
    assert(sym_name.base.count > 0);
    return name_new(
        MOD_PATH_FUN_NAME_TO_SPACE_LOCALS_ALLOC,
        serialize_name(sym_name),
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
}

static void ir_to_bytecode_uint64_t(uint64_t value) {
    bytecode_align();
    size_t old_count = bytecode.code.info.count;
    (void) old_count;

    uint8_t temp[8] = {0};
    memcpy(temp, &value, sizeof(value));
    darr_extend_array(&a_main, &bytecode.code, temp);

    assert(bytecode.code.info.count - old_count == 8);
}

static void ir_to_bytecode_int64_t(int64_t value) {
    ir_to_bytecode_uint64_t(*(uint64_t*)&value);
}

static void ir_to_bytecode_size_t(size_t value) {
    bytecode_align();
    //size_t old_count = bytecode.code.info.count;
    (void) value;


    todo();
    //assert(bytecode.code.info.count - old_count == 8);
}



static_assert(BYTECODE_COUNT == 19, "add function ir_to_bytecode_append_* for new bytecode type if nessessary");
static void ir_to_bytecode_append_store_stack_dir_addr(uint64_t dest, int64_t src_value, uint64_t sizeof_data, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_STORE_STACK_DIR_ADDR);
    ir_to_bytecode_uint64_t(dest);
    ir_to_bytecode_int64_t(src_value);
    ir_to_bytecode_uint64_t(sizeof_data);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_DIR_ADDR_SIZE);
}

static void ir_to_bytecode_append_store_stack(uint64_t dest, uint64_t src, uint64_t sizeof_data, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_STORE_STACK);
    ir_to_bytecode_uint64_t(dest);
    ir_to_bytecode_uint64_t(src);
    ir_to_bytecode_uint64_t(sizeof_data);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_SIZE);
}

static void ir_to_bytecode_append_store_stack_deref_dest(uint64_t dest, uint64_t src, uint64_t sizeof_data, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_STORE_STACK_DEREF_DEST);
    ir_to_bytecode_uint64_t(dest);
    ir_to_bytecode_uint64_t(src);
    ir_to_bytecode_uint64_t(sizeof_data);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_DEREF_DEST_SIZE);
}

static void ir_to_bytecode_append_deref(uint64_t src, uint64_t sizeof_alloca, uint64_t alloca_pos, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_DEREF);
    ir_to_bytecode_uint64_t(src);
    ir_to_bytecode_uint64_t(sizeof_alloca);
    ir_to_bytecode_uint64_t(alloca_pos);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_DEREF_SIZE);
}

static void ir_to_bytecode_append_alloca(uint64_t sizeof_data, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_ALLOCA);
    ir_to_bytecode_uint64_t(sizeof_data);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_ALLOCA_SIZE);
}

static void ir_to_bytecode_append_call_direct(uint64_t addr, uint64_t arg_bytes, uint64_t rtn_alloc_pos, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_CALL_DIRECT);
    ir_to_bytecode_uint64_t(addr);
    ir_to_bytecode_uint64_t(arg_bytes);
    ir_to_bytecode_uint64_t(rtn_alloc_pos);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_CALL_DIRECT_SIZE);
}

static void ir_to_bytecode_append_goto(uint64_t addr, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_GOTO);
    ir_to_bytecode_uint64_t(addr);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_GOTO_SIZE);
}

static void ir_to_bytecode_append_cond_goto(uint64_t if_true, uint64_t if_false, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_COND_GOTO);
    ir_to_bytecode_uint64_t(if_true);
    ir_to_bytecode_uint64_t(if_false);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_COND_GOTO_SIZE);
}

static void ir_to_bytecode_append_return(uint64_t sizeof_rtn, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_RETURN);
    ir_to_bytecode_uint64_t(sizeof_rtn);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_RETURN_SIZE);
}

static void ir_to_bytecode_append_zero_extend(uint64_t sizeof_dest, uint64_t sizeof_src, uint64_t src_pos, uint64_t alloca_pos, uint64_t stack_offset_after) {
    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(BYTECODE_ZERO_EXTEND);
    ir_to_bytecode_uint64_t(sizeof_dest);
    ir_to_bytecode_uint64_t(sizeof_src);
    ir_to_bytecode_uint64_t(src_pos);
    ir_to_bytecode_uint64_t(alloca_pos);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_ZERO_EXTEND_SIZE);
}

static void ir_to_bytecode_append_binary(BYTECODE bin_type, uint64_t start_args, uint64_t lhs_pos, uint64_t rhs_pos, uint64_t alloca_pos, uint64_t sizeof_bin_lang_type, uint64_t stack_offset_after) {
    if (!bytecode_is_backpatching) {
        assert(rhs_pos > lhs_pos);
        assert(start_args > lhs_pos);
    }

    static_assert(BYTECODE_COUNT == 19, "exhausive handling of binary types in below assertion");
    assert(
        (
            bin_type == BYTECODE_ADD_ || 
            bin_type == BYTECODE_SUB || 
            bin_type == BYTECODE_GREATER_THAN || 
            bin_type == BYTECODE_LESS_THAN || 
            bin_type == BYTECODE_DOUBLE_EQUAL || 
            bin_type == BYTECODE_NOT_EQUAL 
        ) && 
        "bin_type is not a binary type"
    );

    uint64_t old_count = bytecode.code.info.count;

    bytecode_append_align(bin_type);
    ir_to_bytecode_uint64_t(start_args);
    ir_to_bytecode_uint64_t(alloca_pos);
    ir_to_bytecode_uint64_t(sizeof_bin_lang_type);
    ir_to_bytecode_uint64_t(stack_offset_after);

    assert(bytecode.code.info.count - old_count == BYTECODE_BINARY_SIZE);
}



static Strv char_print_internal(char ch) {
    return strv_from_f(&a_main/*TODO*/, "%c", ch);
}

// TODO: remove or rename
#define char_print(ch) strv_print(char_print_internal(ch))

static Strv uint8_t_print_internal(uint8_t ch) {
    return strv_from_f(&a_main/*TODO*/, "%"PRIu8"", ch);
}

// TODO: remove or rename
#define uint8_t_print(ch) strv_print(uint8_t_print_internal(ch))

__attribute__((format (printf, 3, 4)))
static void ir_to_bytecode_comment_internal(const char* file, int line, const char* format, ...) {
    uint64_t old_bytecode_count = bytecode.code.info.count;

    // comments are a static size to prevent differences in bytecode size between 1st pass and 2nd pass
    //   (because position, etc. may not be populated yet)
    static const size_t MAX_COMMENT_SIZE = 512;

    (void) line;
    String buf = {0};
    {
        va_list args1;
        va_start(args1, format);

        size_t count = string_extend_f_va(&a_main/*TODO*/, &buf, format, args1); // returns 36
                                                                                 // buf.info.count == 35
        for (size_t idx = count; idx < MAX_COMMENT_SIZE; idx++) {
            //log(LOG_DEBUG, "%zu\n", buf.info.count);
            string_append(&a_main, &buf, ' ');
            //log(LOG_DEBUG, "%zu\n", buf.info.count);
        }
        unwrap(MAX_COMMENT_SIZE >= count && "overflow");

        va_end(args1);
    }
    //log(LOG_DEBUG, "%zu %zu\n", buf.info.count, MAX_COMMENT_SIZE);
    assert(buf.info.count == MAX_COMMENT_SIZE);

    //breakpoint();
    bytecode_align();

    bytecode_append_align(BYTECODE_COMMENT);

    size_t file_len = strlen(file);
    ir_to_bytecode_uint64_t(file_len);
    for (size_t idx = 0; idx < file_len; idx++) {
        darr_append(&a_main, &bytecode.code, (uint8_t)file[idx]);
    }
    log(LOG_DEBUG, "%zu %zu %zu\n", bytecode.code.info.count, old_bytecode_count, bytecode.code.info.count - old_bytecode_count);
    bytecode_align();
    log(LOG_DEBUG, "%zu %zu %zu\n", bytecode.code.info.count, old_bytecode_count, bytecode.code.info.count - old_bytecode_count);
    log(LOG_DEBUG, "%zu %zu %zu\n", bytecode.code.info.count, old_bytecode_count, bytecode.code.info.count - old_bytecode_count);

    ir_to_bytecode_uint64_t((uint64_t)line);

    size_t comment_len = buf.info.count;
    ir_to_bytecode_uint64_t(comment_len);
    {
        darr_foreach(idx, char, curr, buf) {
            darr_append(&a_main, &bytecode.code, (uint8_t)curr);
        }
    }
    
    bytecode_align();

    // TODO: make a function for some of below to reduce duplication?
    assert(
        bytecode.code.info.count - old_bytecode_count 
        == 
        8 /* BYTECODE_COMMENT */ + 
        8 /*file_len */ + get_next_multiple(file_len, 8) /*file*/ + 
        8 /* line */ + 
        8 /* comment_len */+ get_next_multiple(comment_len, 8) /* comment */
    );
}

#define ir_to_bytecode_comment(...) ir_to_bytecode_comment_internal(__FILE__, __LINE__, __VA_ARGS__)

static void ir_to_bytecode_def_inline(Ir_def* def);

static void ir_to_bytecode_alloca(Ir_alloca* lang_alloca, bool is_for_local_var);

static void ir_to_bytecode_store_another_ir(Ir_store_another_ir* lang_store_another_ir);

static void ir_to_bytecode_load_another_ir(Ir_load_another_ir* load);

static uint64_t ir_to_bytecode_push_ir(Ir* store_src, bool is_actual_push, bool is_ptr);

static void ir_to_bytecode_expr_inline(Ir_expr* expr);

static void ir_to_bytecode_unary(Ir_unary* unary);

static uint64_t ir_to_bytecode_pop_internal(uint64_t sizeof_pop);

static void ir_to_bytecode_binary(Ir_binary* bin);

// arg is item to jump to (offset from start of code)
static void ir_to_bytecode_goto(Ir_goto* lang_goto) {
    ir_to_bytecode_comment("goto: "FMT, ir_print(lang_goto));

    size_t old_count = bytecode.code.info.count;

    if (bytecode_is_backpatching) {
        ir_to_bytecode_append_goto(0, 0);
    } else {
        Ir* block_pos_name = NULL;
        unwrap(ir_lookup(&block_pos_name, symbol_name_to_int_name(lang_goto->label)));

        // TODO: put this assertion in cond_goto as well
        if (bytecode_stack_offset_ != bytecode_locals_pos_at_max_alloc) {
            bytecode_dump(stderr, LOG_DEBUG, false, bytecode);
            log(LOG_DEBUG, "bytecode_stack_offset_ = %zu, bytecode_locals_pos_at_max_alloc = %zu\n", bytecode_stack_offset_, bytecode_locals_pos_at_max_alloc);
            unreachable(
                "extra stack space that was allocated in the middle of the function should be freed "
                "before goto or cond_goto to allow for consistant behavior on all code paths"
            );
        }
        ir_to_bytecode_append_goto(
            (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(block_pos_name)))->data,
            bytecode_stack_offset_
        );
    }

    assert(bytecode.code.info.count - old_count == BYTECODE_GOTO_SIZE);
}

// arg is item to jump to (offset from start of code)
static void ir_to_bytecode_cond_goto(Ir_cond_goto* cond_goto) {
    ir_to_bytecode_comment("cond_goto: "FMT, ir_print(cond_goto));
    ir_to_bytecode_comment("if_true: "FMT"\n", name_print(NAME_LOG, cond_goto->if_true, NAME_FULL));
    ir_to_bytecode_comment("if_false: "FMT"\n", name_print(NAME_LOG, cond_goto->if_false, NAME_FULL));

    ir_to_bytecode_push_ir(ir_from_name(cond_goto->condition), true, false);

    size_t old_count = bytecode.code.info.count;

    if (bytecode_is_backpatching) {
        ir_to_bytecode_pop_internal(1);

        ir_to_bytecode_append_cond_goto(0, 0, bytecode_stack_offset_);
    } else {
        Ir* if_true_pos_name = NULL;
        unwrap(ir_lookup(&if_true_pos_name, symbol_name_to_int_name(cond_goto->if_true)));

        Ir* if_false_pos_name = NULL;
        unwrap(ir_lookup(&if_false_pos_name, symbol_name_to_int_name(cond_goto->if_false)));

        ir_to_bytecode_pop_internal(1);

        // TODO: put this assertion in cond_goto as well
        if (bytecode_stack_offset_ != bytecode_locals_pos_at_max_alloc) {
            bytecode_dump(stderr, LOG_DEBUG, false, bytecode);
            log(LOG_DEBUG, "bytecode_stack_offset_ = %zu, bytecode_locals_pos_at_max_alloc = %zu\n", bytecode_stack_offset_, bytecode_locals_pos_at_max_alloc);
            unreachable(
                "extra stack space that was allocated in the middle of the function should be freed "
                "before goto or cond_goto to allow for consistant behavior on all code paths"
            );
        }
        ir_to_bytecode_append_cond_goto(
            (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(if_true_pos_name)))->data,
            (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(if_false_pos_name)))->data,
            bytecode_stack_offset_
        );
    }

    assert(bytecode.code.info.count - old_count == BYTECODE_COND_GOTO_SIZE);
}

// BYTECODE_RETURN
//   arg 1 is sizeof lang_type to return
static void ir_to_bytecode_return(Ir_return* rtn) {
    ir_to_bytecode_comment("return thing thing 76");

    ir_to_bytecode_push_ir(ir_from_name(rtn->child), true, false);

    //breakpoint();
    size_t old_count = SIZE_MAX;
    (void) old_count;

    if (0 && bytecode_is_backpatching) {
        unreachable("");
    } else {
        log(LOG_DEBUG, FMT"\n", ir_print(ir_from_name(rtn->child)));
        log(LOG_DEBUG, FMT"\n", ir_print(ir_from_name(ir_load_another_ir_unwrap(ir_from_name(rtn->child))->ir_src)));
        //bytecode_dump(LOG_DEBUG, bytecode);
        //breakpoint();
        Ir* rtn_child = ir_from_name(rtn->child);
        // TODO: load loads thing to stack, when thing is already on stack?
        //bytecode_dump(LOG_DEBUG, bytecode);
        //uint64_t pos_child = ir_to_bytecode_push_ir(rtn_child, true);
        //(void) pos_child;
        rtn_child = ir_from_name(rtn->child);
        log(LOG_DEBUG, FMT"\n", ir_print(rtn_child));
        //breakpoint();
        //bytecode_dump(LOG_DEBUG, bytecode);

        log(LOG_DEBUG, "bytecode_space_locals_alloca_size = %zu\n", bytecode_space_locals_alloca_size);
        log(LOG_DEBUG, "bytecode_stack_offset = %zu\n", bytecode_stack_offset_);
        //breakpoint();
        if (bytecode_is_backpatching) {
            unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                bytecode_fun_pos,
                (int64_t)bytecode_space_locals_alloca_size,
                ir_lang_type_new_ux(64),
                fun_name_to_space_locals_alloc_size(bytecode_fun_name)
            ))))));
        }

        old_count = bytecode.code.info.count;
        if (ir_get_lang_type(rtn_child).type == IR_LANG_TYPE_VOID) {
            todo();
        }

        uint64_t sizeof_rtn_type = sizeof_ir_lang_type(ir_get_lang_type(rtn_child));

        ir_to_bytecode_append_return(sizeof_rtn_type, bytecode_stack_offset_);
        
        //bytecode_append_align();
    }

    //ir* rtn_child = ir_from_name(rtn->child);
    //if (ir_get_lang_type(ir).type == IR_LANG_TYPE_VOID) {
    //    todo();
    //}
    //bytecode_append_align(BYTECODE_PUSH);

    //(void) rtn;
    //bytecode_append_align(BYTECODE_RETURN);

    //if (bytecode_is_backpatching) {
    //    bytecode_append_align(0);
    //} else {
    //    todo();
    //}

    // TODO: remove this stack offset arg (from all instructions) in release mode, etc.?
    
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_RETURN_SIZE);
}

// TODO: in Ir_load_element_ptr, size_t member should be after Name members?
static void ir_to_bytecode_load_element_ptr(Ir_load_element_ptr* load) {
    ir_to_bytecode_comment("load_element_ptr");

    if (!bytecode_is_backpatching) {
        log(LOG_DEBUG, "prog_count = %zu\n", bytecode.code.info.count);
        //breakpoint();
    }

    Ir* src = ir_from_name(load->ir_src);
    uint64_t offset = offsetof_ir_lang_type_struct(ir_lang_type_struct_const_unwrap(ir_get_lang_type(src)), load->memb_idx);

    bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);

    // TODO: lhs_pos is incorrect the first time, but is calculated and stored in ir_add below, etc.
    ir_to_bytecode_comment("load_element_ptr lhs");
    if (!bytecode_is_backpatching) {
        //breakpoint();
    }
    uint64_t lhs_pos = ir_to_bytecode_push_ir(ir_from_name(load->ir_src), true, true);
    if (!bytecode_is_backpatching && bytecode.code.info.count > 17000) {
        bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);
        log(LOG_DEBUG, FMT"\n", ir_print(ir_from_name(load->ir_src)));
        //todo();
    }

    ir_to_bytecode_comment("load_element_ptr rhs");
    uint64_t rhs_pos = ir_to_bytecode_push_ir(
        ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            load->pos,
            (int64_t)offset,
            ir_lang_type_new_ux(64),
            util_literal_name_new())
        ))),
        true,
        false
    );
    if (!bytecode_is_backpatching && bytecode.code.info.count == 17560) {
        todo();
    }


    uint64_t start_args = rhs_pos;
    if (!bytecode_is_backpatching) {
        assert(rhs_pos > lhs_pos);
    }

    uint64_t alloca_pos = ir_to_bytecode_push_ir(ir_from_name(load->ir_src), false, false);
    if (!bytecode_is_backpatching && bytecode.code.info.count == 17560) {
        todo();
    }

    log(LOG_DEBUG, "alloca_pos = %zu, lhs_pos = %zu\n", alloca_pos, lhs_pos);

    uint64_t sizeof_alloca = sizeof_ir_lang_type(ir_get_lang_type(src));
    bytecode_stack_size_add_aligned(&bytecode_stack_offset_, sizeof_alloca);
    bytecode_stack_size_add_aligned(&bytecode_stack_offset_, sizeof_alloca);
    ir_to_bytecode_append_binary(BYTECODE_SUB, start_args, lhs_pos, rhs_pos, alloca_pos, sizeof_alloca, bytecode_stack_offset_);

    bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);
    //breakpoint();


    //ir_to_bytecode_append_store_stack(dest_pos, load_src_pos, sizeof_lang_type, bytecode_stack_offset_);

    if (alloca_pos == 64) {
        todo();
    }
    if (alloca_pos == 60) {
        todo();
    }
    if (alloca_pos == 56) {
        todo();
    }
    ir_update(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
        load->pos /* TODO*/,
        (int64_t)alloca_pos,
        ir_lang_type_new_ux(64),
        symbol_name_to_int_name(load->name_self/*TODO*/)
    )))));
}

static void ir_to_bytecode_inline(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            ir_to_bytecode_expr_inline(ir_expr_unwrap(ir));
            return;
        case IR_LOAD_ELEMENT_PTR:
            ir_to_bytecode_load_element_ptr(ir_load_element_ptr_unwrap(ir));
            return;
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            ir_to_bytecode_def_inline(ir_def_unwrap(ir));
            return;
        case IR_RETURN:
            ir_to_bytecode_return(ir_return_unwrap(ir));
            return;
        case IR_GOTO:
            ir_to_bytecode_goto(ir_goto_unwrap(ir));
            return;
        case IR_COND_GOTO:
            ir_to_bytecode_cond_goto(ir_cond_goto_unwrap(ir));
            return;
        case IR_ALLOCA:
            ir_to_bytecode_alloca(ir_alloca_unwrap(ir), true);
            return;
        case IR_LOAD_ANOTHER_IR:
            ir_to_bytecode_load_another_ir(ir_load_another_ir_unwrap(ir));
            return;
        case IR_STORE_ANOTHER_IR:
            ir_to_bytecode_store_another_ir(ir_store_another_ir_unwrap(ir));
            return;
        case IR_IMPORT_PATH:
            todo();
        case IR_STRUCT_MEMB_DEF:
            todo();
        case IR_REMOVED:
            return;
    }
    unreachable("");
}

static void ir_to_bytecode_block(Ir_block* block) {
    darr_foreach(idx, Ir*, ir, block->children) {
        //if (ir->type == IR_LITERAL) {
        //    //breakpoint();
        //}
        ir_to_bytecode_inline(ir);
        log(LOG_DEBUG, FMT"\n", ir_print(ir));
        bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);
        do_nothing();
    }
}

static void ir_to_bytecode_import_path(Ir_import_path* import) {
    ir_to_bytecode_block(import->block);
}

// should_emit_push == false if function call or add operation, etc. is doing "push"
uint64_t ir_to_bytecode_alloc_internal(uint64_t alloc_size, bool is_actual_push, bool should_emit_push) {
    assert(get_next_multiple(bytecode_stack_offset_, 8) == bytecode_stack_offset_ && "not implemented");
    assert(get_next_multiple(bytecode_locals_offset, 8) == bytecode_locals_offset && "not implemented");

    if (is_actual_push) {
        assert(bytecode_stack_offset_ < 1000);
        bytecode_stack_offset_ += get_next_multiple(alloc_size, 8);
        bytecode_stack_offset_ = get_next_multiple(bytecode_stack_offset_, 8);
        log(LOG_DEBUG, "%zu\n", bytecode_stack_offset_);

        if (should_emit_push) {
            ir_to_bytecode_append_alloca(alloc_size, bytecode_stack_offset_);
        }

        if (0) {
        }
        if (bytecode_is_backpatching) {
            return 0;
        }
        assert(bytecode_stack_offset_ < 1000);
        return bytecode_stack_offset_;
    }

    // TODO: remove bytecode_locals_offset
    bytecode_locals_offset += get_next_multiple(alloc_size, 8);
    bytecode_space_locals_alloca_size += get_next_multiple(alloc_size, 8);
    assert(bytecode_locals_pos < 1000);
    assert(bytecode_space_locals_alloca_size < 1000);
    return bytecode_locals_pos + bytecode_space_locals_alloca_size;
}

// TODO: this instruction 5takes 2 bytes. make version that takes one?
static void ir_to_bytecode_alloca(Ir_alloca* lang_alloca, bool is_for_local_var) {
    size_t old_count = 0;
    //breakpoint();
    uint64_t sizeof_alloca = sizeof_ir_lang_type(ir_lang_type_pointer_depth_dec(lang_alloca->lang_type));

    if (is_for_local_var) {
        old_count = bytecode.code.info.count;
        assert(bytecode_stack_offset_ < 1000);
        uint64_t alloca_pos = ir_to_bytecode_alloc_internal(sizeof_alloca, false/*TODO*/, false);
        assert(bytecode_stack_offset_ < 1000);

        if (bytecode_is_backpatching) {
            unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                lang_alloca->pos,
                (int64_t)alloca_pos,
                ir_lang_type_new_ux(sizeof_alloca),
                symbol_name_to_int_name(lang_alloca->name)
            ))))));
        }
        assert(bytecode.code.info.count - old_count == 0);
        return;
    }

    ir_to_bytecode_comment("alloca");

    old_count = bytecode.code.info.count;

    {
        uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset_, sizeof_alloca);

        if (bytecode_is_backpatching) {
            unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                lang_alloca->pos,
                (int64_t)alloca_pos,
                ir_lang_type_new_ux(sizeof_alloca),
                symbol_name_to_int_name(lang_alloca->name)
            ))))));
        }
    }

    ir_to_bytecode_append_alloca(sizeof_alloca, bytecode_stack_offset_);

    assert(bytecode.code.info.count - old_count == BYTECODE_ALLOCA_SIZE);
}

// BYTECODE_PUSH
//   arg 1 is size of push
//   arg 2 is item to push
static uint64_t ir_to_bytecode_push_int(Ir_int* lang_int, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    size_t old_count = bytecode.code.info.count;
    log(LOG_DEBUG, "ir_to_bytecode_push_int: old_count = %zu\n", old_count);
    uint64_t sizeof_int = sizeof_ir_lang_type(lang_int->lang_type);
    assert(bytecode_stack_offset_ < 1000);
    log(LOG_DEBUG, "bytecode_stack_offset_ = %zu\n", bytecode_stack_offset_);
    uint64_t int_pos = ir_to_bytecode_alloc_internal(sizeof_int, is_actual_push, true);
    assert(bytecode_stack_offset_ < 1000);

    assert(sizeof_int <= 64);
    // TODO: always call function similar to this
    ir_to_bytecode_append_store_stack_dir_addr(int_pos, lang_int->data, sizeof_int, bytecode_stack_offset_);
    bytecode_dump(stderr, LOG_DEBUG, true, bytecode);
    //breakpoint();


    log(LOG_DEBUG, "%zu %zu %zu\n", int_pos, lang_int->data, sizeof_int);

    //bytecode_append_align(BYTECODE_PUSH);
    //log(LOG_DEBUG, "lang_int->data = %zu, sizeof_int_push = %zu\n", lang_int->data, sizeof_int_push);
    //ir_to_bytecode_uint64_t(sizeof_int_push);

    //uint64_t item_stack_pos = 0;

    //if (bytecode_is_backpatching) {
    //    //unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
    //    //    lang_int->pos,
    //    //    (int64_t)item_stack_pos,
    //    //    ir_lang_type_new_usize(),
    //    //    symbol_name_to_backpatch_stack_size_name(lang_int_push->name)
    //    //))))));
    //    if (sizeof_int_push > 8) {
    //        todo();
    //    }

    //    item_stack_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_int_push);

    //    ir_to_bytecode_int64_t(lang_int->data);
    //} else {
    //    item_stack_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_int_push);

    //    if (sizeof_int_push > 8) {
    //        todo();
    //    }

    //    ir_to_bytecode_int64_t(lang_int->data);
    //}

    //assert(bytecode.code.info.count - old_count == BYTECODE_PUSH_SIZE);
    //return item_stack_pos;
    return int_pos;
}

static uint64_t ir_to_bytecode_push_internal(uint64_t sizeof_load, uint64_t src_pos, bool is_actual_push, bool is_ptr) {
    //bytecode_append_align(BYTECODE_ALLOCA);
    //ir_to_bytecode_uint64_t(sizeof_load);
    assert(bytecode_stack_offset_ < 1000);
    uint64_t alloca_pos = ir_to_bytecode_alloc_internal(sizeof_load, is_actual_push, true);
    assert(bytecode_stack_offset_ < 1000);

    if (is_ptr) {
        assert(sizeof_load == 8 && "pointers should always be 8 bytes");
        log(LOG_DEBUG, "inner thing =  %zu\n", src_pos);
        ir_to_bytecode_append_store_stack_dir_addr(alloca_pos, (int64_t)src_pos, sizeof_load, bytecode_stack_offset_);
        bytecode_dump(stderr, LOG_DEBUG, true, bytecode);
        //breakpoint();

    } else {
        ir_to_bytecode_append_store_stack(alloca_pos, src_pos, sizeof_load, bytecode_stack_offset_);
    }

    //uint64_t sizeof_load = sizeof_ir_lang_type(var_def->lang_type);
    //bytecode_append_align(BYTECODE_ALLOCA);
    //ir_to_bytecode_uint64_t(sizeof_load);
    //uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_load);

    //bytecode_append_align(BYTECODE_STORE_STACK);
    //ir_to_bytecode_uint64_t(alloca_pos);
    //ir_to_bytecode_uint64_t(store_src);
    //ir_to_bytecode_uint64_t(sizeof_load);

    return alloca_pos;
}

// TODO: only access bytecode_stack_offset in ir_to_bytecode_pop_internal, ir_to_bytecode_push_internal, etc.
// returns pos
static uint64_t ir_to_bytecode_pop_internal(uint64_t sizeof_pop) {
    assert(get_next_multiple(bytecode_stack_offset_, 8) == bytecode_stack_offset_ && "not implemented");

    uint64_t pos = bytecode_stack_offset_;
    bytecode_stack_offset_ -= get_next_multiple(sizeof_pop, 8);
    return pos;
}

static void ir_to_bytecode_load_another_ir(Ir_load_another_ir* load) {
    ir_to_bytecode_comment("load_another_ir");
    ir_to_bytecode_comment("load_another_ir");

    size_t old_count = SIZE_MAX;
    (void) load;

    if (0 && bytecode_is_backpatching) {
        unreachable("");
    } else {
        Ir* load_src = ir_from_name(load->ir_src);
        uint64_t sizeof_lang_type = sizeof_ir_lang_type(load->lang_type);
        assert(bytecode_stack_offset_ < 1000);
        uint64_t dest_pos = ir_to_bytecode_alloc_internal(sizeof_lang_type, false, true);
        assert(bytecode_stack_offset_ < 1000);
        log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(dest_pos));
        if (dest_pos == 552) {
            breakpoint();
        }
        ir_to_bytecode_comment("load_another_ir src: "FMT, ir_print(ir_from_name(load->ir_src)));
        if (!bytecode_is_backpatching && bytecode.code.info.count > 22288) {
            //breakpoint();
        }
        uint64_t load_src_pos = ir_to_bytecode_push_ir(load_src, false, false);

        ir_to_bytecode_comment("load_another_ir actual thing: "FMT, ir_print(load));
        ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            load->pos,
            (int64_t)dest_pos,
            ir_lang_type_new_ux(64/*TODO*/),
            symbol_name_to_int_name(load->name)
        )))));


        old_count = bytecode.code.info.count;
        if (old_count == 200) {
            todo();
        }
        ir_to_bytecode_append_store_stack(dest_pos, load_src_pos, sizeof_lang_type, bytecode_stack_offset_);
    }

    //ir_to_bytecode_uint64_t();
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_SIZE);
}

// TODO: move these comments to opcode defintions?
// BYTECODE_STORE_STACK:
//   argument 1 is the destination (number of bytes from the bottom of the stack)
//   argument 2 is the src (number of bytes from the bottom of the stack)
//   argument 3 is the size of the item to store


static uint64_t ir_to_bytecode_push_literal(Ir_literal* lit, bool is_actual_push, bool is_ptr) {
    switch (lit->type) {
        case IR_INT:
            return ir_to_bytecode_push_int(ir_int_unwrap(lit), is_actual_push, is_ptr);
        case IR_FLOAT:
            todo();
        case IR_STRING:
            todo();
        case IR_VOID:
            todo();
        case IR_FUNCTION_NAME:
            todo();
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t ir_to_bytecode_push_alloca(Ir_alloca* lang_alloca, bool is_actual_push, bool is_ptr) {
    Ir* stack_pos_name = NULL;
    unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(lang_alloca->name)));

    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);
    log(LOG_DEBUG, FMT"\n", ir_print(lang_alloca));
    uint64_t alloca_pos = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data;
    return ir_to_bytecode_push_internal(
        sizeof_ir_lang_type(ir_lang_type_pointer_depth_dec(lang_alloca->lang_type)),
        alloca_pos,
        is_actual_push,
        is_ptr
    );
}

static uint64_t ir_to_bytecode_push_load_another_ir(Ir_load_another_ir* load, bool is_from_rtn /* TODO: remove */, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    (void) is_from_rtn; // TODO: remove
    Ir* stack_pos_name = NULL;
    unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(load->name)));
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, symbol_name_to_int_name(load->name), NAME_FULL));
    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);

    //breakpoint();

    // TODO: use ir_to_bytecode_push_internal instead of below

    uint64_t sizeof_load = sizeof_ir_lang_type(load->lang_type);

    //bytecode_append_align(sizeof_ir_lang_type(load->lang_type));
    //ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data);

    uint64_t alloca_pos = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data;
    return ir_to_bytecode_push_internal(sizeof_load, alloca_pos, is_actual_push, is_ptr);
}

static uint64_t ir_to_bytecode_push_load_element_ptr(Ir_load_element_ptr* load, bool is_from_rtn /* TODO: remove */, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    (void) is_from_rtn; // TODO: remove
    Ir* stack_pos_name = NULL;
    unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(load->name_self)));
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, symbol_name_to_int_name(load->name_self), NAME_FULL));
    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);

    //breakpoint();

    // TODO: use ir_to_bytecode_push_internal instead of below

    uint64_t sizeof_load = sizeof_ir_lang_type(load->lang_type);
    log(LOG_DEBUG, FMT"\n", ir_lang_type_print(LANG_TYPE_MODE_LOG, load->lang_type));

    //bytecode_append_align(sizeof_ir_lang_type(load->lang_type));
    //ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data);

    uint64_t elem_ptr = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data;
    log(LOG_DEBUG, "%zu\n", elem_ptr);
    uint64_t dest_pos = ir_to_bytecode_alloc_internal(sizeof_load, false, false);
    // TODO: bytecode_deref does not deref, so I should remove it probably

    ir_to_bytecode_comment("push_load_element_ptr");
    Ir_lang_type inner_lang_type = ir_lang_type_pointer_depth_dec(load->lang_type);
    uint64_t sizeof_inner = sizeof_ir_lang_type(inner_lang_type);
    assert(sizeof_inner == 4);
    ir_to_bytecode_append_deref(elem_ptr, sizeof_inner/*TODO*/, dest_pos, bytecode_stack_offset_);
    bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);
    //todo();
    return ir_to_bytecode_push_internal(sizeof_inner, dest_pos, is_actual_push, is_ptr);
}

// TODO: deduplicate this and simular functions?
// /
// TODO: change IR so that variable def is not loaded
static uint64_t ir_to_bytecode_push_variable_def(Ir_variable_def* var_def, bool is_from_rtn /* TODO: remove */) {
    // TODO: make function to do this calculation in sizeof.c?
    uint64_t store_src = sizeof_prev_ir_params(curr_fun_args, var_def->name_corr_param) + 8;

    log(LOG_DEBUG, FMT"\n", loc_print(var_def->loc));
    log(LOG_DEBUG, FMT"\n", ir_print(var_def));
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, symbol_name_to_int_name(var_def->name_self), NAME_FULL));
    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);

    //breakpoint();

    // TODO: use ir_to_bytecode_push_internal instead of below

    if (is_from_rtn) {
        todo();
        //uint64_t sizeof_load = sizeof_ir_lang_type(var_def->lang_type);
        todo();
        //ir_to_bytecode_append_alloca(sizeof_load);
        todo();
        //uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_load);

        //ir_to_bytecode_append_store_stack(alloca_pos, store_src, sizeof_load);
        todo();
    }

    //bytecode_append_align(sizeof_ir_lang_type(load->lang_type));
    //ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data);

    return store_src;
}

// TODO: consider using int64_t instead of uint64_t to reduce casts
static uint64_t ir_to_bytecode_push_unsafe_cast(Ir_unary* unary, bool is_actual_push) {
    (void) is_actual_push;
    assert(unary->token_type == IR_UNARY_UNSAFE_CAST);

    // TODO: remove these comments
    
    //size_t old_count = bytecode.code.info.count;
    //log(LOG_DEBUG, "ir_to_bytecode_push_unsafe_cast: old_count = %zu\n", old_count);
    //bytecode_append_align(BYTECODE_ALLOCA);
    //uint64_t sizeof_alloca = sizeof_ir_lang_type(unary->lang_type);
    //log(LOG_DEBUG, "%zu\n", sizeof_alloca);
    //ir_to_bytecode_uint64_t(sizeof_alloca);

    //// step 1: do operation
    ////
    //// step 2: return ir_to_bytecode_push_ir(unary->child)
    //bytecode_append_align(BYTECODE_STORE_STACK);
    //todo();


    //ir_to_bytecode_unary(unary);

    Ir* int_thing_ = NULL;
    unwrap(ir_lookup(&int_thing_, symbol_name_to_int_name(unary->name)));
    //Ir_int* int_thing = ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(int_thing_)));
    //uint64_t unary_pos = (uint64_t)int_thing->data;

    todo();
    //return ir_to_bytecode_push_internal(sizeof_ir_lang_type(int_thing->lang_type), unary_pos, is_actual_push, is_ptr);
}

// returns pos
static uint64_t ir_to_bytecode_push_unary(Ir_unary* unary, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    assert(unary->token_type == IR_UNARY_UNSAFE_CAST);
    return ir_to_bytecode_push_unsafe_cast(unary, is_actual_push);
}

// returns pos
static uint64_t ir_to_bytecode_push_binary(Ir_binary* bin, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    Ir* bin_pos_ = NULL;
    unwrap(ir_lookup(&bin_pos_, symbol_name_to_int_name(bin->name)));
    uint64_t bin_pos = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(bin_pos_)))->data;

    // TODO: Ir top level loc is not set to meaningful value
    //log(LOG_DEBUG, FMT"\n", ir_print(bin_pos_));
    //log(LOG_DEBUG, FMT"\n", loc_print(ir_binary_unwrap(ir_operator_unwrap(ir_expr_unwrap(bin_pos_)))->loc));
    //log(LOG_DEBUG, FMT"\n", ir_lang_type_print(LANG_TYPE_MODE_LOG, ir_get_lang_type(bin_pos_)));
    //if (sizeof_ir_lang_type(ir_get_lang_type(bin_pos_)) > 8) {
        //breakpoint();
    //}
    assert(sizeof_ir_lang_type(ir_get_lang_type(bin_pos_)) <= 8);
    return ir_to_bytecode_push_internal(sizeof_ir_lang_type(ir_get_lang_type(bin_pos_)), bin_pos, is_actual_push, is_ptr);
}

// returns pos
static uint64_t ir_to_bytecode_push_function_call(Ir_function_call* call, bool is_actual_push, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    Ir* call_pos_ = NULL;
    unwrap(ir_lookup(&call_pos_, symbol_name_to_int_name(call->name_self)));
    uint64_t call_pos = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(call_pos_)))->data;

    return ir_to_bytecode_push_internal(sizeof_ir_lang_type(ir_get_lang_type(call_pos_)), call_pos, is_actual_push, is_ptr);
}

// returns pos
static uint64_t ir_to_bytecode_push_operator(Ir_operator* oper, bool is_actual_push, bool is_ptr) {
    switch (oper->type) {
        case IR_UNARY:
            return ir_to_bytecode_push_unary(ir_unary_unwrap(oper), is_actual_push, is_ptr);
        case IR_BINARY:
            return ir_to_bytecode_push_binary(ir_binary_unwrap(oper), is_actual_push, is_ptr);
    }
    unreachable("");
}

// returns pos
static uint64_t ir_to_bytecode_push_expr(Ir_expr* expr, bool is_actual_push, bool is_ptr) {
    switch (expr->type) {
        case IR_OPERATOR:
            log(LOG_DEBUG, "ir_operator\n");
            return ir_to_bytecode_push_operator(ir_operator_unwrap(expr), is_actual_push, is_ptr);
        case IR_LITERAL:
            log(LOG_DEBUG, "ir_literal\n");
            return ir_to_bytecode_push_literal(ir_literal_unwrap(expr), is_actual_push, is_ptr);
        case IR_FUNCTION_CALL:
            log(LOG_DEBUG, "ir_function_call\n");
            return ir_to_bytecode_push_function_call(ir_function_call_unwrap(expr), is_actual_push, is_ptr);
        default:
            unreachable("");
    }
    unreachable("");
}

// TODO: this function does not actually push new thing on stack in some cases
// TODO: maybe there should be a separate function for cases where new item on stack is not actually nessessary?

static uint64_t ir_to_bytecode_push_def(Ir_def* def, bool is_from_rtn, bool is_ptr) {
    assert(!is_ptr && "not implemented");
    switch (def->type) {
        case IR_VARIABLE_DEF:
            return ir_to_bytecode_push_variable_def(ir_variable_def_unwrap(def), is_from_rtn);
        case IR_FUNCTION_DEF:
            todo();
        case IR_STRUCT_DEF:
            todo();
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            todo();
        case IR_LABEL:
            todo();
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

// if is_actual_push == true, then pushes will be emitted
//  otherwise, local variables alloc will be used instead
static uint64_t ir_to_bytecode_push_ir(Ir* store_src, bool is_actual_push, bool is_ptr) {
    bool is_from_rtn = false; // TODO: remove
    switch (store_src->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            log(LOG_DEBUG, "ir_expr\n");
            return ir_to_bytecode_push_expr(ir_expr_unwrap(store_src), is_actual_push, is_ptr);
        case IR_LOAD_ELEMENT_PTR:
            return ir_to_bytecode_push_load_element_ptr(ir_load_element_ptr_unwrap(store_src), is_from_rtn, is_actual_push, is_ptr);
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            log(LOG_DEBUG, "ir_def\n");
            return ir_to_bytecode_push_def(ir_def_unwrap(store_src), is_from_rtn, is_ptr);
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            log(LOG_DEBUG, "ir_alloca\n");
            return ir_to_bytecode_push_alloca(ir_alloca_unwrap(store_src), is_actual_push, is_ptr);
        case IR_LOAD_ANOTHER_IR:
            log(LOG_DEBUG, "ir_load_another_ir\n");
            return ir_to_bytecode_push_load_another_ir(ir_load_another_ir_unwrap(store_src), is_from_rtn, is_actual_push, is_ptr);
        case IR_STORE_ANOTHER_IR:
            todo();
        case IR_IMPORT_PATH:
            todo();
        case IR_STRUCT_MEMB_DEF:
            todo();
        case IR_REMOVED:
            todo();
        default:
            unreachable("");
    }
    unreachable("");
}

// TODO: remove sizeof_lang_type parameter/return value
static uint64_t ir_to_bytecode_store_dest(bool* deref_is_req, uint64_t* sizeof_lang_type, Ir* store_dest) {
    *deref_is_req = false;

    switch (store_dest->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            todo();
        case IR_LOAD_ELEMENT_PTR: {
            ir_to_bytecode_comment("ir_to_bytecode_store_dest load_element_ptr");
            Ir_load_element_ptr* load = ir_load_element_ptr_unwrap(store_dest);
            Ir* stack_pos_name = NULL;
            unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(load->name_self)));
            Ir_int* lang_int = ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)));

            log(LOG_DEBUG, "%d\n", ir_lang_type_get_pointer_depth(lang_int->lang_type));
            Ir_lang_type int_lang_type = ir_lang_type_pointer_depth_dec(lang_int->lang_type);
            *sizeof_lang_type = sizeof_ir_lang_type(int_lang_type);
            log(LOG_DEBUG, FMT"\n", ir_print(lang_int));

            *deref_is_req = true;

            return (uint64_t)lang_int->data;

            //uint64_t dest_pos = ir_to_bytecode_alloc_internal(*sizeof_lang_type, false, false);
            //todo();
            //ir_to_bytecode_append_deref((uint64_t)lang_int->data, *sizeof_lang_type, dest_pos, bytecode_stack_offset_);

            //return dest_pos;
            bytecode_dump(stderr, LOG_DEBUG, bytecode_is_backpatching, bytecode);
            todo();
            //ir_to_bytecode_append_deref();
            return (uint64_t)lang_int->data;
        }
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            todo();
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA: {
            Ir_alloca* lang_alloca = ir_alloca_unwrap(store_dest);
            Ir* stack_pos_name = NULL;
            unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(lang_alloca->name)));
            Ir_int* lang_int = ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)));
            *sizeof_lang_type = sizeof_ir_lang_type(lang_int->lang_type);
            log(LOG_DEBUG, FMT"\n", ir_print(lang_int));
            return (uint64_t)lang_int->data;
        }
        case IR_LOAD_ANOTHER_IR:
            todo();
        case IR_STORE_ANOTHER_IR:
            todo();
        case IR_IMPORT_PATH:
            todo();
        case IR_STRUCT_MEMB_DEF:
            todo();
        case IR_REMOVED:
            todo();
        default:
            unreachable("");
    }
    unreachable("");
}

// TODO: change Ir_variable_def so that it does not have two names
static void ir_to_bytecode_store_another_ir(Ir_store_another_ir* store) {
    size_t old_count = SIZE_MAX;
    (void) store;

    if (!bytecode_is_backpatching) {
        if (bytecode.code.info.count == 11896) {
            todo();
        }
    }
    ir_to_bytecode_comment("store_another_ir: "FMT, ir_print(store));
    ir_to_bytecode_comment("store_another_ir src: "FMT, ir_print(ir_from_name(store->ir_src)));

    if (bytecode.code.info.count == 3720) {
        //breakpoint();
    }

    //breakpoint();
    if (0 && bytecode_is_backpatching) {
        unreachable("");
        // TODO: 
        //   do not actually append to darr if bytecode_is_backpatching == true and NDEBUG is defined 
        //   (maybe bytecode_append_align function should only append when !bytecode_is_backpatching)
    } else {
        //breakpoint();
        log(LOG_DEBUG, FMT"\n", ir_print(store));
        Ir* store_src = ir_from_name(store->ir_src);
        Ir* store_dest = ir_from_name(store->ir_dest);
        uint64_t dummy = {0};
        uint64_t sizeof_lang_type = sizeof_ir_lang_type(store->lang_type);
        log(LOG_DEBUG, FMT"\n", ir_print(store_src));
        log(LOG_DEBUG, FMT"\n", ir_print(store_dest));
        log(LOG_DEBUG, "%zu\n", bytecode_stack_offset_);
        uint64_t store_src_pos = ir_to_bytecode_push_ir(store_src, false, false);
        if (store_src_pos > 120) {
            //breakpoint();
        }
        log(LOG_DEBUG, "%zu %zu\n", bytecode_stack_offset_, store_src_pos);
        //assert(bytecode_stack_offset == store_src_pos);
        bool deref_is_req = {0};
        uint64_t store_dest_pos = ir_to_bytecode_store_dest(&deref_is_req, &dummy, store_dest);

        log(LOG_DEBUG, FMT"\n", ir_lang_type_print(LANG_TYPE_MODE_LOG, ir_get_lang_type(store_src)));
        log(LOG_DEBUG, FMT"\n", ir_lang_type_print(LANG_TYPE_MODE_LOG, ir_get_lang_type(store_dest)));
        log(LOG_DEBUG, FMT"\n", loc_print(loc_get(store)));
        assert(
            ir_lang_type_is_equal(ir_lang_type_pointer_depth_inc(ir_get_lang_type(store_src)), ir_get_lang_type(store_dest)) &&
            "this is likely a bug in the add load and store pass"
        );

        old_count = bytecode.code.info.count;
        if (old_count == 200) {
            log(LOG_DEBUG, FMT"\n", ir_print(store_src));
            //todo();
        }
        if (deref_is_req) {
            ir_to_bytecode_append_store_stack_deref_dest(store_dest_pos, store_src_pos, sizeof_lang_type, bytecode_stack_offset_);
        } else {
            ir_to_bytecode_append_store_stack(store_dest_pos, store_src_pos, sizeof_lang_type, bytecode_stack_offset_);
        }
    }

    //bytecode_dump(stderr, LOG_DEBUG, true, bytecode);
    //breakpoint();

    // TODO: uncomment
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_SIZE);
}

static void ir_to_bytecode_function_def(Ir_function_def* def) {
    if (def->decl->params->params.info.count > 0) {
        //todo();
    }

    log(LOG_DEBUG, FMT"\n", ir_print(def));
    if (!bytecode_is_backpatching) {
        //breakpoint();
    }

    bytecode_stack_offset_ = 0;

    Ir_variable_def_darr old_curr_fun_args = curr_fun_args;
    curr_fun_args = def->decl->params->params;

    Strv old_mod_path_curr_file = env.mod_path_curr_file;
    env.mod_path_curr_file = def->decl->name.mod_path;

    Pos old_bytecode_fun_pos = bytecode_fun_pos;
    bytecode_fun_pos = def->pos;

    Name old_bytecode_fun_name = bytecode_fun_name;
    bytecode_fun_name = def->decl->name;

    uint64_t old_bytecode_space_locals_alloca_size = bytecode_space_locals_alloca_size;
    bytecode_space_locals_alloca_size = 0;

    if (bytecode_is_backpatching) {
        //Ir_function_name* fun_name_ = ir_function_name_unwrap(ir_literal_unwrap(ir_expr_unwrap(ir_from_name(def->callee))));
        // TODO: make function for below statement?
        unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            def->pos /* TODO*/,
            (int64_t)bytecode.code.info.count,
            ir_lang_type_new_ux(64),
            function_name_to_loc(def->decl->name/*TODO*/)
        ))))));
    }

    // TODO: make function for Ir_name_is_main or similar
    if (strv_is_equal(def->decl->name.base, sv("main"))) {
        log(LOG_DEBUG, "%zu\n", bytecode.code.info.count);
        bytecode.start_pos = bytecode.code.info.count;
    }

    ir_to_bytecode_comment("START OF FUNCTION "FMT" ("FMT")", name_print(NAME_MSG, def->name_self, NAME_FULL), name_print(NAME_MSG, def->decl->name, NAME_FULL));

    uint64_t old_bytecode_stack_offset = bytecode_stack_offset_;
    bytecode_stack_offset_ = sizeof_ir_params(def->decl->params->params);
    bytecode_stack_offset_ = get_next_multiple(bytecode_stack_offset_, 8);
    uint64_t args_count_bytes = bytecode_stack_offset_;

    static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of values below in allocs");
    {
        ir_to_bytecode_comment("alloca for return value");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_VALUE, args_count_bytes));

        ir_to_bytecode_comment("alloca for return address");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, args_count_bytes));

        ir_to_bytecode_comment("alloca for stack_base_ptr");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, args_count_bytes));

        ir_to_bytecode_comment("alloca for stack_offset");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, args_count_bytes));

        ir_to_bytecode_comment("alloca for arg_bytes");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, args_count_bytes));

        ir_to_bytecode_comment("alloca for rtn_alloc_pos");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_name_new(), 0 /* TODO */), false);
        assert(bytecode_stack_offset_ == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, args_count_bytes));
    }

    {
        uint64_t space_locals_bytes = 0;
        uint64_t start_local_vars = bytecode_stack_offset_;
        if (!bytecode_is_backpatching) {
            Ir* space_locals_ = NULL;
            unwrap(ir_lookup(&space_locals_, fun_name_to_space_locals_alloc_size(bytecode_fun_name)));
            space_locals_bytes = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(space_locals_)))->data;
        }
        ir_to_bytecode_comment("alloca for local variables");
        bytecode_locals_pos = start_local_vars;
        if (!bytecode_is_backpatching) {
            bytecode_locals_pos_at_max_alloc = bytecode_locals_pos + space_locals_bytes;
        }
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(bytes_to_bit_width(space_locals_bytes)), util_literal_name_new(), 0 /* TODO */), false);
    }
                                                                         
    ir_to_bytecode_comment("start of block");
    //breakpoint();
    ir_to_bytecode_block(def->body);

    ir_to_bytecode_comment("END OF FUNCTION "FMT" ("FMT")", name_print(NAME_MSG, def->name_self, NAME_FULL), name_print(NAME_MSG, def->decl->name, NAME_FULL));

    bytecode_stack_offset_ = old_bytecode_stack_offset;
    env.mod_path_curr_file = old_mod_path_curr_file;
    curr_fun_args = old_curr_fun_args;
    bytecode_fun_pos = old_bytecode_fun_pos;
    bytecode_fun_name = old_bytecode_fun_name;
    bytecode_space_locals_alloca_size = old_bytecode_space_locals_alloca_size;
}

static void ir_to_bytecode_def_out_of_line(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            ir_to_bytecode_function_def(ir_function_def_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            // TODO
            return;
        case IR_STRUCT_DEF:
            // TODO
            return;
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            return;
        case IR_LABEL:
            return;
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void ir_to_bytecode_label(Ir_label* label) {
    if (bytecode_is_backpatching) {
        log(LOG_DEBUG, FMT": %zu\n", name_print(NAME_LOG, label->name, NAME_FULL), bytecode.code.info.count);
        unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            label->pos,
            (int64_t)bytecode.code.info.count,
            ir_lang_type_new_ux(64/*TODO*/),
            symbol_name_to_int_name(label->name)
        ))))));
    }
}

static void ir_to_bytecode_def_inline(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            unreachable("");
            return;
        case IR_VARIABLE_DEF:
            return;
        case IR_STRUCT_DEF:
            todo();
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            todo();
        case IR_LABEL:
            ir_to_bytecode_label(ir_label_unwrap(def));
            return;
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void ir_to_bytecode_function_call(Ir_function_call* call) {
    size_t old_count = SIZE_MAX;

    // TODO: make separate ir function call direct and function ptr?

    //darr_foreach(arg_idx, Name, arg, call->args) {
    //    uint64_t arg_pos = ir_to_bytecode_push_ir(ir_from_name(arg), true);
    //    (void) arg_pos;
    //}

    uint64_t sizeof_lang_type = sizeof_ir_lang_type(call->lang_type);
        assert(bytecode_stack_offset_ < 1000);
    //uint64_t temp_alloca_pos = ir_to_bytecode_alloc_internal(sizeof_lang_type, true, false);
        assert(bytecode_stack_offset_ < 1000);
    //(void) temp_alloca_pos;
        assert(bytecode_stack_offset_ < 1000);
    uint64_t alloca_pos = ir_to_bytecode_alloc_internal(sizeof_lang_type, false, false);
        assert(bytecode_stack_offset_ < 1000);
    if (0 && bytecode_is_backpatching) {
        ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            call->pos /* TODO*/,
            (int64_t)alloca_pos,
            ir_lang_type_new_ux(64),
            symbol_name_to_int_name(call->name_self/*TODO*/)
        )))));

        old_count = bytecode.code.info.count;
        ir_to_bytecode_append_call_direct(0, 0, 0, 0);
    } else {
        Ir_function_name* fun_name_ = ir_function_name_unwrap(ir_literal_unwrap(ir_expr_unwrap(ir_from_name(call->callee))));
        log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, fun_name_->fun_name, NAME_FULL));
        Ir* fun_addr_ = NULL;
        uint64_t fun_addr = 0;
        if (!bytecode_is_backpatching) {
            unwrap(ir_lookup(&fun_addr_, function_name_to_loc(fun_name_->fun_name)));
            fun_addr = (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(fun_addr_)))->data;
        }

        uint64_t arg_bytes_lower = bytecode_stack_offset_;
        for (size_t idx = call->args.info.count - 1; call->args.info.count > 0; idx--) {
            ir_to_bytecode_push_ir(ir_from_name(darr_at(call->args, idx)), true, false);

            if (idx < 1) {
                break;
            }
        }
        uint64_t arg_bytes_count = bytecode_stack_offset_ - arg_bytes_lower;
        arg_bytes_count = get_next_multiple(arg_bytes_count, 8 /* alignment */);

        old_count = bytecode.code.info.count;
        ir_to_bytecode_append_call_direct(
            fun_addr,
            arg_bytes_count,
            alloca_pos,
            bytecode_stack_offset_
        );

        bytecode_stack_size_add_aligned(&bytecode_stack_offset_, arg_bytes_count);

        if (bytecode_is_backpatching) {
            ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                call->pos /* TODO*/,
                (int64_t)alloca_pos,
                ir_lang_type_new_ux(64),
                symbol_name_to_int_name(call->name_self/*TODO*/)
            )))));
        }
    }

    //if (ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
    //    unary->pos /* TODO*/,
    //    (int64_t)alloca_pos,
    //    ir_lang_type_new_ux(sizeof_src),
    //    symbol_name_to_int_name(unary->name/*TODO*/)
    //)))))) {
    //    todo();
    //    //ir_to_bytecode_function_def();
    //}

    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_CALL_DIRECT_SIZE);
}

static void ir_to_bytecode_unary(Ir_unary* unary) {
#   define src ir_from_name(unary->child)
#   define dest_name (unary->name)

    log(LOG_DEBUG, FMT"\n", ir_print(unary));
    assert(unary->token_type == IR_UNARY_UNSAFE_CAST && "not implemented");
    assert(unary->lang_type.type == IR_LANG_TYPE_PRIMITIVE && "not implemented");

    //Ir_lang_type_primitive dest_prim_type = ir_lang_type_primitive_const_unwrap(unary->lang_type);
    Ir_lang_type_primitive src_prim_type = ir_lang_type_primitive_const_unwrap(ir_get_lang_type(src));

    log(LOG_DEBUG, FMT"\n", ir_print(unary));
    log(LOG_DEBUG, FMT"\n", ir_print(src));

    uint64_t sizeof_dest = sizeof_ir_lang_type(unary->lang_type);
    uint64_t sizeof_src = sizeof_ir_lang_type(ir_get_lang_type(src));

    uint64_t alloca_pos = ir_to_bytecode_alloc_internal(sizeof_dest, false, false);

    if (src_prim_type.type == IR_LANG_TYPE_UNSIGNED_INT && ir_lang_type_is_int(ir_get_lang_type(src))) {
        if (1 || bytecode_is_backpatching) {
            if (sizeof_dest <= sizeof_src) {
                // not zero extend
                todo();
            }

            // TODO: remove 
            uint64_t src_pos = ir_to_bytecode_push_ir(src, true, false);
            
            bytecode_stack_size_add_aligned(&bytecode_stack_offset_, sizeof_src);

            ir_to_bytecode_append_zero_extend(sizeof_dest, sizeof_src, src_pos, alloca_pos, bytecode_stack_offset_);

            if (bytecode_is_backpatching) {
                unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                    unary->pos /* TODO*/,
                    (int64_t)alloca_pos,
                    ir_lang_type_new_ux(sizeof_src),
                    symbol_name_to_int_name(unary->name/*TODO*/)
                ))))));
            }
        } else {
            todo();

            //ir_to_bytecode_push_ir(src);

            //breakpoint();
            //old_count = bytecode.code.info.count;

            //log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            //bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_src);
            //log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);

            //bytecode_append_align(BYTECODE_ZERO_EXTEND);
            //ir_to_bytecode_uint64_t(sizeof_dest);
            //ir_to_bytecode_uint64_t(sizeof_src);

            //log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            //uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_dest);
            //log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            //log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(alloca_pos));
            ////bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_lang_type); // TODO
            //(void) alloca_pos;
            ////unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            ////    unary->pos /* TODO*/,
            ////    (int64_t)alloca_pos,
            ////    ir_lang_type_new_ux(sizeof_lang_type),
            ////    symbol_name_to_int_name(unary->name/*TODO*/)
            ////))))));
        }
    } else {
        log(LOG_DEBUG, FMT"\n", ir_print(unary));
        todo();
    }

    //if (dest_prim_type.type == IR_LANG_TYPE_UNSIGNED_INT && src_prim_type.type == IR_LANG_TYPE_SIGNED_INT) {
    //    todo();
    //}

    //if (dest_prim_type.type == IR_LANG_TYPE_UNSIGNED_INT && src_prim_type.type == IR_LANG_TYPE_SIGNED_INT) {
    //    todo();
    //}

    //if (dest_prim_type.type != IR_LANG_TYPE_UNSIGNED_INT || src_prim_type.type != IR_LANG_TYPE_UNSIGNED_INT) {
    //    todo();
    //}
    
#   undef src
#   undef dest_name
}

static void ir_to_bytecode_binary(Ir_binary* bin) {
    //breakpoint();
    // TODO: assert that bin->lhs and bin->rhs have same lang_type as bin itself
    log(LOG_DEBUG, FMT"\n", ir_print(bin));
    //todo();
    ir_to_bytecode_comment("add lhs");
    uint64_t lhs_pos = ir_to_bytecode_push_ir(ir_from_name(bin->lhs), true, false);
    //breakpoint();
    ir_to_bytecode_comment("add rhs");
    log(LOG_DEBUG, FMT"\n", ir_print(bin));
    log(LOG_DEBUG, "bytecode_stack_offset_ = %zu\n", bytecode_stack_offset_);
    //breakpoint();
    if (strv_is_equal(bin->name.base, sv("str____574"))) {
        breakpoint();
    }
    uint64_t rhs_pos = ir_to_bytecode_push_ir(ir_from_name(bin->rhs), true, false);
    uint64_t start_args = rhs_pos;
    assert(start_args < 1000);
    assert(rhs_pos < 1000);
    ir_to_bytecode_comment("add lhs: "FMT"\n", ir_print(ir_from_name(bin->lhs)));
    ir_to_bytecode_comment("add lhs pos: %zu\n", start_args);
    ir_to_bytecode_comment("add rhs: "FMT"\n", ir_print(ir_from_name(bin->rhs)));
    ir_to_bytecode_comment("add rhs pos: %zu\n", rhs_pos);
    if (!bytecode_is_backpatching) {
        assert(rhs_pos > lhs_pos);
        assert(start_args > lhs_pos);
    }

    uint64_t old_count = bytecode.code.info.count;

    uint64_t sizeof_bin_lang_type = sizeof_ir_lang_type(bin->lang_type);

        assert(bytecode_stack_offset_ < 1000);
    uint64_t alloca_pos = ir_to_bytecode_alloc_internal(sizeof_bin_lang_type, false, false);
        assert(bytecode_stack_offset_ < 1000);

    BYTECODE bin_type = BYTECODE_NONE;

    switch (bin->token_type) {
        case IR_BINARY_SUB:
            bin_type = BYTECODE_SUB;
            break;
        case IR_BINARY_ADD:
            bin_type = BYTECODE_ADD_;
            break;
        case IR_BINARY_MULTIPLY:
            todo();
        case IR_BINARY_DIVIDE:
            todo();
        case IR_BINARY_MODULO:
            todo();
        case IR_BINARY_LESS_THAN:
            bin_type = BYTECODE_LESS_THAN;
            break;
        case IR_BINARY_LESS_OR_EQUAL:
            todo();
        case IR_BINARY_GREATER_OR_EQUAL:
            todo();
        case IR_BINARY_GREATER_THAN:
            bin_type = BYTECODE_GREATER_THAN;
            break;
        case IR_BINARY_DOUBLE_EQUAL:
            bin_type = BYTECODE_DOUBLE_EQUAL;
            break;
        case IR_BINARY_NOT_EQUAL:
            bin_type = BYTECODE_NOT_EQUAL;
            break;
        case IR_BINARY_BITWISE_XOR:
            todo();
        case IR_BINARY_BITWISE_AND:
            todo();
        case IR_BINARY_BITWISE_OR:
            todo();
        case IR_BINARY_SHIFT_LEFT:
            todo();
        case IR_BINARY_SHIFT_RIGHT:
            todo();
        case IR_BINARY_COUNT:
            unreachable("");
        default:
            unreachable("");
    }
    assert(bin_type != BYTECODE_NONE);

    bytecode_stack_size_add_aligned(&bytecode_stack_offset_, sizeof_bin_lang_type);
    bytecode_stack_size_add_aligned(&bytecode_stack_offset_, sizeof_bin_lang_type);

    ir_to_bytecode_append_binary(bin_type, start_args, alloca_pos, lhs_pos, rhs_pos, sizeof_bin_lang_type, bytecode_stack_offset_);

    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_BINARY_SIZE);

    //bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);
    //bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);
    //uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);
    ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
        bin->pos /* TODO*/,
        (int64_t)alloca_pos,
        ir_lang_type_new_ux(64),
        symbol_name_to_int_name(bin->name/*TODO*/)
    )))));
}

static void ir_to_bytecode_operator(Ir_operator* oper) {
    switch (oper->type) {
        case IR_BINARY:
            ir_to_bytecode_binary(ir_binary_unwrap(oper));
            return;
        case IR_UNARY:
            ir_to_bytecode_unary(ir_unary_unwrap(oper));
            return;
    }
    unreachable("");
}

static void ir_to_bytecode_expr_inline(Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            ir_to_bytecode_operator(ir_operator_unwrap(expr));
            return;
        case IR_LITERAL:
            todo();
        case IR_FUNCTION_CALL:
            ir_to_bytecode_function_call(ir_function_call_unwrap(expr));
            return;
    }
    unreachable("");
}

static void ir_to_bytecode_out_of_line(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            // TODO
            return;
        case IR_EXPR:
            return;
        case IR_LOAD_ELEMENT_PTR:
            return;
        case IR_ARRAY_ACCESS:
            return;
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            ir_to_bytecode_def_out_of_line(ir_def_unwrap(ir));
            return;
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_STORE_ANOTHER_IR:
            return;
        case IR_IMPORT_PATH:
            ir_to_bytecode_import_path(ir_import_path_unwrap(ir));
            return;
        case IR_STRUCT_MEMB_DEF:
            todo();
        case IR_REMOVED:
            todo();
    }
    unreachable("");
}

static void ir_to_bytecode_internal(void) {
    Ir_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        ir_to_bytecode_out_of_line(curr);
    }
}

void ir_to_bytecode(void) {
    bytecode_is_backpatching = false;
    darr_reset(&bytecode.code);
    ir_to_bytecode_internal();
}

void ir_to_bytecode_patch_offsets(void) {
    bytecode_is_backpatching = true;
    ir_to_bytecode_internal();
}
