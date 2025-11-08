#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <lang_type_print.h>
#include <uast_utils.h>
#include <resolve_generics.h>
#include <ulang_type_get_pos.h>
#include <msg.h>
#include <tast_utils.h>
#include <ulang_type_serialize.h>
#include <uast_expr_to_ulang_type.h>
#include <ulang_type_remove_expr.h>

// TODO: remove this forward decl when possible
static inline bool lang_type_atom_is_equal(Lang_type_atom a, Lang_type_atom b);

static inline Lang_type lang_type_from_ulang_type(Ulang_type lang_type);

Ulang_type lang_type_to_ulang_type(Lang_type lang_type);

// TODO: remove Pos parameter
bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Ulang_type lang_type);

// TODO: remove these two forward decls and replace with better system
bool lang_type_atom_is_signed(Lang_type_atom atom);
bool lang_type_atom_is_unsigned(Lang_type_atom atom);
bool lang_type_atom_is_float(Lang_type_atom atom);

bool name_from_uname(Name* new_name, Uname name, Pos name_pos);

static inline bool try_lang_type_from_ulang_type_tuple(
    Lang_type_tuple* new_lang_type,
    Ulang_type_tuple lang_type
);

static inline bool try_lang_type_from_ulang_type_fn(
    Lang_type_fn* new_lang_type,
    Ulang_type_fn lang_type
);

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_tuple(Ulang_type_tuple lang_type) {
    Lang_type_tuple new_tuple = {0};
    unwrap(try_lang_type_from_ulang_type_tuple(&new_tuple, lang_type));
    return lang_type_tuple_const_wrap(new_tuple);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_fn(Ulang_type_fn lang_type) {
    Lang_type_fn new_fn = {0};
    unwrap(try_lang_type_from_ulang_type_fn(&new_fn, lang_type));
    return lang_type_fn_const_wrap(new_fn);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline bool try_lang_type_from_ulang_type_tuple(
    Lang_type_tuple* new_lang_type,
    Ulang_type_tuple lang_type
) {
    Lang_type_vec new_lang_types = {0};
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        if (vec_at(lang_type.ulang_types, idx).type == ULANG_TYPE_GEN_PARAM) {
            continue;
        }

        Lang_type new_child = {0};
        if (!try_lang_type_from_ulang_type(&new_child, vec_at(lang_type.ulang_types, idx))) {
            return false;
        }
        vec_append(&a_main, &new_lang_types, new_child);
    }
    *new_lang_type = lang_type_tuple_new(lang_type.pos, new_lang_types);
    return true;
}

// TODO: figure out way to reduce duplicate vec allocations
static inline bool try_lang_type_from_ulang_type_fn(
    Lang_type_fn* new_lang_type,
    Ulang_type_fn lang_type
) {
    Lang_type_tuple new_params = {0};
    if (!try_lang_type_from_ulang_type_tuple(&new_params, lang_type.params)) {
        assert(env.error_count > 0);
        return false;
    }
    Lang_type* new_rtn_type = arena_alloc(&a_main, sizeof(*new_rtn_type));
    if (!try_lang_type_from_ulang_type(new_rtn_type, *lang_type.return_type)) {
        assert(env.error_count > 0);
        return false;
    }
    *new_lang_type = lang_type_fn_new(lang_type.pos, new_params, new_rtn_type);
    return true;
}

static inline Lang_type lang_type_from_ulang_type_regular_primitive(const Ulang_type_regular lang_type) {
    Name name = {0};
    unwrap(name_from_uname(&name, lang_type.atom.str, lang_type.pos));
    Lang_type_atom atom = lang_type_atom_new(name, lang_type.atom.pointer_depth);
    assert(name.mod_path.count > 0);

    if (lang_type_atom_is_signed(atom)) {
        Lang_type_signed_int new_int = lang_type_signed_int_new(
            lang_type.pos,
            strv_to_int64_t(POS_BUILTIN, strv_slice(atom.str.base, 1, atom.str.base.count - 1)),
            atom.pointer_depth
        );
        return lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(new_int));
    } else if (lang_type_atom_is_unsigned(atom)) {
        Lang_type_unsigned_int new_int = lang_type_unsigned_int_new(
            lang_type.pos,
            strv_to_int64_t(POS_BUILTIN, strv_slice(atom.str.base, 1, atom.str.base.count - 1)),
            atom.pointer_depth
        );
        return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(new_int));
    } else if (lang_type_atom_is_float(atom)) {
        Lang_type_float new_float = lang_type_float_new(
            lang_type.pos,
            strv_to_int64_t(POS_BUILTIN, strv_slice(atom.str.base, 1, atom.str.base.count - 1)),
            atom.pointer_depth
        );
        return lang_type_primitive_const_wrap(lang_type_float_const_wrap(new_float));
    } else if (strv_is_equal(atom.str.base, sv("void"))) {
        return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
    } else if (strv_is_equal(atom.str.base, sv("opaque"))) {
        return lang_type_primitive_const_wrap(lang_type_opaque_const_wrap(lang_type_opaque_new(lang_type.pos, atom.pointer_depth)));
    } else {
        log(LOG_DEBUG, FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(lang_type)));
        todo();
    }
    unreachable("");
}

static inline bool try_lang_type_from_ulang_type_regular(Lang_type* new_lang_type, Ulang_type_regular lang_type) {
    (void) new_lang_type;
    Ulang_type resolved = {0};
    LANG_TYPE_TYPE type = {0};
    if (!resolve_generics_ulang_type_regular(&type, &resolved, lang_type)) {
        return false;
    }

    // report error if generic args are invalid
    vec_foreach(gen_idx, Ulang_type, gen_arg, lang_type.atom.str.gen_args) {
        Lang_type dummy = {0};
        if (!try_lang_type_from_ulang_type(&dummy, gen_arg)) {
            return false;
        }
    }

    Name temp_name = {0};
    if (!name_from_uname(&temp_name, ulang_type_regular_const_unwrap(resolved).atom.str, lang_type.pos)) {
        return false;
    }

    unwrap(name_from_uname(&temp_name, ulang_type_regular_const_unwrap(resolved).atom.str, lang_type.pos));
    Lang_type_atom new_atom = lang_type_atom_new(temp_name, ulang_type_regular_const_unwrap(resolved).atom.pointer_depth);
    switch (type) {
        case LANG_TYPE_STRUCT:
            *new_lang_type = lang_type_struct_const_wrap(lang_type_struct_new(lang_type.pos, new_atom));
            return true;
        case LANG_TYPE_RAW_UNION:
            *new_lang_type = lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type.pos, new_atom));
            return true;
        case LANG_TYPE_ENUM:
            *new_lang_type = lang_type_enum_const_wrap(lang_type_enum_new(lang_type.pos, new_atom));
            return true;
        case LANG_TYPE_PRIMITIVE:
            *new_lang_type = lang_type_from_ulang_type_regular_primitive(ulang_type_regular_const_unwrap(resolved));
            return true;
        case LANG_TYPE_VOID:
            *new_lang_type = lang_type_void_const_wrap(lang_type_void_new(lang_type.pos));
            return true;
        default:
            unreachable("");
    }
    unreachable("");
}

static inline bool try_lang_type_from_ulang_type_array(Lang_type* new_lang_type, Ulang_type_array lang_type) {
    Lang_type item_type = {0};
    if (!try_lang_type_from_ulang_type(&item_type, *lang_type.item_type)) {
      return false;
    }

    *new_lang_type = lang_type_array_const_wrap(lang_type_array_new(
        lang_type.pos,
        arena_dup(&a_main, &item_type),
        lang_type.count,
        0
    ));
    return true;
}

static inline Lang_type lang_type_from_ulang_type_regular(Ulang_type_regular lang_type) {
    Lang_type new_lang_type = {0};
    unwrap(try_lang_type_from_ulang_type_regular(&new_lang_type, lang_type));
    return new_lang_type;
}

static inline Lang_type lang_type_from_ulang_type_array(Ulang_type_array lang_type) {
    Lang_type new_lang_type = {0};
    unwrap(try_lang_type_from_ulang_type_array(&new_lang_type, lang_type));
    return new_lang_type;
}

// TODO: remove this function, and use try_lang_type_from_ulang_type instead?
//   (because this function causes crashes on user errors)
static inline Lang_type lang_type_from_ulang_type(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return lang_type_from_ulang_type_regular(ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_ARRAY:
            return lang_type_from_ulang_type_array(ulang_type_array_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            return lang_type_from_ulang_type_tuple(ulang_type_tuple_const_unwrap(lang_type));
        case ULANG_TYPE_FN:
            return lang_type_from_ulang_type_fn(ulang_type_fn_const_unwrap(lang_type));
        case ULANG_TYPE_GEN_PARAM:
            unreachable("");
        case ULANG_TYPE_EXPR: {
            Ulang_type inner = {0};
            unwrap(ulang_type_remove_expr(&inner, lang_type));
            return lang_type_from_ulang_type(inner);
        }
        case ULANG_TYPE_INT:
            todo();
    }
    unreachable("");
}

static inline Ulang_type_tuple lang_type_tuple_to_ulang_type_tuple(Lang_type_tuple lang_type) {
    // TODO: reduce heap allocations (do sym_tbl_lookup for this?)
    Ulang_type_vec new_types = {0};
    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        vec_append(&a_main, &new_types, lang_type_to_ulang_type(vec_at(lang_type.lang_types, idx)));
    }
    return ulang_type_tuple_new(new_types, lang_type.pos);
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
