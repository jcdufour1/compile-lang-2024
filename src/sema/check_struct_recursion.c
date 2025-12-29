#include <util.h>
#include <arena.h>
#include <ulang_type.h>
#include <uast.h>
#include <name.h>
#include <msg.h>
#include <uast_utils.h>
#include <check_struct_recursion.h>

// TODO: consider using iterative approach to avoid stack overflow risk
static Arena struct_like_rec_a = {0};

static bool check_struct_rec_internal(Ustruct_def_base base, Name_darr rec_stack);

static bool check_struct_rec_internal_def(Uast_def* def, Ulang_type_regular lang_type, Name name, Name_darr rec_stack) {
    for (size_t idx = 0; idx < rec_stack.info.count; idx++) {
        if (name_is_equal(darr_at(rec_stack, idx), name)) {
            msg(
                DIAG_STRUCT_LIKE_RECURSION, lang_type.pos,
                "`"FMT"` recursively includes itself without indirection; consider "
                "storing `"FMT"` by pointer here instead of by value\n",
                name_print(NAME_MSG, uast_def_get_name(def), false),
                name_print(NAME_MSG, uast_def_get_name(def), false)
            );
            Uast_def* prev = def;
            for (size_t idx_stk = idx + 1; idx_stk < rec_stack.info.count; idx_stk++) {
                Uast_def* curr = NULL;
                unwrap(usymbol_lookup(&curr, darr_at(rec_stack, idx_stk)));
                msg(
                    DIAG_NOTE, uast_def_get_pos(prev),
                    "`"FMT"` contains `"FMT"`\n",
                    name_print(NAME_MSG, uast_def_get_name(prev), false),
                    name_print(NAME_MSG, uast_def_get_name(curr), false)
                );

                prev = curr;
            }
            msg(
                DIAG_NOTE, uast_def_get_pos(prev),
                "`"FMT"` contains `"FMT"`\n",
                name_print(NAME_MSG, uast_def_get_name(prev), false),
                name_print(NAME_MSG, uast_def_get_name(def), false)
            );
            return false;
        }
    }

    return check_struct_rec_internal(uast_def_get_struct_def_base(def), rec_stack);
}

static bool check_struct_rec_internal_lang_type_reg(Ulang_type_regular lang_type, Name_darr rec_stack) {
    if (lang_type.pointer_depth > 0) {
        return true;
    }
    Uast_def* def = {0};
    Name name = {0};
    unwrap(name_from_uname(&name, lang_type.name, lang_type.pos));
    if (!usymbol_lookup(&def, name)) {
        assert(env.error_count > 0 && "there is a bug somewhere");
        return false;
    }

    switch (def->type) {
        case UAST_VOID_DEF:
            return true;
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            return true;
        case UAST_MOD_ALIAS:
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_VARIABLE_DEF:
            todo();
        case UAST_STRUCT_DEF:
            fallthrough;
        case UAST_RAW_UNION_DEF:
            fallthrough;
        case UAST_ENUM_DEF:
            return check_struct_rec_internal_def(def, lang_type, name, rec_stack);
        case UAST_LANG_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            return true;
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_LABEL:
            todo();
        case UAST_BUILTIN_DEF:
            unreachable("");
    }
    unreachable("");
}

static bool check_struct_rec_internal_ulang_type(Ulang_type lang_type, Name_darr rec_stack) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular reg = ulang_type_regular_const_unwrap(lang_type);
            if (!check_struct_rec_internal_lang_type_reg(reg, rec_stack)) {
                return false;
            }
            return true;
        }
        case ULANG_TYPE_FN:
            // ULANG_TYPE_FN is always a pointer
            assert(ulang_type_fn_const_unwrap(lang_type).pointer_depth > 0);
            return true;
        case ULANG_TYPE_TUPLE:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case ULANG_TYPE_ARRAY: {
            Ulang_type_array arr = ulang_type_array_const_unwrap(lang_type);
            check_struct_rec_internal_ulang_type(*arr.item_type, rec_stack);
            return true;
        }
        case ULANG_TYPE_EXPR:
            return true;
        case ULANG_TYPE_LIT:
            return true;
        case ULANG_TYPE_REMOVED:
            return true;
    }
    unreachable("");
}

static bool check_struct_rec_internal(Ustruct_def_base base, Name_darr rec_stack) {
    darr_append(&struct_like_rec_a, &rec_stack, base.name);

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Uast_variable_def* curr = darr_at(base.members, idx);
        check_struct_rec_internal_ulang_type(curr->lang_type, rec_stack);
    }

    return true;
}

bool check_struct_for_rec(const Uast_def* def) {
    Strv old_mod_path_curr_file = env.mod_path_curr_file;
    env.mod_path_curr_file = uast_def_get_name(def).mod_path;

    bool status = check_struct_rec_internal(uast_def_get_struct_def_base(def), (Name_darr) {0});
    arena_reset(&struct_like_rec_a);

    env.mod_path_curr_file = old_mod_path_curr_file;
    return status;
}
