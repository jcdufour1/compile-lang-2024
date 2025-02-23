#ifndef TAST_SERIALIZE_H
#define TAST_SERIALIZE_H

#include <lang_type_serialize.h>

Str_view serialize_tast_struct_def(Env* env, const Tast_struct_def* def);

Str_view serialize_tast_raw_union_def(Env* env, const Tast_raw_union_def* def);

Str_view serialize_tast_sum_def(Env* env, const Tast_sum_def* def);

Str_view serialize_tast_def(Env* env, const Tast_def* def);

#endif // TAST_SERIALIZE_H
