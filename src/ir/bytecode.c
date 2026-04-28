#include <bytecode.h>

bool bytecode_is_before_backpatching_complete = false;

void bytecode_stack_dump_internal(LOG_LEVEL log_level, const char* file, int line, uint8_t* stack, uint64_t stack_offset, uint64_t base_ptr) {
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
    
    log(LOG_DEBUG, "%zu\n", stack_offset);
    for (size_t offset = 8/* TODO */; offset <= get_next_multiple(stack_offset, 8); offset += 8) {
        //log(LOG_DEBUG, "%zu %zu\n", inter_base_ptr, offset);
        assert(base_ptr >= offset);
        uint64_t mem_loc = base_ptr - offset;
        uint64_t value = 0;
        //log(LOG_DEBUG, "%zu %zu\n", mem_loc, INTERPRET_STACK_SIZE);
        memcpy(&value, &stack[mem_loc], sizeof(value));
        string_extend_f(&a_temp, &buf, "  %08"PRIu64" (%"PRIu64"): %"PRIu64"\n", mem_loc, offset, value);
    }

    log_internal(log_level, file, line, 0, FMT"\n", string_print(buf));
}

void bytecode_align(void) {
    while (bytecode.code.info.count != get_next_multiple(bytecode.code.info.count, BYTECODE_ALIGN)) {
        darr_append(&a_main, &bytecode.code, 0);
    }
}

void bytecode_append_align(BYTECODE opcode) {
    darr_append(&a_main, &bytecode.code, opcode);
    bytecode_align();
}

size_t bytecode_read_size_t(size_t index) {
    size_t value = 0;
    memcpy(&value, darr_at_ref(&bytecode.code, index), sizeof(value));
    return value;
}

uint64_t bytecode_read_uint64_t(uint64_t index) {
    uint64_t value = 0;
    memcpy(&value, darr_at_ref(&bytecode.code, index), sizeof(value));
    return value;
}

uint8_t bytecode_read_uint8_t(uint64_t index) {
    uint8_t value = 0;
    memcpy(&value, darr_at_ref(&bytecode.code, index), sizeof(value));
    return value;
}

char bytecode_read_char(uint64_t index) {
    return (char)bytecode_read_uint8_t(index);
}

// TODO: remove this function?
Strv bytecode_alloca_pos_print_internal(uint64_t raw_pos) {
    return strv_from_f(&a_temp, "%"PRIu64, raw_pos);
}

static uint64_t bytecode_dump_read_uint64_t(uint64_t* idx) {
    uint64_t value = bytecode_read_uint64_t(*idx);
    (*idx) += 8;
    return value;
}

static char bytecode_dump_read_char(uint64_t* idx) {
    char value = bytecode_read_char(*idx);
    (*idx)++;
    return value;
}

static void bytecode_dump_read_and_extend_stack_offset(
    String* buf,
    uint64_t* idx,
    uint64_t store_location /* could be zero if there is no store location */
) {
    uint64_t expected_offset = bytecode_dump_read_uint64_t(idx);

    string_extend_f(
        &a_temp,
        buf,
        "    stack offset after instruction: %"PRIu64"\n",
        expected_offset
    );

    log(LOG_DEBUG, "idx = %zu, store_location = %zu, expected_offset = %zu\n", *idx, store_location, expected_offset);
    if (!bytecode_is_before_backpatching_complete) {
        if (store_location > expected_offset) {
            log(LOG_DEBUG, FMT"\n", string_print(*buf));
            unreachable("store location is greater offset than current stack offset here");
        }
    }
}

static void bytecode_dump_internal_binary(String* buf, Strv bin_name, uint64_t old_idx, uint64_t* idx, uint64_t* stack_size) {
    (void) stack_size;
    // TODO: change bytecode and interpreter so that start_args and alloca_pos do not need to be
    //   stored in bytecode?
    uint64_t start_args = bytecode_dump_read_uint64_t(idx);
    uint64_t alloca_pos = bytecode_dump_read_uint64_t(idx);
    uint64_t alloca_size = bytecode_dump_read_uint64_t(idx);

    uint64_t pos_rhs = start_args;
    uint64_t pos_lhs = pos_rhs;
    log(LOG_DEBUG, "%zu\n", pos_lhs);
    if (pos_lhs < 16) {
        pos_lhs = 80;
    }
    log(LOG_DEBUG, "%zu\n", pos_lhs);
    bytecode_stack_size_add_aligned(&pos_lhs, alloca_size);

    string_extend_f(&a_temp, buf, "  %"PRIu64": "FMT": (store location: "FMT")\n", old_idx, strv_print(bin_name), bytecode_alloca_pos_print(alloca_pos));

    string_extend_f(&a_temp, buf, "    pos lhs: "FMT" \n", bytecode_alloca_pos_print(pos_lhs));
    string_extend_f(&a_temp, buf, "    pos rhs: "FMT" \n", bytecode_alloca_pos_print(pos_rhs));

    log(LOG_DEBUG, "idx = %zu\n", *idx);
    bytecode_dump_read_and_extend_stack_offset(buf, idx, alloca_pos);

    bytecode_stack_size_add_aligned(stack_size, alloca_size);
    bytecode_stack_size_add_aligned(stack_size, alloca_size);

    assert(*idx - old_idx == BYTECODE_ADD_SIZE);
    assert(*idx - old_idx == BYTECODE_BINARY_SIZE);
}

typedef struct {
    uint64_t fun_start;
    uint64_t arg_bytes_count;
} Bytecode_dump_mapping;

static Bytecode_dump_mapping bytecode_dump_mapping_new(uint64_t fun_start, uint64_t arg_bytes_count) {
    return (Bytecode_dump_mapping) {.fun_start = fun_start, .arg_bytes_count = arg_bytes_count};
}

typedef struct {
    Vec_base info;
    Bytecode_dump_mapping* buf;
} Bytecode_dump_mapping_darr;

static int bytecode_mapping_cmp(const void* lhs_, const void* rhs_) {
    const Bytecode_dump_mapping* lhs = lhs_;
    const Bytecode_dump_mapping* rhs = rhs_;

    if (lhs->fun_start < rhs->fun_start) {
        return QSORT_LESS_THAN;
    }
    if (lhs->fun_start > rhs->fun_start) {
        return QSORT_MORE_THAN;
    }

    return QSORT_EQUAL;
}

static void bytecode_dump_internal_2(
    FILE* dest,
    const char* file,
    int line,
    Bytecode_dump_mapping_darr* mapping, // TODO: rename to mappings
    LOG_LEVEL log_level,
    Bytecode bytecode,
    bool is_first_step
) {
    if (is_first_step) {
        // do main function
        darr_append(&a_temp, mapping, bytecode_dump_mapping_new(
            bytecode.start_pos,
            0/* TODO: change this if main function has more than zero args */
        ));
    }

    // TODO: prevent allocating this string twice?
    String buf = {0};

    string_extend_f(&a_temp, &buf, "\n");
    if (is_first_step) {
        string_extend_f(&a_temp, &buf, "is_backpatching (1st pass)\n\n");
    } else {
        string_extend_f(&a_temp, &buf, "is_not_backpatching (2nd pass)\n\n");
    }

    size_t idx = 0; // TODO: rename to bytecode_idx?
    uint64_t stack_offset = 0;
    while (idx < bytecode.code.info.count) {
        if (!is_first_step) {
            Bytecode_dump_mapping key = bytecode_dump_mapping_new(idx, 0);
            const Bytecode_dump_mapping* result = bsearch(
                &key,
                mapping->buf,
                mapping->info.count,
                sizeof(mapping->buf[0]),
                bytecode_mapping_cmp
            );
            if (result) {
                stack_offset = result->arg_bytes_count;
            }
        }

        size_t old_idx = idx;

        BYTECODE curr_opcode = darr_at(bytecode.code, idx);
        idx += 8;

        switch (curr_opcode) {
            case BYTECODE_COMMENT: {
                log(LOG_TRACE, "comment\n");
                string_extend_f(&a_temp, &buf, "\n  %"PRIu64":// ", old_idx);
                
                uint64_t file_len = bytecode_dump_read_uint64_t(&idx);
                for (uint64_t file_idx = 0; file_idx < file_len; file_idx++) {
                    string_append(&a_temp, &buf, bytecode_dump_read_char(&idx));
                }
                idx = get_next_multiple(idx, 8);

                uint64_t line = bytecode_dump_read_uint64_t(&idx);
                string_extend_f(&a_temp, &buf, ":%"PRIu64":", line);

                // TODO: prevent need for repeated idx += 8 (make wrapper functions to do it automatically)
                uint64_t comment_len = bytecode_dump_read_uint64_t(&idx);

                for (uint64_t com_idx = 0; com_idx < comment_len; com_idx++) {
                    string_append(&a_temp, &buf, bytecode_dump_read_char(&idx));
                }
                idx = get_next_multiple(idx, 8);

                string_append(&a_temp, &buf, '\n');
                idx = get_next_multiple(idx, 8);

                assert(idx % 8 == 0);
                // TODO: make a function for some of below to reduce duplication?
                assert(
                    idx - old_idx
                    == 
                    8 /* BYTECODE_COMMENT */ + 
                    8 /* file_len */ + get_next_multiple(file_len, 8) /* file*/ + 
                    8 /* line */ + 
                    8 /* comment_len */+ get_next_multiple(comment_len, 8) /* comment */
                );
                break;
            }
            case BYTECODE_ALLOCA: {
                log(LOG_TRACE, "alloca\n");
                uint64_t alloca_size = bytecode_dump_read_uint64_t(&idx);
                uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&stack_offset, alloca_size);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": alloca: %"PRIu64" bytes (store location: %"PRIu64")\n", old_idx, alloca_size, alloca_pos);
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, alloca_pos);

                assert(idx - old_idx == BYTECODE_ALLOCA_SIZE);
                break;
            }
            case BYTECODE_RETURN:
                log(LOG_TRACE, "return\n");
                string_extend_f(&a_temp, &buf, "  %"PRIu64": return: (sizeof rtn_lang_type: %"PRIu64")\n", old_idx, bytecode_dump_read_uint64_t(&idx));
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                stack_offset = 0;

                assert(idx - old_idx == BYTECODE_RETURN_SIZE);
                break;
            case BYTECODE_GOTO:
                log(LOG_TRACE, "goto\n");
                string_extend_f(&a_temp, &buf, "  %"PRIu64": goto: %"PRIu64"\n", old_idx, bytecode_dump_read_uint64_t(&idx));
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_GOTO_SIZE);
                break;
            case BYTECODE_COND_GOTO:
                log(LOG_TRACE, "cond_goto\n");
                string_extend_f(&a_temp, &buf, "  %"PRIu64": cond_goto:\n", old_idx);

                string_extend_f(&a_temp, &buf, "    if_true: %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));

                string_extend_f(&a_temp, &buf, "    if_false: %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));

                bytecode_stack_size_add_aligned(&stack_offset, 1);

                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_COND_GOTO_SIZE);
                break;
            case BYTECODE_STORE_STACK:
                log(LOG_TRACE, "store_stack\n");
                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": store: \n", old_idx);

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    dest: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    src: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    sizeof copy: %"PRIu64" bytes\n", bytecode_dump_read_uint64_t(&idx));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_STORE_STACK_SIZE);
                break;
            case BYTECODE_STORE_STACK_DIR_ADDR:
                log(LOG_TRACE, "store_stack_dir_addr\n");
                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": store dir_addr: \n", old_idx);

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    dest: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    src (value): "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    sizeof copy: %"PRIu64" bytes\n", bytecode_dump_read_uint64_t(&idx));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_STORE_STACK_DIR_ADDR_SIZE);
                break;
            case BYTECODE_PUSH: {
                log(LOG_TRACE, "push\n");
                uint64_t alloca_size = bytecode_dump_read_uint64_t(&idx);
                uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&stack_offset, alloca_size);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": push: (store location: "FMT")\n", old_idx, bytecode_alloca_pos_print(alloca_pos));

                string_extend_f(&a_temp, &buf, "    sizeof item: %"PRIu64" bytes\n", alloca_size);

                string_extend_f(&a_temp, &buf, "    item: %"PRIu64"\n", bytecode_dump_read_uint64_t(&idx));

                assert(idx - old_idx == BYTECODE_PUSH_SIZE);
                break;
            }
            // TODO: make ir_zero_extend, etc. instead of ir_unsafe_cast
            case BYTECODE_ZERO_EXTEND: {
                log(LOG_TRACE, "zero_extend\n");
                // TODO: encode stack position in bytecode so that assertions can be added here to ensure that
                //   stack is correctly handled

                uint64_t alloca_size = bytecode_dump_read_uint64_t(&idx);
                bytecode_stack_size_add_aligned(&stack_offset, alloca_size);
                uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&stack_offset, alloca_size);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": zero extend: (store location: "FMT")\n", old_idx, bytecode_alloca_pos_print(alloca_pos));

                string_extend_f(&a_temp, &buf, "    sizeof(dest): %"PRIu64" \n", alloca_size);
                string_extend_f(&a_temp, &buf, "    sizeof(src): %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));
                string_extend_f(&a_temp, &buf, "    src_pos: %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));
                string_extend_f(&a_temp, &buf, "    alloca_pos: %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));

                bytecode_stack_size_add_aligned(&stack_offset, alloca_size);
                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_ZERO_EXTEND_SIZE);
                break;
            }


            // --- BINARY OPERATORS ---
            case BYTECODE_ADD_: {
                log(LOG_TRACE, "add\n");
                bytecode_dump_internal_binary(&buf, sv("add"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_SUB: {
                log(LOG_TRACE, "sub\n");
                bytecode_dump_internal_binary(&buf, sv("sub"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_GREATER_THAN: {
                log(LOG_TRACE, "gr_than\n");
                bytecode_dump_internal_binary(&buf, sv("greater_than"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_LESS_THAN: {
                log(LOG_TRACE, "less_than\n");
                bytecode_dump_internal_binary(&buf, sv("less_than"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_DOUBLE_EQUAL: {
                log(LOG_TRACE, "double_eq\n");
                bytecode_dump_internal_binary(&buf, sv("double_equal"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_NOT_EQUAL: {
                log(LOG_TRACE, "not_eq\n");
                bytecode_dump_internal_binary(&buf, sv("not_equal"), old_idx, &idx, &stack_offset);
                break;
            }



            case BYTECODE_CALL_DIRECT:
                log(LOG_TRACE, "call_dir\n");
                string_extend_f(&a_temp, &buf, "  %"PRIu64": call direct\n", old_idx);

                uint64_t start_pos = bytecode_dump_read_uint64_t(&idx);
                string_extend_f(&a_temp, &buf, "    jump to: %"PRIu64" \n", start_pos);

                uint64_t arg_bytes_count = bytecode_dump_read_uint64_t(&idx);
                string_extend_f(&a_temp, &buf, "    arg bytes: %"PRIu64" \n", arg_bytes_count);

                uint64_t rtn_alloc_pos = bytecode_dump_read_uint64_t(&idx);
                string_extend_f(&a_temp, &buf, "    rtn_alloc_pos: %"PRIu64" \n", rtn_alloc_pos);

                if (is_first_step) {
                    darr_append(&a_temp, mapping, bytecode_dump_mapping_new(
                        start_pos,
                        arg_bytes_count
                    ));
                }

                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);
                bytecode_stack_size_add_aligned(&stack_offset, arg_bytes_count);

                assert(idx - old_idx == BYTECODE_CALL_DIRECT_SIZE);
                break;
            case BYTECODE_NONE:
                log(LOG_TRACE, "noen\n");
                string_extend_f(&a_temp, &buf, "  %"PRIu64": warning: none\n", old_idx);

                bytecode_dump_read_uint64_t(&idx);

                bytecode_dump_read_and_extend_stack_offset(&buf, &idx, 0);

                assert(idx - old_idx == BYTECODE_NONE_SIZE);
                break;
            case BYTECODE_COUNT:
                unreachable("");
            default:
                log(LOG_DEBUG, FMT"\n", string_print(buf));
                unreachable("");
        }
    }

    if (!is_first_step) {
        log_internal_custom_dest(dest, log_level, file, line, 0, FMT"\n", string_print(buf));
    }
}

void bytecode_dump_internal(FILE* dest, const char* file, int line, LOG_LEVEL log_level, bool is_before_backpatching_complete, Bytecode bytecode) {
    bytecode_is_before_backpatching_complete = is_before_backpatching_complete;

    Bytecode_dump_mapping_darr mappings = {0};
    bytecode_dump_internal_2(dest, file, line, &mappings, log_level, bytecode, true);
    log(LOG_DEBUG, "%zu\n", mappings.info.count);
    darr_foreach(idx, Bytecode_dump_mapping, curr_mapping, mappings) {
        log(LOG_DEBUG, "%zu %zu\n", curr_mapping.fun_start, curr_mapping.arg_bytes_count);
    }
    qsort(mappings.buf, mappings.info.count, sizeof(mappings.buf[0]), bytecode_mapping_cmp);
    bytecode_dump_internal_2(dest, file, line, &mappings, log_level, bytecode, false);
}

