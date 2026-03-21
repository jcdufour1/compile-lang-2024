#define INTER_RTN_ADDR_EXIT (2LU << 60)

static uint64_t inter_prog_counter = 0;
static uint64_t inter_stack_size_ = INTERPRET_STACK_SIZE; // TODO: remove this variable?
static uint64_t inter_stack_offset = 0;
static uint64_t inter_base_ptr = INTERPRET_STACK_SIZE;
static uint8_t inter_stack[INTERPRET_STACK_SIZE] = {0};
//static uint64_t inter_fun_rtn_addr = UINT64_MAX;

static void inter_stack_dump_internal(LOG_LEVEL log_level, const char* file, int line) {
    //uint64_t count_rows = get_next_multiple(INTERPRET_STACK_SIZE, 8)/8;

    String buf = {0};
    string_extend_f(&a_temp, &buf, "inter_stack_dump:\n");
    //for (size_t row = 0; row < count_rows; row++) {
    //    uint64_t mem_location = row*8;
    //    if (mem_location < inter_base_ptr - inter_stack_offset) {
    //        continue;
    //    }

    //    uint64_t value = 0;
    //    memcpy(&value, array_at_ref(inter_stack, mem_location), sizeof(value));
    //    string_extend_f(&a_temp, &buf, "  %08"PRIu64" (%"PRIu64"): %"PRIu64"\n", mem_location, mem_location, value);
    //}
    
    log(LOG_DEBUG, "%zu\n", inter_stack_offset);
    for (size_t offset = 8/* TODO */; offset <= get_next_multiple(inter_stack_offset, 8); offset += 8) {
        log(LOG_DEBUG, "%zu %zu\n", inter_base_ptr, offset);
        assert(inter_base_ptr >= offset);
        uint64_t mem_loc = inter_base_ptr - offset;
        uint64_t value = 0;
        //log(LOG_DEBUG, "%zu %zu\n", mem_loc, INTERPRET_STACK_SIZE);
        memcpy(&value, array_at_ref(inter_stack, mem_loc), sizeof(value));
        string_extend_f(&a_temp, &buf, "  %08"PRIu64" (%"PRIu64"): %"PRIu64"\n", mem_loc, offset, value);
    }

    log_internal(log_level, file, line, 0, FMT"\n", string_print(buf));
}

// TODO: figure out why inter_stack_dump seems to print nothing
#define inter_stack_dump(log_level) inter_stack_dump_internal(log_level, __FILE__, __LINE__)

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
        uint64_t alloca_size = interpret_read_uint64_t_aligned(); \
        uint64_t rhs = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, alloca_size); \
        uint64_t lhs = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, alloca_size); \
        bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, lhs bin rhs, alloca_size); \
        assert(inter_prog_counter - old_prog_counter == BYTECODE_BINARY_SIZE); \
    } while (0)

// TODO: use inter prefix?
// returns true if the program is still running
static bool interpret_instruction(void) {
    uint64_t old_prog_counter = inter_prog_counter;
    (void) old_prog_counter;
    BYTECODE opcode = interpret_read_uint8_t_aligned();
    switch (opcode) {
        case BYTECODE_COMMENT: {
            log(LOG_TRACE, "bytecode_comment\n");

            uint64_t file_len = interpret_read_uint64_t_aligned();
            inter_prog_counter += get_next_multiple(file_len, 8);

            interpret_read_uint64_t_aligned(); // line

            uint64_t comment_len = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            inter_prog_counter += get_next_multiple(comment_len, 8);

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

            assert(inter_prog_counter - old_prog_counter == BYTECODE_ALLOCA_SIZE);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_STORE_STACK: {
            log(LOG_TRACE, "bytecode_store_stack\n");

            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t dest_pos = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t src_pos = interpret_read_uint64_t_aligned();
            uint64_t sizeof_store = interpret_read_uint64_t_aligned();

            uint64_t value = bytecode_stack_at(inter_stack, src_pos, inter_base_ptr);
            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);
            //memcpy(array_at_ref(inter_stack, dest_pos), , sizeof_store);
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            assert(inter_prog_counter - old_prog_counter == BYTECODE_STORE_STACK_SIZE);
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

            assert(inter_prog_counter - old_prog_counter == BYTECODE_GOTO_SIZE);

            inter_prog_counter = new_prog_counter;
            return true;
        }
        case BYTECODE_RETURN: {
            log(LOG_TRACE, "bytecode_return\n");

            //breakpoint();
            uint64_t sizeof_rtn = interpret_read_uint64_t_aligned();
            log(LOG_DEBUG, "inter_stack_size = %zu\n", inter_stack_offset);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            log(LOG_DEBUG, "INTERPRET_STACK_SIZE = %zu\n", INTERPRET_STACK_SIZE);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            // TODO: rename INTERPRET_STACK_SIZE to avoid confusion with inter_stack_size
            log(LOG_DEBUG, "%zu\n", inter_stack_offset);
            inter_stack_dump(LOG_DEBUG);

            log(LOG_DEBUG, "%zu %zu %zu %zu\n", inter_stack_offset, inter_base_ptr, INTERPRET_STACK_SIZE, (inter_base_ptr) - inter_stack_offset);
            inter_stack_dump(LOG_DEBUG);
            uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_rtn);
            inter_stack_dump(LOG_DEBUG);
            bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_RTN_VALUE, inter_base_ptr, sizeof_rtn, rtn_value);
            inter_stack_dump(LOG_DEBUG);

            bytecode_stack_size_add_aligned(&inter_stack_offset, inter_stack_offset - BYTECODE_COUNT_RTN_ITEMS*8);
            inter_stack_dump(LOG_DEBUG);

            uint64_t rtn_addr = 0;
            uint64_t base_ptr = 0;
            uint64_t offset = 0;
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 4, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value is not popped here, because it should remain in the stack

                //breakpoint();
                inter_stack_dump(LOG_DEBUG);

                assert(inter_stack_offset == BYTECODE_CALL_STACK_OFFSET);
                offset = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));

                assert(inter_stack_offset == BYTECODE_CALL_STACK_BASE_PTR);
                base_ptr = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(base_ptr));
                assert(inter_base_ptr > 0);

                assert(inter_stack_offset == BYTECODE_CALL_STACK_RTN_ADDR);
                rtn_addr = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof(rtn_addr));
                //rtn_addr = ((inter_stack)[(inter_base_ptr) - bytecode_stack_size_add_aligned(&inter_stack_offset, sizeof(uint64_t))]);

                assert(inter_stack_offset == BYTECODE_CALL_STACK_RTN_VALUE);
                assert(BYTECODE_CALL_STACK_RTN_VALUE == 8);
            }

            assert(inter_base_ptr > 0);
            log(LOG_DEBUG, "rtn_value = %zu\n", rtn_value);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            inter_stack_dump(LOG_DEBUG);
            assert(inter_base_ptr > 0);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_prog_counter - old_prog_counter == BYTECODE_RETURN_SIZE);

            static_assert(BYTECODE_COUNT_RTN_ITEMS == 4, "update below if nessessary");
            log(LOG_DEBUG, "%zu %zu\n", rtn_addr, INTER_RTN_ADDR_EXIT);
            inter_prog_counter = rtn_addr;
            inter_base_ptr = base_ptr;
            inter_stack_offset = offset;
            assert(inter_base_ptr > 0);

            log(LOG_DEBUG, "inter_prog_counter after return: %zu\n", inter_prog_counter);

            return rtn_addr != INTER_RTN_ADDR_EXIT;
        }
        case BYTECODE_PUSH: {
            log(LOG_TRACE, "bytecode_push\n");

            uint64_t sizeof_alloca = interpret_read_uint64_t_aligned();
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t value = interpret_read_uint64_t_aligned();
            bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_alloca);
            assert(inter_stack_offset % 8 == 0); // TODO: remove

            assert(inter_prog_counter - old_prog_counter == BYTECODE_PUSH_SIZE);
            return true;
        }
        case BYTECODE_ZERO_EXTEND: {
            log(LOG_TRACE, "bytecode_zero_extend\n");

            //breakpoint();
            uint64_t sizeof_dest = interpret_read_uint64_t_aligned();
            uint64_t sizeof_src = interpret_read_uint64_t_aligned();
            assert(sizeof_src <= 8 && "not implemented");
            assert(sizeof_dest <= 8 && "not implemented");

            assert(inter_stack_offset % 8 == 0); // TODO: remove

            uint64_t ones = 0;
            for (uint64_t idx = 0; idx < 8*sizeof_src; idx++) {
                ones = ones << 1;
                ones++;
            }
            // TODO: bytecode does not actually replace stack place with new thing
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_src);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            value &= (0x00 | ones);
            bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_dest);

            assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(inter_prog_counter - old_prog_counter == BYTECODE_ZERO_EXTEND_SIZE);
            assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_CALL_DIRECT: {
            log(LOG_TRACE, "bytecode_call_direct\n");

            static_assert(BYTECODE_CALL_DIRECT_SIZE == 16, "implement functions with arguments?");

            uint64_t addr = interpret_read_uint64_t_aligned();

            assert(inter_base_ptr > 0);
            uint64_t old_base_ptr = inter_base_ptr;
            uint64_t old_offset = inter_stack_offset;
            inter_base_ptr -= get_prev_multiple(inter_stack_offset, 8);
            inter_stack_offset = 0;
            
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 4, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value not explititly handled here

                bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_RTN_ADDR, inter_base_ptr, sizeof(uint64_t), inter_prog_counter);
                bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_BASE_PTR, inter_base_ptr, sizeof(uint64_t), old_base_ptr);
                bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_OFFSET, inter_base_ptr, sizeof(uint64_t), old_offset);
                //bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_dest);
            }

            assert(inter_prog_counter - old_prog_counter == BYTECODE_CALL_DIRECT_SIZE);
            inter_prog_counter = addr;
            return true;
        }


        // --- BINARY OPERATORS ---
        case BYTECODE_ADD: {
            inter_binary(+);
            return true;
        }
        case BYTECODE_SUB: {
            inter_binary(-);
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
    static_assert(BYTECODE_COUNT_RTN_ITEMS == 4, "exhausive handling of main function stack frame initial state");
    {
        // return value is not explititily handled here
        bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_RTN_ADDR, inter_base_ptr, sizeof(uint64_t), INTER_RTN_ADDR_EXIT);
        bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_BASE_PTR, inter_base_ptr, sizeof(uint64_t), INTERPRET_STACK_SIZE);
        bytecode_stack_write(inter_stack, BYTECODE_CALL_STACK_OFFSET, inter_base_ptr, sizeof(uint64_t), 8/*program return value*/);
    }

    inter_prog_counter = bytecode.start_pos;

    log(LOG_DEBUG, "%zu\n", inter_prog_counter);

    while (interpret_instruction()) {
        //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - inter_stack_size);
        inter_stack_dump(LOG_DEBUG);
        //breakpoint();
        do_nothing();
    }

    assert(inter_stack_offset <= 8);

    uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, 4);
    msg(DIAG_INFO, POS_BUILTIN, "interpreted program returned %"PRIu64"\n", rtn_value);
    local_exit(rtn_value);
}
