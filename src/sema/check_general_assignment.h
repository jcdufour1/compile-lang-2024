#ifndef CHECK_GENERAL_ASSIGNMENT_H
#define CHECK_GENERAL_ASSIGNMENT_H

#include <tast.h>
#include <uast.h>
#include <lang_type.h>
#include <util.h>
#include <type_checking.h>

typedef enum {
    CHECK_ASSIGN_OK,
    CHECK_ASSIGN_INVALID, // error was not printed to the user (caller should print error),
                          // and new_src is valid for printing purposes
    CHECK_ASSIGN_ERROR, // error was printed, and new_src is not valid for printing purposes
} CHECK_ASSIGN_STATUS;

CHECK_ASSIGN_STATUS check_general_assignment(
    Type_checking_env* check_env,
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    Uast_expr* src,
    Pos pos
);

bool do_implicit_convertions(
    Lang_type dest,
    Tast_expr** new_src,
    Tast_expr* src,
    bool src_is_zero,
    bool implicit_pointer_depth
);

#endif // CHECK_GENERAL_ASSIGNMENT_H
