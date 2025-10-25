#ifndef COMMON_H
#define COMMON_H

#include <util.h>
#include <ir.h>

bool is_extern_c(const Ir* ir);

void ir_extend_name(String* output, Name name);

#endif // COMMON_H
