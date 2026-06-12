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

Symbol_collection symbol_tables;

Name_darr scope_to_name;

Scope_id_darr scope_id_to_parent;

