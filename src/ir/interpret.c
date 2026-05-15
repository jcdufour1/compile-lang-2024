#define INTER_RTN_ADDR_EXIT (2LU << 60)

static Bytes inter_prog_counter = bytes_new_macro(0);
static Bytes inter_stack_size_ = INTERPRET_STACK_SIZE; // TODO: remove this variable?
static Bytes inter_stack_offset = bytes_new_macro(0);
static Bytes arg_bytes_count = bytes_new_macro(0); // TODO: use inter prefix
static Bytes inter_rtn_alloc_pos = bytes_new_macro(0);
static Bytes inter_base_ptr = INTERPRET_STACK_SIZE;
static uint8_t inter_stack[(1024*1024UL)/*TODO: dont hardcode here*/] = {0};
//static uint64_t inter_fun_rtn_addr = UINT64_MAX;

// TODO: figure out why inter_stack_dump seems to print nothing
#define inter_stack_dump(log_level) bytecode_stack_dump_internal(log_level, __FILE__, __LINE__, inter_stack, inter_stack_offset, inter_base_ptr)

static uint8_t interpret_read_uint8_t(void) {
    uint64_t value = 0;
    // TODO: this memcpys 8 bytes for 1 bytes integer
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter.value/*TODO*/), sizeof(value));
    bytes_increment(&inter_prog_counter);

    return value;
}

static uint8_t interpret_read_uint8_t_aligned(void) {
    uint64_t value = 0;
    // TODO: this memcpys 8 bytes for 1 bytes integer
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter.value/*TODO*/), sizeof(value));
    bytes_increment(&inter_prog_counter);

    inter_prog_counter = get_next_multiple(inter_prog_counter, bytes_new(8)/*TODO*/);

    return value;
}

static uint64_t interpret_read_uint64_t_aligned(void) {
    uint64_t value = 0;
    memcpy(&value, darr_at_ref(&bytecode.code, inter_prog_counter.value/*TODO*/), sizeof(value));
    bytes_increment(&inter_prog_counter);

    inter_prog_counter = get_next_multiple(inter_prog_counter, bytes_new(8)/*TODO*/);

    return value;
}

static Bytes interpret_read_bytes_aligned(void) {
    return bytes_new(interpret_read_uint64_t_aligned());
}

#define inter_binary(bin) \
    do { \
        inter_stack_dump(LOG_DEBUG); \
        \
        Bytes start_args = interpret_read_bytes_aligned(); \
        Bytes alloca_pos = interpret_read_bytes_aligned(); \
        Bytes alloca_size = interpret_read_bytes_aligned(); \
        Bytes expected_offset = interpret_read_bytes_aligned(); \
        \
        Bytes pos_lhs = start_args; \
        Bytes pos_rhs = pos_lhs; \
        bytecode_stack_size_add_aligned(&pos_rhs, alloca_size); \
        \
        log(LOG_DEBUG, #bin" alloca_pos = "FMT"\n", bytes_print(alloca_pos)); \
        log(LOG_DEBUG, #bin" pos_lhs = "FMT"\n", bytes_print(pos_lhs)); \
        log(LOG_DEBUG, #bin" pos_rhs = "FMT"\n", bytes_print(pos_rhs)); \
        uint64_t lhs = uint8_t_view_cast_to_uint64_t(bytecode_stack_read(inter_stack, pos_lhs, inter_base_ptr, alloca_size)); \
        uint64_t rhs = uint8_t_view_cast_to_uint64_t(bytecode_stack_read(inter_stack, pos_rhs, inter_base_ptr, alloca_size)); \
        log(LOG_DEBUG, #bin" lhs = %"PRIi64"\n", lhs); \
        log(LOG_DEBUG, #bin" rhs = %"PRIi64"\n", rhs); \
        log(LOG_DEBUG, #bin" value = %"PRIi64"\n", (int64_t)(lhs bin rhs)); \
        \
        uint64_t result = lhs bin rhs; \
        bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, alloca_size, uint8_t_view_from_uint64_t(&result)); \
        \
        bytecode_stack_size_add_aligned(&inter_stack_offset, alloca_size); \
        bytecode_stack_size_add_aligned(&inter_stack_offset, alloca_size); \
        \
        assert(bytes_is_equal(expected_offset, inter_stack_offset)); \
        assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_BINARY_SIZE)); \
    } while (0)

// TODO: use inter prefix?
// returns true if the program is still running
static bool interpret_instruction(void) {
    if (bytes_is_equal(inter_prog_counter, bytes_new(5464))) {
        //breakpoint();
    }
    Bytes old_prog_counter = inter_prog_counter;
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
            inter_prog_counter = get_next_multiple(inter_prog_counter, bytes_new(8));
            //inter_prog_counter += get_next_multiple(file_len, 8);

            uint64_t line = interpret_read_uint64_t_aligned(); // line
            string_extend_f(&a_temp, &buf, ":%"PRIu64":", line);

            uint64_t comment_len = interpret_read_uint64_t_aligned();
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            for (uint64_t file_idx = 0; file_idx < comment_len; file_idx++) {
                string_append(&a_temp, &buf, (char)interpret_read_uint8_t());
            }
            inter_prog_counter = get_next_multiple(inter_prog_counter, bytes_new(8));
            //inter_prog_counter += get_next_multiple(comment_len, 8);

            log(LOG_TRACE, FMT"\n", string_print(buf));

            assert(
                inter_prog_counter.value - old_prog_counter.value
                == 
                8 /* BYTECODE_COMMENT */ + 
                8 /* file_len */ + get_next_multiple(file_len, 8) /* file*/ + 
                8 /* line */ + 
                8 /* comment_len */+ get_next_multiple(comment_len, 8) /* comment */
            );
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_ALLOCA: {
            log(LOG_TRACE, "bytecode_alloca\n");

            Bytes sizeof_alloca = interpret_read_bytes_aligned();
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            bytecode_stack_size_sub_aligned(&inter_stack_offset, sizeof_alloca);

            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_ALLOCA_SIZE));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            return true;
        }
        case BYTECODE_STORE_STACK: { // TODO: rename to BYTECODE_STORE
            log(LOG_TRACE, "bytecode_store_stack\n");

            //if (inter_prog_counter > 22896) {
                //breakpoint();
            //}
            Bytes dest_pos = interpret_read_bytes_aligned();
            Bytes src_pos = interpret_read_bytes_aligned();
            Bytes sizeof_store = interpret_read_bytes_aligned();

            Uint8_t_view value = bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_store);
            log(
                LOG_TRACE,
                "in the middle of bytecode_store_stack ("
                    "inter_prog_counter = "FMT", "
                    "dest_pos = "FMT", "
                    "src_pos = "FMT", "
                    "sizeof_store = "FMT", "
                    "value = "FMT
                ")\n",
                bytes_print(inter_prog_counter),
                bytes_print(dest_pos),
                bytes_print(src_pos),
                bytes_print(sizeof_store),
                uint8_t_view_print(value)
            );

            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);
            //memcpy(array_at_ref(inter_stack, dest_pos), , sizeof_store);
            //assert(inter_stack_offset % 8 == 0); // TODO: remove

            Bytes expected_offset = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "inter_prog_counter = "FMT", expected_offset = "FMT", inter_stack_offset = "FMT"\n", bytes_print(inter_prog_counter), bytes_print(expected_offset), bytes_print(inter_stack_offset));
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_STORE_STACK_SIZE));
            return true;
        }
        case BYTECODE_STORE_STACK_DEREF_DEST: {
            log(LOG_TRACE, "bytecode_store_stack_deref_dest\n");

            Bytes dest_pos_ptr = interpret_read_bytes_aligned();
            Bytes src_pos = interpret_read_bytes_aligned();
            Bytes sizeof_store = interpret_read_bytes_aligned();

            Bytes dest_pos = uint8_t_view_to_bytes(bytecode_stack_read(inter_stack, dest_pos_ptr, inter_base_ptr, bytes_new(8)));
            Uint8_t_view value = bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_store);
            log(
                LOG_TRACE,
                "bytecode_store_stack (dest_pos_ptr = "FMT", "
                    "dest_pos = "FMT", "
                    "src_pos = "FMT", "
                    "sizeof_store = "FMT
                ")\n",
                bytes_print(dest_pos_ptr),
                bytes_print(dest_pos),
                bytes_print(src_pos),
                bytes_print(sizeof_store)
            );
            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, value);
            //memcpy(array_at_ref(inter_stack, dest_pos), , sizeof_store);

            Bytes expected_offset = interpret_read_bytes_aligned();
            //log(LOG_DEBUG, "inter_prog_counter = %zu, expected_offset = %zu, inter_stack_offset = %zu\n", inter_prog_counter, expected_offset, inter_stack_offset);
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_STORE_STACK_SIZE));
            return true;
        }
        case BYTECODE_DEREF: {
            log(LOG_TRACE, "deref\n");

            Bytes src_ptr = interpret_read_bytes_aligned();
            Bytes sizeof_alloca = interpret_read_bytes_aligned();
            Bytes alloca_pos = interpret_read_bytes_aligned();

            Bytes src = uint8_t_view_to_bytes(bytecode_stack_read(inter_stack, src_ptr, inter_base_ptr, bytes_new(8)));
            Uint8_t_view derefed = bytecode_stack_read(inter_stack, src, inter_base_ptr, sizeof_alloca);
            log(LOG_DEBUG, "src_ptr = "FMT", src = "FMT", sizeof_alloca = "FMT", derefed = "FMT"\n", bytes_print(src_ptr), bytes_print(src), bytes_print(sizeof_alloca), uint8_t_view_print(derefed));
            breakpoint();
            bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, sizeof_alloca, derefed);

            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_DEREF_SIZE));
            return true;
        }
        case BYTECODE_STORE_STACK_DIR_ADDR: {
            log(LOG_TRACE, "bytecode_store_stack_dir_addr; inter_prog_counter = "FMT"\n", bytes_print(inter_prog_counter));

            Bytes dest_pos = interpret_read_bytes_aligned();
            uint64_t value = interpret_read_uint64_t_aligned();
            Bytes sizeof_store = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "value = %"PRIu64"\n", value);
            bytecode_stack_write(inter_stack, dest_pos, inter_base_ptr, sizeof_store, uint8_t_view_from_uint64_t(&value));

            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_STORE_STACK_DIR_ADDR_SIZE));
            return true;
        }
        case BYTECODE_GOTO: {
            log(LOG_TRACE, "bytecode_goto\n");

            log(LOG_DEBUG, "inter_prog_counter = "FMT"\n", bytes_print(inter_prog_counter));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            Bytes new_prog_counter = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "inter_prog_counter = "FMT"\n", bytes_print(inter_prog_counter));
            log(LOG_DEBUG, "new_prog_counter = "FMT"\n", bytes_print(new_prog_counter));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove

            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_GOTO_SIZE));

            inter_prog_counter = new_prog_counter;
            return true;
        }
        case BYTECODE_COND_GOTO: {
            log(LOG_TRACE, "bytecode_cond_goto\n");
            breakpoint();

            Bytes if_true_prog_counter = interpret_read_bytes_aligned();
            Bytes if_false_prog_counter = interpret_read_bytes_aligned();

            uint64_t cond = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(1));
            Bytes new_prog_counter = if_false_prog_counter;
            if (cond) {
                new_prog_counter = if_true_prog_counter;
            }

            Bytes expected_offset = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "expected_offset = "FMT", inter_stack_offset = "FMT"\n", bytes_print(expected_offset), bytes_print(inter_stack_offset));
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_COND_GOTO_SIZE));

            inter_prog_counter = new_prog_counter;
            return true;
        }
        case BYTECODE_RETURN: {
            log(LOG_TRACE, "bytecode_return\n");

            breakpoint();
            Bytes sizeof_rtn = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "inter_stack_size = "FMT"\n", bytes_print(inter_stack_offset));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            log(LOG_DEBUG, "INTERPRET_STACK_SIZE = "FMT"\n", bytes_print(INTERPRET_STACK_SIZE));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            // TODO: rename INTERPRET_STACK_SIZE to avoid confusion with inter_stack_size
            //log(LOG_DEBUG, "%zu\n", inter_stack_offset);
            //inter_stack_dump(LOG_DEBUG);

            // TODO: this expected_offset is not really after entire return instruction executes
            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            //log(LOG_DEBUG, "%zu %zu %zu %zu\n", inter_stack_offset, inter_base_ptr, INTERPRET_STACK_SIZE, (inter_base_ptr) - inter_stack_offset);
            inter_stack_dump(LOG_DEBUG);
            uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_rtn);
            inter_stack_dump(LOG_DEBUG);
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_VALUE, arg_bytes_count),
                inter_base_ptr,
                sizeof_rtn,
                uint8_t_view_from_uint64_t(&rtn_value)
            );
            inter_stack_dump(LOG_DEBUG);

            bytecode_stack_size_add_aligned(&inter_stack_offset, bytes_subtract(bytes_subtract(inter_stack_offset, bytes_new(BYTECODE_COUNT_RTN_ITEMS*8)), arg_bytes_count));
            inter_stack_dump(LOG_DEBUG);

            Bytes curr_rtn_alloc_pos = inter_rtn_alloc_pos;
            Bytes curr_arg_bytes_count = arg_bytes_count;

            Bytes arg_bytes = bytes_new(0);
            Bytes rtn_addr = bytes_new(0);
            Bytes base_ptr = bytes_new(0);
            Bytes offset = bytes_new(0);
            Bytes rtn_alloc_pos = bytes_new(0);
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value is not popped here, because it should remain in the stack

                //breakpoint();
                inter_stack_dump(LOG_DEBUG);

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count)));
                rtn_alloc_pos = bytes_new(bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(sizeof(base_ptr))));

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count)));
                arg_bytes = bytes_new(bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(sizeof(base_ptr))));

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count)));
                offset = bytes_new(bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(sizeof(base_ptr))));

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count)));
                base_ptr = bytes_new(bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(sizeof(base_ptr))));
                assert(bytes_is_greater(inter_base_ptr, bytes_new(0)));

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count)));
                rtn_addr = bytes_new(bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(sizeof(rtn_addr))));
                //rtn_addr = ((inter_stack)[(inter_base_ptr) - bytecode_stack_size_add_aligned(&inter_stack_offset, sizeof(uint64_t))]);

                assert(bytes_is_equal(inter_stack_offset, bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_VALUE, arg_bytes_count)));
                assert(
                    bytes_is_equal(inter_stack_offset, bytes_add(bytes_new(8), arg_bytes_count)) && 
                    "BYTECODE_CALL_STACK_TYPE_RTN_VALUE should == 0, and return value should be directly on top of function args in stack frame"
                );
            }

            offset = get_next_multiple(offset, bytes_new(8));
            offset = bytes_add(offset, get_next_multiple(sizeof_rtn, bytes_new(8)));

            assert(bytes_is_greater(inter_base_ptr, bytes_new(0)));
            log(LOG_DEBUG, "rtn_value = %zu\n", rtn_value);
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            inter_stack_dump(LOG_DEBUG);
            assert(bytes_is_greater(inter_base_ptr, bytes_new(0)));
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_RETURN_SIZE));

            static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "update below if nessessary");
            //log(LOG_DEBUG, "%zu %zu\n", rtn_addr, INTER_RTN_ADDR_EXIT);
            inter_prog_counter = rtn_addr;
            inter_base_ptr = base_ptr;
            inter_stack_offset = offset;
            arg_bytes_count = arg_bytes;
            inter_rtn_alloc_pos = rtn_alloc_pos;
            assert(bytes_is_greater(inter_base_ptr, bytes_new(0)));

            if (!bytes_is_equal(rtn_addr, bytes_new(INTER_RTN_ADDR_EXIT)/*TODO*/)) {
                uint64_t temp_rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, sizeof_rtn);
                assert(rtn_value == temp_rtn_value);
                //log(LOG_DEBUG, "%zu\n", curr_rtn_alloc_pos);
                bytecode_stack_write(inter_stack, curr_rtn_alloc_pos, inter_base_ptr, sizeof_rtn, uint8_t_view_from_uint64_t(&rtn_value));
            }

            if (bytes_is_greater(curr_arg_bytes_count, bytes_new(0))) {
                bytecode_stack_size_add_aligned(&inter_stack_offset, curr_arg_bytes_count);
                //todo();
            }

            log(LOG_DEBUG, "inter_prog_counter after return: "FMT"\n", bytes_print(inter_prog_counter));

            return !bytes_is_equal(rtn_addr, bytes_new(INTER_RTN_ADDR_EXIT));
        }
        case BYTECODE_PUSH: {
            log(LOG_TRACE, "bytecode_push\n");

            Bytes sizeof_alloca = interpret_read_bytes_aligned();
            //assert(inter_stack_offset % 8 == 0); // TODO: remove
            uint64_t value = interpret_read_uint64_t_aligned();
            if (value == 7) {
                //breakpoint();
            }
            inter_stack_dump(LOG_DEBUG);
            bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_alloca);
            inter_stack_dump(LOG_DEBUG);
            //assert(inter_stack_offset % 8 == 0); // TODO: remove

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_PUSH_SIZE));
            return true;
        }
        case BYTECODE_ZERO_EXTEND: {
            log(LOG_TRACE, "bytecode_zero_extend\n");

            //breakpoint();
            Bytes sizeof_dest = interpret_read_bytes_aligned();
            Bytes sizeof_src = interpret_read_bytes_aligned();
            Bytes src_pos = interpret_read_bytes_aligned();
            Bytes alloca_pos = interpret_read_bytes_aligned();

            assert(bytes_is_less_or_equal(sizeof_src, bytes_new(8)) && "not implemented");
            assert(bytes_is_less_or_equal(sizeof_dest, bytes_new(8)) && "not implemented");

            uint64_t ones = 0;
            for (uint64_t idx = 0; idx < 8*sizeof_src.value/*TODO*/; idx++) {
                ones = ones << 1;
                ones++;
            }
            uint64_t value = uint8_t_view_cast_to_uint64_t(bytecode_stack_read(inter_stack, src_pos, inter_base_ptr, sizeof_src));
            value &= (0x00 | ones);
            bytecode_stack_write(inter_stack, alloca_pos, inter_base_ptr, sizeof_dest, uint8_t_view_from_uint64_t(&value));

            bytecode_stack_size_add_aligned(&inter_stack_offset, sizeof_src);

            Bytes expected_offset = interpret_read_bytes_aligned();
            assert(bytes_is_equal(expected_offset, inter_stack_offset));

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_ZERO_EXTEND_SIZE));
            return true;
        }
        case BYTECODE_CALL_DIRECT: {
            breakpoint();
            log(LOG_TRACE, "bytecode_call_direct\n");

            assert(BYTECODE_CALL_DIRECT_SIZE.value == 40 && "implement functions with arguments?");

            Bytes addr = interpret_read_bytes_aligned();
            Bytes arg_bytes = interpret_read_bytes_aligned();
            assert(bytes_is_equal(get_next_multiple(arg_bytes, bytes_new(8)), arg_bytes));

            Bytes old_rtn_alloc_pos = inter_rtn_alloc_pos;
            inter_rtn_alloc_pos = interpret_read_bytes_aligned();

            Bytes expected_stack_offset = interpret_read_bytes_aligned();
            log(LOG_DEBUG, "expected_stack_offset = "FMT", inter_stack_offset = "FMT"\n", bytes_print(expected_stack_offset), bytes_print(inter_stack_offset));
            assert(bytes_is_equal(expected_stack_offset, inter_stack_offset));

            assert(bytes_is_greater(inter_base_ptr, bytes_new(0)));
            Bytes old_base_ptr = inter_base_ptr;
            Bytes old_offset = inter_stack_offset;
            Bytes old_arg_bytes = arg_bytes_count;
            inter_base_ptr = bytes_subtract(inter_base_ptr, get_prev_multiple(inter_stack_offset, bytes_new(8)));
            log(LOG_DEBUG, FMT"\n", bytes_print(arg_bytes));
            inter_base_ptr = bytes_add(inter_base_ptr, arg_bytes);
            inter_stack_offset = arg_bytes;
            arg_bytes_count = arg_bytes;
            
            {
                static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of BYTECODE_CALL_STACK_* in this block");
                // return value not explititly handled here

                {
                    uint64_t temp = inter_prog_counter.value/*TODO*/;
                    bytecode_stack_write(
                        inter_stack,
                        bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count),
                        inter_base_ptr,
                        bytes_new(sizeof(uint64_t)),
                        uint8_t_view_from_uint64_t(&temp)
                    );
                }
                {
                    uint64_t temp = old_base_ptr.value/*TODO*/;
                    bytecode_stack_write(
                        inter_stack,
                        bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count),
                        inter_base_ptr,
                        bytes_new(sizeof(uint64_t)),
                        uint8_t_view_from_uint64_t(&temp)
                    );
                }
                {
                    uint64_t temp = old_offset.value/*TODO*/;
                    bytecode_stack_write(
                        inter_stack,
                        bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count),
                        inter_base_ptr,
                        bytes_new(sizeof(uint64_t)),
                        uint8_t_view_from_uint64_t(&temp)
                    );
                }
                {
                    uint64_t temp = old_arg_bytes.value/*TODO*/;
                    bytecode_stack_write(
                        inter_stack,
                        bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count),
                        inter_base_ptr,
                        bytes_new(sizeof(uint64_t)),
                        uint8_t_view_from_uint64_t(&temp)
                    );
                }
                {
                    uint64_t temp = old_rtn_alloc_pos.value/*TODO*/;
                    bytecode_stack_write(
                        inter_stack,
                        bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count),
                        inter_base_ptr,
                        bytes_new(sizeof(uint64_t)),
                        uint8_t_view_from_uint64_t(&temp)
                    );
                }
                //bytecode_stack_push(inter_stack, &inter_stack_offset, inter_base_ptr, value, sizeof_dest);
            }

            assert(bytes_is_equal(bytes_subtract(inter_prog_counter, old_prog_counter), BYTECODE_CALL_DIRECT_SIZE));
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
    assert(array_count(inter_stack) == INTERPRET_STACK_SIZE.value);

    log(LOG_DEBUG, "%zu\n", bytecode.code.info.count);
    breakpoint();
    assert(bytes_is_equal(inter_stack_offset, bytes_new(0)));
    static_assert(BYTECODE_COUNT_RTN_ITEMS == 6, "exhausive handling of main function stack frame initial state");
    {
        // return value is not explititily handled here
        {
            uint64_t temp = INTER_RTN_ADDR_EXIT;
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ADDR, arg_bytes_count),
                inter_base_ptr,
                bytes_new(sizeof(uint64_t)),
                uint8_t_view_from_uint64_t(&temp)
            );
        }
        {
            uint64_t temp = INTERPRET_STACK_SIZE.value/*TODO*/;
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_BASE_PTR, arg_bytes_count),
                inter_base_ptr,
                bytes_new(sizeof(uint64_t)),
                uint8_t_view_from_uint64_t(&temp)
            );
        }
        {
            uint64_t temp = 0;
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_OFFSET, arg_bytes_count),
                inter_base_ptr,
                bytes_new(sizeof(uint64_t)),
                uint8_t_view_from_uint64_t(&temp)
            );
        }
        {
            uint64_t temp = 0;
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_ARG_BYTES, arg_bytes_count),
                inter_base_ptr,
                bytes_new(sizeof(uint64_t)),
                uint8_t_view_from_uint64_t(&temp)
            );
        }
        {
            uint64_t temp = 0;
            bytecode_stack_write(
                inter_stack,
                bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE_RTN_ALLOC_POS, arg_bytes_count),
                inter_base_ptr,
                bytes_new(sizeof(uint64_t)),
                uint8_t_view_from_uint64_t(&temp)
            );
        }
    }
    inter_stack_dump(LOG_DEBUG);
    //bytecode_dump(stderr, LOG_DEBUG, false, bytecode);

    inter_prog_counter = bytes_new(bytecode.start_pos);

    //log(LOG_DEBUG, "%zu\n", inter_prog_counter);

    while (interpret_instruction()) {
        ////log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - inter_stack_size);
        ////inter_stack_dump(LOG_DEBUG);
        //log(LOG_DEBUG, "%zu\n", inter_prog_counter);
        ////if (inter_prog_counter == 2144) {
        //    //breakpoint();
        ////}
        ////breakpoint();
        static bool should_break = false;
        if (darr_at(bytecode.code, inter_prog_counter.value/*TODO*/) == BYTECODE_DEREF) {
            //should_break = true;
        }
        if (should_break) {
            breakpoint();
        }
        if (!bytes_is_equal(inter_base_ptr, INTERPRET_STACK_SIZE)) {
            //breakpoint();
        }
        //breakpoint();
        //if (inter_stack_offset >= 144) {
        //    //breakpoint();
        //}
        do_nothing();
    }

    //log(LOG_DEBUG, "%zu\n", inter_stack_offset);
    assert(bytes_is_less_or_equal(inter_stack_offset, bytes_new(8)));

    uint64_t rtn_value = bytecode_stack_pop(inter_stack, &inter_stack_offset, inter_base_ptr, bytes_new(4));
    msg(DIAG_INFO, POS_BUILTIN, "interpreted program returned %"PRIu64"\n", rtn_value);
    local_exit(rtn_value);
}
