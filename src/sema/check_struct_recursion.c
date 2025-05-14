#include <util.h>
#include <arena.h>
#include <ulang_type.h>
#include <uast.h>
#include <name.h>
#include <ulang_type_get_pos.h>
#include <msg.h>
#include <uast_utils.h>
#include <check_struct_recursion.h>

// TODO: consider using iterative approach to avoid stack overflow risk
static Arena struct_like_rec_a = {0};

static bool check_struct_rec_internal(Ustruct_def_base base, Name_vec rec_stack);

static bool check_struct_rec_internal_def(Uast_def* def, Ulang_type_regular lang_type, Name name, Name_vec rec_stack) {
    for (size_t idx = 0; idx < rec_stack.info.count; idx++) {
        if (name_is_equal(vec_at(&rec_stack, idx), name)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_LIKE_RECURSION, lang_type.pos,
                "`"TAST_FMT"` recursively includes itself without indirection; consider "
                "storing `"TAST_FMT"` by pointer here instead of by value\n",
                name_print(NAME_MSG, uast_def_get_name(def)),
                name_print(NAME_MSG, uast_def_get_name(def))
            );
            Uast_def* prev = def;
            for (size_t idx_stk = idx + 1; idx_stk < rec_stack.info.count; idx_stk++) {
                Uast_def* curr = NULL;
                unwrap(usymbol_lookup(&curr, vec_at(&rec_stack, idx_stk)));
                msg(
                    LOG_NOTE, EXPECT_FAIL_NOTE, uast_def_get_pos(prev),
                    "`"TAST_FMT"` contains `"TAST_FMT"`\n",
                    name_print(NAME_MSG, uast_def_get_name(prev)),
                    name_print(NAME_MSG, uast_def_get_name(curr))
                );

                prev = curr;
            }
            msg(
                LOG_NOTE, EXPECT_FAIL_NOTE, uast_def_get_pos(prev),
                "`"TAST_FMT"` contains `"TAST_FMT"`\n",
                name_print(NAME_MSG, uast_def_get_name(prev)),
                name_print(NAME_MSG, uast_def_get_name(def))
            );
            return false;
        }
    }

    return true;
}

static bool check_struct_rec_internal_lang_type_reg(Ulang_type_regular lang_type, Name_vec rec_stack /* TODO: consider using hash table for O(1) time */) {
    if (lang_type.atom.pointer_depth > 0) {
        return true;
    }
    Uast_def* def = {0};
    Name name = {0};
    unwrap(name_from_uname(&name, lang_type.atom.str));
    if (!usymbol_lookup(&def, name)) {
        assert(error_count > 0 && "there is a bug somewhere");
        return false;
    }

    switch (def->type) {
        case UAST_POISON_DEF:
            todo();
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
            // fallthrough
        case UAST_RAW_UNION_DEF:
            // fallthrough
        case UAST_SUM_DEF:
            if (!check_struct_rec_internal_def(def, lang_type, name, rec_stack)) {
                return false;
            }
            // TODO: move below statement to check_struct_rec_internal_def?
            return check_struct_rec_internal(uast_def_get_struct_def_base(def), rec_stack);
        case UAST_ENUM_DEF:
            return true;
        case UAST_LANG_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            return true;
        case UAST_FUNCTION_DECL:
            todo();
    }
    unreachable("");
}

static bool check_struct_rec_internal(Ustruct_def_base base, Name_vec rec_stack) {
    vec_append(&struct_like_rec_a, &rec_stack, base.name);

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Uast_variable_def* curr = vec_at(&base.members, idx);
        switch (curr->lang_type.type) {
            case ULANG_TYPE_REGULAR: {
                Ulang_type_regular reg = ulang_type_regular_const_unwrap(curr->lang_type);
                if (!check_struct_rec_internal_lang_type_reg(reg, rec_stack)) {
                    return false;
                }
                break;
            }
            case ULANG_TYPE_FN:
                // ULANG_TYPE_FN is always a pointer
                break;
            case ULANG_TYPE_TUPLE:
                todo();
            default:
                unreachable("");
        }
    }

    return true;
}

bool check_struct_for_rec(const Uast_def* def) {
    bool status = check_struct_rec_internal(uast_def_get_struct_def_base(def), (Name_vec) {0});
    arena_reset(&struct_like_rec_a);
    return status;
}
