#ifndef TAST_SERIALIZE_H
#define TAST_SERIALIZE_H

static inline Name serialize_tast_struct_def(const Tast_struct_def* def) {
    (void) def;
    todo();
}

static inline Name serialize_tast_raw_union_def(const Tast_raw_union_def* def) {
    (void) def;
    todo();
}

static inline Name serialize_tast_enum_def(const Tast_enum_def* def) {
    (void) def;
    todo();
}

Strv serialize_tast_def(const Tast_def* def);

#endif // TAST_SERIALIZE_H
