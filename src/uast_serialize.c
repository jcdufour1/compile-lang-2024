#include <uast_serialize.h>

Str_view serialize_uast_struct_def(Env* env, const Uast_struct_def* def) {
    Uast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "struct def must be in symbol table to be serialized");
    return serialize_lang_type(env, uast_struct_def_get_lang_type(def));
}

Str_view serialize_uast_raw_union_def(Env* env, const Uast_raw_union_def* def) {
    Uast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "raw_union def must be in symbol table to be serialized");
    return serialize_lang_type(env, uast_raw_union_def_get_lang_type(def));
}

Str_view serialize_uast_sum_def(Env* env, const Uast_sum_def* def) {
    Uast_def* dummy = NULL;
    assert(symbol_lookup(&dummy, env, def->base.name) && "sum def must be in symbol table to be serialized");
    return serialize_lang_type(env, uast_sum_def_get_lang_type(def));
}

Str_view serialize_uast_def(Env* env, const Uast_def* def) {
    switch (def->type) {
        case UAST_STRUCT_DEF:
            return serialize_uast_struct_def(env, uast_struct_def_const_unwrap(def));
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            return serialize_uast_raw_union_def(env, uast_raw_union_def_const_unwrap(def));
        case UAST_ENUM_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            return serialize_uast_sum_def(env, uast_sum_def_const_unwrap(def));
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}
