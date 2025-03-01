#ifndef UAST_SERIALIZE_H
#define UAST_SERIALIZE_H

#include <lang_type_serialize.h>

Str_view serialize_uast_struct_def(Env* env, const Uast_struct_def* def);

Str_view serialize_uast_raw_union_def(Env* env, const Uast_raw_union_def* def);

Str_view serialize_uast_sum_def(Env* env, const Uast_sum_def* def);

Str_view serialize_uast_def(Env* env, const Uast_def* def);

Str_view serialize_uast_def(Env* env, const Uast_def* def);

#endif // UAST_SERIALIZE_H
