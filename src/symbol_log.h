#ifndef SYMBOL_LOG_H
#define SYMBOL_LOG_H

#include <env.h>
#include <symbol_table.h>

// ----------------
// ---- usymbol ---
// ----------------

#define usymbol_log(log_level, env) usymbol_log_internal(log_level, __FILE__, __LINE__,  0);

#define usymbol_level_log(log_level, level) usymbol_level_log_internal(log_level, __FILE__, __LINE__, level, 0);

static inline void usymbol_level_log_internal(LOG_LEVEL log_level, const char* file, int line, Usymbol_table level, int recursion_depth) {
    String buf = {0};
    usymbol_extend_table_internal(&buf, level, recursion_depth);
    log_internal(log_level, file, line, recursion_depth + INDENT_WIDTH, STR_VIEW_FMT"\n", string_print(buf));
}

static inline void usymbol_log_internal(LOG_LEVEL log_level, const char* file, int line, const int recursion_depth) {
    log_internal(log_level, file, line, 0, "----start usymbol table----\n");
    for (size_t idx = 0; idx < env.ancesters.info.count; idx++) {
        Usymbol_table curr = vec_at(&env.ancesters, idx)->usymbol_table;
        log_internal(log_level, file, line, 0, "level: %zu\n", idx);
        usymbol_level_log_internal(log_level, file, line, curr, recursion_depth);
    }
    log_internal(log_level, file, line, 0, "----end usymbol table------\n");
}

// ----------------
// ---- symbol ----
// ----------------

#define symbol_log(log_level, env) symbol_log_internal(log_level, __FILE__, __LINE__,  0);

#define symbol_level_log(log_level, level) symbol_level_log_internal(log_level, __FILE__, __LINE__, level, 0);

static inline void symbol_level_log_internal(LOG_LEVEL log_level, const char* file, int line, Symbol_table level, int recursion_depth) {
    String buf = {0};
    symbol_extend_table_internal(&buf, level, recursion_depth);
    log_internal(log_level, file, line, recursion_depth + INDENT_WIDTH, STR_VIEW_FMT"\n", string_print(buf));
}

static inline void symbol_log_internal(LOG_LEVEL log_level, const char* file, int line, const int recursion_depth) {
    log_internal(log_level, file, line, 0, "----start symbol table----\n");
    for (size_t idx = 0; idx < env.ancesters.info.count; idx++) {
        Symbol_table curr = vec_at(&env.ancesters, idx)->symbol_table;
        log_internal(log_level, file, line, 0, "level: %zu\n", idx);
        symbol_level_log_internal(log_level, file, line, curr, recursion_depth);
    }
    log_internal(log_level, file, line, 0, "----end symbol table------\n");
}

#endif // SYMBOL_LOG_H
