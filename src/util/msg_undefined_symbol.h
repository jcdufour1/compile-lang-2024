#ifndef MSG_UNDEFINED_SYMBOL_H
#define MSG_UNDEFINED_SYMBOL_H

// TODO: move this function and macro elsewhere
static inline void msg_undefined_symbol_internal(const char* file, int line, Name sym_name, Pos sym_pos) {
    msg_internal(
        file, line,
        DIAG_UNDEFINED_SYMBOL, sym_pos,
        "symbol `"FMT"` is not defined\n", name_print(NAME_MSG, sym_name)
    );
}

#define msg_undefined_symbol(sym_name, sym_pos) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, sym_name, sym_pos)

#endif // MSG_UNDEFINED_SYMBOL_H
