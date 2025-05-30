
#include <ir.h>
#include <tast.h>

static inline void log_env_internal(const char* file, int line, LOG_LEVEL log_level) {
    (void) file;
    (void) line;
    (void) log_level;
    String buf = {0};

    string_extend_cstr(&a_print, &buf, "\n");

    todo();
    //for (size_t idx = 0; idx < env.ancesters.info.count; idx++) {
    //    string_extend_cstr(&a_print, &buf, "level ");
    //    string_extend_size_t(&a_print, &buf, idx);
    //    string_extend_cstr(&a_print, &buf, ":\n");
    //    Symbol_collection curr = *vec_at(&env.ancesters, idx);
    //    
    //    string_extend_cstr_indent(&a_print, &buf, "alloca_table\n", (1 + idx)*INDENT_WIDTH);
    //    alloca_extend_table_internal(&buf, curr.alloca_table, 2*(1 + idx)*INDENT_WIDTH);

    //    string_extend_cstr_indent(&a_print, &buf, "symbol_table\n", (1 + idx)*INDENT_WIDTH);
    //    symbol_extend_table_internal(&buf, curr.symbol_table, 2*(1 + idx)*INDENT_WIDTH);
    //}

    //log_internal(log_level, file, line, 0, STRING_FMT"\n", string_print(buf));
}

#define log_env(log_level, env) \
    log_env_internal(__FILE__, __LINE__, log_level, env)
