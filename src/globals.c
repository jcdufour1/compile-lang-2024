#include "symbol_table.h"
#include "parameters.h"
#include "arena.h"
#include <util.h>

Parameters params = {0};

Arena a_leak = {0};
Arena a_main = {0};
Arena a_pass = {0};
Arena a_temp = {0};

Tast* root_of_tree = NULL;

Pos prev_pos = {0};

Env env = {0};

// TODO: make macro for convenience?
char PATH_SEPARATOR = '/';
