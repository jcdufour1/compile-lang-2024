#include "symbol_table.h"
#include "parameters.h"
#include "arena.h"
#include <util.h>

Parameters params = {0};

Arena a_main = {0};
Arena print_arena = {0};

Tast* root_of_tree = NULL;

size_t error_count = 0;
size_t warning_count = 0;

Pos prev_pos = {0};

Env env = {0};

char PATH_SEPARATOR = '/';
