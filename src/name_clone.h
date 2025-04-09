#ifndef NAME_CLONE
#define NAME_CLONE

static inline Name name_clone(Name name) {
    return (Name) {.mod_path = name.mod_path, .base = name.base, .gen_args = ulang_type_vec_clone(name.gen_args)};
}

#endif // NAME_CLONE
