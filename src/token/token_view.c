#include <token_view.h>

void log_tokens_gdb(Tk_view tk_view) {
    log_tokens_internal("", 0, LOG_NOTE, tk_view);
}

