#ifndef COMMON_H
#define COMMON_H

#include <util.h>
#include <ir.h>

static inline const Ir_expr* ir_expr_lookup_from_name(Name name) {
    Ir* child = NULL;
    unwrap(alloca_lookup(&child, name));
    return ir_expr_const_unwrap(child);
}

bool is_extern_c(const Ir* ir);

void ir_extend_name(String* output, Name name);

#endif // COMMON_H
