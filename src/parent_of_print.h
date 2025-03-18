#ifndef PARENT_OF_PRINT_H
#define PARENT_OF_PRINT_H

#include <env.h>
#include <str_view.h>
#include <util.h>

static inline Str_view parent_of_print_internal(PARENT_OF parent_of) {
    switch (parent_of) {
        case PARENT_OF_NONE:
            return str_view_from_cstr("PARENT_OF_NONE");
        case PARENT_OF_CASE:
            return str_view_from_cstr("PARENT_OF_CASE");
        case PARENT_OF_ASSIGN_RHS:
            return str_view_from_cstr("PARENT_OF_ASSIGN_RHS");
        case PARENT_OF_RETURN:
            return str_view_from_cstr("PARENT_OF_RETURN");
        case PARENT_OF_BREAK:
            return str_view_from_cstr("PARENT_OF_BREAK");
        case PARENT_OF_IF:
            return str_view_from_cstr("PARENT_OF_IF");
    }
    unreachable("");
}

#define parent_of_print(parent_of) str_view_print(parent_of_print_internal(parent_of))

#endif // PARENT_OF_PRINT_H

