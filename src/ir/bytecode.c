#include <bytecode.h>

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

static void bytecode_dump_internal_binary(String* buf, Strv bin_name, uint64_t old_idx, uint64_t* idx, uint64_t* stack_size) {
    uint64_t alloca_size = bytecode_dump_read_uint64_t(idx);

    uint64_t pos_rhs = bytecode_stack_size_add_aligned(stack_size, alloca_size);
    uint64_t pos_lhs = bytecode_stack_size_add_aligned(stack_size, alloca_size);
    uint64_t alloca_pos = bytecode_stack_size_sub_aligned(stack_size, alloca_size);

    string_extend_f(&a_temp, buf, "  %"PRIu64": "FMT": (store location: "FMT")\n", old_idx, strv_print(bin_name), bytecode_alloca_pos_print(alloca_pos));

    string_extend_f(&a_temp, buf, "    pos lhs: "FMT" \n", bytecode_alloca_pos_print(pos_lhs));
    string_extend_f(&a_temp, buf, "    pos rhs: "FMT" \n", bytecode_alloca_pos_print(pos_rhs));

    assert(*idx - old_idx == BYTECODE_ADD_SIZE);
}

void bytecode_dump_internal(const char* file, int line, LOG_LEVEL log_level, Bytecode bytecode) {
    String buf = {0};

    string_extend_f(&a_temp, &buf, "\n");

    size_t idx = 0;
    uint64_t stack_offset = 0;
    while (idx < bytecode.code.info.count) {
        size_t old_idx = idx;

        BYTECODE curr_opcode = darr_at(bytecode.code, idx);
        idx += 8;

        switch (curr_opcode) {
            case BYTECODE_COMMENT: {
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
                uint64_t alloca_size = bytecode_dump_read_uint64_t(&idx);
                uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&stack_offset, alloca_size);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": alloca: %"PRIu64" bytes (store location: "FMT")\n", old_idx, alloca_size, bytecode_alloca_pos_print(alloca_pos));

                assert(idx - old_idx == BYTECODE_ALLOCA_SIZE);
                break;
            }
            case BYTECODE_RETURN:
                string_extend_f(&a_temp, &buf, "  %"PRIu64": return: (sizeof rtn_lang_type: %"PRIu64")\n", old_idx, bytecode_dump_read_uint64_t(&idx));

                assert(idx - old_idx == BYTECODE_RETURN_SIZE);
                break;
            case BYTECODE_GOTO:
                string_extend_f(&a_temp, &buf, "  %"PRIu64": goto: %"PRIu64"\n", old_idx, bytecode_dump_read_uint64_t(&idx));

                assert(idx - old_idx == BYTECODE_GOTO_SIZE);
                break;
            case BYTECODE_STORE_STACK:
                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": store: \n", old_idx);

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    dest: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    src: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                string_extend_f(&a_temp, &buf, "    sizeof copy: %"PRIu64" bytes\n", bytecode_dump_read_uint64_t(&idx));

                log(LOG_DEBUG, "%zu %zu %zu\n", idx, old_idx, idx - old_idx);
                assert(idx - old_idx == BYTECODE_STORE_STACK_SIZE);
                break;
            case BYTECODE_PUSH: {
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
                // TODO: encode stack position in bytecode so that assertions can be added here to ensure that
                //   stack is correctly handled

                uint64_t alloca_size = bytecode_dump_read_uint64_t(&idx);
                bytecode_stack_size_add_aligned(&stack_offset, alloca_size);
                uint64_t alloca_pos = bytecode_stack_size_sub_aligned(&stack_offset, alloca_size);
                string_extend_f(&a_temp, &buf, "  %"PRIu64": zero extend: (store location: "FMT")\n", old_idx, bytecode_alloca_pos_print(alloca_pos));

                string_extend_f(&a_temp, &buf, "    sizeof(dest): %"PRIu64" \n", alloca_size);

                string_extend_f(&a_temp, &buf, "    sizeof(src): %"PRIu64" \n", bytecode_dump_read_uint64_t(&idx));

                assert(idx - old_idx == BYTECODE_ZERO_EXTEND_SIZE);
                break;
            }


            // --- BINARY OPERATORS ---
            case BYTECODE_ADD: {
                bytecode_dump_internal_binary(&buf, sv("add"), old_idx, &idx, &stack_offset);
                break;
            }
            case BYTECODE_SUB: {
                bytecode_dump_internal_binary(&buf, sv("sub"), old_idx, &idx, &stack_offset);
                break;
            }



            case BYTECODE_CALL_DIRECT:
                string_extend_f(&a_temp, &buf, "  %"PRIu64": call direct\n", old_idx);

                string_extend_f(&a_temp, &buf, "    jump to: "FMT" \n", bytecode_alloca_pos_print(bytecode_dump_read_uint64_t(&idx)));

                assert(idx - old_idx == BYTECODE_RETURN_SIZE);
                break;
            case BYTECODE_NONE:
                string_extend_f(&a_temp, &buf, "  %"PRIu64": warning: none\n", old_idx);

                bytecode_dump_read_uint64_t(&idx);

                assert(idx - old_idx == BYTECODE_NONE_SIZE);
                break;
            case BYTECODE_COUNT:
                unreachable("");
            default:
                log(LOG_DEBUG, FMT"\n", string_print(buf));
                unreachable("");
        }
    }

    log_internal(log_level, file, line, 0, FMT"\n", string_print(buf));
}

