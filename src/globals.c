#include "nodes.h"
#include "symbol_table.h"
#include "parameters.h"
#include "arena.h"

Nodes nodes = {0};

Symbol_table symbol_table = {0};

Parameters params = {0};

Arena arena = {0};

void* arena_buffers[100000] = {0};
size_t arena_buffers_count = 0;
