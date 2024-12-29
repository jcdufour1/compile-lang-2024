
#include <llvm.h>
#include <node.h>

static inline void log_env_internal(const char* file, int line, LOG_LEVEL log_level, const Env* env) {
    String buf = {0};

    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < env->ancesters.info.count; idx++) {
        string_extend_cstr(&print_arena, &buf, "level ");
        string_extend_size_t(&print_arena, &buf, idx);
        string_extend_cstr(&print_arena, &buf, ":\n");
        Symbol_collection curr = *vec_at(&env->ancesters, idx);
        
        string_extend_cstr_indent(&print_arena, &buf, "alloca_table\n", (1 + idx)*INDENT_WIDTH);
        alloca_extend_table_internal(&buf, curr.alloca_table, 2*(1 + idx)*INDENT_WIDTH);

        string_extend_cstr_indent(&print_arena, &buf, "symbol_table\n", (1 + idx)*INDENT_WIDTH);
        symbol_extend_table_internal(&buf, curr.symbol_table, 2*(1 + idx)*INDENT_WIDTH);
    }

    log_internal(log_level, file, line, 0, STRING_FMT"\n", string_print(buf));
}

#define log_env(log_level, env) \
    log_env_internal(__FILE__, __LINE__, log_level, env)
