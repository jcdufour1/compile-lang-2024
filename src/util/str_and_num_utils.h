#ifndef STR_AND_NUM_UTILS_H
#define STR_AND_NUM_UTILS_H

#include <util.h>
#include <name.h>
#include <strv.h>

#define util_literal_strv_new() \
    util_literal_strv_new_internal(__FILE__, __LINE__, sv(""))

Name util_literal_name_new_prefix_scope_internal(
    const char* file,
    int line,
    Strv debug_prefix,
    Scope_id scope_id
);

#define util_literal_name_new_prefix(debug_prefix) \
    util_literal_name_new_prefix_scope_internal(__FILE__, __LINE__, debug_prefix, SCOPE_TOP_LEVEL)

#define util_literal_ir_name_new_prefix(debug_prefix) \
    util_literal_ir_name_new_prefix_scope_internal(__FILE__, __LINE__, debug_prefix, SCOPE_TOP_LEVEL)

#define util_literal_name_new_prefix_scope(debug_prefix, scope_id) \
    util_literal_name_new_prefix_scope_internal(__FILE__, __LINE__, debug_prefix, scope_id)

#define util_literal_name_new() \
    util_literal_name_new_prefix_scope_internal(__FILE__, __LINE__, sv(""), SCOPE_TOP_LEVEL)

static inline Name util_literal_name_new_poison(void) {
    return name_new(MOD_PATH_BUILTIN, sv("poison"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

static inline Ir_name util_literal_ir_name_new_poison(void) {
    return ir_name_new(MOD_PATH_BUILTIN, sv("poison"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
}

#define util_literal_ir_name_new(ir_name_tables) \
    util_literal_ir_name_new_prefix_scope_internal(__FILE__, __LINE__, sv(""), SCOPE_TOP_LEVEL)

bool try_strv_hex_after_0x_to_int64_t(int64_t* result, const Pos pos, Strv strv);

static inline bool ishex(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

size_t get_count_excape_seq(Strv strv);

// \n excapes are actually stored as is in tokens and irs, but should be printed as \0a
void string_extend_strv_eval_escapes(Arena* arena, String* string, Strv strv);

static bool isdigit_no_underscore(char prev, char curr);

bool try_strv_octal_after_0o_to_int64_t(int64_t* result, const Pos pos, Strv strv);

bool try_strv_hex_after_0x_to_int64_t(int64_t* result, const Pos pos, Strv strv);

bool try_strv_bin_after_0b_to_int64_t(int64_t* result, const Pos pos, Strv strv);

bool try_strv_to_int64_t(int64_t* result, const Pos pos, Strv strv);

bool try_strv_to_double(double* result, const Pos pos, Strv strv);

bool try_strv_to_char(char* result, const Pos pos, Strv strv);

bool try_strv_to_size_t(size_t* result, Strv strv);

bool try_strv_consume_size_t(size_t* result, Strv* strv, bool ignore_underscore);

int64_t strv_to_int64_t(const Pos pos, Strv strv);

Strv util_literal_strv_new_internal(const char* file, int line, Strv debug_prefix);

Name util_literal_name_new_prefix_scope_internal(const char* file, int line, Strv debug_prefix, Scope_id scope_id);

Ir_name util_literal_ir_name_new_prefix_scope_internal(const char* file, int line, Strv debug_prefix, Scope_id scope_id);

Strv serialize_double(double num);

#endif // STR_AND_NUM_UTILS_H
