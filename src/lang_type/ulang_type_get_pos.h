#ifndef ULANG_TYPE_GET_POS_H
#define ULANG_TYPE_GET_POS_H

// TODO: include header file and remove this forward declaration
bool name_from_uname(Name* new_name, Uname name, Pos name_pos);

static inline Pos ulang_type_get_pos(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_GEN_PARAM:
            return ulang_type_gen_param_const_unwrap(lang_type).pos;
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).pos;
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_const_unwrap(lang_type).pos;
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_unwrap(lang_type).pos;
    }
    unreachable("");
}


#endif // ULANG_TYPE_GET_POS_H
