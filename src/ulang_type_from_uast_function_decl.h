#ifndef ULANG_TYPE_FROM_UAST_FUNCTION_DECL_H
#define ULANG_TYPE_FROM_UAST_FUNCTION_DECL_H


static inline Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl) {
    Ulang_type_vec params = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        vec_append(&a_main, &params, vec_at(&decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* return_type = arena_alloc(&a_main, sizeof(*return_type));
    *return_type = decl->return_type->lang_type;
    Ulang_type_fn fn = ulang_type_fn_new(ulang_type_tuple_new(params), return_type);
    return ulang_type_fn_const_wrap(fn);
}

#endif // ULANG_TYPE_FROM_UAST_FUNCTION_DECL_H
