#ifndef SYMBOL_LOG_H
#define SYMBOL_LOG_H

#include <env.h>
#include <symbol_table.h>

// ----------------
// ---- usymbol ---
// ----------------

#define usymbol_log(log_level, scope_id) usymbol_log_internal(log_level, __FILE__, __LINE__,  0, scope_id);

#define usymbol_log_level(dest, log_level, scope_id) usymbol_level_log_internal(dest, log_level, __FILE__, __LINE__, vec_at(env.symbol_tables, scope_id).usymbol_table, 0);

static inline void usymbol_level_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Usymbol_table level, Indent indent) {
    String buf = {0};
    usymbol_extend_table_internal(&buf, level, indent);
    log_internal_ex(dest, log_level, file, line, indent + INDENT_WIDTH, "\n"FMT"\n", string_print(buf));
}

static inline void usymbol_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Indent indent, Scope_id scope_id) {
    log_internal(log_level, file, line, 0, "----start usymbol table----\n");
    Scope_id curr_scope = scope_id;
    size_t idx = 0;
    while (true) {
        Usymbol_table curr = vec_at(env.symbol_tables, curr_scope).usymbol_table;
        log_internal(log_level, file, line, 0, "level: %zu\n", idx);
        usymbol_level_log_internal(dest, log_level, file, line, curr, indent + INDENT_WIDTH);
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }
    log_internal(log_level, file, line, 0, "----end usymbol table------\n");
}

// ----------------
// ---- symbol ----
// ----------------

#define symbol_log(log_level, scope_id) symbol_log_internal(log_level, __FILE__, __LINE__,  0, scope_id);

#define symbol_log_level(dest, log_level, scope_id) symbol_level_log_internal(dest, log_level, __FILE__, __LINE__, vec_at(env.symbol_tables, scope_id).symbol_table, 0);

static inline void symbol_level_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Symbol_table level, Indent indent) {
    String buf = {0};
    symbol_extend_table_internal(&buf, level, indent);
    log_internal_ex(dest, log_level, file, line, indent + INDENT_WIDTH, "\n"FMT"\n", string_print(buf));
}

static inline void symbol_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, const Indent indent, Scope_id scope_id) {
    log_internal(log_level, file, line, 0, "----start symbol table----\n");
    Scope_id curr_scope = scope_id;
    size_t idx = 0;
    while (true) {
        Symbol_table curr = vec_at(env.symbol_tables, curr_scope).symbol_table;
        log_internal(log_level, file, line, 0, "level: %zu\n", idx);
        symbol_level_log_internal(dest, log_level, file, line, curr, indent);
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }
    log_internal(log_level, file, line, 0, "----end symbol table------\n");
}

// ----------------
// ---- ir ------
// ----------------

#define ir_log(log_level, scope_id) ir_log_internal(log_level, __FILE__, __LINE__,  0, scope_id);

#define ir_log_level(dest, log_level, scope_id) ir_level_log_internal(dest, log_level, __FILE__, __LINE__, vec_at(env.symbol_tables, scope_id).alloca_table, 0);

static inline void ir_level_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Ir_table level, Indent indent) {
    String buf = {0};
    alloca_extend_table_internal(&buf, level, indent);
    log_internal_ex(dest, log_level, file, line, indent + INDENT_WIDTH, "\n"FMT"\n", string_print(buf));
}

static inline void ir_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, const Indent indent, Scope_id scope_id) {
    log_internal(log_level, file, line, 0, "----start ir table----\n");
    Scope_id curr_scope = scope_id;
    size_t idx = 0;
    while (true) {
        Ir_table curr = vec_at(env.symbol_tables, curr_scope).alloca_table;
        log_internal_ex(dest, log_level, file, line, 0, "level: %zu\n", idx);
        ir_level_log_internal(dest, log_level, file, line, curr, indent);
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }
    log_internal_ex(dest, log_level, file, line, 0, "----end ir table------\n");
}

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
