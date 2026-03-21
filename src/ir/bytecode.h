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
    BYTECODE_STORE_STACK,
    BYTECODE_GOTO,
    BYTECODE_RETURN,
    BYTECODE_PUSH,
    BYTECODE_ZERO_EXTEND,
    BYTECODE_CALL_DIRECT,
    BYTECODE_COMMENT,

    // binary operators
    BYTECODE_ADD,
    BYTECODE_SUB,

    BYTECODE_COUNT,
} BYTECODE;
// TODO: figure out if this static assert is actually needed
static_assert(
    BYTECODE_COUNT < UINT8_MAX,
    "overflow will occur because only one byte is used to store bytecode opcode in bytecode representation"
);

#define BYTECODE_BINARY_SIZE 16

// BYTECODE_COMMENT_SIZE is not defined because comment size is variable
static_assert(BYTECODE_COUNT == 11, "exhausive handling of bytecode opcode types");
#define BYTECODE_ALLOCA_SIZE 16
#define BYTECODE_STORE_STACK_SIZE 32
#define BYTECODE_GOTO_SIZE 16
#define BYTECODE_RETURN_SIZE 16
#define BYTECODE_PUSH_SIZE 24
#define BYTECODE_ZERO_EXTEND_SIZE 24
#define BYTECODE_ADD_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_SUB_SIZE BYTECODE_BINARY_SIZE
#define BYTECODE_CALL_DIRECT_SIZE 16
#define BYTECODE_NONE_SIZE 16



// each value here must increment by 8
#define BYTECODE_CALL_STACK_RTN_VALUE 8
#define BYTECODE_CALL_STACK_RTN_ADDR 16
#define BYTECODE_CALL_STACK_BASE_PTR 24
#define BYTECODE_CALL_STACK_OFFSET 32
#define BYTECODE_COUNT_RTN_ITEMS 4
static_assert(__LINE__ == 69, "update BYTECODE_COUNT_RTN_ITEMS if nessessary");

Strv bytecode_alloca_pos_print_internal(uint64_t raw_pos);

#define bytecode_alloca_pos_print(raw_pos) strv_print(bytecode_alloca_pos_print_internal(raw_pos))

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

static inline void bytecode_stack_write_internal(uint8_t* stack, size_t stack_len, uint64_t stack_offset, uint64_t stack_base_ptr, uint64_t sizeof_value, uint64_t value) {
    uint64_t stack_index = stack_base_ptr - stack_offset;
    unwrap(stack_index < stack_len && "out of bounds");

    // TODO: this and similar memcpys will only work on little endian platforms
    memcpy(&stack[stack_index], &value, sizeof_value);
}

#define bytecode_stack_write(stack, stack_offset, stack_base_ptr, sizeof_value, value) \
    bytecode_stack_write_internal(stack, array_count(stack), stack_offset, stack_base_ptr, sizeof_value, value)

// return the value popped
#define bytecode_stack_push(stack, stack_offset, stack_base_ptr, value, value_size) \
    (bytecode_stack_size_sub_aligned(stack_offset, value_size), *array_at_ref(stack, stack_base_ptr - *stack_offset) = value)

#define bytecode_stack_at(stack, stack_offset, stack_base_ptr) (array_at(stack, (stack_base_ptr) - (stack_offset)))

#endif // BYTECODE_H
