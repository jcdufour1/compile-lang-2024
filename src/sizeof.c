#include <sizeof.h>
#include <lang_type.h>
#include <lang_type_after.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <tast.h>
#include <llvm.h>
#include <lang_type_print.h>

uint64_t sizeof_primitive(Lang_type_primitive primitive) {
    // TODO: platform specific pointer size, etc.
    if (lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, primitive) > 0) {
        return 8;
    }

    switch (primitive.type) {
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(primitive).bit_width/8;
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(primitive).bit_width/8;
        case LANG_TYPE_CHAR:
            return 1;
        case LANG_TYPE_ANY:
            unreachable("");
    }
    unreachable("");
}

uint64_t sizeof_lang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return sizeof_primitive(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def,  lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def( def);
        }
        case LANG_TYPE_ENUM: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def,  lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def( def);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def,  lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case LANG_TYPE_VOID:
            return 0;
        case LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case LANG_TYPE_FN:
            todo();
    }
    unreachable(LANG_TYPE_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t sizeof_expr(const Tast_expr* expr) {
    (void) env;
    switch (expr->type) {
        case TAST_LITERAL:
            return sizeof_lang_type( tast_literal_get_lang_type(tast_literal_const_unwrap(expr)));
        default:
            unreachable("");
    }
}

uint64_t sizeof_def(const Tast_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type( tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return sizeof_struct_def_base( &tast_struct_def_const_unwrap(def)->base);
        case TAST_ENUM_DEF:
            return sizeof_struct_def_base( &tast_enum_def_const_unwrap(def)->base);
        case TAST_RAW_UNION_DEF:
            return sizeof_struct_def_base( &tast_raw_union_def_const_unwrap(def)->base);
        default:
            unreachable("");
    }
}

uint64_t sizeof_stmt(const Tast_stmt* stmt) {
    (void) env;
    switch (stmt->type) {
        case TAST_EXPR:
            return sizeof_expr( tast_expr_const_unwrap(stmt));
        case TAST_DEF:
            return sizeof_def( tast_def_const_unwrap(stmt));
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Tast_struct_literal* struct_literal) {
    Tast_def* def_ = NULL;
    unwrap(symbol_lookup(&def_,  lang_type_get_str(LANG_TYPE_MODE_LOG, struct_literal->lang_type)));
    return sizeof_struct_def_base( &tast_struct_def_unwrap(def_)->base);
}

uint64_t sizeof_struct_def_base(const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type( memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct_expr(const Tast_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case TAST_STRUCT_LITERAL:
            return sizeof_struct_literal( tast_struct_literal_const_unwrap(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t llvm_sizeof_expr(const Llvm_expr* expr) {
    (void) env;
    switch (expr->type) {
        case LLVM_LITERAL:
            return sizeof_lang_type( llvm_literal_get_lang_type(llvm_literal_const_unwrap(expr)));
        default:
            unreachable("");
    }
}

static uint64_t llvm_sizeof_def(const Llvm_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type( llvm_variable_def_const_unwrap(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_item(const Llvm* item) {
    (void) env;
    switch (item->type) {
        case TAST_EXPR:
            return llvm_sizeof_expr( llvm_expr_const_unwrap(item));
        case TAST_DEF:
            return llvm_sizeof_def( llvm_def_const_unwrap(item));
        default:
            unreachable("");
    }
}

uint64_t llvm_sizeof_struct_def_base(const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(&base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type( memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

