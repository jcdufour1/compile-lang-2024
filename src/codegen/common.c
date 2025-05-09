#include <common.h>

// TODO: rename this file (and corresponding header) because -I /src/codegen could cause conflicts in the future otherwise

bool is_extern_c(const Llvm* llvm) {
    if (llvm->type != LLVM_DEF) {
        return false;
    }

    switch (llvm_def_const_unwrap(llvm)->type) {
        case LLVM_FUNCTION_DEF:
            return false;
        case LLVM_FUNCTION_DECL:
            return true;
        case LLVM_VARIABLE_DEF:
            return false;
        case LLVM_STRUCT_DEF:
            return false;
        case LLVM_PRIMITIVE_DEF:
            return false;
        case LLVM_LABEL:
            return false;
        case LLVM_LITERAL_DEF:
            return false;
    }
    unreachable("");
}

void llvm_extend_name(String* output, Name name) {
    Llvm* result = NULL;
    if (alloca_lookup(&result, name) && is_extern_c(result)) {
        memset(&name.mod_path, 0, sizeof(name.mod_path));
        name.scope_id = SCOPE_BUILTIN;
        assert(name.gen_args.info.count < 1 && "extern c generic function should not be allowed");
    } else if (str_view_cstr_is_equal(name.base, "main")) {
        name.scope_id = SCOPE_BUILTIN;
    } else if (name.mod_path.count < 1) {
        name.mod_path = str_view_from_cstr("PREFIX"); // TODO: make variable or similar for this
    }

    extend_name(NAME_EMIT_LLVM, output, name);
}

