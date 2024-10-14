#include "nodes.h"
#include "symbol_table.h"
#include "parameters.h"
#include "arena.h"

Symbol_table symbol_table = {0};

Parameters params = {0};

Arena a_main = {0};
Arena print_arena = {0};

Node* root_of_tree = NULL;

void* arena_buffers[100000] = {0};
size_t arena_buffers_count = 0;

uint8_t zero_block[100000000] = {0};

size_t error_count = 0;
size_t warning_count = 0;

Pos prev_pos = {0};
