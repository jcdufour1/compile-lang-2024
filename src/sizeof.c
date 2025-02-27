#include <sizeof.h>
#include <lang_type.h>
#include <lang_type_after.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <tast.h>
#include <llvm.h>

uint64_t sizeof_primitive(Lang_type_primitive primitive) {
    // TODO: platform specific pointer size, etc.
    if (lang_type_primitive_get_pointer_depth(primitive) > 0) {
        return 8;
    }

    switch (primitive.type) {
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(primitive).bit_width/8;
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(primitive).bit_width/8;
        case LANG_TYPE_CHAR:
            return 1;
        case LANG_TYPE_STRING:
            unreachable("");
        case LANG_TYPE_ANY:
            unreachable("");
    }
    unreachable("");
}

uint64_t sizeof_lang_type(Env* env, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_ENUM:
            unreachable("");
        case LANG_TYPE_PRIMITIVE:
            return sizeof_primitive(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            return sizeof_def(env, def);
        }
        case LANG_TYPE_SUM: {
            Tast_def* def = NULL;
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            return sizeof_def(env, def);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            return sizeof_def(env, def);
        }
        case LANG_TYPE_VOID:
            return 0;
        default:
            unreachable(LANG_TYPE_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
    }
}

uint64_t sizeof_expr(Env* env, const Tast_expr* expr) {
    (void) env;
    switch (expr->type) {
        case TAST_LITERAL:
            return sizeof_lang_type(env, tast_literal_get_lang_type(tast_literal_const_unwrap(expr)));
        default:
            unreachable("");
    }
}

uint64_t sizeof_def(Env* env, const Tast_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(env, tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return sizeof_struct_def_base(env, &tast_struct_def_const_unwrap(def)->base);
        case TAST_SUM_DEF:
            return sizeof_struct_def_base(env, &tast_sum_def_const_unwrap(def)->base);
        case TAST_RAW_UNION_DEF:
            return sizeof_struct_def_base(env, &tast_raw_union_def_const_unwrap(def)->base);
        default:
            unreachable("");
    }
}

uint64_t sizeof_stmt(Env* env, const Tast_stmt* stmt) {
    (void) env;
    switch (stmt->type) {
        case TAST_EXPR:
            return sizeof_expr(env, tast_expr_const_unwrap(stmt));
        case TAST_DEF:
            return sizeof_def(env, tast_def_const_unwrap(stmt));
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(Env* env, const Tast_struct_literal* struct_literal) {
    Tast_def* def_ = NULL;
    try(symbol_lookup(&def_, env, lang_type_get_str(struct_literal->lang_type)));
    return sizeof_struct_def_base(env, &tast_struct_def_unwrap(def_)->base);
}

uint64_t sizeof_struct_def_base(Env* env, const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(env, memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct_expr(Env* env, const Tast_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case TAST_STRUCT_LITERAL:
            return sizeof_struct_literal(env, tast_struct_literal_const_unwrap(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t llvm_sizeof_expr(Env* env, const Llvm_expr* expr) {
    (void) env;
    switch (expr->type) {
        case LLVM_LITERAL:
            return sizeof_lang_type(env, llvm_literal_get_lang_type(llvm_literal_const_unwrap(expr)));
        default:
            unreachable("");
    }
}

static uint64_t llvm_sizeof_def(Env* env, const Llvm_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(env, llvm_variable_def_const_unwrap(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_item(Env* env, const Llvm* item) {
    (void) env;
    switch (item->type) {
        case TAST_EXPR:
            return llvm_sizeof_expr(env, llvm_expr_const_unwrap(item));
        case TAST_DEF:
            return llvm_sizeof_def(env, llvm_def_const_unwrap(item));
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_struct_def_base(Env* env, const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(env, memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

