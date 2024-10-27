#include "nodes.h"
#include "symbol_table.h"
#include "parameters.h"
#include "arena.h"

Parameters params = {0};

Arena a_main = {0};
Arena print_arena = {0};

Node* root_of_tree = NULL;

size_t error_count = 0;
size_t warning_count = 0;

Pos prev_pos = {0};
