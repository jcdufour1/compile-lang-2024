#ifndef SYMBOL_LOG_H
#define SYMBOL_LOG_H

#include <env.h>
#include <symbol_table.h>

// ----------------
// ---- init ------
// ----------------

// TODO: fix these macros
//#define init_log(log_level, scope_id) init_log_internal(log_level, __FILE__, __LINE__,  0, scope_id);

//#define init_log_level(log_level, scope_id) init_level_log_internal(log_level, __FILE__, __LINE__, vec_at(&env.symbol_tables, scope_id).init_table, 0);

static inline void init_level_log_internal(LOG_LEVEL log_level, const char* file, int line, Scope_id scope_id, Init_table level, Indent indent) {
    (void) scope_id;
    String buf = {0};
    init_extend_table_internal(&buf, level, indent);
    if (buf.info.count > 0) {
        log_internal(log_level, file, line, indent + INDENT_WIDTH, "\n"FMT"\n", string_print(buf));
    }
}

static inline void init_log_internal(LOG_LEVEL log_level, const char* file, int line, const Indent indent, Init_table_vec* init_tables) {
    for (size_t idx = 0; idx < init_tables->info.count; idx++) {
        Init_table curr = vec_at(*init_tables, idx);
        init_level_log_internal(log_level, file, line, idx, curr, indent + INDENT_WIDTH);
    }
}

#endif // SYMBOL_LOG_H
