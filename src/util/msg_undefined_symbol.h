#ifndef MSG_UNDEFINED_SYMBOL_H
#define MSG_UNDEFINED_SYMBOL_H

// TODO: move this function and macro elsewhere
static inline void msg_undefined_symbol_internal(const char* file, int line, const Uast_symbol* sym_call) {
    msg_internal(
        file, line,
        EXPECT_FAIL_UNDEFINED_SYMBOL, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", name_print(NAME_MSG, sym_call->name)
    );
}

#define msg_undefined_symbol(sym_call) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, sym_call)

#endif // MSG_UNDEFINED_SYMBOL_H
