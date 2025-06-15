#include <tast.h>
#include <uast.h>
#include <ir.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <log_env.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <llvm_lang_type_after.h>
#include <lang_type_from_ulang_type.h>
#include <token_type_to_operator_type.h>
#include <symbol_log.h>
#include <symbol_iter.h>
#include <sizeof.h>

typedef enum {
    DEFER_PARENT_OF_FUN,
    DEFER_PARENT_OF_FOR,
    DEFER_PARENT_OF_IF,
    DEFER_PARENT_OF_BLOCK,
    DEFER_PARENT_OF_TOP_LEVEL,
} DEFER_PARENT_OF;

// a defered statement
typedef struct {
    Tast_defer* defer;
    Tast_label* label;
} Defer_pair;

// TODO: move this macro?
#define defer_pair_print(pair) strv_print(defer_pair_print_internal(pair))

// TODO: move this function?
static inline Strv defer_pair_print_internal(Defer_pair pair) {
    String buf = {0};
    string_extend_strv(&a_print, &buf, tast_defer_print_internal(pair.defer, 0));
    string_extend_strv(&a_print, &buf, tast_label_print_internal(pair.label, 0));
    return string_to_strv(buf);
}

// all defered statements in one scope
typedef struct {
    Vec_base info;
    Defer_pair* buf;
} Defer_pair_vec;

// all defered statements and parent_of
typedef struct {
    Defer_pair_vec pairs;
    DEFER_PARENT_OF parent_of;
    Tast_expr* rtn_val;
    Name is_brking;
    Name is_conting;
} Defer_collection;

// stack of scope defered statements
typedef struct {
    Vec_base info;
    Defer_collection* buf;
} Defer_collection_vec;

typedef struct {
    Defer_collection_vec coll_stack;
    Name is_rtning;
} Defer_colls;

//
// globals
//

static Tast_variable_def* rtn_def;

static Defer_colls defered_collections;

static Name load_break_symbol_name;
static Name label_if_break;
static Name label_if_after;
static Name label_after_for;
static Name label_if_continue;

static Name struct_rtn_name_parent_function;

static Name name_parent_fn;

#define if_for_add_cond_goto(old_oper, new_block, label_name_if_true, label_name_if_false) \
    if_for_add_cond_goto_internal(loc_new(), old_oper, new_block, label_name_if_true, label_name_if_false)

#define add_label(block, label_name, pos) add_label_internal(loc_new(), block, label_name, pos)

#define load_assignment(new_block, old_assign) \
    load_assignment_internal(__FILE__, __LINE__, new_block, old_assign)

//
// forward declarations
//

static void if_for_add_cond_goto_internal(
    Loc loc,
    Tast_operator* old_oper,
    Ir_block* new_block,
    Name label_name_if_true,
    Name label_name_if_false
);

static Llvm_lang_type rm_tuple_lang_type(Lang_type lang_type, Pos lang_type_pos);

static Name load_symbol(Ir_block* new_block, Tast_symbol* old_symbol);

static void load_struct_def(Tast_struct_def* old_struct_def);

static void load_raw_union_def(Tast_raw_union_def* old_def);

static Name load_ptr_symbol(Ir_block* new_block, Tast_symbol* old_sym);

static Ir_block* load_block(
    bool* rtn_in_block,
    Tast_block* old_block,
    DEFER_PARENT_OF parent_of,
    Lang_type lang_type
);

static Name load_expr(Ir_block* new_block, Tast_expr* old_expr);

static Name load_ptr_expr(Ir_block* new_block, Tast_expr* old_expr);

static void load_stmt(bool* rtn_in_block, Ir_block* new_block, Tast_stmt* old_stmt, bool is_defered);

static Name load_operator(Ir_block* new_block, Tast_operator* old_oper);

static void load_variable_def(Ir_block* new_block, Tast_variable_def* old_var_def);

static Name load_if_else_chain(Ir_block* new_block, Tast_if_else_chain* old_if_else);

static void load_label(Ir_block* new_block, Tast_label* old_label);

static Name load_return(Ir_block* new_block, Tast_return* old_return);

static void load_break(Ir_block* new_block, Tast_break* old_break);

static Name load_assignment_internal(const char* file, int line, Ir_block* new_block, Tast_assignment* old_assign);

static Tast_symbol* tast_symbol_new_from_variable_def(Pos pos, const Tast_variable_def* def) {
    return tast_symbol_new(
        pos,
        (Sym_typed_base) {.lang_type = def->lang_type, .name = def->name}
    );
}

static void load_block_stmts(
    bool* rtn_in_block,
    Ir_block* new_block,
    Tast_stmt_vec children,
    DEFER_PARENT_OF parent_of,
    Pos pos,
    Lang_type lang_type
) {
    // TODO: avoid making this def on LANG_TYPE_VOID?
    Tast_variable_def* local_rtn_def = NULL;
    if (lang_type.type != LANG_TYPE_VOID) {
        local_rtn_def = tast_variable_def_new(pos, lang_type, false, util_literal_name_new_prefix(sv("rtn_val")));
    }

    Name is_rtning_name = {0};
    switch (parent_of) {
        case DEFER_PARENT_OF_FUN: {
            is_rtning_name = util_literal_name_new_prefix(sv("is_rtning_fun"));
            break;
        }
        case DEFER_PARENT_OF_FOR: {
            assert(label_if_break.base.count > 0);
            is_rtning_name = util_literal_name_new_prefix(sv("is_rtning_for"));
            break;
        }
        case DEFER_PARENT_OF_IF: {
            assert(label_if_break.base.count > 0);
            is_rtning_name = util_literal_name_new_prefix(sv("is_rtning_if"));
            break;
        }
        case DEFER_PARENT_OF_BLOCK: {
            is_rtning_name = util_literal_name_new_prefix(sv("is_rtning_block"));
            break;
        }
        case DEFER_PARENT_OF_TOP_LEVEL: {
            is_rtning_name = util_literal_name_new_prefix(sv("is_rtning_top_level"));
            break;
        }
        default:
            unreachable("");
    }

    Tast_variable_def* is_rtning = tast_variable_def_new(pos, lang_type_new_u1(), false, is_rtning_name);
    Tast_assignment* is_rtn_assign = tast_assignment_new(
        pos,
        tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {
            .lang_type = is_rtning->lang_type, .name = is_rtning->name
        })),
        tast_literal_wrap(tast_int_wrap(tast_int_new(pos, 0, lang_type_new_u1())))
    );

    Name is_brking_name = {0};
    switch (parent_of) {
        case DEFER_PARENT_OF_FUN: {
            is_brking_name = util_literal_name_new_prefix(sv("is_brking_fun"));
            break;
        }
        case DEFER_PARENT_OF_FOR: {
            assert(label_if_continue.base.count > 0);
            is_brking_name = util_literal_name_new_prefix(sv("is_brking_for"));
            break;
        }
        case DEFER_PARENT_OF_IF: {
            is_brking_name = util_literal_name_new_prefix(sv("is_brking_if"));
            break;
        }
        case DEFER_PARENT_OF_BLOCK: {
            is_brking_name = util_literal_name_new_prefix(sv("is_brking_block"));
            break;
        }
        case DEFER_PARENT_OF_TOP_LEVEL: {
            is_brking_name = util_literal_name_new_prefix(sv("is_brking_top_level"));
            break;
        }
        default:
            unreachable("");
    }

    Name is_conting_name = {0};
    switch (parent_of) {
        case DEFER_PARENT_OF_FUN: {
            is_conting_name = util_literal_name_new_prefix(sv("is_conting_fun"));
            break;
        }
        case DEFER_PARENT_OF_FOR: {
            assert(label_if_continue.base.count > 0);
            is_conting_name = util_literal_name_new_prefix(sv("is_conting_for"));
            break;
        }
        case DEFER_PARENT_OF_IF: {
            is_conting_name = util_literal_name_new_prefix(sv("is_conting_if"));
            break;
        }
        case DEFER_PARENT_OF_BLOCK: {
            is_conting_name = util_literal_name_new_prefix(sv("is_conting_block"));
            break;
        }
        case DEFER_PARENT_OF_TOP_LEVEL: {
            is_conting_name = util_literal_name_new_prefix(sv("is_conting_top_level"));
            break;
        }
        default:
            todo();
    }

    Tast_variable_def* is_brking = tast_variable_def_new(pos, lang_type_new_u1(), false, is_brking_name);
    Tast_assignment* is_brk_assign = tast_assignment_new(
        pos,
        tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {
            .lang_type = is_brking->lang_type, .name = is_brking->name
        })),
        tast_literal_wrap(tast_int_wrap(tast_int_new(pos, 0, lang_type_new_u1())))
    );

    Tast_variable_def* is_conting = tast_variable_def_new(pos, lang_type_new_u1(), false, is_conting_name);
    Tast_assignment* is_cont_assign = tast_assignment_new(
        pos,
        tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {
            .lang_type = is_conting->lang_type, .name = is_conting->name
        })),
        tast_literal_wrap(tast_int_wrap(tast_int_new(pos, 0, lang_type_new_u1())))
    );


    Tast_expr* rtn_val = {0};

    if (lang_type.type == LANG_TYPE_VOID) {
        rtn_val = tast_literal_wrap(tast_void_wrap(tast_void_new(pos)));
    } else {
        rtn_val = tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {.lang_type = lang_type, .name = local_rtn_def->name}));
    }
    Tast_defer* defer = NULL;
    Tast_variable_def* old_rtn_def = NULL;
    switch (parent_of) {
        case DEFER_PARENT_OF_FUN: {
            old_rtn_def = rtn_def;
            rtn_def = local_rtn_def;

            Tast_return* actual_rtn = tast_return_new(pos, rtn_val, true);
            defer = tast_defer_new(pos, tast_return_wrap(actual_rtn));
            break;
        }
        case DEFER_PARENT_OF_FOR: {
            Tast_break* actual_brk = tast_break_new(pos, false, rtn_val);
            defer = tast_defer_new(pos, tast_break_wrap(actual_brk));
            break;
        }
        case DEFER_PARENT_OF_IF: {
            Tast_break* actual_brk = tast_break_new(pos, false, rtn_val);
            defer = tast_defer_new(pos, tast_break_wrap(actual_brk));
            break;
        }
        case DEFER_PARENT_OF_BLOCK: {
            if (lang_type.type != LANG_TYPE_VOID) {
                Tast_break* actual_brk = tast_break_new(pos, false, rtn_val);
                defer = tast_defer_new(pos, tast_break_wrap(actual_brk));
            }
            break;
        }
        case DEFER_PARENT_OF_TOP_LEVEL: {
            for (size_t idx = 0; idx < children.info.count; idx++) {
                load_stmt(rtn_in_block, new_block, vec_at(&children, idx), false);
            }
            //assert(vec_top(&defered_collections).pairs.info.count < 1 && "this should have been caught in the type checking pass");
            return;
        }
        default:
            todo();
    }

    if (defered_collections.coll_stack.info.count < 1) {
        defered_collections.is_rtning = is_rtning->name;
    }
    vec_append(&a_main, &defered_collections.coll_stack, ((Defer_collection) {
        .pairs = (Defer_pair_vec) {0},
        .parent_of = parent_of,
        .rtn_val = rtn_val,
        .is_brking = is_brking->name,
        .is_conting = is_conting->name
    }));

    switch (parent_of) {
        case DEFER_PARENT_OF_FUN: {
            vec_append(&a_main, &vec_top_ref(&defered_collections.coll_stack)->pairs, ((Defer_pair) {
                defer,
                tast_label_new(defer->pos, util_literal_name_new_prefix(sv("actual_return_parent_of_fun")))
            }));
            break;
        }
        case DEFER_PARENT_OF_FOR: {
            vec_append(&a_main, &vec_top_ref(&defered_collections.coll_stack)->pairs, ((Defer_pair) {
                defer,
                tast_label_new(defer->pos, util_literal_name_new_prefix(sv("actual_return_parent_of_for")))
            }));
            break;
        }
        case DEFER_PARENT_OF_IF: {
            vec_append(&a_main, &vec_top_ref(&defered_collections.coll_stack)->pairs, ((Defer_pair) {
                defer,
                tast_label_new(defer->pos, util_literal_name_new_prefix(sv("actual_return_parent_of_if")))
            }));
            break;
        }
        case DEFER_PARENT_OF_BLOCK: {
            vec_append(&a_main, &vec_top_ref(&defered_collections.coll_stack)->pairs, ((Defer_pair) {
                defer,
                tast_label_new(defer->pos, util_literal_name_new_prefix(sv("actual_return_parent_of_block")))
            }));
            break;
        }
        case DEFER_PARENT_OF_TOP_LEVEL: {
            unreachable("");
        }
        default:
            todo();
    }

    if (lang_type.type != LANG_TYPE_VOID) {
        unwrap(symbol_add(tast_variable_def_wrap(local_rtn_def)));
        load_variable_def(new_block, local_rtn_def);
    }
    unwrap(symbol_add(tast_variable_def_wrap(is_rtning)));
    unwrap(symbol_add(tast_variable_def_wrap(is_brking)));
    unwrap(symbol_add(tast_variable_def_wrap(is_conting)));
    load_variable_def(new_block, is_rtning);
    load_variable_def(new_block, is_conting);
    load_assignment(new_block, is_rtn_assign);
    load_assignment(new_block, is_brk_assign);
    load_assignment(new_block, is_cont_assign);

    for (size_t idx = 0; idx < children.info.count; idx++) {
        load_stmt(rtn_in_block, new_block, vec_at(&children, idx), false);
    }

    Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
    while (pairs->info.count > 0) {
        Defer_pair_vec dummy_stmts = {0};
        Defer_pair pair = vec_top(pairs);
        load_label(new_block, pair.label);
        if (pairs->info.count == 1 && parent_of == DEFER_PARENT_OF_FOR) {
            //Name goto_cond = ir_
            if_for_add_cond_goto(
                // if this condition evaluates to true, we are not breaking right now
                tast_binary_wrap(tast_binary_new(
                    pos,
                    tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {
                        .lang_type = tast_lang_type_from_name(vec_top(&defered_collections.coll_stack).is_brking),
                        .name = vec_top(&defered_collections.coll_stack).is_brking
                    })),
                    tast_literal_wrap(tast_int_wrap(tast_int_new(pos, 0, lang_type_new_u1()))),
                    BINARY_DOUBLE_EQUAL,
                    lang_type_new_u1()
                )),
                new_block,
                label_if_continue,
                label_if_break
            );

            //Name goto_cond = ir_
            if_for_add_cond_goto(
                // if this condition evaluates to true, we are not breaking right now
                tast_binary_wrap(tast_binary_new(
                    pos,
                    tast_symbol_wrap(tast_symbol_new(pos, (Sym_typed_base) {
                        .lang_type = tast_lang_type_from_name(vec_top(&defered_collections.coll_stack).is_conting),
                        .name = vec_top(&defered_collections.coll_stack).is_conting
                    })),
                    tast_literal_wrap(tast_int_wrap(tast_int_new(pos, 0, lang_type_new_u1()))),
                    BINARY_DOUBLE_EQUAL,
                    lang_type_new_u1()
                )),
                new_block,
                label_if_continue,
                label_if_break
            );
        }

        load_stmt(rtn_in_block, new_block, pair.defer->child, true);
        vec_rem_last(pairs);
        if (dummy_stmts.info.count > 0) {
            // `defer defer` used
            // TODO: expected failure test
            todo();
        }
    }

    if (parent_of == DEFER_PARENT_OF_FUN) {
        rtn_def = old_rtn_def;
    }
    vec_rem_last(&defered_collections.coll_stack);
}

static Lang_type_struct rm_tuple_lang_type_tuple(Lang_type_tuple lang_type, Pos lang_type_pos) {
    Tast_variable_def_vec members = {0};

    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        Tast_variable_def* memb = tast_variable_def_new(
            lang_type_pos,
            vec_at_const(lang_type.lang_types, idx),
            false,
            util_literal_name_new_prefix(sv("rm_tuple_lang_type_tuple"))
        );
        vec_append(&a_main, &members, memb);
    }

    Struct_def_base base = {
        .members = members,
        .name = util_literal_name_new()
    };
    Tast_struct_def* struct_def = tast_struct_def_new(lang_type_pos, base);
    sym_tbl_add(tast_struct_def_wrap(struct_def));
    return lang_type_struct_new(lang_type_pos, lang_type_atom_new(base.name, 0));
}

// note: will not clone everything
static Tast_raw_union_def* get_raw_union_def_from_enum_def(Tast_enum_def* enum_def) {
#   ifndef DNDEBUG
        size_t largest_idx = struct_def_base_get_idx_largest_member(enum_def->base);
        assert(
            vec_at(&enum_def->base.members, largest_idx)->lang_type.type != LANG_TYPE_VOID &&
            "enum_def with inner types of only void should never be passed here"
        );
#   endif // DNDEBUG
       
    Tast_raw_union_def* cached_def = NULL;
    if (raw_union_of_enum_lookup(&cached_def, enum_def->base.name)) {
        return cached_def;
    }

    Tast_raw_union_def* union_def = tast_raw_union_def_new(enum_def->pos, enum_def->base);
    union_def->base.name = util_literal_name_new_prefix(union_def->base.name.base);
    unwrap(raw_union_of_enum_add(union_def, enum_def->base.name));
    load_raw_union_def(union_def);
    return union_def;
}

static Tast_struct_def* enum_get_struct_def(Name enum_name, Tast_variable_def_vec membs, Pos pos) {
    Tast_struct_def* cached_def = NULL;
    // TODO: rename this hash table
    if (struct_to_struct_lookup(&cached_def, enum_name)) {
        return cached_def;
    }

    Tast_struct_def* new_def = tast_struct_def_new(pos, (Struct_def_base) {
        .members = membs,
        .name = util_literal_name_new_prefix(enum_name.base)
    });
    unwrap(struct_to_struct_add(new_def, enum_name));
    load_struct_def(new_def);
    return new_def;
}

static Llvm_lang_type rm_tuple_lang_type_enum(Lang_type_enum lang_type, Pos lang_type_pos) {
    Tast_def* lang_type_def_ = NULL; 
    unwrap(symbol_lookup(&lang_type_def_, lang_type.atom.str));
    Tast_variable_def_vec members = {0};

    Lang_type tag_lang_type = lang_type_new_usize();
    size_t largest_idx = struct_def_base_get_idx_largest_member(tast_enum_def_unwrap(lang_type_def_)->base);
    if (vec_at(&tast_enum_def_unwrap(lang_type_def_)->base.members, largest_idx)->lang_type.type == LANG_TYPE_VOID) {
        return rm_tuple_lang_type(tag_lang_type, lang_type_pos);
    }

    Tast_variable_def* tag = tast_variable_def_new(
        lang_type_pos,
        tag_lang_type,
        false,
        util_literal_name_new_prefix(sv("rm_tuple_lang_type_enum_tag"))
    );
    vec_append(&a_main, &members, tag);

    Tast_raw_union_def* item_type_def = get_raw_union_def_from_enum_def(tast_enum_def_unwrap(lang_type_def_));

    Tast_variable_def* item = tast_variable_def_new(
        lang_type_pos,
        lang_type_raw_union_const_wrap(lang_type_raw_union_new(
            item_type_def->pos,
            lang_type_atom_new(item_type_def->base.name, 0)
        )),
        false,
        util_literal_name_new_prefix(sv("rm_tuple_lang_type_enum_item"))
    );
    vec_append(&a_main, &members, item);
    
    Tast_struct_def* struct_def = enum_get_struct_def(
        tast_enum_def_unwrap(lang_type_def_)->base.name,
        members,
        item->pos
    );
    Tast_def* dummy = NULL;
    unwrap(sym_tbl_lookup(&dummy, struct_def->base.name));

    load_struct_def(struct_def);
    return rm_tuple_lang_type(tast_struct_def_get_lang_type(struct_def), lang_type_pos);
}

static Llvm_lang_type_atom rm_tuple_lang_type_atom(Lang_type_atom atom) {
    return llvm_lang_type_atom_new(atom.str, atom.pointer_depth);
}

static Llvm_lang_type_primitive rm_tuple_lang_type_primitive(Lang_type_primitive lang_type, Pos lang_type_pos) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR: {
            // TODO: should this always be signed
            Lang_type_char lang_char = lang_type_char_const_unwrap(lang_type);
            return llvm_lang_type_unsigned_int_const_wrap(llvm_lang_type_unsigned_int_new(
                lang_type_pos,
                8,
                rm_tuple_lang_type_atom(lang_char.atom).pointer_depth
            ));
        }
        case LANG_TYPE_SIGNED_INT: {
            Lang_type_signed_int num = lang_type_signed_int_const_unwrap(lang_type);
            return llvm_lang_type_signed_int_const_wrap(llvm_lang_type_signed_int_new(
                lang_type_pos,
                num.bit_width,
                num.pointer_depth
            ));
        }
        case LANG_TYPE_UNSIGNED_INT: {
            Lang_type_unsigned_int num = lang_type_unsigned_int_const_unwrap(lang_type);
            return llvm_lang_type_unsigned_int_const_wrap(llvm_lang_type_unsigned_int_new(
                lang_type_pos,
                num.bit_width,
                num.pointer_depth
            ));
        }
        case LANG_TYPE_FLOAT: {
            Lang_type_float num = lang_type_float_const_unwrap(lang_type);
            return llvm_lang_type_float_const_wrap(llvm_lang_type_float_new(
                lang_type_pos,
                num.bit_width,
                num.pointer_depth
            ));
        }
        case LANG_TYPE_OPAQUE: {
            Lang_type_opaque opaque = lang_type_opaque_const_unwrap(lang_type);
            return llvm_lang_type_opaque_const_wrap(llvm_lang_type_opaque_new(
                lang_type_pos,
                rm_tuple_lang_type_atom(opaque.atom)
            ));
        }
    }
    unreachable("");
}

static Llvm_lang_type_fn rm_tuple_lang_type_fn(Lang_type_fn lang_type, Pos lang_type_pos) {
    Llvm_lang_type* new_rtn_type = arena_alloc(&a_main, sizeof(*new_rtn_type));
    *new_rtn_type = rm_tuple_lang_type(*lang_type.return_type, lang_type_pos);

    Llvm_lang_type_vec params = {0};
    for (size_t idx = 0; idx < lang_type.params.lang_types.info.count; idx++) {
        vec_append(&a_main, &params, rm_tuple_lang_type(vec_at(&lang_type.params.lang_types, idx), lang_type_pos));
    }

    return llvm_lang_type_fn_new(
        lang_type_pos,
        llvm_lang_type_tuple_new(lang_type_pos, params),
        new_rtn_type
    );
}

static Llvm_lang_type rm_tuple_lang_type(Lang_type lang_type, Pos lang_type_pos) {
    switch (lang_type.type) {
        case LANG_TYPE_ENUM: {
            return rm_tuple_lang_type_enum(lang_type_enum_const_unwrap(lang_type), lang_type_pos);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* lang_type_def_ = NULL; 
            unwrap(symbol_lookup(&lang_type_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));

            sym_tbl_add(lang_type_def_);

            load_raw_union_def(tast_raw_union_def_unwrap(lang_type_def_));

            Tast_def* new_def = NULL;
            unwrap(symbol_lookup(&new_def, tast_raw_union_def_unwrap(lang_type_def_)->base.name));
            //log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, tast_raw_union_def_get_lang_type(item_type_def)));
            log(LOG_DEBUG, FMT"\n", tast_def_print(new_def));
            return llvm_lang_type_struct_const_wrap(llvm_lang_type_struct_new(
                lang_type_pos,
                rm_tuple_lang_type_atom(lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type))
            ));
        }
        case LANG_TYPE_TUPLE:
            return rm_tuple_lang_type(lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(lang_type_tuple_const_unwrap(lang_type), lang_type_pos)), lang_type_pos);
        case LANG_TYPE_PRIMITIVE:
            return llvm_lang_type_primitive_const_wrap(rm_tuple_lang_type_primitive(lang_type_primitive_const_unwrap(lang_type), lang_type_pos));
        case LANG_TYPE_STRUCT: {
            return llvm_lang_type_struct_const_wrap(llvm_lang_type_struct_new(
                lang_type_pos,
                rm_tuple_lang_type_atom(lang_type_struct_const_unwrap(lang_type).atom)
            ));
        }
        case LANG_TYPE_VOID:
            return llvm_lang_type_void_const_wrap(llvm_lang_type_void_new(lang_type_pos));
        case LANG_TYPE_FN:
            return llvm_lang_type_fn_const_wrap(rm_tuple_lang_type_fn(lang_type_fn_const_unwrap(lang_type), lang_type_pos));
        default:
            unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
    }
    unreachable("");
}

static bool binary_is_short_circuit(BINARY_TYPE type) {
    switch (type) {
        case BINARY_SINGLE_EQUAL:
            return false;
        case BINARY_SUB:
            return false;
        case BINARY_ADD:
            return false;
        case BINARY_MULTIPLY:
            return false;
        case BINARY_DIVIDE:
            return false;
        case BINARY_MODULO:
            return false;
        case BINARY_LESS_THAN:
            return false;
        case BINARY_LESS_OR_EQUAL:
            return false;
        case BINARY_GREATER_OR_EQUAL:
            return false;
        case BINARY_GREATER_THAN:
            return false;
        case BINARY_DOUBLE_EQUAL:
            return false;
        case BINARY_NOT_EQUAL:
            return false;
        case BINARY_BITWISE_XOR:
            return false;
        case BINARY_BITWISE_AND:
            return false;
        case BINARY_BITWISE_OR:
            return false;
        case BINARY_LOGICAL_AND:
            return true;
        case BINARY_LOGICAL_OR:
            return true;
        case BINARY_SHIFT_LEFT:
            return false;
        case BINARY_SHIFT_RIGHT:
            return false;
    }
    unreachable("");
}

static Ir_variable_def* load_variable_def_clone(Tast_variable_def* old_var_def);

static Ir_alloca* add_load_and_store_alloca_new(Ir_variable_def* var_def) {
    Ir_alloca* alloca = ir_alloca_new(
        var_def->pos,
        var_def->lang_type,
        var_def->name_corr_param
    );
    alloca_add(ir_alloca_wrap(alloca));
    assert(alloca);
    return alloca;
}

static Ir_function_params* load_function_params_clone(Tast_function_params* old_params) {
    Ir_function_params* new_params = ir_function_params_new(old_params->pos, util_literal_name_new(), (Ir_variable_def_vec){0});

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        vec_append(&a_main, &new_params->params, load_variable_def_clone(vec_at(&old_params->params, idx)));
    }

    return new_params;
}

static Ir_function_decl* load_function_decl_clone(Tast_function_decl* old_decl) {
    return ir_function_decl_new(
        old_decl->pos,
        load_function_params_clone(old_decl->params),
        rm_tuple_lang_type(old_decl->return_type, old_decl->pos),
        old_decl->name
    );
}

static Ir_variable_def* load_variable_def_clone(Tast_variable_def* old_var_def) {
    return ir_variable_def_new(
        old_var_def->pos,
        rm_tuple_lang_type(old_var_def->lang_type, old_var_def->pos),
        old_var_def->is_variadic,
        util_literal_name_new(),
        old_var_def->name
    );
}

static Ir_struct_def* load_struct_def_clone(const Tast_struct_def* old_def) {
    Ir_variable_def_vec new_membs = {0};
    for (size_t idx = 0; idx < old_def->base.members.info.count; idx++) {
        vec_append(&a_main, &new_membs, load_variable_def_clone(vec_at(&old_def->base.members, idx)));
    }
    return ir_struct_def_new(
        old_def->pos,
        ((Ir_struct_def_base) {.members = new_membs, .name = old_def->base.name})
    );
}

static Ir_struct_def* load_raw_union_def_clone(const Tast_raw_union_def* old_def) {
    size_t largest_idx = struct_def_base_get_idx_largest_member(old_def->base);
    Ir_variable_def_vec new_membs = {0};
    vec_append(&a_main, &new_membs, load_variable_def_clone(vec_at(&old_def->base.members, largest_idx)));
    return ir_struct_def_new(
        old_def->pos,
        ((Ir_struct_def_base) {.members = new_membs, .name = old_def->base.name})
    );
}

static void do_function_def_alloca_param(Ir_function_params* new_params, Ir_block* new_block, Ir_variable_def* param) {
    if (params.backend_info.struct_rtn_through_param && llvm_is_struct_like(param->lang_type.type)) {
        param->name_self = param->name_corr_param;
        alloca_add(ir_def_wrap(ir_variable_def_wrap(param)));
    } else {
        vec_insert(&a_main, &new_block->children, 0, ir_alloca_wrap(
            add_load_and_store_alloca_new(param)
        ));
    }

    vec_append(&a_main, &new_params->params, param);
}

static Ir_function_params* do_function_def_alloca(
    Lang_type* new_rtn_type,
    Lang_type rtn_type,
    Ir_block* new_block,
    const Tast_function_params* old_params
) {
    Ir_function_params* new_params = ir_function_params_new(
        old_params->pos,
        util_literal_name_new(),
        (Ir_variable_def_vec) {0}
    );

    Lang_type rtn_lang_type = rtn_type;
    if (params.backend_info.struct_rtn_through_param && is_struct_like(rtn_type.type)) {
        lang_type_set_pointer_depth(&rtn_lang_type, lang_type_get_pointer_depth(rtn_lang_type) + 1);
        Tast_variable_def* new_def = tast_variable_def_new(
            old_params->pos,
            rtn_lang_type,
            false,
            util_literal_name_new()
        );
        Ir_variable_def* param = load_variable_def_clone(new_def);
        do_function_def_alloca_param(new_params, new_block, param);
        *new_rtn_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
        struct_rtn_name_parent_function = vec_at(&new_params->params, 0)->name_self;
    } else {
        *new_rtn_type = rtn_type;
    }

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Ir_variable_def* param = load_variable_def_clone(vec_at(&old_params->params, idx));
        do_function_def_alloca_param(new_params, new_block, param);
    }

    return new_params;
}

static void add_label_internal(Loc loc, Ir_block* block, Name label_name, Pos pos) {
    Ir_label* label = ir_label_new_internal(pos, loc, label_name);
    unwrap(label_name.base.count > 0);
    label->name = label_name;
    unwrap(alloca_add(ir_def_wrap(ir_label_wrap(label))));
    vec_append(&a_main, &block->children, ir_def_wrap(ir_label_wrap(label)));
}

static void if_for_add_cond_goto_internal(
    Loc loc,
    Tast_operator* old_oper,
    Ir_block* new_block,
    Name label_name_if_true,
    Name label_name_if_false
) {
    Pos pos = tast_operator_get_pos(old_oper);

    assert(label_name_if_true.base.count > 0);
    assert(label_name_if_false.base.count > 0);
    Ir_cond_goto* cond_goto = ir_cond_goto_new_internal(
        pos,
        loc,
        util_literal_name_new(),
        load_operator(new_block, old_oper),
        label_name_if_true,
        label_name_if_false
    );

    vec_append(&a_main, &new_block->children, ir_cond_goto_wrap(cond_goto));
}

static Name load_function_call(Ir_block* new_block, Tast_function_call* old_call) {
    bool rtn_is_struct = is_struct_like(old_call->lang_type.type);

    Name_vec new_args = {0};

    Name def_name = {0};
    Llvm_lang_type fun_lang_type = rm_tuple_lang_type(old_call->lang_type, old_call->pos);
    if (params.backend_info.struct_rtn_through_param && rtn_is_struct) {
        def_name = util_literal_name_new();
        Tast_variable_def* def = tast_variable_def_new(old_call->pos, old_call->lang_type, false, def_name);
        unwrap(sym_tbl_add(tast_variable_def_wrap(def)));
        
        vec_append(&a_main, &new_args, def_name);
        load_variable_def(new_block, def);
        fun_lang_type = llvm_lang_type_void_const_wrap(llvm_lang_type_void_new(POS_BUILTIN));
        //unreachable(FMT, tast_function_call_print(old_call));
    }

    Ir_function_call* new_call = ir_function_call_new(
        old_call->pos,
        new_args,
        util_literal_name_new_prefix(sv("fun_call")),
        load_expr(new_block, old_call->callee),
        fun_lang_type
    );
    unwrap(alloca_add(ir_expr_wrap(ir_function_call_wrap(new_call))));

    for (size_t idx = 0; idx < old_call->args.info.count; idx++) {
        Tast_expr* old_arg = vec_at(&old_call->args, idx);
        Name thing = load_expr(new_block, old_arg);
        vec_append(&a_main, &new_call->args, thing);
        Ir* result = NULL;
        unwrap(alloca_lookup(&result, thing));
    }

    vec_append(&a_main, &new_block->children, ir_expr_wrap(ir_function_call_wrap(new_call)));

    if (params.backend_info.struct_rtn_through_param && rtn_is_struct) {
        assert(def_name.base.count > 0);

        Tast_symbol* new_sym = tast_symbol_new(old_call->pos, (Sym_typed_base) {
           .name = def_name,
           .lang_type = old_call->lang_type
        });

        Name result = load_expr(new_block, tast_symbol_wrap(new_sym));
        return result;
    } else {
        return new_call->name_self;
    }
}

// this function is needed for situations such as switching directly on enum
static Name load_ptr_function_call(Ir_block* new_block, Tast_function_call* old_call) {
    Tast_variable_def* new_var = tast_variable_def_new(
        old_call->pos,
        old_call->lang_type,
        false,
        util_literal_name_new()
    );
    unwrap(symbol_add(tast_variable_def_wrap(new_var)));
    load_variable_def(new_block, new_var);

    Tast_assignment* new_assign = tast_assignment_new(
        old_call->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_call->pos, new_var)),
        tast_function_call_wrap(old_call)
    );
    load_assignment(new_block, new_assign);

    return load_ptr_symbol(new_block, tast_symbol_new_from_variable_def(old_call->pos, new_var));
}

static Tast_variable_def* load_struct_literal_internal(Ir_block* new_block, Tast_struct_literal* old_lit) {
    Tast_variable_def* new_var = tast_variable_def_new(
        old_lit->pos,
        old_lit->lang_type,
        false,
        util_literal_name_new()
    );
    unwrap(symbol_add(tast_variable_def_wrap(new_var)));
    load_variable_def(new_block, new_var);

    Tast_def* struct_def_ = NULL;
    unwrap(symbol_lookup(&struct_def_, llvm_lang_type_get_str(LANG_TYPE_MODE_LOG, rm_tuple_lang_type(old_lit->lang_type, old_lit->pos))));
    Struct_def_base base = tast_def_get_struct_def_base(struct_def_);

    for (size_t idx = 0; idx < old_lit->members.info.count; idx++) {
        Name memb_name = vec_at(&base.members, idx)->name;
        Lang_type memb_lang_type = vec_at(&base.members, idx)->lang_type;
        Tast_member_access* access = tast_member_access_new(
            old_lit->pos,
            memb_lang_type,
            memb_name.base,
            tast_symbol_wrap(tast_symbol_new_from_variable_def(old_lit->pos, new_var))
        );

        Tast_assignment* assign = tast_assignment_new(
            access->pos,
            tast_member_access_wrap(access),
            vec_at(&old_lit->members, idx)
        );

        load_assignment(new_block, assign);
    }

    return new_var;
}

static Name load_ptr_struct_literal(Ir_block* new_block, Tast_struct_literal* old_lit) {
    Tast_variable_def* new_var = load_struct_literal_internal(new_block, old_lit);
    return load_ptr_symbol(new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Name load_struct_literal(Ir_block* new_block, Tast_struct_literal* old_lit) {
    Tast_variable_def* new_var = load_struct_literal_internal(new_block, old_lit);
    return load_symbol(new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Name load_string(Tast_string* old_lit) {
    Ir_string* string = ir_string_new(
        old_lit->pos,
        old_lit->data,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_string_wrap(string)))));
    return string->name;
}

static Name load_void(Pos pos) {
    Ir_void* new_void = ir_void_new(pos, util_literal_name_new());
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_void_wrap(new_void)))));
    return new_void->name;
}

static Name load_enum_tag_lit(Tast_enum_tag_lit* old_lit) {
    Ir_int* enum_tag_lit = ir_int_new(
        old_lit->pos,
        old_lit->data,
        rm_tuple_lang_type(old_lit->lang_type, old_lit->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(enum_tag_lit)))));
    return enum_tag_lit->name;
}

// TODO: rename to load_int
static Name load_number(Tast_int* old_lit) {
    Ir_int* number = ir_int_new(
        old_lit->pos,
        old_lit->data,
        rm_tuple_lang_type(old_lit->lang_type, old_lit->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(number)))));
    return number->name;
}

static Name load_float(Tast_float* old_lit) {
    Ir_float* number = ir_float_new(
        old_lit->pos,
        old_lit->data,
        rm_tuple_lang_type(old_lit->lang_type, old_lit->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_float_wrap(number)))));
    return number->name;
}

static Name load_char(Tast_char* old_lit) {
    Lang_type new_lang_type = lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(old_lit->pos, 8, 0)));
    Ir_int* lang_char = ir_int_new(old_lit->pos, old_lit->data, rm_tuple_lang_type(new_lang_type, old_lit->pos), util_literal_name_new());
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_int_wrap(lang_char)))));
    return lang_char->name;
}

static Name load_function_lit(Tast_function_lit* old_lit) {
    Ir_function_name* name = ir_function_name_new(
        old_lit->pos,
        old_lit->name,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_expr_wrap(ir_literal_wrap(ir_function_name_wrap(name)))));
    return name->name_self;
}

static Name load_enum_lit(Ir_block* new_block, Tast_enum_lit* old_lit) {
    Tast_def* enum_def_ = NULL;
    unwrap(symbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, old_lit->enum_lang_type)));
    Tast_enum_def* enum_def = tast_enum_def_unwrap(enum_def_);
    
    size_t largest_idx = struct_def_base_get_idx_largest_member(enum_def->base);
    if (vec_at(&enum_def->base.members, largest_idx)->lang_type.type == LANG_TYPE_VOID) {
        // inner lang_type is always void for this enum, so we will just use number instead of tagged enum
        return load_enum_tag_lit(old_lit->tag);
    }

    Lang_type new_lang_type = old_lit->enum_lang_type;

    Tast_raw_union_def* item_def = get_raw_union_def_from_enum_def(enum_def);

    Tast_expr_vec members = {0};
    vec_append(&a_main, &members, tast_literal_wrap(tast_enum_tag_lit_wrap(old_lit->tag)));
    vec_append(&a_main, &members, tast_literal_wrap(tast_raw_union_lit_wrap(
        tast_raw_union_lit_new(
            old_lit->pos,
            old_lit->tag,
            tast_raw_union_def_get_lang_type(item_def),
            old_lit->item
        )
    )));

    log(LOG_DEBUG, FMT"\n", tast_struct_literal_print(tast_struct_literal_new(
        old_lit->pos,
        members,
        util_literal_name_new(),
        new_lang_type
    )));
    log(LOG_DEBUG, FMT"\n", lang_type_print(LANG_TYPE_MODE_LOG, new_lang_type));
    // this is an actual tagged enum
    return load_struct_literal(new_block, tast_struct_literal_new(
        old_lit->pos,
        members,
        util_literal_name_new(),
        new_lang_type
    ));
}

static Name load_raw_union_lit(Ir_block* new_block, Tast_raw_union_lit* old_lit) {
    Tast_def* union_def_ = NULL;
    unwrap(symbol_lookup(&union_def_, llvm_lang_type_get_str(LANG_TYPE_MODE_LOG, rm_tuple_lang_type(old_lit->lang_type, (Pos) {0}))));
    Tast_raw_union_def* union_def = tast_raw_union_def_unwrap(union_def_);
    Tast_variable_def* active_memb = vec_at(&union_def->base.members, (size_t)old_lit->tag->data);

    Tast_variable_def* new_var = tast_variable_def_new(
        union_def->pos,
        old_lit->lang_type,
        false,
        util_literal_name_new()
    );
    unwrap(symbol_add(tast_variable_def_wrap(new_var)));
    load_variable_def(new_block, new_var);
    Tast_member_access* access = tast_member_access_new(
        new_var->pos,
        active_memb->lang_type,
        active_memb->name.base,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(new_var->pos, new_var))
    );

    if (active_memb->lang_type.type != LANG_TYPE_VOID) {
        Tast_assignment* assign = tast_assignment_new(
            new_var->pos,
            tast_member_access_wrap(access),
            old_lit->item
        );
        load_assignment(new_block, assign);
    }

    return load_symbol(new_block, tast_symbol_new_from_variable_def(new_var->pos, new_var));
}

static Name load_literal(Ir_block* new_block, Tast_literal* old_lit) {
    switch (old_lit->type) {
        case TAST_STRING:
            return load_string(tast_string_unwrap(old_lit));
        case TAST_VOID:
            return load_void(tast_void_unwrap(old_lit)->pos);
        case TAST_ENUM_TAG_LIT:
            return load_enum_tag_lit(tast_enum_tag_lit_unwrap(old_lit));
        case TAST_CHAR:
            return load_char(tast_char_unwrap(old_lit));
        case TAST_INT:
            return load_number(tast_int_unwrap(old_lit));
        case TAST_FLOAT:
            return load_float(tast_float_unwrap(old_lit));
        case TAST_FUNCTION_LIT:
            return load_function_lit(tast_function_lit_unwrap(old_lit));
        case TAST_ENUM_LIT:
            return load_enum_lit(new_block, tast_enum_lit_unwrap(old_lit));
        case TAST_RAW_UNION_LIT:
            return load_raw_union_lit(new_block, tast_raw_union_lit_unwrap(old_lit));
    }
    unreachable("");
}

static Name load_ptr_symbol(Ir_block* new_block, Tast_symbol* old_sym) {
    if (old_sym->base.lang_type.type == LANG_TYPE_VOID) {
        msg(DIAG_ASSIGNMENT_TO_VOID, old_sym->pos, "cannot assign to void\n");
    }

    Tast_def* var_def_ = NULL;
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, old_sym->base.name));
    unwrap(symbol_lookup(&var_def_, old_sym->base.name));
    Ir_variable_def* var_def = load_variable_def_clone(tast_variable_def_unwrap(var_def_));
    Ir* alloca = NULL;
    if (!alloca_lookup(&alloca, var_def->name_corr_param)) {
        load_variable_def(new_block, tast_variable_def_unwrap(var_def_));
        unwrap(alloca_lookup(&alloca, var_def->name_corr_param));
    }

    assert(var_def);

    //Lang_type new_lang_type = rm_tuple_lang_type(old_sym->lang_type, old_sym->pos);
    switch (old_sym->base.lang_type.type) {
        case LANG_TYPE_ENUM:
            // TODO: should this always be interpreted as loading the tag of the enum?
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_VOID:
            // fallthrough
        case LANG_TYPE_TUPLE:
            // fallthrough
        case LANG_TYPE_FN:
            // fallthrough
        return ir_tast_get_name(alloca);
    }
    unreachable("");


}

static Name load_ptr_enum_callee(Ir_block* new_block, Tast_enum_callee* old_callee) {
    (void) new_block;
    (void) env;
    (void) old_callee;

    //Tast_def* var_def_ = NULL;
    //unwrap(symbol_lookup(&var_def_, old_callee->name));
    //return load_ptr_member_access(new_block, tast_member_access_new(
    //    old_callee->pos,
    //    old_callee->lang_type,
    //    old_callee->
    //));
    todo();

    //return ir_tast_get_name(alloca);
}

static Name load_symbol(Ir_block* new_block, Tast_symbol* old_sym) {
    Pos pos = tast_symbol_get_pos(old_sym);

    Name ptr = load_ptr_symbol(new_block, old_sym);

    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        pos,
        ptr,
        rm_tuple_lang_type(old_sym->base.lang_type, old_sym->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_binary_short_circuit(Ir_block* new_block, Tast_binary* old_bin) {
    BINARY_TYPE if_true_type = {0};
    int if_false_val = 0;

    switch (old_bin->token_type) {
        case BINARY_LOGICAL_AND:
            if_true_type = BINARY_NOT_EQUAL;
            if_false_val = 0;
            break;
        case BINARY_LOGICAL_OR:
            if_true_type = BINARY_DOUBLE_EQUAL;
            if_false_val = 1;
            break;
        default:
            unreachable("");
    }

    Tast_variable_def* new_var = tast_variable_def_new(
        old_bin->pos,
        lang_type_new_u1(),
        false,
        util_literal_name_new()
    );
    unwrap(sym_tbl_add(tast_variable_def_wrap(new_var)));

    Tast_stmt_vec if_true_stmts = {0};
    vec_append(&a_main, &if_true_stmts, tast_expr_wrap(tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_bin->pos, new_var)),
        old_bin->rhs
    ))));

    Tast_stmt_vec if_false_stmts = {0};
    vec_append(&a_main, &if_false_stmts, tast_expr_wrap(tast_assignment_wrap(tast_assignment_new(
        old_bin->pos,
        tast_symbol_wrap(tast_symbol_new_from_variable_def(old_bin->pos, new_var)),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t(if_false_val, TOKEN_INT_LITERAL, old_bin->pos)
        )
    ))));

    Tast_if* if_true = tast_if_new(
        old_bin->pos,
        tast_condition_new(old_bin->pos, tast_binary_wrap(tast_binary_new(
            old_bin->pos,
            old_bin->lhs,
            tast_literal_wrap(
                util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
            ),
            if_true_type,
            lang_type_new_u1()
        ))),
        // TODO: load inner expr in block, and update new symbol
        tast_block_new(
            old_bin->pos,
            if_true_stmts,
            old_bin->pos,
            lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN)),
            symbol_collection_new(new_block->scope_id)
        ),
        lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
    );

    Tast_if* if_false = tast_if_new(
        old_bin->pos,
        tast_condition_new(old_bin->pos, tast_binary_wrap(tast_binary_new(
            old_bin->pos,
            tast_literal_wrap(
                util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
            ),
            tast_literal_wrap(
                util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, old_bin->pos)
            ),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        ))),
        // TODO: load inner expr in block, and update new symbol
        tast_block_new(
            old_bin->pos,
            if_false_stmts,
            old_bin->pos,
            lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN)),
            symbol_collection_new(new_block->scope_id)
        ),
        lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN))
    );

    Tast_if_vec ifs = {0};
    vec_append(&a_main, &ifs, if_true);
    vec_append(&a_main, &ifs, if_false);
    Tast_if_else_chain* if_else = tast_if_else_chain_new(old_bin->pos, ifs, false);
    
    load_variable_def(new_block, new_var);
    load_if_else_chain(new_block, if_else);
    return load_symbol(new_block, tast_symbol_new_from_variable_def(old_bin->pos, new_var));
}

static Name load_binary(Ir_block* new_block, Tast_binary* old_bin) {
    if (binary_is_short_circuit(old_bin->token_type)) {
        return load_binary_short_circuit(new_block, old_bin);
    }

    Ir_binary* new_bin = ir_binary_new(
        old_bin->pos,
        load_expr(new_block, old_bin->lhs),
        load_expr(new_block, old_bin->rhs),
        old_bin->token_type,
        rm_tuple_lang_type(old_bin->lang_type, old_bin->pos),
        util_literal_name_new()
    );

    unwrap(alloca_add(ir_expr_wrap(ir_operator_wrap(ir_binary_wrap(new_bin)))));

    vec_append(&a_main, &new_block->children, ir_expr_wrap(ir_operator_wrap(ir_binary_wrap(new_bin))));
    return new_bin->name;
}

static Name load_deref(Ir_block* new_block, Tast_unary* old_unary) {
    assert(old_unary->token_type == UNARY_DEREF);

    Name ptr = load_expr(new_block, old_unary->child);
    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_unary->pos,
        ptr,
        rm_tuple_lang_type(old_unary->lang_type, old_unary->pos),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_unary(Ir_block* new_block, Tast_unary* old_unary) {
    switch (old_unary->token_type) {
        case UNARY_DEREF:
            return load_deref(new_block, old_unary);
        case UNARY_REFER:
            return load_ptr_expr(new_block, old_unary->child);
        case UNARY_COUNTOF:
            unreachable("this should have been eliminated in the type checking pass");
        case UNARY_SIZEOF: {
            uint32_t size = sizeof_llvm_lang_type(rm_tuple_lang_type(
                tast_expr_get_lang_type(old_unary->child), old_unary->pos
            ));
            return load_number(tast_int_new(old_unary->pos, size, lang_type_new_usize()));
        }
        case UNARY_UNSAFE_CAST:
            switch (old_unary->lang_type.type) {
                case LANG_TYPE_ENUM:
                    return load_expr(new_block, old_unary->child);
                case LANG_TYPE_STRUCT:
                    return load_expr(new_block, old_unary->child);
                case LANG_TYPE_RAW_UNION:
                    if (lang_type_is_equal(tast_expr_get_lang_type(old_unary->child), old_unary->lang_type)) {
                        return load_expr(new_block, old_unary->child);
                    }
                    break;
                default:
                    break;
            }

            Name new_child = load_expr(new_block, old_unary->child);
            (void) new_child;
            if (llvm_lang_type_is_equal(rm_tuple_lang_type(old_unary->lang_type, old_unary->pos), lang_type_from_get_name(new_child))) {
                return new_child;
            }

            if (lang_type_get_pointer_depth(old_unary->lang_type) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(old_unary->child)) > 0) {
                return new_child;
            }

            Ir_unary* new_unary = ir_unary_new(
                old_unary->pos,
                new_child,
                old_unary->token_type,
                rm_tuple_lang_type(old_unary->lang_type, old_unary->pos),
                util_literal_name_new()
            );
            unwrap(alloca_add(ir_expr_wrap(ir_operator_wrap(ir_unary_wrap(new_unary)))));

            vec_append(&a_main, &new_block->children, ir_expr_wrap(ir_operator_wrap(ir_unary_wrap(new_unary))));
            return new_unary->name;
        case UNARY_NOT:
            unreachable("not should not still be present here");
    }
    unreachable("");
}

static Name load_operator(Ir_block* new_block, Tast_operator* old_oper) {
    switch (old_oper->type) {
        case TAST_BINARY:
            return load_binary(new_block, tast_binary_unwrap(old_oper));
        case TAST_UNARY:
            return load_unary(new_block, tast_unary_unwrap(old_oper));
    }
    unreachable("");
}

static Name load_ptr_member_access(Ir_block* new_block, Tast_member_access* old_access) {
    Name new_callee = load_ptr_expr(new_block, old_access->callee);

    Tast_def* def = NULL;
    unwrap(symbol_lookup(&def, llvm_lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type_from_get_name(new_callee))));

    int64_t struct_index = {0};
    switch (def->type) {
        case TAST_STRUCT_DEF: {
            Tast_struct_def* struct_def = tast_struct_def_unwrap(def);
            log(LOG_DEBUG, FMT"\n", tast_struct_def_print(struct_def));
            log(LOG_DEBUG, FMT"\n", strv_print(old_access->member_name));
            struct_index = tast_get_member_index(&struct_def->base, old_access->member_name);
            break;
        }
        case TAST_RAW_UNION_DEF: {
            struct_index = 0;
            break;
        }
        default:
            unreachable("");
    }
    
    Ir_load_element_ptr* new_load = ir_load_element_ptr_new(
        old_access->pos,
        rm_tuple_lang_type(old_access->lang_type, old_access->pos),
        struct_index,
        new_callee,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_element_ptr_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_element_ptr_wrap(new_load));
    return new_load->name_self;
}

static Name load_ptr_index(Ir_block* new_block, Tast_index* old_index) {
    Ir_array_access* new_load = ir_array_access_new(
        old_index->pos,
        rm_tuple_lang_type(old_index->lang_type, old_index->pos),
        load_expr(new_block, old_index->index),
        load_expr(new_block, old_index->callee),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_array_access_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_array_access_wrap(new_load));
    return new_load->name_self;
}

static Name load_member_access(Ir_block* new_block, Tast_member_access* old_access) {
    Name ptr = load_ptr_member_access(new_block, old_access);

    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_access->pos,
        ptr,
        lang_type_from_get_name(ptr),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_index(Ir_block* new_block, Tast_index* old_index) {
    Name ptr = load_ptr_index(new_block, old_index);

    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_index->pos,
        ptr,
        lang_type_from_get_name(ptr),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_ptr_enum_get_tag(Ir_block* new_block, Tast_enum_get_tag* old_access) {
    Tast_def* enum_def_ = NULL;
    unwrap(symbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, tast_expr_get_lang_type(old_access->callee))));
    Tast_enum_def* enum_def = tast_enum_def_unwrap(enum_def_);
    Name new_enum = load_ptr_expr(new_block, old_access->callee);
    
    size_t largest_idx = struct_def_base_get_idx_largest_member(enum_def->base);
    if (vec_at(&enum_def->base.members, largest_idx)->lang_type.type == LANG_TYPE_VOID) {
        // all enum inner types are void; new_enum will actually just be a number
        return new_enum;
    }

    Ir_load_element_ptr* new_tag = ir_load_element_ptr_new(
        old_access->pos,
        rm_tuple_lang_type(lang_type_new_usize(), POS_BUILTIN),
        0,
        new_enum,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_element_ptr_wrap(new_tag)));
    vec_append(&a_main, &new_block->children, ir_load_element_ptr_wrap(new_tag));

    return new_tag->name_self;
}

static Name load_enum_get_tag(Ir_block* new_block, Tast_enum_get_tag* old_access) {
    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_access->pos,
        load_ptr_enum_get_tag(new_block, old_access),
        rm_tuple_lang_type(lang_type_new_usize(), POS_BUILTIN),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_ptr_enum_access(Ir_block* new_block, Tast_enum_access* old_access) {
    Tast_def* enum_def_ = NULL;
    unwrap(symbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, tast_expr_get_lang_type(old_access->callee))));
    Tast_enum_def* enum_def = tast_enum_def_unwrap(enum_def_);
    Name new_callee = load_ptr_expr(new_block, old_access->callee);
    Tast_raw_union_def* union_def = get_raw_union_def_from_enum_def(enum_def);
    
    Ir_load_element_ptr* new_union = ir_load_element_ptr_new(
        old_access->pos,
        rm_tuple_lang_type(tast_raw_union_def_get_lang_type(union_def), union_def->pos),
        1,
        new_callee,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_element_ptr_wrap(new_union)));
    vec_append(&a_main, &new_block->children, ir_load_element_ptr_wrap(new_union));

    Ir_load_element_ptr* new_item = ir_load_element_ptr_new(
        old_access->pos,
        rm_tuple_lang_type(old_access->lang_type, old_access->pos),
        0,
        new_union->name_self,
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_element_ptr_wrap(new_item)));
    vec_append(&a_main, &new_block->children, ir_load_element_ptr_wrap(new_item));

    return new_item->name_self;
}

static Name load_enum_access(Ir_block* new_block, Tast_enum_access* old_access) {
    Name ptr = load_ptr_enum_access(new_block, old_access);

    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_access->pos,
        ptr,
        lang_type_from_get_name(ptr),
        util_literal_name_new()
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));
    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_enum_case(Tast_enum_case* old_case) {
    return load_enum_tag_lit(old_case->tag);
}

static Name load_tuple(Ir_block* new_block, Tast_tuple* old_tuple) {
    Lang_type new_lang_type = lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(
         old_tuple->lang_type, old_tuple->pos
    ));
    Name new_lit = load_struct_literal(new_block, tast_struct_literal_new(
        old_tuple->pos, old_tuple->members, util_literal_name_new(), new_lang_type
    ));

    Ir* dummy = NULL;
    unwrap(alloca_lookup(&dummy, new_lit));
    return new_lit;
}

// TODO: make separate tuple types for lhs and rhs
static Name load_tuple_ptr(Ir_block* new_block, Tast_tuple* old_tuple) {
    (void) new_block;
    Lang_type new_lang_type = lang_type_struct_const_wrap(rm_tuple_lang_type_tuple(
         old_tuple->lang_type, old_tuple->pos
    ));
    (void) new_lang_type;

    for (size_t idx = 0; idx < old_tuple->members.info.count; idx++) {
        todo();
        //Tast_expr* curr_lhs = vec_at(&old_tuple->members, idx);
        todo();
    }

    //Ir* dummy = NULL;
    //unwrap(alloca_lookup(&dummy, new_lit));
    todo();
    //return new_lit;
}

static Name load_expr(Ir_block* new_block, Tast_expr* old_expr) {
    switch (old_expr->type) {
        case TAST_ASSIGNMENT:
            return load_assignment(new_block, tast_assignment_unwrap(old_expr));
        case TAST_FUNCTION_CALL:
            return load_function_call(new_block, tast_function_call_unwrap(old_expr));
        case TAST_LITERAL:
            return load_literal(new_block, tast_literal_unwrap(old_expr));
        case TAST_SYMBOL:
            return load_symbol(new_block, tast_symbol_unwrap(old_expr));
        case TAST_OPERATOR:
            return load_operator(new_block, tast_operator_unwrap(old_expr));
        case TAST_MEMBER_ACCESS:
            return load_member_access(new_block, tast_member_access_unwrap(old_expr));
        case TAST_INDEX:
            return load_index(new_block, tast_index_unwrap(old_expr));
        case TAST_ENUM_ACCESS:
            return load_enum_access(new_block, tast_enum_access_unwrap(old_expr));
        case TAST_ENUM_CASE:
            return load_enum_case(tast_enum_case_unwrap(old_expr));
        case TAST_STRUCT_LITERAL:
            return load_struct_literal(new_block, tast_struct_literal_unwrap(old_expr));
        case TAST_TUPLE:
            return load_tuple(new_block, tast_tuple_unwrap(old_expr));
        case TAST_ENUM_CALLEE:
            todo();
        case TAST_ENUM_GET_TAG:
            return load_enum_get_tag(new_block, tast_enum_get_tag_unwrap(old_expr));
        case TAST_IF_ELSE_CHAIN:
            return load_if_else_chain(new_block, tast_if_else_chain_unwrap(old_expr));
        case TAST_MODULE_ALIAS:
            unreachable("");
    }
    unreachable("");
}

static Ir_function_params* load_function_parameters(
    Ir_block* new_fun_body,
    Lang_type* new_lang_type,
    Lang_type rtn_type,
    Tast_function_params* old_params
) {
    Ir_function_params* new_params = do_function_def_alloca(new_lang_type, rtn_type, new_fun_body, old_params);

    for (size_t idx = 0; idx < new_params->params.info.count; idx++) {
        Ir_variable_def* param = vec_at(&new_params->params, idx);

        Ir* dummy = NULL;

        bool is_struct = llvm_is_struct_like(param->lang_type.type);

        if (!params.backend_info.struct_rtn_through_param || !is_struct) {
            unwrap(alloca_add(ir_def_wrap(ir_variable_def_wrap(param))));

            Ir_store_another_ir* new_store = ir_store_another_ir_new(
                param->pos,
                param->name_self,
                param->name_corr_param,
                param->lang_type,
                util_literal_name_new_prefix(sv("load_function_parameters_store"))
            );
            unwrap(alloca_add(ir_store_another_ir_wrap(new_store)));

            vec_append(&a_main, &new_fun_body->children, ir_store_another_ir_wrap(new_store));
        }

        unwrap(alloca_lookup(&dummy, param->name_corr_param));
    }

    return new_params;
}

static void load_function_def(Tast_function_def* old_fun_def) {
    Name old_fun_name = name_parent_fn;
    name_parent_fn = old_fun_def->decl->name;
    Pos pos = old_fun_def->pos;

    Ir_function_decl* new_decl = ir_function_decl_new(
        pos,
        NULL,
        rm_tuple_lang_type(old_fun_def->decl->return_type, old_fun_def->pos),
        old_fun_def->decl->name
    );

    Ir_function_def* new_fun_def = ir_function_def_new(
        pos,
        util_literal_name_new(),
        new_decl,
        ir_block_new(
            pos,
            util_literal_name_new(),
            (Ir_vec) {0},
            old_fun_def->body->pos_end,
            old_fun_def->body->scope_id
        )
    );

    Lang_type new_lang_type = {0};
    new_fun_def->decl->params = load_function_parameters(
         new_fun_def->body,
         &new_lang_type,
         old_fun_def->decl->return_type,
         old_fun_def->decl->params
    );
    new_fun_def->decl->return_type = rm_tuple_lang_type(new_lang_type, old_fun_def->pos);
    bool rtn_in_block = false;
    load_block_stmts(
        &rtn_in_block,
        new_fun_def->body,
        old_fun_def->body->children,
        DEFER_PARENT_OF_FUN,
        old_fun_def->pos,
        old_fun_def->decl->return_type
    );

    unwrap(alloca_add(ir_def_wrap(ir_function_def_wrap(new_fun_def))));
    name_parent_fn = old_fun_name;
}

static void load_function_decl(Tast_function_decl* old_fun_decl) {
    unwrap(alloca_add(ir_def_wrap(ir_function_decl_wrap(load_function_decl_clone(old_fun_decl)))));
}

static Name load_return(Ir_block* new_block, Tast_return* old_return) {
    Pos pos = old_return->pos;

    Tast_def* fun_def_ = NULL;
    unwrap(symbol_lookup(&fun_def_, name_parent_fn));

    Tast_function_decl* fun_decl = NULL;
    switch (fun_def_->type) {
        case TAST_FUNCTION_DEF:
            fun_decl = tast_function_def_unwrap(fun_def_)->decl;
            break;
        case TAST_FUNCTION_DECL:
            fun_decl = tast_function_decl_unwrap(fun_def_);
            break;
        default:
            unreachable("");
    }

    //assert(fun_decl->return_type->lang_type.info.count == 1);
    Llvm_lang_type rtn_type = rm_tuple_lang_type(fun_decl->return_type, fun_decl->pos);

    bool rtn_is_struct = llvm_is_struct_like(rtn_type.type);

    if (params.backend_info.struct_rtn_through_param && rtn_is_struct) {
        Ir* dest_ = NULL;
        unwrap(alloca_lookup(&dest_, struct_rtn_name_parent_function));
        Name dest = struct_rtn_name_parent_function;
        Name src = load_expr(new_block, old_return->child);

        Ir_store_another_ir* new_store = ir_store_another_ir_new(
            pos,
            src,
            dest,
            rtn_type,
            util_literal_name_new_prefix(sv("return_store_another"))
        );
        vec_append(&a_main, &new_block->children, ir_store_another_ir_wrap(new_store));
        
        Tast_void* new_void = tast_void_new(old_return->pos);
        Ir_return* new_return = ir_return_new(
            pos,
            util_literal_name_new(),
            load_literal(new_block, tast_void_wrap(new_void)),
            old_return->is_auto_inserted
        );
        vec_append(&a_main, &new_block->children, ir_return_wrap(new_return));
    } else {
        Name result = load_expr(new_block, old_return->child);
        Ir_return* new_return = ir_return_new(
            pos,
            util_literal_name_new(),
            result,
            old_return->is_auto_inserted
        );

        vec_append(&a_main, &new_block->children, ir_return_wrap(new_return));
    }

    return (Name) {0};
}

static Name load_assignment_internal(const char* file, int line, Ir_block* new_block, Tast_assignment* old_assign) {
    assert(old_assign->lhs);
    assert(old_assign->rhs);

    Pos pos = old_assign->pos;

    log(LOG_DEBUG, FMT"\n", tast_expr_print(old_assign->lhs));
    Name new_lhs = load_ptr_expr(new_block, old_assign->lhs);
    Name new_rhs = load_expr(new_block, old_assign->rhs);

    Ir_store_another_ir* new_store = ir_store_another_ir_new_internal(
        pos,
        (Loc) {.file = file, .line = line},
        new_rhs,
        new_lhs,
        rm_tuple_lang_type(tast_expr_get_lang_type(old_assign->lhs), old_assign->pos),
        util_literal_name_new_prefix(sv("store_for_assign"))
    );
    unwrap(alloca_add(ir_store_another_ir_wrap(new_store)));

    assert(new_store->ir_src.base.count > 0);
    assert(new_store->ir_dest.base.count > 0);
    assert(old_assign->rhs);

    vec_append(&a_main, &new_block->children, ir_store_another_ir_wrap(new_store));

    return new_store->name;
}

static void load_variable_def(Ir_block* new_block, Tast_variable_def* old_var_def) {
    Ir_variable_def* new_var_def = load_variable_def_clone(old_var_def);

    Ir* alloca = NULL;
    if (!alloca_lookup(&alloca, new_var_def->name_self)) {
        alloca = ir_alloca_wrap(add_load_and_store_alloca_new(new_var_def));
        vec_insert(&a_main, &new_block->children, 0, alloca);
    }

    vec_append(&a_main, &new_block->children, ir_def_wrap(ir_variable_def_wrap(new_var_def)));

    assert(alloca);
}

static void load_struct_def(Tast_struct_def* old_def) {
    all_tbl_add(ir_def_wrap(ir_struct_def_wrap(load_struct_def_clone(old_def))));

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, old_def->base.name)) {
        Tast_struct_def* new_def = tast_struct_def_new(old_def->pos, (Struct_def_base) {
            .members = old_def->base.members, 
            .name = old_def->base.name
        });
        unwrap(sym_tbl_add(tast_struct_def_wrap(new_def)));
        load_struct_def(new_def);
    }
}

static Ir_block* if_statement_to_branch(Tast_if* if_statement, Name next_if) {
    Tast_block* old_block = if_statement->body;
    bool rtn_in_block = false;
    Ir_block* inner_block = load_block(
        &rtn_in_block,
        old_block,
        DEFER_PARENT_OF_IF,
        if_statement->yield_type
    );
    Ir_block* new_block = ir_block_new(
        old_block->pos,
        util_literal_name_new(),
        (Ir_vec) {0},
        inner_block->pos_end,
        if_statement->body->scope_id
    );


    Tast_condition* if_cond = if_statement->condition;

    Tast_operator* old_oper = if_cond->child;

    Name if_body = util_literal_name_new_prefix(sv("if_body"));

    if_for_add_cond_goto(old_oper, new_block, if_body, next_if);

    add_label(new_block, if_body, old_block->pos);

    vec_extend(&a_main, &new_block->children, &inner_block->children);

    // is_rtn_check
    // TODO: this should be in load_block_stmts?
    Name after_is_rtn = util_literal_name_new_prefix(sv("after_is_rtn_check_if_to_branch"));
    Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
    unwrap(pairs->info.count > 0 && "not implemented");
    if_for_add_cond_goto(
        // if this condition evaluates to true, we are not returning right now
        tast_binary_wrap(tast_binary_new(
            if_statement->pos,
            tast_symbol_wrap(tast_symbol_new(if_statement->pos, (Sym_typed_base) {
                .lang_type = tast_lang_type_from_name(defered_collections.is_rtning),
                .name = defered_collections.is_rtning
            })),
            tast_literal_wrap(tast_int_wrap(tast_int_new(if_statement->pos, 0, lang_type_new_u1()))),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        )),
        new_block,
        after_is_rtn,
        vec_top(pairs).label->name
    );
    add_label(new_block, after_is_rtn, if_statement->pos);

    Ir_goto* jmp_to_after_chain = ir_goto_new(
        old_block->pos,
        util_literal_name_new(),
        label_if_after
    );
    vec_append(&a_main, &new_block->children, ir_goto_wrap(jmp_to_after_chain));

    return new_block;
}

static Name if_else_chain_to_branch(Ir_block** new_block, Tast_if_else_chain* if_else) {
    assert(if_else->tasts.info.count > 0);
    Name old_label_if_after = label_if_after;

    *new_block = ir_block_new(
        if_else->pos,
        util_literal_name_new(),
        (Ir_vec) {0},
        (Pos) {0},
        symbol_collection_new(scope_get_parent_tbl_lookup(vec_at(&if_else->tasts, 0)->body->scope_id))
    );

    Tast_variable_def* yield_dest = NULL;
    if (tast_if_else_chain_get_lang_type(if_else).type != LANG_TYPE_VOID) {
        yield_dest = tast_variable_def_new(
            (*new_block)->pos,
            tast_if_else_chain_get_lang_type(if_else),
            false,
            util_literal_name_new_prefix(sv("yield_dest"))
        );
        unwrap(symbol_add(tast_variable_def_wrap(yield_dest)));
        load_variable_def(*new_block, yield_dest);
        load_break_symbol_name = yield_dest->name;
    }

    Name if_after = util_literal_name_new_prefix(sv("if_after"));

    Name old_label_if_break = label_if_break;
    if (if_else->is_switch) {
        label_if_break = if_after;
    } else if (label_after_for.base.count > 0) {
        label_if_break = label_after_for;
    } else {
        label_if_break = if_after;
    }
    label_if_after = if_after;
    
    Ir* dummy = NULL;
    Tast_def* dummy_def = NULL;

    Name next_if = {0};
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        if (idx + 1 == if_else->tasts.info.count) {
            next_if = util_literal_name_new_prefix(sv("dummy_next_if"));
        } else {
            next_if = util_literal_name_new_prefix(sv("next_if"));
        }

        assert(label_if_break.base.count > 0);
        Ir_block* if_block = if_statement_to_branch(vec_at(&if_else->tasts, idx), next_if);
        scope_get_parent_tbl_update(vec_at(&if_else->tasts, idx)->body->scope_id, (*new_block)->scope_id);
        vec_append(&a_main, &(*new_block)->children, ir_block_wrap(if_block));

        if (idx + 1 < if_else->tasts.info.count) {
            assert(!alloca_lookup(&dummy, next_if));
            add_label((*new_block), next_if, vec_at(&if_else->tasts, idx)->pos);
            assert(alloca_lookup(&dummy, next_if));
        } else {
            //assert(strv_is_equal(next_if, label_if_break));
        }
    }

    assert(!symbol_lookup(&dummy_def, next_if));

    // is_rtn_check
    Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
    unwrap(pairs->info.count > 0 && "not implemented");
    if_for_add_cond_goto(
        // if this condition evaluates to true, we are not returning right now
        tast_binary_wrap(tast_binary_new(
            if_else->pos,
            tast_symbol_wrap(tast_symbol_new(if_else->pos, (Sym_typed_base) {
                .lang_type = tast_lang_type_from_name(defered_collections.is_rtning),
                .name = defered_collections.is_rtning
            })),
            tast_literal_wrap(tast_int_wrap(tast_int_new(if_else->pos, 0, lang_type_new_u1()))),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        )),
        *new_block,
        if_after,
        vec_top(pairs).label->name
    );
    add_label((*new_block), if_after, if_else->pos);

    // is_brk_check
    Name after_is_brk = util_literal_name_new_prefix(sv("after_is_brk_check_if_else_chain_to_branch"));
    unwrap(pairs->info.count > 0 && "not implemented");
    if_for_add_cond_goto(
        // if this condition evaluates to true, we are not breaking right now
        tast_binary_wrap(tast_binary_new(
            if_else->pos,
            tast_symbol_wrap(tast_symbol_new(if_else->pos, (Sym_typed_base) {
                .lang_type = tast_lang_type_from_name(vec_top(&defered_collections.coll_stack).is_brking),
                .name = vec_top(&defered_collections.coll_stack).is_brking
            })),
            tast_literal_wrap(tast_int_wrap(tast_int_new(if_else->pos, 0, lang_type_new_u1()))),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        )),
        *new_block,
        after_is_brk,
        vec_top(pairs).label->name
    );
    add_label((*new_block), after_is_brk, if_else->pos);

    // is_cont_check
    unwrap(pairs->info.count > 0 && "not implemented");
    if_for_add_cond_goto(
        // if this condition evaluates to true, we are not breaking right now
        tast_binary_wrap(tast_binary_new(
            if_else->pos,
            tast_symbol_wrap(tast_symbol_new(if_else->pos, (Sym_typed_base) {
                .lang_type = tast_lang_type_from_name(vec_top(&defered_collections.coll_stack).is_conting),
                .name = vec_top(&defered_collections.coll_stack).is_conting
            })),
            tast_literal_wrap(tast_int_wrap(tast_int_new(if_else->pos, 0, lang_type_new_u1()))),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        )),
        *new_block,
        next_if,
        vec_top(pairs).label->name
    );
    add_label((*new_block), next_if, if_else->pos);
    assert(alloca_lookup(&dummy, next_if));

    label_if_break = old_label_if_break;
    label_if_after = old_label_if_after;

    if (tast_if_else_chain_get_lang_type(if_else).type == LANG_TYPE_VOID) {
        return (Name) {0};
    } else {
        return load_symbol(*new_block, tast_symbol_new_from_variable_def(yield_dest->pos, yield_dest));
    }
    unreachable("");
}

static Name load_if_else_chain(Ir_block* new_block, Tast_if_else_chain* old_if_else) {
    Ir_block* new_if_else = NULL;
    Name result = if_else_chain_to_branch(&new_if_else, old_if_else);
    vec_append(&a_main, &new_block->children, ir_block_wrap(new_if_else));

    return result;
}

static Ir_block* for_with_cond_to_branch(bool* rtn_in_block, Tast_for_with_cond* old_for) {
    Name old_after_for = label_after_for;
    Name old_if_continue = label_if_continue;
    Name old_if_break = label_if_break;

    Pos pos = old_for->pos;

    Ir_block* new_branch_block = ir_block_new(
        pos,
        util_literal_name_new(),
        (Ir_vec) {0},
        old_for->body->pos_end,
        old_for->body->scope_id
    );

    size_t for_count = 0;
    for (size_t idx_ = defered_collections.coll_stack.info.count; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;
        if (vec_at(&defered_collections.coll_stack, idx).parent_of == DEFER_PARENT_OF_FOR) {
            for_count++;
        }
    }
    String for_template = {0};
    string_extend_cstr(&a_main, &for_template, "for_");
    string_extend_size_t(&a_main, &for_template, for_count);
    string_extend_cstr(&a_main, &for_template, "_");

    String check_cond_ = string_clone(&a_main, for_template);
    string_extend_cstr(&a_main, &check_cond_, "check_cond");

    String after_chk_ = string_clone(&a_main, for_template);
    string_extend_cstr(&a_main, &after_chk_, "after_chk");

    String after_loop_ = string_clone(&a_main, for_template);
    string_extend_cstr(&a_main, &after_loop_, "after_loop");

    Tast_operator* operator = old_for->condition->child;
    label_if_continue = util_literal_name_new_prefix(string_to_strv(check_cond_));
    Ir_goto* jmp_to_check_cond_label = ir_goto_new(old_for->pos, util_literal_name_new(), label_if_continue);
    Name after_check_label = util_literal_name_new_prefix(string_to_strv(after_chk_));
    Name after_for_loop_label = util_literal_name_new_prefix(string_to_strv(after_loop_));

    label_after_for = after_for_loop_label;
    label_if_break = after_for_loop_label;

    assert(label_if_continue.base.count > 0);

    vec_append(&a_main, &new_branch_block->children, ir_goto_wrap(jmp_to_check_cond_label));

    add_label(new_branch_block, label_if_continue, pos);

    load_operator(new_branch_block, operator);

    if_for_add_cond_goto(
        operator,
        new_branch_block,
        after_check_label,
        after_for_loop_label
    );

    add_label(new_branch_block, after_check_label, pos);
    Name after_is_rtn = util_literal_name_new_prefix(sv("after_is_rtn_check"));
    Name after_inner_block = util_literal_name_new_prefix(sv("after_inner_block"));

    load_block_stmts(
        rtn_in_block,
        new_branch_block,
        old_for->body->children,
        DEFER_PARENT_OF_FOR,
        old_for->pos,
        lang_type_void_const_wrap(lang_type_void_new(pos)) /* TODO */
    );
    add_label(new_branch_block, after_inner_block, pos);

    vec_append(&a_main, &new_branch_block->children, ir_goto_wrap(
        ir_goto_new(old_for->pos, util_literal_name_new(), label_if_continue)
    ));
    add_label(new_branch_block, after_for_loop_label, pos);

    // is_rtn_check
    Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
    unwrap(pairs->info.count > 0 && "not implemented");
    if_for_add_cond_goto(
        // if this condition evaluates to true, we are not returning right now
        tast_binary_wrap(tast_binary_new(
            old_for->pos,
            tast_symbol_wrap(tast_symbol_new(old_for->pos, (Sym_typed_base) {
                .lang_type = tast_lang_type_from_name(defered_collections.is_rtning),
                .name = defered_collections.is_rtning
            })),
            tast_literal_wrap(tast_int_wrap(tast_int_new(old_for->pos, 0, lang_type_new_u1()))),
            BINARY_DOUBLE_EQUAL,
            lang_type_new_u1()
        )),
        new_branch_block,
        after_is_rtn,
        vec_top(pairs).label->name
    );
    add_label(new_branch_block, after_is_rtn, old_for->pos);

    label_if_continue = old_if_continue;
    label_after_for = old_after_for;
    label_if_break = old_if_break;

    return new_branch_block;
}

static void load_for_with_cond(bool* rtn_in_block, Ir_block* new_block, Tast_for_with_cond* old_for) {
    Ir_block* new_for = for_with_cond_to_branch(rtn_in_block, old_for);
    vec_append(&a_main, &new_block->children, ir_block_wrap(new_for));
}

static void load_break(Ir_block* new_block, Tast_break* old_break) {
    if (label_if_break.base.count < 1) {
        return;
    }

    if (old_break->do_break_expr) {
        load_assignment(new_block, tast_assignment_new(
            old_break->pos,
            tast_symbol_wrap(tast_symbol_new(old_break->pos, (Sym_typed_base) {
                .lang_type = tast_expr_get_lang_type(old_break->break_expr),
                .name = load_break_symbol_name
            })),
            old_break->break_expr
        ));
    }

    assert(label_if_break.base.count > 0);
}

static void load_label(Ir_block* new_block, Tast_label* old_label) {
    Ir_label* new_label = ir_label_new(old_label->pos, old_label->name);
    vec_append(&a_main, &new_block->children, ir_def_wrap(ir_label_wrap(new_label)));
    assert(new_label->name.base.count > 0);
    alloca_add(ir_def_wrap(ir_label_wrap(new_label)));
}

static void load_continue(Ir_block* new_block, Tast_continue* old_continue) {
    if (label_if_continue.base.count < 1) {
        msg(
            DIAG_CONTINUE_INVALID_LOCATION, old_continue->pos,
            "continue statement outside of a for loop\n"
        );
        return;
    }

    Ir_goto* new_goto = ir_goto_new(old_continue->pos, util_literal_name_new(), label_if_continue);
    vec_append(&a_main, &new_block->children, ir_goto_wrap(new_goto));
}

static void load_raw_union_def(Tast_raw_union_def* old_def) {
    if (!all_tbl_add(ir_def_wrap(ir_struct_def_wrap(load_raw_union_def_clone(old_def))))) {
        return;
    };

    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, old_def->base.name)) {
        Tast_raw_union_def* new_def = tast_raw_union_def_new(old_def->pos, (Struct_def_base) {
            .members = old_def->base.members, 
            .name = old_def->base.name
        });
        unwrap(sym_tbl_add(tast_raw_union_def_wrap(new_def)));
        load_raw_union_def(new_def);
    }
}

static Name load_ptr_deref(Ir_block* new_block, Tast_unary* old_unary) {
    assert(old_unary->token_type == UNARY_DEREF);

    switch (old_unary->lang_type.type) {
        case LANG_TYPE_STRUCT:
            // TODO: this may not do the right thing for the ir backend. Fix underlying issues instead of relying on this hack for ir?
            // TODO: (if this is nessessary) make params.backend_info.load_ptr_deref_break arg instead of just doing BACKEND_C check this way
            if (params.backend_info.backend == BACKEND_C) {
                break;
            }
            return load_ptr_expr(new_block, old_unary->child);
        case LANG_TYPE_PRIMITIVE:
            break;
        case LANG_TYPE_RAW_UNION:
            if (params.backend_info.backend == BACKEND_C) {
                todo();
                break;
            }
            return load_ptr_expr(new_block, old_unary->child);
        default:
            todo();
    }

    Name ptr = load_ptr_expr(new_block, old_unary->child);
    Ir_load_another_ir* new_load = ir_load_another_ir_new(
        old_unary->pos,
        ptr,
        rm_tuple_lang_type(old_unary->lang_type, old_unary->pos),
        util_literal_name_new_prefix(sv("load_another_ir"))
    );
    unwrap(alloca_add(ir_load_another_ir_wrap(new_load)));
    llvm_lang_type_set_pointer_depth(&new_load->lang_type, llvm_lang_type_get_pointer_depth(new_load->lang_type) + 1);

    vec_append(&a_main, &new_block->children, ir_load_another_ir_wrap(new_load));
    return new_load->name;
}

static Name load_ptr_unary(Ir_block* new_block, Tast_unary* old_unary) {
    switch (old_unary->token_type) {
        case UNARY_DEREF:
            return load_ptr_deref(new_block, old_unary);
        default:
            todo();
    }
    todo();
}

static Name load_ptr_operator(Ir_block* new_block, Tast_operator* old_oper) {
    switch (old_oper->type) {
        case TAST_BINARY:
            todo();
        case TAST_UNARY:
            return load_ptr_unary(new_block, tast_unary_unwrap(old_oper));
        default:
            unreachable("");
    }
}

static Name load_ptr_expr(Ir_block* new_block, Tast_expr* old_expr) {
    switch (old_expr->type) {
        case TAST_SYMBOL:
            return load_ptr_symbol(new_block, tast_symbol_unwrap(old_expr));
        case TAST_MEMBER_ACCESS:
            return load_ptr_member_access(new_block, tast_member_access_unwrap(old_expr));
        case TAST_OPERATOR:
            return load_ptr_operator(new_block, tast_operator_unwrap(old_expr));
        case TAST_INDEX:
            return load_ptr_index(new_block, tast_index_unwrap(old_expr));
        case TAST_LITERAL:
            // TODO: deref string literal (detect in src/type_checking.c if possible)
            unreachable("");
        case TAST_FUNCTION_CALL:
            return load_ptr_function_call(new_block, tast_function_call_unwrap(old_expr));
        case TAST_STRUCT_LITERAL:
            return load_ptr_struct_literal(new_block, tast_struct_literal_unwrap(old_expr));
        case TAST_TUPLE:
            unreachable("");
        case TAST_ENUM_CALLEE:
            return load_ptr_enum_callee(new_block, tast_enum_callee_unwrap(old_expr));
        case TAST_ENUM_CASE:
            unreachable("");
        case TAST_ENUM_ACCESS:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_ENUM_GET_TAG:
            return load_ptr_enum_get_tag(new_block, tast_enum_get_tag_unwrap(old_expr));
        case TAST_IF_ELSE_CHAIN:
            todo();
        case TAST_MODULE_ALIAS:
            unreachable("");
    }
    unreachable("");
}

static void load_def(Ir_block* new_block, Tast_def* old_def) {
    switch (old_def->type) {
        case TAST_FUNCTION_DEF:
            load_function_def(tast_function_def_unwrap(old_def));
            return;
        case TAST_FUNCTION_DECL:
            load_function_decl(tast_function_decl_unwrap(old_def));
            return;
        case TAST_VARIABLE_DEF:
            load_variable_def(new_block, tast_variable_def_unwrap(old_def));
            return;
        case TAST_STRUCT_DEF:
            load_struct_def(tast_struct_def_unwrap(old_def));
            return;
        case TAST_RAW_UNION_DEF:
            load_raw_union_def(tast_raw_union_def_unwrap(old_def));
            return;
        case TAST_ENUM_DEF:
            unreachable("enum def should not make it here");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_IMPORT:
            todo();
    }
    unreachable("");
}

typedef Name (*Get_is_brking_or_conting)(const Defer_collection* item);

Name get_is_brking(const Defer_collection* item) {
    return item->is_brking;
}

Name get_is_conting(const Defer_collection* item) {
    return item->is_conting;
}

// TODO: try to come up with a better name for this function
static void load_brking_or_conting_set_etc(Ir_block* new_block, Tast_stmt* old_stmt, bool is_brking) {
    Get_is_brking_or_conting get_is_brking_or_conting = is_brking ? get_is_brking : get_is_conting;
    Defer_collection coll = vec_top(&defered_collections.coll_stack);
    Defer_pair_vec* pairs = &coll.pairs;

    // TODO: extract this into separate function? (and for can use same function as continue, etc.
    Tast_assignment* is_cont_assign = tast_assignment_new(
        tast_stmt_get_pos(old_stmt),
        tast_symbol_wrap(tast_symbol_new(tast_stmt_get_pos(old_stmt), (Sym_typed_base) {
            .lang_type = tast_lang_type_from_name(get_is_brking_or_conting(&coll)),
            .name = get_is_brking_or_conting(&coll)
        })),
        tast_literal_wrap(tast_int_wrap(tast_int_new(tast_stmt_get_pos(old_stmt), 1, lang_type_new_u1())))
    );
    load_assignment(new_block, is_cont_assign);

    // the purpose of these two for loops: 
    //   if we are breaking/continuing out of for loop nested in multiple ifs, etc.,
    //   we need to set is_brking of multiple scopes to break/continue out of for
    bool is_for = false;
    size_t for_pos = 0;
    // these two for loops exclude the top of defered_collections.coll_stack
    assert(defered_collections.coll_stack.info.count > 0 && "will underflow");
    for (size_t idx_ = defered_collections.coll_stack.info.count - 1; idx_ > 0; idx_--) {
        size_t idx = idx_ - 1;
        if (vec_at(&defered_collections.coll_stack, idx).parent_of == DEFER_PARENT_OF_FOR) {
            is_for = true;
            for_pos = idx;
        }
    }
    if (is_for) {
        for (size_t idx_ = defered_collections.coll_stack.info.count - 1; idx_ > 0; idx_--) {
            size_t idx = idx_ - 1;
            if (vec_at(&defered_collections.coll_stack, idx).parent_of == DEFER_PARENT_OF_FOR || vec_at(&defered_collections.coll_stack, idx).parent_of == DEFER_PARENT_OF_IF) {
                Tast_assignment* is_cont_assign_aux = tast_assignment_new(
                    tast_stmt_get_pos(old_stmt),
                    tast_symbol_wrap(tast_symbol_new(tast_stmt_get_pos(old_stmt), (Sym_typed_base) {
                        .lang_type = tast_lang_type_from_name(get_is_brking_or_conting(vec_at_ref(&defered_collections.coll_stack, for_pos))),
                        .name = get_is_brking_or_conting(vec_at_ref(&defered_collections.coll_stack, idx))
                    })),
                    tast_literal_wrap(tast_int_wrap(tast_int_new(tast_stmt_get_pos(old_stmt), 1, lang_type_new_u1())))
                );
                load_assignment(new_block, is_cont_assign_aux);
            }
        }
    }

    if (pairs->info.count > 0) {
        // jump to the top of the defer stack to execute the defered statements
        Ir_goto* new_goto = ir_goto_new(tast_stmt_get_pos(old_stmt), util_literal_name_new(), vec_top(pairs).label->name);
        vec_append(&a_main, &new_block->children, ir_goto_wrap(new_goto));
    }
}

static void load_stmt(bool* rtn_in_block, Ir_block* new_block, Tast_stmt* old_stmt, bool is_defered) {
    switch (old_stmt->type) {
        case TAST_EXPR:
            load_expr(new_block, tast_expr_unwrap(old_stmt));
            return;
        case TAST_DEF:
            load_def(new_block, tast_def_unwrap(old_stmt));
            return;
        case TAST_RETURN: {
            *rtn_in_block = true;

            if (is_defered) {
                load_return(new_block, tast_return_unwrap(old_stmt));
                return;
            }

            Defer_collection coll = vec_at(&defered_collections.coll_stack, 0);

            Tast_return* rtn = tast_return_unwrap(old_stmt);
            if (tast_expr_get_lang_type(coll.rtn_val).type != LANG_TYPE_VOID) {
                Tast_assignment* rtn_assign = tast_assignment_new(
                    tast_stmt_get_pos(old_stmt),
                    tast_symbol_wrap(tast_symbol_new(tast_stmt_get_pos(old_stmt), (Sym_typed_base) {
                        .lang_type = tast_expr_get_lang_type(coll.rtn_val), .name = tast_expr_get_name(coll.rtn_val)
                    })),
                    rtn->child
                );
                load_assignment(new_block, rtn_assign);
            }

            Tast_assignment* is_rtn_assign = tast_assignment_new(
                tast_stmt_get_pos(old_stmt),
                tast_symbol_wrap(tast_symbol_new(tast_stmt_get_pos(old_stmt), (Sym_typed_base) {
                    .lang_type = tast_lang_type_from_name(defered_collections.is_rtning),
                    .name = defered_collections.is_rtning
                })),
                tast_literal_wrap(tast_int_wrap(tast_int_new(tast_stmt_get_pos(old_stmt), 1, lang_type_new_u1())))
            );
            load_assignment(new_block, is_rtn_assign);

            Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
            if (pairs->info.count > 0) {
                Ir_goto* new_goto = ir_goto_new(rtn->pos, util_literal_name_new(), vec_top(pairs).label->name);
                vec_append(&a_main, &new_block->children, ir_goto_wrap(new_goto));
            }
            return;
        }
        case TAST_FOR_WITH_COND:
            load_for_with_cond(rtn_in_block, new_block, tast_for_with_cond_unwrap(old_stmt));
            return;
        case TAST_BREAK: {
            if (is_defered) {
                load_break(new_block, tast_break_unwrap(old_stmt));
                return;
            }

            if (label_if_break.base.count < 1) {
                msg(
                    DIAG_BREAK_INVALID_LOCATION, tast_break_unwrap(old_stmt)->pos,
                    "break statement outside of a for loop\n"
                );
                return;
            }

            Tast_break* brk = tast_break_unwrap(old_stmt);
            if (brk->do_break_expr) {
                Tast_assignment* new_assign = tast_assignment_new(
                    tast_stmt_get_pos(old_stmt),
                    tast_symbol_wrap(tast_symbol_new(tast_stmt_get_pos(old_stmt), (Sym_typed_base) {
                        .lang_type = tast_lang_type_from_name(load_break_symbol_name),
                        .name = load_break_symbol_name
                    })),
                    brk->break_expr
                );
                load_assignment(new_block, new_assign);
            }

            load_brking_or_conting_set_etc(new_block, old_stmt, true);
            return;
        }
        case TAST_CONTINUE: {
            if (is_defered) {
                return;
            }

            if (label_if_continue.base.count < 1) {
                msg(
                    DIAG_CONTINUE_INVALID_LOCATION, tast_continue_unwrap(old_stmt)->pos,
                    "continue statement outside of a for loop\n"
                );
                return;
            }

            load_brking_or_conting_set_etc(new_block, old_stmt, false);
            return;
        }
        case TAST_BLOCK:
            vec_append(
                &a_main,
                &new_block->children,
                ir_block_wrap(load_block(rtn_in_block, tast_block_unwrap(old_stmt), DEFER_PARENT_OF_BLOCK, (Lang_type) {0}))
            );
            return;
        case TAST_LABEL:
            load_label(new_block, tast_label_unwrap(old_stmt));
            return;
        case TAST_DEFER: {
            Defer_pair_vec* pairs = &vec_top_ref(&defered_collections.coll_stack)->pairs;
            Tast_defer* defer = tast_defer_unwrap(old_stmt);
            vec_append(&a_main, pairs, ((Defer_pair) {
                defer,
                tast_label_new(defer->pos, util_literal_name_new_prefix(sv("defered_thing")))
            }));
            return;
        }
    }
    unreachable("");
}

static void load_def_sometimes(Tast_def* old_def) {
    switch (old_def->type) {
        case TAST_FUNCTION_DEF:
            load_function_def(tast_function_def_unwrap(old_def));
            return;
        case TAST_FUNCTION_DECL:
            load_function_decl(tast_function_decl_unwrap(old_def));
            return;
        case TAST_VARIABLE_DEF:
            return;
        case TAST_STRUCT_DEF:
            load_struct_def(tast_struct_def_unwrap(old_def));
            return;
        case TAST_RAW_UNION_DEF:
            load_raw_union_def(tast_raw_union_def_unwrap(old_def));
            return;
        case TAST_ENUM_DEF:
            return;
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_IMPORT:
            todo();
    }
    unreachable("");
}

static Ir_block* load_block(
    bool* rtn_in_block,
    Tast_block* old_block,
    DEFER_PARENT_OF parent_of,
    Lang_type lang_type
) {
    Ir_block* new_block = ir_block_new(
        old_block->pos,
        util_literal_name_new(),
        (Ir_vec) {0},
        old_block->pos_end,
        old_block->scope_id
    );

    Symbol_iter iter = sym_tbl_iter_new(old_block->scope_id);
    Tast_def* curr = NULL;
    while (sym_tbl_iter_next(&curr, &iter)) {
        load_def_sometimes(curr);
    }

    load_block_stmts(rtn_in_block, new_block, old_block->children, parent_of, old_block->pos, lang_type);

    return new_block;
}

Ir_block* add_load_and_store(Tast_block* old_root) {
    Symbol_iter iter = sym_tbl_iter_new(0);
    Tast_def* curr = NULL;
    while (sym_tbl_iter_next(&curr, &iter)) {
        load_def_sometimes(curr);
    }

    bool rtn_in_block = false;
    Ir_block* block = load_block(&rtn_in_block, old_root, DEFER_PARENT_OF_TOP_LEVEL, lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN)));
    return block;
}


