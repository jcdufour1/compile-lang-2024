#ifndef BYTECODE_H
#define BYTECODE_H

#include <uint8_t_darr.h>
#include <util.h>

typedef struct {
    Uint8_t_darr code;
    size_t start_pos;
} Bytecode;

extern Bytecode bytecode;

typedef union {
    void* ptr;
    size_t size_t_val;
    uint64_t u64_val;
} Bytecode_align_union;

#define BYTECODE_ALIGN sizeof(Bytecode_align_union)
static_assert(sizeof(BYTECODE_ALIGN) == 8, "not implemented"); // TODO

typedef enum {
    BYTECODE_NONE,
    BYTECODE_ALLOCA,
    BYTECODE_STORE_STACK, // TODO: rename to BYTECODE_STORE
    BYTECODE_STORE_STACK_DIR_ADDR, // TODO: rename to BYTECODE_STORE_DIR_ADDR
    BYTECODE_GOTO,
    BYTECODE_COND_GOTO,
    BYTECODE_RETURN,
    BYTECODE_PUSH,
    BYTECODE_ZERO_EXTEND,
    BYTECODE_CALL_DIRECT_,
    BYTECODE_COMMENT,

    // binary operators
    BYTECODE_ADD_,
    BYTECODE_SUB,
    BYTECODE_GREATER_THAN,
    BYTECODE_DOUBLE_EQUAL,
    BYTECODE_NOT_EQUAL,

    BYTECODE_COUNT,
} BYTECODE;
// TODO: figure out if this static assert is actually needed
static_assert(
    BYTECODE_COUNT < UINT8_MAX,
    "overflow will occur because only one byte is used to store bytecode opcode in bytecode representation"
);

#define BYTECODE_BINARY_SIZE 32

// BYTECODE_COMMENT_SIZE is not defined because comment size is variable
static_assert(BYTECODE_COUNT == 16, "exhausive handling of bytecode opcode types");
#define BYTECODE_ALLOCA_SIZE 16
#define BYTECODE_STORE_STACK_SIZE 32
#define BYTECODE_STORE_STACK_DIR_ADDR_SIZE 32
#define BYTECODE_GOTO_SIZE 16
#define BYTECODE_COND_GOTO_SIZE 24
#define BYTECODE_RETURN_SIZE 16
#define BYTECODE_PUSH_SIZE 24
#define BYTECODE_ZERO_EXTEND_SIZE 24
#define BYTECODE_ADD_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_SUB_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_GREATER_THAN_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_DOUBLE_EQUAL_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_NOT_EQUAL_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_CALL_DIRECT_SIZE 32
#define BYTECODE_NONE_SIZE 16



typedef enum {
    BYTECODE_CALL_STACK_TYPE_RTN_VALUE,
    BYTECODE_CALL_STACK_TYPE_RTN_ADDR,
    BYTECODE_CALL_STACK_TYPE_BASE_PTR,
    BYTECODE_CALL_STACK_TYPE_OFFSET,
    BYTECODE_CALL_STACK_TYPE_ARG_BYTES,

    BYTECODE_COUNT_RTN_ITEMS
} BYTECODE_CALL_STACK_TYPE;

static inline uint64_t bytecode_call_stack_get_offset(BYTECODE_CALL_STACK_TYPE offset_type, uint64_t arg_bytes_count) {
    uint64_t base_bytes_call_stack_type = 8*(((uint64_t)offset_type) + 1);
    assert(arg_bytes_count == get_next_multiple(arg_bytes_count, 8) && "not implemented");
    return base_bytes_call_stack_type + arg_bytes_count;
}

Strv bytecode_alloca_pos_print_internal(uint64_t raw_pos);

#define bytecode_alloca_pos_print(raw_pos) strv_print(bytecode_alloca_pos_print_internal(raw_pos))

void bytecode_stack_dump_internal(LOG_LEVEL log_level, const char* file, int line, uint8_t* stack, uint64_t stack_offset, uint64_t base_ptr);

#define bytecode_stack_dump(log_level, stack, stack_offset, base_ptr) bytecode_stack_dump_internal(log_level, file, line, stack, stack_offset, base_ptr)

void bytecode_align(void);

void bytecode_append_align(BYTECODE opcode);

size_t bytecode_read_size_t(size_t index);

uint64_t bytecode_read_uint64_t(uint64_t index);

void bytecode_dump_internal(const char* file, int line, LOG_LEVEL log_level, Bytecode bytecode);

#define bytecode_dump(log_level, bytecode) \
    bytecode_dump_internal(__FILE__, __LINE__, log_level, bytecode)

// returns the position (offset) of the new stack space
static uint64_t bytecode_stack_size_add_aligned(uint64_t* stack_offset, uint64_t alloc_size) {
    log(LOG_DEBUG, "%zu\n", *stack_offset);
    assert(*stack_offset < INTERPRET_STACK_SIZE);
    assert(*stack_offset % 8 == 0); // TODO: remove this assertion if allocs smaller than 8 are actually done (but also remove // ALIGN statement below)
                                  
    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);
    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);
    assert(*stack_offset <= INTERPRET_STACK_SIZE);
    uint64_t alloc_pos = *stack_offset;
    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);
    assert(*stack_offset >= alloc_size);
    *stack_offset -= alloc_size;
    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);

    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);
    *stack_offset = get_prev_multiple(*stack_offset, 8); // ALIGN
    //log(LOG_DEBUG, "%zu\n", INTERPRET_STACK_SIZE - *stack_size);

    assert(*stack_offset <= INTERPRET_STACK_SIZE);
    return alloc_pos;
}

// returns the position of the new stack space
static uint64_t bytecode_stack_size_sub_aligned(uint64_t* stack_offset, uint64_t alloc_size) {
    assert(*stack_offset % 8 == 0); // TODO: remove this assertion if allocs smaller than 8 are actually done
                                  
    *stack_offset += alloc_size;
    *stack_offset = get_next_multiple(*stack_offset, 8/*TODO: make smaller if alloc size is tiny*/);
    uint64_t alloc_pos = *stack_offset;
    log(LOG_DEBUG, FMT"\n", bytecode_alloca_pos_print(alloc_pos));
    //breakpoint();

    assert(*stack_offset % 8 == 0); // TODO: remove this assertion if allocs smaller than 8 are actually done
    return alloc_pos;
}

// return the value popped
// TODO: array_at macro is not currently being used becuause of side effect issues (but maybe should be)

static inline uint64_t bytecode_stack_pop_internal(uint8_t* stack, uint64_t stack_len, uint64_t* stack_offset, uint64_t stack_base_ptr, uint64_t sizeof_value) {
    uint64_t stack_index = stack_base_ptr - *stack_offset;
    unwrap(stack_index < stack_len && "out of bounds");

    uint64_t value = 0;
    // TODO: this and similar memcpys will only work on little endian platforms
    memcpy(&value, &stack[stack_index], sizeof_value);

    bytecode_stack_size_add_aligned(stack_offset, sizeof_value);

    return value;
}

//#define bytecode_stack_pop(stack, stack_offset, stack_base_ptr, value_size) \
    //(((uint64_t* /* TODO */)(stack))[(stack_base_ptr) - bytecode_stack_size_add_aligned(stack_offset, value_size)])

#define bytecode_stack_pop(stack, stack_offset, stack_base_ptr, value_size) \
    bytecode_stack_pop_internal(stack, array_count(stack), stack_offset, stack_base_ptr, value_size)

static inline void bytecode_stack_write_internal(
    const char* file,
    int line,
    uint8_t* stack,
    size_t stack_len,
    uint64_t stack_offset, // TODO: rename to pos?
    uint64_t stack_base_ptr,
    uint64_t sizeof_value,
    uint64_t value
) {
    uint64_t stack_index = stack_base_ptr - stack_offset;
    log_internal(LOG_DEBUG, file, line, 0, "stack_index = %zu; stack_base_ptr = %zu, stack_offset = %zu\n", stack_index, stack_base_ptr, stack_offset);
    log_internal(LOG_DEBUG, file, line, 0, "sizeof_value = %zu\n", sizeof_value);
    log_internal(LOG_DEBUG, file, line, 0, "value = %zu\n", value);
    unwrap(stack_index < stack_len && "out of bounds");

    // TODO: this and similar memcpys will only work on little endian platforms
    memcpy(&stack[stack_index], &value, sizeof_value);
    bytecode_stack_dump(LOG_DEBUG, stack, stack_offset, stack_base_ptr);
}

#define bytecode_stack_write(stack, stack_offset, stack_base_ptr, sizeof_value, value) \
    bytecode_stack_write_internal(__FILE__, __LINE__, stack, array_count(stack), stack_offset, stack_base_ptr, sizeof_value, value)

static inline uint64_t bytecode_stack_read_internal(
    const char* file,
    int line,
    uint8_t* stack,
    size_t stack_len,
    uint64_t stack_offset, // TODO: rename to pos?
    uint64_t stack_base_ptr,
    uint64_t sizeof_value
) {
    uint64_t stack_index = stack_base_ptr - stack_offset;
    log_internal(LOG_DEBUG, file, line, 0, "stack_index = %zu; stack_base_ptr = %zu, stack_offset = %zu\n", stack_index, stack_base_ptr, stack_offset);
    log_internal(LOG_DEBUG, file, line, 0, "sizeof_value = %zu\n", sizeof_value);
    unwrap(stack_index < stack_len && "out of bounds");

    // TODO: this and similar memcpys will only work on little endian platforms
    uint64_t result = 0;
    assert(sizeof_value <= 8);
    memcpy(&result, &stack[stack_index], sizeof_value);
    bytecode_stack_dump(LOG_DEBUG, stack, stack_offset, stack_base_ptr);
    return result;
}

#define bytecode_stack_read(stack, stack_offset, stack_base_ptr, sizeof_value) \
    bytecode_stack_read_internal(__FILE__, __LINE__, stack, array_count(stack), stack_offset, stack_base_ptr, sizeof_value)

// return the value popped
#define bytecode_stack_push(stack, stack_offset, stack_base_ptr, value, value_size) \
    (bytecode_stack_size_sub_aligned(stack_offset, value_size), *array_at_ref(stack, stack_base_ptr - *stack_offset) = value)

#define bytecode_stack_at(stack, stack_offset, stack_base_ptr) (array_at(stack, (stack_base_ptr) - (stack_offset)))

typedef struct {
    uint64_t stack_offset;
    size_t bytecode_count;
} Bytecode_state;

static Bytecode_state bytecode_state_save(uint64_t stack_offset, size_t bytecode_count) {
    return (Bytecode_state) {.stack_offset = stack_offset, .bytecode_count = bytecode_count};
}

static void bytecode_state_restore(uint64_t* stack_offset, size_t* bytecode_count, Bytecode_state state) {
    *stack_offset = state.stack_offset;
    *bytecode_count = state.bytecode_count;
}

#endif // BYTECODE_H
