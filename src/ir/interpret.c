#define INTER_RTN_ADDR_EXIT (2LU << 60)

static uint64_t inter_prog_counter = 0;
static uint64_t inter_stack_size_ = INTERPRET_STACK_SIZE; // TODO: remove this variable?
static uint64_t inter_stack_offset = 0;
static uint64_t arg_bytes_count = 0; // TODO: use inter prefix
static uint64_t inter_rtn_alloc_pos = 0;
static uint64_t inter_base_ptr = INTERPRET_STACK_SIZE;
static uint8_t inter_stack[INTERPRET_STACK_SIZE] = {0};
//static uint64_t inter_fun_rtn_addr = UINT64_MAX;

// TODO: figure out why inter_stack_dump seems to print nothing
#define inter_stack_dump(log_level) bytecode_stack_dump_internal(log_level, __FILE__, __LINE__, inter_stack, inter_stack_offset, inter_base_ptr)

static uint8_t interpret_read_uint8_t(void) {
    uint64_t value = 0;
    // TODO: this memcpys 8 bytes for 1 bytes integer
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter), sizeof(value));
    inter_prog_counter++;

    return value;
}

static uint8_t interpret_read_uint8_t_aligned(void) {
    uint64_t value = 0;
    // TODO: this memcpys 8 bytes for 1 bytes integer
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter), sizeof(value));
    inter_prog_counter++;

    inter_prog_counter = get_next_multiple(inter_prog_counter, 8/*TODO*/);

    return value;
}

static uint64_t interpret_read_uint64_t_aligned(void) {
    uint64_t value = 0;
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter), sizeof(value));
    inter_prog_counter++;

    inter_prog_counter = get_next_multiple(inter_prog_counter, 8/*TODO*/);

    return value;
}

#define inter_binary(bin) \
    do { \
        inter_stack_dump(LOG_DEBUG); \
        \
        uint64_t start_args = interpret_read_uint64_t_aligned(); \
        uint64_t alloca_pos = interpret_read_uint64_t_aligned(); \
        uint64_t alloca_size = interpret_read_uint64_t_aligned(); \
        uint64_t expected_offset = interpret_read_uint64_t_aligned(); \
        \
        uint64_t pos_lhs = start_args; \
        uint64_t pos_rhs = pos_lhs; \
        bytecode_stack_size_add_aligned(&pos_rhs, alloca_size); \
        \
        log(LOG_DEBUG, #bin" alloca_pos = %zu\n", alloca_pos); \
        log(LOG_DEBUG, #bin" pos_lhs = %zu\n", pos_lhs); \
        log(LOG_DEBUG, #bin" pos_rhs = %zu\n", pos_rhs); \
        uint64_t lhs = bytecode_stack_read(inter_stack, pos_lhs, inter_base_ptr, alloca_size); \
        uint64_t rhs = bytecode_stack_read(inter_stack, pos_rhs, inter_base_ptr, alloca_size); \
        log(LOG_DEBUG, #bin" lhs = %zu\n", lhs); \
        log(LOG_DEBUG, #bin" rhs = %zu\n", rhs); \
        log(LOG_DEBUG, #bin" value = %"PRIi64"\n", (int64_t)(lhs bin rhs)); \
        \
        bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, alloca_size, lhs bin rhs); \
        \
        bytecode_stack_size_add_aligned(&inter_stack_offset, alloca_size); \
        bytecode_stack_size_add_aligned(&inter_stack_offset, alloca_size); \
        \
        assert(expected_offset == inter_stack_offset); \
        assert(inter_prog_counter - old_prog_counter == BYTECODE_BINARY_SIZE); \
    } while (0)

// TODO: use inter prefix?
// returns true if the program is still running
static bool interpret_instruction(void) {
    if (inter_prog_counter ==  5464) {
        //breakpoint();
    }
    uint64_t old_prog_counter = inter_prog_counter;
    (void) old_prog_counter;
    BYTECODE opcode = interpret_read_uint8_t_aligned();
    switch (opcode) {
        case BYTECODE_COMMENT: {
            String buf = {0};
            log(LOG_TRACE, "bytecode_comment\n");

            uint64_t file_len = interpret_read_uint64_t_aligned();
            for (uint64_t file_idx = 0; file_idx < file_len; file_idx++) {
                string_append(&a_temp, &buf, (char)interpret_read_uint8_t());
            }
            inter_prog_counter = get_next_multiple(inter_prog_counter, 8);
            //inter_prog_counter += get_next_multiple(file_len, 8);

            uint64_t line = interpret_read_uint64_t_aligned(); // line
            string_extend_f(&a_temp, &buf, ":%"PRIu64":", line);

            uint64_t comment_len = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            for (uint64_t file_idx = 0; file_idx < comment_len; file_idx++) {
                string_append(&a_temp, &buf, (char)interpret_read_uint8_t());
            }
            inter_prog_counter = get_next_multiple(inter_prog_counter, 8);
            //inter_prog_counter += get_next_multiple(comment_len, 8);

            log(LOG_TRACE, FMT"\n", string_print(buf));

            assert(
                inter_prog_counter - old_prog_counter
                == 
                8 /* BYTECODE_COMMENT */ + 
                8 /* file_len */ + get_next_multiple(file_len, 8) /* file*/ + 
                8 /* line */ + 
                8 /* comment_len */+ get_next_multiple(comment_len, 8) /* comment */
            );
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_ALLOCA: {
            log(LOG_TRACE, "bytecode_alloca\n");

            uint64_t sizeof_alloca = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            bytecode_stack_size_sub_aligned(&inter_stack_offset, sizeof_alloca);

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_ALLOCA_SIZE);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_STORE_STACK: { // TODO: rename to BYTECODE_STORE
            log(LOG_TRACE, "bytecode_store_stack\n");

            if (inter_prog_counter > 22896) {
                //breakpoint();
            }
            uint64_t dest_pos = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t src_pos = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t sizeof_store = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            uint64_t value = bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_store);
            log(
                LOG_TRACE,
                "bytecode_store_stack ("
                    "inter_prog_counter = %"PRIu64", "
                    "dest_pos = %"PRIu64", "
                    "src_pos = %"PRIu64", "
                    "sizeof_store = %"PRIu64", "
                    "value = %"PRIu64
                ")\n",
                inter_prog_counter,
                dest_pos,
                src_pos,
                sizeof_store,
                value
            );

            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);
            //memcpy(array_at_ref(inter_stack, dest_pos), , sizeof_store);
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "inter_prog_counter = %zu, expected_offset = %zu, inter_stack_offset = %zu\n", inter_prog_counter, expected_offset, inter_stack_offset);
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_STORE_STACK_SIZE);
            return true;
        }
        case BYTECODE_STORE_STACK_DEREF_DEST: {
            log(LOG_TRACE, "bytecode_store_stack_deref_dest\n");

            uint64_t dest_pos_ptr = interpret_read_uint64_t_aligned();
            uint64_t src_pos = interpret_read_uint64_t_aligned();
            uint64_t sizeof_store = interpret_read_uint64_t_aligned();

            uint64_t dest_pos = bytecode_stack_read(inter_stack, dest_pos_ptr, inter_base_ptr, 8);
            uint64_t value = bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_store);
            log(
                LOG_TRACE,
                "bytecode_store_stack (dest_pos_ptr = %"PRIu64", "
                    "dest_pos = %"PRIu64", "
                    "src_pos = %"PRIu64", "
                    "sizeof_store = %"PRIu64
                ")\n",
                dest_pos_ptr,
                dest_pos,
                src_pos,
                sizeof_store
            );
            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);
            //memcpy(array_at_ref(inter_stack, dest_pos), , sizeof_store);

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "inter_prog_counter = %zu, expected_offset = %zu, inter_stack_offset = %zu\n", inter_prog_counter, expected_offset, inter_stack_offset);
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_STORE_STACK_SIZE);
            return true;
        }
        case BYTECODE_DEREF: {
            log(LOG_TRACE, "deref\n");

            uint64_t src_ptr = interpret_read_uint64_t_aligned();
            uint64_t sizeof_alloca = interpret_read_uint64_t_aligned();
            uint64_t alloca_pos = interpret_read_uint64_t_aligned();

            uint64_t src = bytecode_stack_read(inter_stack, src_ptr, inter_base_ptr, 8);
            uint64_t derefed = bytecode_stack_read(inter_stack, src, inter_base_ptr, sizeof_alloca);
            log(LOG_DEBUG, "src_ptr = %zu, src = %zu, sizeof_alloca = %zu, derefed = %zu\n", src_ptr, src, sizeof_alloca, derefed);
            breakpoint();
            bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, sizeof_alloca, derefed);

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_DEREF_SIZE);
            return true;
        }
        case BYTECODE_STORE_STACK_DIR_ADDR: {
            log(LOG_TRACE, "bytecode_store_stack_dir_addr; inter_prog_counter = %"PRIu64"\n", inter_prog_counter);

            uint64_t dest_pos = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t value = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t sizeof_store = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_STORE_STACK_DIR_ADDR_SIZE);
            return true;
        }
        case BYTECODE_GOTO: {
            log(LOG_TRACE, "bytecode_goto\n");

            log(LOG_DEBUG, "inter_prog_counter = %zu\n", inter_prog_counter);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t new_prog_counter = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "inter_prog_counter = %"PRIu64"\n", inter_prog_counter);
            log(LOG_DEBUG, "new_prog_counter = %"PRIu64"\n", new_prog_counter);
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_GOTO_SIZE);

            inter_prog_counter = new_prog_counter;
            return true;
        }
        case BYTECODE_COND_GOTO: {
            log(LOG_TRACE, "bytecode_cond_goto\n");
            breakpoint();

            uint64_t if_true_prog_counter = interpret_read_uint64_t_aligned();
            uint64_t if_false_prog_counter = interpret_read_uint64_t_aligned();

            uint64_t cond = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, 1);
            uint64_t new_prog_counter = if_false_prog_counter;
            if (cond) {
                new_prog_counter = if_true_prog_counter;
            }

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "expected_offset = %zu, inter_stack_offset = %zu\n", expected_offset, inter_stack_offset);
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_COND_GOTO_SIZE);

            inter_prog_counter = new_prog_counter;
            return true;
        }
        case BYTECODE_RETURN: {
            log(LOG_TRACE, "bytecode_return\n");

            breakpoint();
            uint64_t sizeof_rtn = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "inter_stack_size = %zu\n", inter_stack_offset);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            log(LOG_DEBUG, "INTERPRET_STACK_SIZE = %zu\n", INTERPRET_STACK_SIZE);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            // TODO: rename INTERPRET_STACK_SIZE to avoid confusion with inter_stack_size
            log(LOG_DEBUG, "%zu\n", inter_stack_offset);
            //inter_stack_dump(LOG_DEBUG);

            // TODO: this expected_offset is not really after entire return instruction executes
            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            log(LOG_DEBUG, "%zu %zu %zu %zu\n", inter_stack_offset, inter_base_ptr, INTERPRET_STACK_SIZE, (inter_base_ptr) - inter_stack_offset);
            inter_stack_dump(LOG_DEBUG);
            uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_rtn);
            inter_stack_dump(LOG_DEBUG);
            bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_VALUE, arg_bytes_count), inter_base_ptr, sizeof_rtn, rtn_value);
            inter_stack_dump(LOG_DEBUG);

            bytecode_stack_size_add_aligned(&inter_stack_offset, inter_stack_offset - BYTECODE_COUNT_RTN_ITEMS*8 - arg_bytes_count);
            inter_stack_dump(LOG_DEBUG);

            uint64_t curr_rtn_alloc_pos = inter_rtn_alloc_pos;
            uint64_t curr_arg_bytes_count = arg_bytes_count;

            uint64_t arg_bytes = 0;
            uint64_t rtn_addr = 0;
            uint64_t base_ptr = 0;
            uint64_t offset = 0;
            uint64_t rtn_alloc_pos = 0;
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value is not popped here, because it should remain in the stack

                //breakpoint();
                inter_stack_dump(LOG_DEBUG);

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count));
                rtn_alloc_pos = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count));
                arg_bytes = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count));
                offset = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count));
                base_ptr = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));
                assert(inter_base_ptr > 0);

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count));
                rtn_addr = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(rtn_addr));
                //rtn_addr = ((inter_stack)[(inter_base_ptr) - bytecode_stack_size_add_aligned(&inter_stack_offset, sizeof(uint64_t))]);

                assert(inter_stack_offset == bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_VALUE, arg_bytes_count));
                assert(
                    inter_stack_offset == 8 + arg_bytes_count && 
                    "BYTECODE_CALL_STACK_TYPE_RTN_VALUE should == 0, and return value should be directly on top of function args in stack frame"
                );
            }

            offset = get_next_multiple(offset, 8);
            offset += get_next_multiple(sizeof_rtn, 8);

            assert(inter_base_ptr > 0);
            log(LOG_DEBUG, "rtn_value = %zu\n", rtn_value);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            inter_stack_dump(LOG_DEBUG);
            assert(inter_base_ptr > 0);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_prog_counter - old_prog_counter == BYTECODE_RETURN_SIZE);

            static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "update below if nessessary");
            log(LOG_DEBUG, "%zu %zu\n", rtn_addr, INTER_RTN_ADDR_EXIT);
            inter_prog_counter = rtn_addr;
            inter_base_ptr = base_ptr;
            inter_stack_offset = offset;
            arg_bytes_count = arg_bytes;
            inter_rtn_alloc_pos = rtn_alloc_pos;
            assert(inter_base_ptr > 0);

            if (rtn_addr != INTER_RTN_ADDR_EXIT) {
                uint64_t temp_rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_rtn);
                assert(rtn_value == temp_rtn_value);
                log(LOG_DEBUG, "%zu\n", curr_rtn_alloc_pos);
                bytecode_stack_write(inter_stack, curr_rtn_alloc_pos, inter_base_ptr, sizeof_rtn, rtn_value);
            }

            if (curr_arg_bytes_count > 0) {
                bytecode_stack_size_add_aligned(&inter_stack_offset, curr_arg_bytes_count);
                //todo();
            }

            log(LOG_DEBUG, "inter_prog_counter after return: %zu\n", inter_prog_counter);

            return rtn_addr != INTER_RTN_ADDR_EXIT;
        }
        case BYTECODE_PUSH: {
            log(LOG_TRACE, "bytecode_push\n");

            uint64_t sizeof_alloca = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t value = interpret_read_uint64_t_aligned();
            if (value == 7) {
                //breakpoint();
            }
            inter_stack_dump(LOG_DEBUG);
            bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_alloca);
            inter_stack_dump(LOG_DEBUG);
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            assert(inter_prog_counter - old_prog_counter == BYTECODE_PUSH_SIZE);
            return true;
        }
        case BYTECODE_ZERO_EXTEND: {
            log(LOG_TRACE, "bytecode_zero_extend\n");

            //breakpoint();
            uint64_t sizeof_dest = interpret_read_uint64_t_aligned();
            uint64_t sizeof_src = interpret_read_uint64_t_aligned();
            uint64_t src_pos = interpret_read_uint64_t_aligned();
            uint64_t alloca_pos = interpret_read_uint64_t_aligned();

            assert(sizeof_src <= 8 && "not implemented");
            assert(sizeof_dest <= 8 && "not implemented");

            uint64_t ones = 0;
            for (uint64_t idx = 0; idx < 8*sizeof_src; idx++) {
                ones = ones << 1;
                ones++;
            }
            uint64_t value = bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_src);
            value &= (0x00 | ones);
            bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, sizeof_dest, value);

            bytecode_stack_size_add_aligned(&inter_stack_offset, sizeof_src);

            uint64_t expected_offset = interpret_read_uint64_t_aligned();
            assert(expected_offset == inter_stack_offset);

            assert(inter_prog_counter - old_prog_counter == BYTECODE_ZERO_EXTEND_SIZE);
            return true;
        }
        case BYTECODE_CALL_DIRECT: {
            breakpoint();
            log(LOG_TRACE, "bytecode_call_direct\n");

            static_assert(BYTECODE_CALL_DIRECT_SIZE == 40, "implement functions with arguments?");

            uint64_t addr = interpret_read_uint64_t_aligned();
            uint64_t arg_bytes = interpret_read_uint64_t_aligned();
            assert(get_next_multiple(arg_bytes, 8) == arg_bytes);

            uint64_t old_rtn_alloc_pos = inter_rtn_alloc_pos;
            inter_rtn_alloc_pos = interpret_read_uint64_t_aligned();

            uint64_t expected_stack_offset = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "expected_stack_offset = %zu, inter_stack_offset = %zu\n", expected_stack_offset, inter_stack_offset);
            assert(expected_stack_offset == inter_stack_offset);

            assert(inter_base_ptr > 0);
            uint64_t old_base_ptr = inter_base_ptr;
            uint64_t old_offset = inter_stack_offset;
            uint64_t old_arg_bytes = arg_bytes_count;
            inter_base_ptr -= get_prev_multiple(inter_stack_offset, 8);
            log(LOG_DEBUG, "%zu\n", arg_bytes);
            inter_base_ptr += arg_bytes;
            inter_stack_offset = arg_bytes;
            arg_bytes_count = arg_bytes;
            
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value not explititly handled here

                bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), inter_prog_counter);
                bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), old_base_ptr);
                bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), old_offset);
                bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), old_arg_bytes);
                bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), old_rtn_alloc_pos);
                //bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_dest);
            }

            assert(inter_prog_counter - old_prog_counter == BYTECODE_CALL_DIRECT_SIZE);
            inter_prog_counter = addr;
            return true;
        }


        // --- BINARY OPERATORS ---
        case BYTECODE_ADD_: {
            log(LOG_TRACE, "bytecode_add\n");
            inter_binary(+);
            return true;
        }
        case BYTECODE_SUB: {
            log(LOG_TRACE, "bytecode_sub\n");
            inter_binary(-);
            return true;
        }
        case BYTECODE_GREATER_THAN: {
            log(LOG_TRACE, "bytecode_greater_than\n");
            inter_binary(>);
            return true;
        }
        case BYTECODE_LESS_THAN: {
            log(LOG_TRACE, "bytecode_less_than\n");
            inter_binary(<);
            return true;
        }
        case BYTECODE_DOUBLE_EQUAL: {
            log(LOG_TRACE, "bytecode_double_equal\n");
            inter_binary(==);
            return true;
        }
        case BYTECODE_NOT_EQUAL: {
            log(LOG_TRACE, "bytecode_not_equal\n");
            inter_binary(!=);
            return true;
        }


        case BYTECODE_COUNT:
            unreachable("");
        case BYTECODE_NONE:
            unreachable("");
        default:
            log(LOG_DEBUG, "%x\n", opcode);
            unreachable("");
    }
    unreachable("");
}

void interpret(void) {
    log(LOG_DEBUG, "%zu\n", bytecode.code.info.count);
    breakpoint();
    static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of main function stack frame initial state");
    {
        // return value is not explititily handled here
        bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), INTER_RTN_ADDR_EXIT);
        bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), INTERPRET_STACK_SIZE);
        bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), 0);
        bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), 0);
        bytecode_stack_write(inter_stack, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count), inter_base_ptr, sizeof(uint64_t), 0);
    }

    inter_prog_counter = bytecode.start_pos;

    log(LOG_DEBUG, "%zu\n", inter_prog_counter);

    while (interpret_instruction()) {
        //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - inter_stack_size);
        //inter_stack_dump(LOG_DEBUG);
        log(LOG_DEBUG, "%zu\n", inter_prog_counter);
        //if (inter_prog_counter == 2144) {
            //breakpoint();
        //}
        breakpoint();
        static bool should_break = false;
        if (darr_at(bytecode.code, inter_prog_counter) == BYTECODE_DEREF) {
            //should_break = true;
        }
        if (should_break) {
            breakpoint();
        }
        if (inter_base_ptr != INTERPRET_STACK_SIZE) {
            //breakpoint();
        }
        if (inter_stack_offset >= 144) {
            //breakpoint();
        }
        do_nothing();
    }

    log(LOG_DEBUG, "%zu\n", inter_stack_offset);
    assert(inter_stack_offset <= 8);

    uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, 4);
    msg(DIAG_INFO, POS_BUILTIN, "interpreted program returned %"PRIu64"\n", rtn_value);
    local_exit(rtn_value);
}
