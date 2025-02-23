#include <tast_serialize.h>

Str_view serialize_tast_struct_def(Env* env, const Tast_struct_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "struct def must be in symbol table to be serialized");
    return serialize_lang_type(env, tast_struct_def_get_lang_type(def));
}

Str_view serialize_tast_raw_union_def(Env* env, const Tast_raw_union_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "raw_union def must be in symbol table to be serialized");
    return serialize_lang_type(env, tast_raw_union_def_get_lang_type(def));
}

Str_view serialize_tast_sum_def(Env* env, const Tast_sum_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "sum def must be in symbol table to be serialized");
    return serialize_lang_type(env, tast_sum_def_get_lang_type(def));
}

Str_view serialize_tast_def(Env* env, const Tast_def* def) {
    switch (def->type) {
        case TAST_STRUCT_DEF:
            return serialize_tast_struct_def(env, tast_struct_def_const_unwrap(def));
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_VARIABLE_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            return serialize_tast_raw_union_def(env, tast_raw_union_def_const_unwrap(def));
        case TAST_ENUM_DEF:
            unreachable("");
        case TAST_SUM_DEF:
            return serialize_tast_sum_def(env, tast_sum_def_const_unwrap(def));
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}
