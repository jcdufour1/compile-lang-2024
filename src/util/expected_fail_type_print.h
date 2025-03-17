#ifndef EXPECTED_FAIL_TYPE_PRINT_H
#define EXPECTED_FAIL_TYPE_PRINT_H

#include <str_view.h>
#include <expected_fail_type.h>

static inline Str_view expect_fail_type_print_internal(EXPECT_FAIL_TYPE type) {
    switch (type) {
        case EXPECT_FAIL_NONE:
            return str_view_from_cstr("none");
        case EXPECT_FAIL_UNDEFINED_SYMBOL:
            return str_view_from_cstr("undefined_symbol");
        case EXPECT_FAIL_UNDEFINED_FUNCTION:
            return str_view_from_cstr("undefined_function");
        case EXPECT_FAIL_EXPECTED_EXPRESSION:
            return str_view_from_cstr("expected_expression");
        case EXPECT_FAIL_UNARY_MISMATCHED_TYPES:
            return str_view_from_cstr("unary_mismatched_types");
        case EXPECT_FAIL_BINARY_MISMATCHED_TYPES:
            return str_view_from_cstr("binary_mismatched_types");
        case EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES:
            return str_view_from_cstr("assignment_mismatched_types");
        case EXPECT_FAIL_INVALID_FUN_ARG:
            return str_view_from_cstr("invalid_fun_arg");
        case EXPECT_FAIL_INVALID_COUNT_FUN_ARGS:
            return str_view_from_cstr("invalid_count_fun_args");
        case EXPECT_FAIL_MISSING_RETURN:
            return str_view_from_cstr("missing_return");
        case EXPECT_FAIL_REDEFINITION_SYMBOL:
            return str_view_from_cstr("redefinition_symbol");
        case EXPECT_FAIL_INVALID_MEMBER_ACCESS:
            return str_view_from_cstr("invalid_member_access");
        case EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL:
            return str_view_from_cstr("invalid_member_in_literal");
        case EXPECT_FAIL_MISMATCHED_RETURN_TYPE:
            return str_view_from_cstr("mismatched_return_type");
        case EXPECT_FAIL_PARSER_EXPECTED:
            return str_view_from_cstr("parser_expected");
        case EXPECT_FAIL_MISSING_CLOSE_CURLY_BRACE:
            return str_view_from_cstr("missing_close_curly_brace");
        case EXPECT_FAIL_MISSING_CLOSE_PAR:
            return str_view_from_cstr("missing_close_par");
        case EXPECT_FAIL_MISSING_CLOSE_SQ_BRACKET:
            return str_view_from_cstr("missing_close_sq_bracket");
        case EXPECT_FAIL_MISSING_CLOSE_GENERIC:
            return str_view_from_cstr("missing_close_generic");
        case EXPECT_FAIL_INVALID_TOKEN:
            return str_view_from_cstr("invalid_token");
        case EXPECT_FAIL_INVALID_EXTERN_TYPE:
            return str_view_from_cstr("invalid_extern_type");
        case EXPECT_FAIL_NO_NEW_LINE_AFTER_STATEMENT:
            return str_view_from_cstr("no_new_line_after_statement");
        case EXPECT_FAIL_MISSING_CLOSE_DOUBLE_QUOTE:
            return str_view_from_cstr("missing_close_double_quote");
        case EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION:
            return str_view_from_cstr("struct_init_on_raw_union");
        case EXPECT_FAIL_STRUCT_INIT_ON_SUM:
            return str_view_from_cstr("struct_init_on_sum");
        case EXPECT_FAIL_STRUCT_INIT_ON_PRIMITIVE:
            return str_view_from_cstr("struct_init_on_primitive");
        case EXPECT_FAIL_UNDEFINED_TYPE:
            return str_view_from_cstr("undefined_type");
        case EXPECT_FAIL_DEREF_NON_POINTER:
            return str_view_from_cstr("deref_non_pointer");
        case EXPECT_FAIL_BREAK_INVALID_LOCATION:
            return str_view_from_cstr("break_invalid_location");
        case EXPECT_FAIL_CONTINUE_INVALID_LOCATION:
            return str_view_from_cstr("continue_invalid_location");
        case EXPECT_FAIL_MISMATCHED_TUPLE_COUNT:
            return str_view_from_cstr("mismatched_tuple_count");
        case EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH:
            return str_view_from_cstr("non_exhaustive_switch");
        case EXPECT_FAIL_SUM_LIT_INVALID_ARG:
            return str_view_from_cstr("sum_lit_invalid_arg");
        case EXPECT_FAIL_NOT_YET_IMPLEMENTED:
            return str_view_from_cstr("not_yet_implemented");
        case EXPECT_FAIL_DUPLICATE_DEFAULT:
            return str_view_from_cstr("duplicate_default");
        case EXPECT_FAIL_DUPLICATE_CASE:
            return str_view_from_cstr("duplicate_case");
        case EXPECT_FAIL_INVALID_COUNT_GENERIC_ARGS:
            return str_view_from_cstr("invalid_count_generic_args");
    }
    unreachable("");
}

#define expect_fail_type_print(type) str_view_print(expect_fail_type_print_internal(type))

#endif // EXPECTED_FAIL_TYPE_PRINT_H
