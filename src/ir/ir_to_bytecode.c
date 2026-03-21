#include <ir_to_bytecode.h>
#include <uint8_t_darr.h>
#include <bytecode.h>

// TODO: move this definition?
Bytecode bytecode;

uint64_t bytecode_stack_offset = 0;

static bool bytecode_is_backpatching = false;

static Ir_name symbol_name_to_int_name(Ir_name sym_name) {
    assert(sym_name.base.count > 0);
    return ir_name_new(
        MOD_PATH_BACKPATCH_STACK_SIZE,
        serialize_ir_name(sym_name),
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
}

static Ir_name function_name_to_loc(Ir_name sym_name) {
    assert(sym_name.base.count > 0);
    return ir_name_new(
        MOD_PATH_FUNCTION_TO_LOC,
        serialize_ir_name(sym_name),
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

    (void) line;
    String buf = {0};
    {
        va_list args1;
        va_start(args1, format);

        string_extend_f_va(&a_main/*TODO*/, &buf, format, args1);

        va_end(args1);
    }

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

static void ir_to_bytecode_alloca(Ir_alloca* lang_alloca);

static void ir_to_bytecode_store_another_ir(Ir_store_another_ir* lang_store_another_ir);

static void ir_to_bytecode_load_another_ir(Ir_load_another_ir* load);

static uint64_t ir_to_bytecode_push_ir(Ir* store_src, bool is_from_rtn);

static void ir_to_bytecode_expr_inline(Ir_expr* expr);

static void ir_to_bytecode_unary(Ir_unary* unary);

// arg is item to jump to (offset from start of code)
static void ir_to_bytecode_goto(Ir_goto* lang_goto) {
    ir_to_bytecode_comment("goto");

    size_t old_count = bytecode.code.info.count;
    (void) old_count;
    bytecode_append_align(BYTECODE_GOTO);

    if (bytecode_is_backpatching) {
        bytecode_append_align(0);
    } else {
        Ir* block_pos_name = NULL;
        unwrap(ir_lookup(&block_pos_name, symbol_name_to_int_name(lang_goto->label)));
        ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(block_pos_name)))->data);
    }

    assert(bytecode.code.info.count - old_count == BYTECODE_GOTO_SIZE);
}

// BYTECODE_RETURN
//   arg 1 is sizeof lang_type to return
static void ir_to_bytecode_return(Ir_return* rtn) {
    ir_to_bytecode_comment("return");

    //breakpoint();
    size_t old_count = SIZE_MAX;
    (void) old_count;

    if (0 && bytecode_is_backpatching) {
        // TODO: assert that loads add return arg or function args to stack in the correct order
        old_count = bytecode.code.info.count;
        bytecode_append_align(BYTECODE_RETURN);
        bytecode_append_align(0);
    } else {
        log(LOG_DEBUG, FMT"\n", ir_print(ir_from_ir_name(rtn->child)));
        log(LOG_DEBUG, FMT"\n", ir_print(ir_from_ir_name(ir_load_another_ir_unwrap(ir_from_ir_name(rtn->child))->ir_src)));
        bytecode_dump(LOG_DEBUG, bytecode);
        //breakpoint();
        Ir* rtn_child = ir_from_ir_name(rtn->child);
        // TODO: load loads thing to stack, when thing is already on stack?
        bytecode_dump(LOG_DEBUG, bytecode);
        uint64_t pos_child = ir_to_bytecode_push_ir(rtn_child, true);
        (void) pos_child;
        rtn_child = ir_from_ir_name(rtn->child);
        log(LOG_DEBUG, FMT"\n", ir_print(rtn_child));
        //breakpoint();
        bytecode_dump(LOG_DEBUG, bytecode);

        old_count = bytecode.code.info.count;
        bytecode_append_align(BYTECODE_RETURN);
        if (ir_get_lang_type(rtn_child).type == IR_LANG_TYPE_VOID) {
            todo();
        }

        uint64_t sizeof_rtn_type = sizeof_ir_lang_type(ir_get_lang_type(rtn_child));
        ir_to_bytecode_uint64_t(sizeof_rtn_type);
        
        //bytecode_append_align();
    }

    //ir* rtn_child = ir_from_ir_name(rtn->child);
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
    
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_RETURN_SIZE);
}

static void ir_to_bytecode_inline(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            ir_to_bytecode_expr_inline(ir_expr_unwrap(ir));
            return;
        case IR_LOAD_ELEMENT_PTR:
            todo();
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
            todo();
        case IR_ALLOCA:
            ir_to_bytecode_alloca(ir_alloca_unwrap(ir));
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
        log(LOG_DEBUG, FMT"\n", ir_print(ir));
        ir_to_bytecode_inline(ir);
    }
}

static void ir_to_bytecode_import_path(Ir_import_path* import) {
    ir_to_bytecode_block(import->block);
}

// TODO: this instruction takes 2 bytes. make version that takes one?
static void ir_to_bytecode_alloca(Ir_alloca* lang_alloca) {
    ir_to_bytecode_comment("alloca");

    size_t old_count = bytecode.code.info.count;
    (void) old_count;
    bytecode_append_align(BYTECODE_ALLOCA);
    uint64_t sizeof_alloca = sizeof_ir_lang_type(ir_lang_type_pointer_depth_dec(lang_alloca->lang_type));
    ir_to_bytecode_uint64_t(sizeof_alloca);

    uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_alloca);

    if (bytecode_is_backpatching) {
        unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            lang_alloca->pos,
            (int64_t)alloca_pos,
            ir_lang_type_new_ux(sizeof_alloca),
            symbol_name_to_int_name(lang_alloca->name)
        ))))));
    }
    assert(bytecode.code.info.count - old_count == BYTECODE_ALLOCA_SIZE);
}

// BYTECODE_PUSH
//   arg 1 is size of push
//   arg 2 is item to push
static uint64_t ir_to_bytecode_push_int(Ir_int* lang_int) {
    size_t old_count = bytecode.code.info.count;
    log(LOG_DEBUG, "ir_to_bytecode_push_int: old_count = %zu\n", old_count);
    bytecode_append_align(BYTECODE_PUSH);
    uint64_t sizeof_int_push = sizeof_ir_lang_type(lang_int->lang_type);
    log(LOG_DEBUG, "lang_int->data = %zu, sizeof_int_push = %zu\n", lang_int->data, sizeof_int_push);
    ir_to_bytecode_uint64_t(sizeof_int_push);

    uint64_t item_stack_pos = 0;

    if (bytecode_is_backpatching) {
        //unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
        //    lang_int->pos,
        //    (int64_t)item_stack_pos,
        //    ir_lang_type_new_usize(),
        //    symbol_name_to_backpatch_stack_size_name(lang_int_push->name)
        //))))));
        if (sizeof_int_push > 8) {
            todo();
        }

        item_stack_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_int_push);

        ir_to_bytecode_int64_t(lang_int->data);
    } else {
        item_stack_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_int_push);

        if (sizeof_int_push > 8) {
            todo();
        }

        ir_to_bytecode_int64_t(lang_int->data);
    }

    assert(bytecode.code.info.count - old_count == BYTECODE_PUSH_SIZE);
    return item_stack_pos;
}

static uint64_t ir_to_bytecode_load_another_ir_alloca(uint64_t* sizeof_lang_type, Ir_load_another_ir* load) {
    size_t old_count = bytecode.code.info.count;
    (void) old_count;

    *sizeof_lang_type = sizeof_ir_lang_type(load->lang_type);

    bytecode_append_align(BYTECODE_ALLOCA);
    ir_to_bytecode_uint64_t(*sizeof_lang_type);

    uint64_t alloca_stack_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, *sizeof_lang_type);

    assert(bytecode.code.info.count - old_count == BYTECODE_ALLOCA_SIZE);
    return alloca_stack_pos;
}

static void ir_to_bytecode_load_another_ir(Ir_load_another_ir* load) {
    ir_to_bytecode_comment("load_another_ir");

    size_t old_count = SIZE_MAX;
    (void) load;

    if (0 && bytecode_is_backpatching) {
        unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            load->pos,
            (int64_t)bytecode_stack_offset,
            ir_lang_type_new_ux(64/*TODO*/),
            symbol_name_to_int_name(load->name)
        ))))));

        old_count = bytecode.code.info.count;
        bytecode_append_align(BYTECODE_STORE_STACK);
        bytecode_append_align(0);
        bytecode_append_align(0);
        bytecode_append_align(0);
    } else {
        Ir* load_src = ir_from_ir_name(load->ir_src);
        uint64_t sizeof_lang_type = {0};
        uint64_t dest_pos = ir_to_bytecode_load_another_ir_alloca(&sizeof_lang_type, load);
        log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(dest_pos));
        uint64_t load_src_pos = ir_to_bytecode_push_ir(load_src, false);

        ir_to_bytecode_comment("load_another_ir actual thing");
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
        bytecode_append_align(BYTECODE_STORE_STACK);
        ir_to_bytecode_uint64_t(dest_pos);
        ir_to_bytecode_uint64_t(load_src_pos);
        ir_to_bytecode_uint64_t(sizeof_lang_type);
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


static uint64_t ir_to_bytecode_push_literal(Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            return ir_to_bytecode_push_int(ir_int_unwrap(lit));
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

static uint64_t ir_to_bytecode_push_alloca(Ir_alloca* lang_alloca) {
    Ir* stack_pos_name = NULL;
    unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(lang_alloca->name)));
    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);
    return (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data;
}

// TODO: this and similar functions should return void
static uint64_t ir_to_bytecode_push_load_another_ir(Ir_load_another_ir* load, bool is_from_rtn /* TODO: remove */) {
    Ir* stack_pos_name = NULL;
    unwrap(ir_lookup(&stack_pos_name, symbol_name_to_int_name(load->name)));
    log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, symbol_name_to_int_name(load->name)));
    log(LOG_DEBUG, "ir_to_bytecode_push_alloca: old_count = %zu\n", bytecode.code.info.count);

    //breakpoint();

    // TODO: make a helper function to create this alloca?

    if (is_from_rtn) {
    uint64_t sizeof_load = sizeof_ir_lang_type(load->lang_type);
    bytecode_append_align(BYTECODE_ALLOCA);
    ir_to_bytecode_uint64_t(sizeof_load);
    uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_load);

    log(LOG_DEBUG, FMT" "FMT"\n", bytecode_alloca_pos_print(alloca_pos), bytecode_alloca_pos_print((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data));
    bytecode_append_align(BYTECODE_STORE_STACK);
    ir_to_bytecode_uint64_t(alloca_pos);
    ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data);
    ir_to_bytecode_uint64_t(sizeof_load);
    }

    //bytecode_append_align(sizeof_ir_lang_type(load->lang_type));
    //ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data);

    return (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(stack_pos_name)))->data;
}

// TODO: consider using int64_t instead of uint64_t to reduce casts
static uint64_t ir_to_bytecode_push_unsafe_cast(Ir_unary* unary) {
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

    Ir* int_thing = NULL;
    unwrap(ir_lookup(&int_thing, symbol_name_to_int_name(unary->name)));
    return (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(int_thing)))->data;
}

// returns pos
static uint64_t ir_to_bytecode_push_unary(Ir_unary* unary) {
    assert(unary->token_type == IR_UNARY_UNSAFE_CAST);
    return ir_to_bytecode_push_unsafe_cast(unary);
}

// returns pos
static uint64_t ir_to_bytecode_push_binary(Ir_binary* bin) {
    Ir* bin_pos_ = NULL;
    unwrap(ir_lookup(&bin_pos_, symbol_name_to_int_name(bin->name)));
    return (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(bin_pos_)))->data;
}

// returns pos
static uint64_t ir_to_bytecode_push_function_call(Ir_function_call* call) {
    Ir* call_pos_ = NULL;
    unwrap(ir_lookup(&call_pos_, symbol_name_to_int_name(call->name_self)));
    return (uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(call_pos_)))->data;
}

// returns pos
static uint64_t ir_to_bytecode_push_operator(Ir_operator* oper) {
    switch (oper->type) {
        case IR_UNARY:
            return ir_to_bytecode_push_unary(ir_unary_unwrap(oper));
        case IR_BINARY:
            return ir_to_bytecode_push_binary(ir_binary_unwrap(oper));
    }
    unreachable("");
}

// returns pos
static uint64_t ir_to_bytecode_push_expr(Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            return ir_to_bytecode_push_operator(ir_operator_unwrap(expr));
        case IR_LITERAL:
            return ir_to_bytecode_push_literal(ir_literal_unwrap(expr));
        case IR_FUNCTION_CALL:
            return ir_to_bytecode_push_function_call(ir_function_call_unwrap(expr));
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t ir_to_bytecode_push_ir(Ir* store_src, bool is_from_rtn) {
    switch (store_src->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            return ir_to_bytecode_push_expr(ir_expr_unwrap(store_src));
        case IR_LOAD_ELEMENT_PTR:
            todo();
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
        case IR_ALLOCA:
            return ir_to_bytecode_push_alloca(ir_alloca_unwrap(store_src));
        case IR_LOAD_ANOTHER_IR:
            return ir_to_bytecode_push_load_another_ir(ir_load_another_ir_unwrap(store_src), is_from_rtn);
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
static uint64_t ir_to_bytecode_store_dest(uint64_t* sizeof_lang_type, Ir* store_dest) {
    switch (store_dest->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            todo();
        case IR_LOAD_ELEMENT_PTR:
            todo();
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

static void ir_to_bytecode_store_another_ir(Ir_store_another_ir* store) {
    size_t old_count = SIZE_MAX;
    (void) store;

    ir_to_bytecode_comment("store_another_ir");

    if (0 && bytecode_is_backpatching) {
        // TODO: 
        //   do not actually append to darr if bytecode_is_backpatching == true and NDEBUG is defined 
        //   (maybe bytecode_append_align function should only append when !bytecode_is_backpatching)
        old_count = bytecode.code.info.count;
        bytecode_append_align(BYTECODE_STORE_STACK);
        bytecode_append_align(0);
        bytecode_append_align(0);
        bytecode_append_align(0);
    } else {
        log(LOG_DEBUG, FMT"\n", ir_print(store));
        Ir* store_src = ir_from_ir_name(store->ir_src);
        Ir* store_dest = ir_from_ir_name(store->ir_dest);
        uint64_t dummy = {0};
        uint64_t sizeof_lang_type = sizeof_ir_lang_type(store->lang_type);
        log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(bytecode_stack_offset));
        uint64_t store_src_pos = ir_to_bytecode_push_ir(store_src, false);
        assert(bytecode_stack_offset == store_src_pos);
        uint64_t store_dest_pos = ir_to_bytecode_store_dest(&dummy, store_dest);

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
        bytecode_append_align(BYTECODE_STORE_STACK);
        ir_to_bytecode_uint64_t(store_dest_pos);
        ir_to_bytecode_uint64_t(store_src_pos);
        ir_to_bytecode_uint64_t(sizeof_lang_type);
    }

    // TODO: uncomment
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_STORE_STACK_SIZE);
}

static void ir_to_bytecode_function_def(Ir_function_def* def) {
    if (def->decl->params->params.info.count > 0) {
        todo();
    }

    Strv old_mod_path_curr_file = env.mod_path_curr_file;
    env.mod_path_curr_file = def->decl->name.mod_path;

    if (bytecode_is_backpatching) {
        //Ir_function_name* fun_name_ = ir_function_name_unwrap(ir_literal_unwrap(ir_expr_unwrap(ir_from_ir_name(def->callee))));
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

    ir_to_bytecode_comment("START OF FUNCTION "FMT, ir_name_print(NAME_MSG, def->name_self));

    uint64_t old_bytecode_stack_offset = bytecode_stack_offset;
    bytecode_stack_offset = 0;

    static_assert(BYTECODE_COUNT_RTN_ITEMS == 4, "exhausive handling of values below in allocs");
    {
        ir_to_bytecode_comment("alloca for return value");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_ir_name_new(), 0 /* TODO */));
        assert(bytecode_stack_offset == BYTECODE_CALL_STACK_RTN_VALUE);

        ir_to_bytecode_comment("alloca for return address");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_ir_name_new(), 0 /* TODO */));
        assert(bytecode_stack_offset == BYTECODE_CALL_STACK_RTN_ADDR);

        ir_to_bytecode_comment("alloca for stack_base_ptr");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_ir_name_new(), 0 /* TODO */));
        assert(bytecode_stack_offset == BYTECODE_CALL_STACK_BASE_PTR);

        ir_to_bytecode_comment("alloca for stack_offset");
        ir_to_bytecode_alloca(ir_alloca_new(def->pos, ir_lang_type_new_ux(64), util_literal_ir_name_new(), 0 /* TODO */));
        assert(bytecode_stack_offset == BYTECODE_CALL_STACK_OFFSET);
    }
                                                                         
    ir_to_bytecode_comment("start of block");
    ir_to_bytecode_block(def->body);

    ir_to_bytecode_comment("END OF FUNCTION "FMT, ir_name_print(NAME_MSG, def->name_self));

    bytecode_stack_offset = old_bytecode_stack_offset;
    env.mod_path_curr_file = old_mod_path_curr_file;
}

static void ir_to_bytecode_def_out_of_line(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            ir_to_bytecode_function_def(ir_function_def_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            todo();
        case IR_STRUCT_DEF:
            todo();
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
        log(LOG_DEBUG, FMT": %zu\n", ir_name_print(NAME_LOG, label->name), bytecode.code.info.count);
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
    if (call->args.info.count > 0) {
        todo();
    }

    (void) call;
    // TODO: make separate ir function call direct and function ptr?

    uint64_t sizeof_lang_type = sizeof_ir_lang_type(call->lang_type);
    uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_lang_type);
    if (bytecode_is_backpatching) {
        ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            call->pos /* TODO*/,
            (int64_t)alloca_pos,
            ir_lang_type_new_ux(64),
            symbol_name_to_int_name(call->name_self/*TODO*/)
        )))));

        bytecode_append_align(BYTECODE_CALL_DIRECT);
        ir_to_bytecode_uint64_t(0);
    } else {
        Ir_function_name* fun_name_ = ir_function_name_unwrap(ir_literal_unwrap(ir_expr_unwrap(ir_from_ir_name(call->callee))));
        log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, fun_name_->fun_name));
        Ir* fun_addr_ = NULL;
        unwrap(ir_lookup(&fun_addr_, function_name_to_loc(fun_name_->fun_name)));

        bytecode_append_align(BYTECODE_CALL_DIRECT);
        ir_to_bytecode_uint64_t((uint64_t)ir_int_unwrap(ir_literal_unwrap(ir_expr_unwrap(fun_addr_)))->data);
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

}

static void ir_to_bytecode_unary(Ir_unary* unary) {
#   define src ir_from_ir_name(unary->child)
#   define dest_name (unary->name)

    size_t old_count = SIZE_MAX;
    (void) old_count;

    log(LOG_DEBUG, FMT"\n", ir_print(unary));
    assert(unary->token_type == IR_UNARY_UNSAFE_CAST && "not implemented");
    assert(unary->lang_type.type == IR_LANG_TYPE_PRIMITIVE && "not implemented");

    //Ir_lang_type_primitive dest_prim_type = ir_lang_type_primitive_const_unwrap(unary->lang_type);
    Ir_lang_type_primitive src_prim_type = ir_lang_type_primitive_const_unwrap(ir_get_lang_type(src));

    log(LOG_DEBUG, FMT"\n", ir_print(unary));
    log(LOG_DEBUG, FMT"\n", ir_print(src));

    uint64_t sizeof_dest = sizeof_ir_lang_type(unary->lang_type);
    uint64_t sizeof_src = sizeof_ir_lang_type(ir_get_lang_type(src));

    if (src_prim_type.type == IR_LANG_TYPE_UNSIGNED_INT && ir_lang_type_is_int(ir_get_lang_type(src))) {
        if (bytecode_is_backpatching) {
            // TODO: remove 
            //ir_to_bytecode_push_ir(src);
            
            old_count = bytecode.code.info.count;

            bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_src);

            bytecode_append_align(BYTECODE_ZERO_EXTEND);
            ir_to_bytecode_uint64_t(sizeof_dest);
            ir_to_bytecode_uint64_t(sizeof_src);

            uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_dest);
            //bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_lang_type); // TODO
            unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
                unary->pos /* TODO*/,
                (int64_t)alloca_pos,
                ir_lang_type_new_ux(sizeof_src),
                symbol_name_to_int_name(unary->name/*TODO*/)
            ))))));

            // TODO: add thing to get result where it needs to go
        } else {

            //ir_to_bytecode_push_ir(src);

            //breakpoint();
            old_count = bytecode.code.info.count;

            log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_src);
            log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);

            bytecode_append_align(BYTECODE_ZERO_EXTEND);
            ir_to_bytecode_uint64_t(sizeof_dest);
            ir_to_bytecode_uint64_t(sizeof_src);

            log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_dest);
            log(LOG_DEBUG, "%zu\n", bytecode_stack_offset);
            log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(alloca_pos));
            //bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_lang_type); // TODO
            (void) alloca_pos;
            //unwrap(ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
            //    unary->pos /* TODO*/,
            //    (int64_t)alloca_pos,
            //    ir_lang_type_new_ux(sizeof_lang_type),
            //    symbol_name_to_int_name(unary->name/*TODO*/)
            //))))));
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
    
    assert(old_count != SIZE_MAX);
    assert(bytecode.code.info.count - old_count == BYTECODE_ZERO_EXTEND_SIZE);

#   undef src
#   undef dest_name
}

static void ir_to_bytecode_binary(Ir_binary* bin) {
    // TODO: assert that bin->lhs and bin->rhs have same lang_type as bin itself
    ir_to_bytecode_push_ir(ir_from_ir_name(bin->lhs), false);
    ir_to_bytecode_push_ir(ir_from_ir_name(bin->rhs), false);

    switch (bin->token_type) {
        case IR_BINARY_SUB:
            bytecode_append_align(BYTECODE_SUB);
            break;
        case IR_BINARY_ADD:
            bytecode_append_align(BYTECODE_ADD);
            break;
        case IR_BINARY_MULTIPLY:
            todo();
        case IR_BINARY_DIVIDE:
            todo();
        case IR_BINARY_MODULO:
            todo();
        case IR_BINARY_LESS_THAN:
            todo();
        case IR_BINARY_LESS_OR_EQUAL:
            todo();
        case IR_BINARY_GREATER_OR_EQUAL:
            todo();
        case IR_BINARY_GREATER_THAN:
            todo();
        case IR_BINARY_DOUBLE_EQUAL:
            todo();
        case IR_BINARY_NOT_EQUAL:
            todo();
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
    }

    uint64_t sizeof_bin_lang_type = sizeof_ir_lang_type(bin->lang_type);
    ir_to_bytecode_uint64_t(sizeof_bin_lang_type);

    bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);
    bytecode_stack_size_add_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);
    uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&bytecode_stack_offset, sizeof_bin_lang_type);

    ir_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(ir_int_new(
        bin->pos /* TODO*/,
        (int64_t)alloca_pos,
        ir_lang_type_new_ux(alloca_pos),
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
            todo();
        case IR_ARRAY_ACCESS:
            todo();
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
