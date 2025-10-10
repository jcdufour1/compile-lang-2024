#ifndef UAST_SERIALIZE_H
#define UAST_SERIALIZE_H

#include <uast_utils.h>

static inline Strv serialize_uast_struct_def(const Uast_struct_def* def) {
    Uast_def* dummy = NULL;
    assert(usymbol_lookup(&dummy,  def->base.name) && "struct def must be in symbol table to be serialized");
    todo();
    //return serialize_ulang_type(ustruct_def_base_get_lang_type(def->base));
}

static inline Strv serialize_uast_raw_union_def(const Uast_raw_union_def* def) {
    Uast_def* dummy = NULL;
    assert(usymbol_lookup(&dummy,  def->base.name) && "raw_union def must be in symbol table to be serialized");
    todo();
    //return serialize_ulang_type(ustruct_def_base_get_lang_type(def->base));
}

static inline Strv serialize_uast_enum_def(const Uast_enum_def* def) {
    Uast_def* dummy = NULL;
    assert(usymbol_lookup(&dummy,  def->base.name) && "enum def must be in symbol table to be serialized");
    todo();
    //return serialize_ulang_type(ustruct_def_base_get_lang_type(def->base));
}

static inline Strv serialize_uast_def(const Uast_def* def) {
    switch (def->type) {
        case UAST_POISON_DEF:
            todo();
        case UAST_MOD_ALIAS:
            todo();
        case UAST_IMPORT_PATH:
            todo();
        case UAST_STRUCT_DEF:
            return serialize_uast_struct_def(uast_struct_def_const_unwrap(def));
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            return serialize_uast_raw_union_def(uast_raw_union_def_const_unwrap(def));
        case UAST_ENUM_DEF:
            return serialize_uast_enum_def(uast_enum_def_const_unwrap(def));
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_LANG_DEF:
            todo();
        case UAST_VOID_DEF:
            todo();
        case UAST_LABEL:
            todo();
        case UAST_BUILTIN_DEF:
            todo();
    }
    unreachable("");
}

#endif // UAST_SERIALIZE_H
