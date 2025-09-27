#include <codegen_common.h>

bool is_extern_c(const Ir* ir) {
    if (ir->type != IR_DEF) {
        return false;
    }

    switch (ir_def_const_unwrap(ir)->type) {
        case IR_FUNCTION_DEF:
            return false;
        case IR_FUNCTION_DECL:
            return true;
        case IR_VARIABLE_DEF:
            return false;
        case IR_STRUCT_DEF:
            return false;
        case IR_PRIMITIVE_DEF:
            return false;
        case IR_LABEL:
            return false;
        case IR_LITERAL_DEF:
            return false;
    }
    unreachable("");
}

void ir_extend_name(String* output, Name name) {
    Ir* result = NULL;
    if (ir_lookup(&result, name) && is_extern_c(result)) {
        memset(&name.mod_path, 0, sizeof(name.mod_path));
        name.scope_id = SCOPE_BUILTIN;
        assert(name.gen_args.info.count < 1 && "extern c generic function should not be allowed");
    } else if (strv_is_equal(name.base, sv("main"))) {
        memset(&name.mod_path, 0, sizeof(name.mod_path));
        assert(name.gen_args.info.count < 1 && "generic main function should not be allowed");
        name.scope_id = SCOPE_BUILTIN;
    }

    extend_name(NAME_EMIT_IR, output, name);
}

