
#include <ir.h>
#include <newstring.h>
#include <do_passes.h>
#include <symbol_table.h>
#include <parameters.h>
#include <file.h>
#include <ir_utils.h>
#include <errno.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <lang_type_print.h>
#include <lang_type_get_pos.h>
#include <symbol_iter.h>
#include <codegen_common.h>
#include <do_passes.h>

//static void emit_block(String* struct_defs, String* output, String* literals, const Ir_block* fun_block);
//
//static void emit_out_of_line(String* struct_defs, String* output, String* literals, const Ir* ir);
//
//static void emit_symbol_normal(String* literals, Name key, const Ir_literal* lit);
//
//static bool ir_is_literal(const Ir* ir) {
//    if (ir->type != IR_EXPR) {
//        return false;
//    }
//    return ir_expr_const_unwrap(ir)->type == IR_LITERAL;
//}
//
//static void extend_literal(String* output, const Ir_literal* literal) {
//    switch (literal->type) {
//        case IR_STRING:
//            string_extend_strv(&a_main, output, ir_string_const_unwrap(literal)->data);
//            return;
//        case IR_INT:
//            string_extend_int64_t(&a_main, output, ir_int_const_unwrap(literal)->data);
//            return;
//        case IR_FLOAT:
//            string_extend_double(&a_main, output, ir_int_const_unwrap(literal)->data);
//            return;
//        case IR_VOID:
//            return;
//        case IR_FUNCTION_NAME:
//            ir_extend_name(output, ir_function_name_const_unwrap(literal)->fun_name);
//            return;
//    }
//    unreachable("");
//}
//
//static void extend_type_call_str(String* output, Lang_type lang_type) {
//    if (lang_type_get_pointer_depth(lang_type) != 0) {
//        string_extend_cstr(&a_main, output, "ptr");
//        return;
//    }
//
//    switch (lang_type.type) {
//        case LANG_TYPE_FN:
//            unreachable("");
//        case LANG_TYPE_TUPLE:
//            unreachable("");
//        case LANG_TYPE_STRUCT:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, lang_type_struct_const_unwrap(lang_type).atom.str);
//            return;
//        case LANG_TYPE_RAW_UNION:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, lang_type_raw_union_const_unwrap(lang_type).atom.str);
//            return;
//        case LANG_TYPE_VOID:
//            lang_type = lang_type_void_const_wrap(lang_type_void_new(lang_type_get_pos(lang_type)));
//            string_extend_strv(&a_main, output, serialize_lang_type(lang_type));
//            return;
//        case LANG_TYPE_PRIMITIVE:
//            if (lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_UNSIGNED_INT) {
//                Lang_type_unsigned_int old_num = lang_type_unsigned_int_const_unwrap(lang_type_primitive_const_unwrap(lang_type));
//                lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_get_pos(lang_type), old_num.bit_width, old_num.pointer_depth)));
//            }
//            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lang_type);
//            return;
//        case LANG_TYPE_ENUM:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, lang_type_enum_const_unwrap(lang_type).atom.str);
//            return;
//    }
//    unreachable("");
//}
//
//static bool ir_is_variadic(const Ir* ir) {
//    if (ir->type == IR_EXPR) {
//        switch (ir_expr_const_unwrap(ir)->type) {
//            case IR_LITERAL:
//                return false;
//            default:
//                unreachable(FMT, ir_print(ir));
//        }
//    } else {
//        const Ir_def* def = ir_def_const_unwrap(ir);
//        switch (def->type) {
//            case IR_VARIABLE_DEF:
//                return ir_variable_def_const_unwrap(ir_def_const_unwrap(ir))->is_variadic;
//            default:
//                unreachable(FMT, ir_print(ir));
//        }
//    }
//}
//
//static void ir_extend_type_decl_str(String* output, const Ir* var_def_or_lit, bool noundef) {
//    if (ir_is_variadic(var_def_or_lit)) {
//        string_extend_cstr(&a_main, output, "...");
//        return;
//    }
//
//    extend_type_call_str(output, ir_get_lang_type(var_def_or_lit));
//    if (noundef) {
//        string_extend_cstr(&a_main, output, " noundef");
//    }
//}
//
//static void extend_literal_decl_prefix(String* output, String* literals, const Ir_literal* literal) {
//    if (literal->type == IR_FUNCTION_NAME) {
//        string_extend_cstr(&a_main, output, " @");
//        ir_extend_name(output, ir_function_name_const_unwrap(literal)->fun_name);
//    } else if (
//        ir_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE &&
//        lang_type_primitive_const_unwrap(ir_literal_get_lang_type(literal)).type == LANG_TYPE_CHAR &&
//        lang_type_get_pointer_depth(ir_literal_get_lang_type(literal)) > 0
//    ) {
//        if (lang_type_get_pointer_depth(ir_literal_get_lang_type(literal)) != 1) {
//            todo();
//        }
//        string_extend_cstr(&a_main, output, " @.");
//        ir_extend_name(output, ir_literal_get_name(literal));
//    } else if (lang_type_atom_is_signed(lang_type_get_atom(LANG_TYPE_MODE_LOG, ir_literal_get_lang_type(literal)))) {
//        unwrap(ir_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE);
//        if (lang_type_get_pointer_depth(ir_literal_get_lang_type(literal)) != 0) {
//            todo();
//        }
//        vec_append(&a_main, output, ' ');
//        extend_literal(output, literal);
//    } else if (lang_type_atom_is_unsigned(lang_type_get_atom(LANG_TYPE_MODE_LOG, ir_literal_get_lang_type(literal)))) {
//        unwrap(ir_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE);
//        if (lang_type_get_pointer_depth(ir_literal_get_lang_type(literal)) != 0) {
//            todo();
//        }
//        vec_append(&a_main, output, ' ');
//        extend_literal(output, literal);
//    } else {
//        unreachable(FMT"\n", ir_print(ir_expr_const_wrap(ir_literal_const_wrap(literal))));
//    }
//
//    if (literal->type == IR_STRING) {
//        emit_symbol_normal(literals, ir_literal_get_name(literal), literal);
//    }
//}
//
//static void extend_literal_decl(String* output, String* literals, const Ir_literal* literal, bool noundef) {
//    ir_extend_type_decl_str(output, ir_expr_const_wrap(ir_literal_const_wrap(literal)), noundef);
//    extend_literal_decl_prefix(output, literals, literal);
//}
//
//static void emit_function_params(String* output, const Ir_function_params* fun_params) {
//    for (size_t idx = 0; idx < fun_params->params.info.count; idx++) {
//        const Ir_variable_def* curr_param = vec_at(fun_params->params, idx);
//
//        if (idx > 0) {
//            string_extend_cstr(&a_main, output, ", ");
//        }
//
//        if (is_struct_like(curr_param->lang_type.type)) {
//            if (lang_type_get_pointer_depth(curr_param->lang_type) < 0) {
//                unreachable("");
//            } else if (lang_type_get_pointer_depth(curr_param->lang_type) > 0) {
//                extend_type_call_str(output, curr_param->lang_type);
//            } else {
//                string_extend_cstr(&a_main, output, "ptr noundef byval(");
//                extend_type_call_str(output, curr_param->lang_type);
//                string_extend_cstr(&a_main, output, ")");
//            }
//        } else {
//            ir_extend_type_decl_str(output, ir_def_const_wrap(ir_variable_def_const_wrap(curr_param)), true);
//        }
//
//        if (curr_param->is_variadic) {
//            return;
//        }
//        string_extend_cstr(&a_main, output, " %");
//        ir_extend_name(output, curr_param->name_self);
//    }
//}
//
//static void emit_function_call_arg_load_another_ir(
//     
//    String* output,
//    String* literals,
//    const Ir_load_another_ir* load
//) {
//    Ir* src = NULL;
//    unwrap(ir_lookup(&src, load->name));
//
//    if (ir_is_literal(src)) {
//        extend_literal_decl(output, literals, ir_literal_const_unwrap(ir_expr_const_unwrap(src)), true);
//    } else {
//        if (is_struct_like(load->lang_type.type)) {
//            string_extend_cstr(&a_main, output, "ptr noundef byval(");
//            extend_type_call_str(output, load->lang_type);
//            string_extend_cstr(&a_main, output, ")");
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, load->ir_src);
//        } else {
//            extend_type_call_str(output, load->lang_type);
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, load->name);
//        }
//    }
//}
//
//static void emit_function_arg_expr(String* output, String* literals, const Ir_expr* argument) {
//    switch (argument->type) {
//        case IR_LITERAL:
//            extend_literal_decl(output, literals, ir_literal_const_unwrap(argument), true);
//            break;
//        case IR_FUNCTION_CALL:
//            extend_type_call_str(output, ir_expr_get_lang_type(argument));
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_expr_get_name(argument));
//            break;
//        case IR_OPERATOR:
//            extend_type_call_str(output, ir_expr_get_lang_type(argument));
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_expr_get_name(argument));
//            break;
//        default:
//            unreachable("");
//    }
//}
//
//static void emit_function_call_arguments(String* output, String* literals, const Ir_function_call* fun_call) {
//    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
//        Name arg_name = vec_at(fun_call->args, idx);
//        Ir* argument = NULL;
//        unwrap(ir_lookup(&argument, arg_name));
//
//        if (idx > 0) {
//            string_extend_cstr(&a_main, output, ", ");
//        }
//
//        switch (argument->type) {
//            case IR_EXPR:
//                emit_function_arg_expr(output, literals, ir_expr_const_unwrap(argument));
//                break;
//            case IR_LOAD_ANOTHER_IR:
//                emit_function_call_arg_load_another_ir(output, literals, ir_load_another_ir_const_unwrap(argument));
//                break;
//            case IR_ALLOCA:
//                string_extend_cstr(&a_main, output, "ptr %");
//                ir_extend_name(output, ir_get_name(argument));
//                break;
//            default:
//                unreachable(FMT"\n", ir_print(argument));
//        }
//    }
//}
//
//static void emit_function_call(String* output, String* literals, const Ir_function_call* fun_call) {
//    //unwrap(fun_call->ir_id == 0);
//
//    // start of actual function call
//    string_extend_cstr(&a_main, output, "    ");
//    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
//        string_extend_cstr(&a_main, output, "%");
//        ir_extend_name(output, fun_call->name_self);
//        string_extend_cstr(&a_main, output, " = ");
//    } else {
//        unwrap(!strv_is_equal(lang_type_get_str(LANG_TYPE_MODE_EMIT_LLVM, fun_call->lang_type).base, sv("void")));
//    }
//    string_extend_cstr(&a_main, output, "call ");
//    extend_type_call_str(output, fun_call->lang_type);
//    Ir* callee = NULL;
//    unwrap(ir_lookup(&callee, fun_call->callee));
//
//    switch (callee->type) {
//        case IR_EXPR:
//            string_extend_cstr(&a_main, output, " @");
//            ir_extend_name(output, ir_function_name_unwrap(ir_literal_unwrap(ir_expr_unwrap(((callee)))))->fun_name);
//            break;
//        case IR_LOAD_ANOTHER_IR:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_load_another_ir_const_unwrap(callee)->name);
//            break;
//        default:
//            unreachable("");
//    }
//    //string_extend_strv(&a_main, output, fun_call->name_fun_to_call);
//
//    // arguments
//    string_extend_cstr(&a_main, output, "(");
//    emit_function_call_arguments(output, literals, fun_call);
//    string_extend_cstr(&a_main, output, ")");
//
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_alloca(String* output, const Ir_alloca* lang_alloca) {
//    string_extend_cstr(&a_main, output, "    %");
//    ir_extend_name(output, lang_alloca->name);
//    string_extend_cstr(&a_main, output, " = lang_alloca ");
//    extend_type_call_str(output, lang_alloca->lang_type);
//    string_extend_cstr(&a_main, output, ", align 8");
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_unary_type(String* output, const Ir_unary* unary) {
//    switch (unary->token_type) {
//        case UNARY_UNSAFE_CAST:
//            if (lang_type_get_pointer_depth(unary->lang_type) > 0 && lang_type_is_number(lang_type_from_ir_name(unary->child))) {
//                string_extend_cstr(&a_main, output, "inttoptr ");
//                extend_type_call_str(output, lang_type_from_ir_name(unary->child));
//                string_extend_cstr(&a_main, output, " ");
//            } else if (lang_type_is_number(unary->lang_type) && lang_type_get_pointer_depth(lang_type_from_ir_name(unary->child)) > 0) {
//                string_extend_cstr(&a_main, output, "ptrtoint ");
//                extend_type_call_str(output, lang_type_from_ir_name(unary->child));
//                string_extend_cstr(&a_main, output, " ");
//            } else if (lang_type_is_unsigned(lang_type_from_ir_name(unary->child)) && lang_type_is_number(lang_type_from_ir_name(unary->child))) {
//                if (i_lang_type_atom_to_bit_width(lang_type_get_atom(LANG_TYPE_MODE_LOG, unary->lang_type)) > i_lang_type_atom_to_bit_width(
//                    lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type_from_ir_name(unary->child))
//                )) {
//                    string_extend_cstr(&a_main, output, "zext ");
//                } else {
//                    string_extend_cstr(&a_main, output, "trunc ");
//                }
//                extend_type_call_str(output, lang_type_from_ir_name(unary->child));
//                string_extend_cstr(&a_main, output, " ");
//            } else if (lang_type_is_signed(lang_type_from_ir_name(unary->child)) && lang_type_is_number(lang_type_from_ir_name(unary->child))) {
//                if (i_lang_type_atom_to_bit_width(lang_type_get_atom(LANG_TYPE_MODE_LOG, unary->lang_type)) > i_lang_type_atom_to_bit_width(lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type_from_ir_name(unary->child)))) {
//                    string_extend_cstr(&a_main, output, "sext ");
//                } else {
//                    string_extend_cstr(&a_main, output, "trunc ");
//                }
//                extend_type_call_str(output, lang_type_from_ir_name(unary->child));
//                string_extend_cstr(&a_main, output, " ");
//            } else {
//                log(LOG_DEBUG, FMT, ir_unary_print(unary));
//                todo();
//            }
//            break;
//        default:
//            unreachable(FMT"\n", ir_unary_print(unary));
//    }
//}
//
//static void emit_binary_type_signed(String* output, const Ir_binary* binary) {
//    switch (binary->token_type) {
//        case BINARY_SINGLE_EQUAL:
//            unreachable("= should not still be a binary expr at this point");
//            return;
//        case BINARY_SUB:
//            string_extend_cstr(&a_main, output, "sub nsw ");
//            return;
//        case BINARY_ADD:
//            string_extend_cstr(&a_main, output, "add nsw ");
//            return;
//        case BINARY_MULTIPLY:
//            string_extend_cstr(&a_main, output, "mul nsw ");
//            return;
//        case BINARY_DIVIDE:
//            string_extend_cstr(&a_main, output, "sdiv ");
//            return;
//        case BINARY_MODULO:
//            string_extend_cstr(&a_main, output, "srem ");
//            return;
//        case BINARY_LESS_THAN:
//            string_extend_cstr(&a_main, output, "icmp slt ");
//            return;
//        case BINARY_LESS_OR_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp sle ");
//            return;
//        case BINARY_GREATER_OR_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp sge ");
//            return;
//        case BINARY_GREATER_THAN:
//            string_extend_cstr(&a_main, output, "icmp sgt ");
//            return;
//        case BINARY_DOUBLE_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp eq ");
//            return;
//        case BINARY_NOT_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp ne ");
//            return;
//        case BINARY_BITWISE_XOR:
//            string_extend_cstr(&a_main, output, "xor ");
//            return;
//        case BINARY_BITWISE_AND:
//            string_extend_cstr(&a_main, output, "and ");
//            return;
//        case BINARY_BITWISE_OR:
//            string_extend_cstr(&a_main, output, "or ");
//            return;
//        case BINARY_SHIFT_LEFT:
//            unreachable("");
//        case BINARY_SHIFT_RIGHT:
//            unreachable("");
//        case BINARY_LOGICAL_AND:
//            fallthrough;
//        case BINARY_LOGICAL_OR:
//            unreachable("logical operators should not make it this far");
//    }
//    unreachable("");
//}
//
//static void emit_binary_type_unsigned(String* output, const Ir_binary* binary) {
//    switch (binary->token_type) {
//        case BINARY_SINGLE_EQUAL:
//            unreachable("= should not still be a binary expr at this point");
//            return;
//        case BINARY_SUB:
//            string_extend_cstr(&a_main, output, "sub nsw ");
//            return;
//        case BINARY_ADD:
//            string_extend_cstr(&a_main, output, "add nsw ");
//            return;
//        case BINARY_MULTIPLY:
//            string_extend_cstr(&a_main, output, "mul nsw ");
//            return;
//        case BINARY_DIVIDE:
//            string_extend_cstr(&a_main, output, "udiv ");
//            return;
//        case BINARY_MODULO:
//            string_extend_cstr(&a_main, output, "urem ");
//            return;
//        case BINARY_LESS_THAN:
//            string_extend_cstr(&a_main, output, "icmp ult ");
//            return;
//        case BINARY_LESS_OR_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp ule ");
//            return;
//        case BINARY_GREATER_OR_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp uge ");
//            return;
//        case BINARY_GREATER_THAN:
//            string_extend_cstr(&a_main, output, "icmp ugt ");
//            return;
//        case BINARY_DOUBLE_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp eq ");
//            return;
//        case BINARY_NOT_EQUAL:
//            string_extend_cstr(&a_main, output, "icmp ne ");
//            return;
//        case BINARY_BITWISE_XOR:
//            string_extend_cstr(&a_main, output, "xor ");
//            return;
//        case BINARY_BITWISE_AND:
//            string_extend_cstr(&a_main, output, "and ");
//            return;
//        case BINARY_BITWISE_OR:
//            string_extend_cstr(&a_main, output, "or ");
//            return;
//        case BINARY_SHIFT_LEFT:
//            string_extend_cstr(&a_main, output, "shl ");
//            return;
//        case BINARY_SHIFT_RIGHT:
//            string_extend_cstr(&a_main, output, "lshr ");
//            return;
//        case BINARY_LOGICAL_AND:
//            fallthrough;
//        case BINARY_LOGICAL_OR:
//            unreachable("logical operators should not make it this far");
//    }
//    unreachable("");
//}
//
//static void emit_binary_type(String* output, const Ir_binary* binary) {
//    if (lang_type_is_signed(lang_type_from_ir_name(binary->lhs))) {
//        emit_binary_type_signed(output, binary);
//    } else {
//        emit_binary_type_unsigned(output, binary);
//    }
//
//    extend_type_call_str(output, lang_type_from_ir_name(binary->lhs));
//    string_extend_cstr(&a_main, output, " ");
//}
//
//static void emit_unary_suffix(String* output, const Ir_unary* unary) {
//    switch (unary->token_type) {
//        case UNARY_UNSAFE_CAST:
//            string_extend_cstr(&a_main, output, " to ");
//            extend_type_call_str(output, unary->lang_type);
//            return;
//        case UNARY_DEREF:
//            // TODO: return here to simplify things?
//            unreachable("suffix not needed for UNARY_DEREF");
//        case UNARY_REFER:
//            unreachable("suffix not needed for UNARY_REFER");
//        case UNARY_LOGICAL_NOT:
//            unreachable("not should not still be present here");
//        case UNARY_SIZEOF:
//            unreachable("sizeof should not still be present here");
//    }
//    unreachable("");
//}
//
//static void emit_operator_operand_expr(String* output, const Ir_expr* operand) {
//    switch (operand->type) {
//        case IR_LITERAL:
//            extend_literal(output, ir_literal_const_unwrap(operand));
//            break;
//        case IR_FUNCTION_CALL:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_expr_get_name(operand));
//            break;
//        case IR_OPERATOR:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_expr_get_name(operand));
//            break;
//        default:
//            unreachable(FMT, ir_expr_print(operand));
//    }
//}
//
//static void emit_operator_operand(String* output, const Name operand_name) {
//    Ir* operand = NULL;
//    unwrap(ir_lookup(&operand, operand_name));
//
//    switch (operand->type) {
//        case IR_EXPR:
//            emit_operator_operand_expr(output, ir_expr_const_unwrap(operand));
//            break;
//        case IR_LOAD_ANOTHER_IR:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_get_name(operand));
//            break;
//        default:
//            unreachable(FMT, ir_print(operand));
//    }
//}
//
//static void emit_operator(String* output, const Ir_operator* operator) {
//    string_extend_cstr(&a_main, output, "    %");
//    ir_extend_name(output, ir_operator_get_name(operator));
//    string_extend_cstr(&a_main, output, " = ");
//
//    if (operator->type == IR_UNARY) {
//        emit_unary_type(output, ir_unary_const_unwrap(operator));
//    } else if (operator->type == IR_BINARY) {
//        emit_binary_type(output, ir_binary_const_unwrap(operator));
//    } else {
//        unreachable("");
//    }
//
//    if (operator->type == IR_UNARY) {
//        emit_operator_operand(output, ir_unary_const_unwrap(operator)->child);
//    } else if (operator->type == IR_BINARY) {
//        const Ir_binary* binary = ir_binary_const_unwrap(operator);
//        emit_operator_operand(output, binary->lhs);
//        string_extend_cstr(&a_main, output, ", ");
//        emit_operator_operand(output, binary->rhs);
//    } else {
//        unreachable("");
//    }
//
//    if (operator->type == IR_UNARY) {
//        emit_unary_suffix(output, ir_unary_const_unwrap(operator));
//    } else if (operator->type == IR_BINARY) {
//    } else {
//        unreachable("");
//    }
//
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_load_another_ir(String* output, const Ir_load_another_ir* load_ir) {
//    string_extend_cstr(&a_main, output, "    %");
//    ir_extend_name(output, load_ir->name);
//    string_extend_cstr(&a_main, output, " = load ");
//    extend_type_call_str(output, load_ir->lang_type);
//    string_extend_cstr(&a_main, output, ", ");
//    string_extend_cstr(&a_main, output, "ptr");
//    string_extend_cstr(&a_main, output, " %");
//    ir_extend_name(output, load_ir->ir_src);
//    string_extend_cstr(&a_main, output, ", align 8");
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_store_another_ir_src_literal(String* output, const Ir_literal* literal) {
//    string_extend_cstr(&a_main, output, " ");
//
//    switch (literal->type) {
//        case IR_STRING:
//            string_extend_cstr(&a_main, output, " @.");
//            ir_extend_name(output, ir_string_const_unwrap(literal)->name);
//            return;
//        case IR_INT:
//            string_extend_int64_t(&a_main, output, ir_int_const_unwrap(literal)->data);
//            return;
//        case IR_FLOAT:
//            string_extend_double(&a_main, output, ir_int_const_unwrap(literal)->data);
//            return;
//        case IR_VOID:
//            return;
//        case IR_FUNCTION_NAME:
//            unreachable("");
//    }
//    unreachable("");
//}
//
//static void emit_store_another_ir_src_expr(String* output, String* literals, const Ir_expr* expr) {
//    (void) env;
//
//    switch (expr->type) {
//        case IR_LITERAL: {
//            const Ir_literal* lit = ir_literal_const_unwrap(expr);
//            if (lit->type == IR_STRING) {
//                emit_symbol_normal(literals, ir_literal_get_name(lit), lit);
//            }
//            emit_store_another_ir_src_literal(output, lit);
//            return;
//        }
//        case IR_FUNCTION_CALL:
//            fallthrough;
//        case IR_OPERATOR:
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, ir_expr_get_name(expr));
//            break;
//        default:
//            unreachable(FMT"\n", ir_print(ir_expr_const_wrap(expr)));
//    }
//}
//
//static void emit_store_another_ir(String* output, String* literals, const Ir_store_another_ir* store) {
//    Ir* src = NULL;
//    unwrap(ir_lookup(&src, store->ir_src));
//
//    string_extend_cstr(&a_main, output, "    store ");
//    extend_type_call_str(output, store->lang_type);
//    string_extend_cstr(&a_main, output, " ");
//
//    switch (src->type) {
//        case IR_DEF: {
//            const Ir_def* src_def = ir_def_const_unwrap(src);
//            const Ir_variable_def* src_var_def = ir_variable_def_const_unwrap(src_def);
//            (void) src_var_def;
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, ir_get_name(src));
//            break;
//        }
//        case IR_EXPR:
//            emit_store_another_ir_src_expr(output, literals, ir_expr_const_unwrap(src));
//            break;
//        case IR_LOAD_ANOTHER_IR:
//            string_extend_cstr(&a_main, output, "%");
//            ir_extend_name(output, ir_get_name(src));
//            break;
//        case IR_ALLOCA:
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, ir_get_name(src));
//            break;
//        default:
//            unreachable(FMT"\n", ir_print(src));
//    }
//    //string_extend_cstr(&a_main, output, " %");
//    //ir_extend_name(output, ir_get_name(store->ir_src.ir));
//
//    string_extend_cstr(&a_main, output, ", ptr %");
//    ir_extend_name(output, store->ir_dest);
//    string_extend_cstr(&a_main, output, ", align 8");
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_function_def(String* struct_defs, String* output, String* literals, const Ir_function_def* fun_def) {
//    string_extend_cstr(&a_main, output, "define dso_local ");
//
//    extend_type_call_str(output, fun_def->decl->return_type);
//
//    string_extend_cstr(&a_main, output, " @");
//    ir_extend_name(output, ir_get_name(ir_def_const_wrap(ir_function_def_const_wrap(fun_def))));
//
//    vec_append(&a_main, output, '(');
//    emit_function_params(output, fun_def->decl->params);
//    vec_append(&a_main, output, ')');
//
//    string_extend_cstr(&a_main, output, " {\n");
//    emit_block(struct_defs, output, literals, fun_def->body);
//    string_extend_cstr(&a_main, output, "}\n");
//}
//
//static void emit_return_expr(String* output, const Ir_expr* child) {
//    switch (child->type) {
//        case IR_LITERAL: {
//            const Ir_literal* literal = ir_literal_const_unwrap(child);
//            string_extend_cstr(&a_main, output, "    ret ");
//            extend_type_call_str(output, ir_literal_get_lang_type(literal));
//            string_extend_cstr(&a_main, output, " ");
//            extend_literal(output, literal);
//            string_extend_cstr(&a_main, output, "\n");
//            break;
//        }
//        case IR_OPERATOR: {
//            const Ir_operator* operator = ir_operator_const_unwrap(child);
//            string_extend_cstr(&a_main, output, "    ret ");
//            extend_type_call_str(output, ir_operator_get_lang_type(operator));
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, ir_expr_get_name(child));
//            string_extend_cstr(&a_main, output, "\n");
//            break;
//        }
//        case IR_FUNCTION_CALL: {
//            const Ir_function_call* function_call = ir_function_call_const_unwrap(child);
//            string_extend_cstr(&a_main, output, "    ret ");
//            extend_type_call_str(output, function_call->lang_type);
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, ir_expr_get_name(child));
//            string_extend_cstr(&a_main, output, "\n");
//            break;
//        }
//        default:
//            unreachable(FMT"\n", ir_expr_print(child));
//    }
//}
//
//static void emit_return(String* output, const Ir_return* fun_return) {
//    Ir* sym_to_return = NULL;
//    unwrap(ir_lookup(&sym_to_return, fun_return->child));
//
//    switch (sym_to_return->type) {
//        case IR_EXPR:
//            emit_return_expr(output, ir_expr_const_unwrap(sym_to_return));
//            return;
//        case IR_LOAD_ANOTHER_IR: {
//            const Ir_load_another_ir* load = ir_load_another_ir_const_unwrap(sym_to_return);
//            string_extend_cstr(&a_main, output, "    ret ");
//            extend_type_call_str(output, load->lang_type);
//            string_extend_cstr(&a_main, output, " %");
//            ir_extend_name(output, load->name);
//            string_extend_cstr(&a_main, output, "\n");
//            return;
//        }
//        default:
//            unreachable(FMT"\n", ir_print(sym_to_return));
//    }
//}
//
//static void emit_function_decl(String* output, const Ir_function_decl* fun_decl) {
//    string_extend_cstr(&a_main, output, "declare i32");
//    //extend_literal_decl(output, fun_decl); // TODO
//    string_extend_cstr(&a_main, output, " @");
//    // emit base of name only for extern("c")
//    string_extend_strv(&a_main, output, fun_decl->name.base);
//    vec_append(&a_main, output, '(');
//    emit_function_params(output, fun_decl->params);
//    vec_append(&a_main, output, ')');
//    string_extend_cstr(&a_main, output, "\n");
//}
//
//static void emit_label(String* output, const Ir_label* label) {
//    string_extend_cstr(&a_main, output, "\n");
//    ir_extend_name(output, label->name);
//    string_extend_cstr(&a_main, output, ":\n");
//}
//
//static void emit_goto(String* output, const Ir_goto* lang_goto) {
//    string_extend_cstr(&a_main, output, "    br label %");
//    ir_extend_name(output, lang_goto->label);
//    vec_append(&a_main, output, '\n');
//}
//
//static void emit_cond_goto(String* output, const Ir_cond_goto* cond_goto) {
//    string_extend_cstr(&a_main, output, "    br i1 %");
//    ir_extend_name(output, cond_goto->condition);
//    string_extend_cstr(&a_main, output, ", label %");
//    ir_extend_name(output, cond_goto->if_true);
//    string_extend_cstr(&a_main, output, ", label %");
//    ir_extend_name(output, cond_goto->if_false);
//    vec_append(&a_main, output, '\n');
//}
//
//static void emit_ir_struct_def_base(String* output, const Ir_struct_def_base* base) {
//    ir_extend_name(output, base->name);
//    string_extend_cstr(&a_main, output, " = type { ");
//    bool is_first = true;
//
//    for (size_t idx = 0; idx < base->members.info.count; idx++) {
//        if (!is_first) {
//            string_extend_cstr(&a_main, output, ", ");
//        }
//        ir_extend_type_decl_str(output, ir_def_wrap(ir_variable_def_wrap(vec_at(base->members, idx))), false);
//        is_first = false;
//    }
//
//    string_extend_cstr(&a_main, output, " }\n");
//}
//
//static void emit_struct_def(String* output, const Ir_struct_def* struct_def) {
//    string_extend_cstr(&a_main, output, "%");
//    emit_ir_struct_def_base(output, &struct_def->base);
//}
//
//static void emit_load_element_ptr(String* output, const Ir_load_element_ptr* load) {
//    string_extend_cstr(&a_main, output, "    %"); 
//    ir_extend_name(output, load->name_self);
//
//    string_extend_cstr(&a_main, output, " = getelementptr inbounds ");
//
//    Lang_type lang_type = lang_type_from_ir_name(load->ir_src);
//    lang_type_set_pointer_depth(&lang_type, 0);
//    extend_type_call_str(output, lang_type);
//    string_extend_cstr(&a_main, output, ", ptr %");
//    ir_extend_name(output, load->ir_src);
//    string_extend_cstr(&a_main, output, ", i32 0, i32 ");
//    string_extend_size_t(&a_main, output, load->memb_idx);
//    vec_append(&a_main, output, '\n');
//}
//
//static void emit_array_access(String* output, const Ir_array_access* load) {
//    string_extend_cstr(&a_main, output, "    %"); 
//    ir_extend_name(output, load->name_self);
//
//    string_extend_cstr(&a_main, output, " = getelementptr inbounds ");
//
//    Lang_type lang_type = lang_type_from_ir_name(load->callee);
//    lang_type_set_pointer_depth(&lang_type, 0);
//    extend_type_call_str(output, lang_type);
//    string_extend_cstr(&a_main, output, ", ptr %");
//    ir_extend_name(output, load->callee);
//    string_extend_cstr(&a_main, output, ", ");
//    extend_type_call_str(output, lang_type_from_ir_name(load->index));
//    string_extend_cstr(&a_main, output, " ");
//
//    Ir* struct_index = NULL;
//    unwrap(ir_lookup(&struct_index,  load->index));
//    if (struct_index->type == IR_LOAD_ANOTHER_IR) {
//        string_extend_cstr(&a_main, output, "%");
//        ir_extend_name(output, ir_get_name(struct_index));
//    } else {
//        emit_operator_operand(output, load->index);
//    }
//
//    vec_append(&a_main, output, '\n');
//}
//
//static void emit_expr(String* output, String* literals, const Ir_expr* expr) {
//    switch (expr->type) {
//        case IR_OPERATOR:
//            emit_operator(output, ir_operator_const_unwrap(expr));
//            return;
//        case IR_FUNCTION_CALL:
//            emit_function_call(output, literals, ir_function_call_const_unwrap(expr));
//            return;
//        case IR_LITERAL:
//            extend_literal_decl(output, literals, ir_literal_const_unwrap(expr), true);
//            return;
//    }
//    unreachable("");
//}
//
//static void emit_def(String* struct_defs, String* output, String* literals, const Ir_def* def) {
//    switch (def->type) {
//        case IR_FUNCTION_DEF:
//            emit_function_def(struct_defs, output, literals, ir_function_def_const_unwrap(def));
//            return;
//        case IR_VARIABLE_DEF:
//            return;
//        case IR_FUNCTION_DECL:
//            todo();
//            emit_function_decl(output, ir_function_decl_const_unwrap(def));
//            return;
//        case IR_LABEL:
//            emit_label(output, ir_label_const_unwrap(def));
//            return;
//        case IR_STRUCT_DEF:
//            emit_struct_def(struct_defs, ir_struct_def_const_unwrap(def));
//            return;
//        case IR_PRIMITIVE_DEF:
//            todo();
//        case IR_LITERAL_DEF:
//            todo();
//    }
//    unreachable("");
//}
//
//static void emit_block(String* struct_defs, String* output, String* literals, const Ir_block* block) {
//    for (size_t idx = 0; idx < block->children.info.count; idx++) {
//        const Ir* stmt = vec_at(block->children, idx);
//        switch (stmt->type) {
//            case IR_EXPR:
//                emit_expr(output, literals, ir_expr_const_unwrap(stmt));
//                break;
//            case IR_DEF:
//                emit_def(struct_defs, output, literals, ir_def_const_unwrap(stmt));
//                break;
//            case IR_RETURN:
//                emit_return(output, ir_return_const_unwrap(stmt));
//                break;
//            case IR_BLOCK:
//                emit_block(struct_defs, output, literals, ir_block_const_unwrap(stmt));
//                break;
//            case IR_COND_GOTO:
//                emit_cond_goto(output, ir_cond_goto_const_unwrap(stmt));
//                break;
//            case IR_GOTO:
//                emit_goto(output, ir_goto_const_unwrap(stmt));
//                break;
//            case IR_ALLOCA:
//                emit_alloca(output, ir_alloca_const_unwrap(stmt));
//                break;
//            case IR_LOAD_ELEMENT_PTR:
//                emit_load_element_ptr(output, ir_load_element_ptr_const_unwrap(stmt));
//                break;
//            case IR_ARRAY_ACCESS:
//                emit_array_access(output, ir_array_access_const_unwrap(stmt));
//                break;
//            case IR_LOAD_ANOTHER_IR:
//                emit_load_another_ir(output, ir_load_another_ir_const_unwrap(stmt));
//                break;
//            case IR_STORE_ANOTHER_IR:
//                emit_store_another_ir(output, literals, ir_store_another_ir_const_unwrap(stmt));
//                break;
//            default:
//                log(LOG_ERROR, FMT"\n", string_print(*output));
//                ir_printf(stmt);
//                todo();
//        }
//    }
//
//    Alloca_iter iter = ir_tbl_iter_new(block->scope_id);
//    Ir* curr = NULL;
//    while (ir_tbl_iter_next(&curr, &iter)) {
//        emit_out_of_line(struct_defs, output, literals, curr);
//    }
//}
//
//// this is only intended for alloca_table, etc.
//static void emit_def_out_of_line(String* struct_defs, String* output, String* literals, const Ir_def* def) {
//    switch (def->type) {
//        case IR_FUNCTION_DEF:
//            emit_function_def(struct_defs, output, literals, ir_function_def_const_unwrap(def));
//            return;
//        case IR_VARIABLE_DEF:
//            return;
//        case IR_FUNCTION_DECL:
//            emit_function_decl(output, ir_function_decl_const_unwrap(def));
//            return;
//        case IR_LABEL:
//            return;
//        case IR_STRUCT_DEF:
//            emit_struct_def(struct_defs, ir_struct_def_const_unwrap(def));
//            return;
//        case IR_PRIMITIVE_DEF:
//            todo();
//        case IR_LITERAL_DEF:
//            todo();
//    }
//    unreachable("");
//}
//
//// this is only intended for alloca_table, etc.
//static void emit_out_of_line(String* struct_defs, String* output, String* literals, const Ir* ir) {
//    switch (ir->type) {
//        case IR_DEF:
//            emit_def_out_of_line(struct_defs, output, literals, ir_def_const_unwrap(ir));
//            return;
//        case IR_BLOCK:
//            unreachable("");
//        case IR_EXPR:
//            return;
//        case IR_LOAD_ELEMENT_PTR:
//            return;
//        case IR_ARRAY_ACCESS:
//            return;
//        case IR_FUNCTION_PARAMS:
//            unreachable("");
//        case IR_RETURN:
//            unreachable("");
//            return;
//        case IR_GOTO:
//            unreachable("");
//            return;
//        case IR_ALLOCA:
//            return;
//        case IR_COND_GOTO:
//            unreachable("");
//            return;
//        case IR_STORE_ANOTHER_IR:
//            return;
//        case IR_LOAD_ANOTHER_IR:
//            return;
//    }
//    unreachable("");
//}
//
//static void emit_symbol_normal(String* literals, Name key, const Ir_literal* lit) {
//    Strv data = {0};
//    switch (lit->type) {
//        case IR_STRING:
//            data = ir_string_const_unwrap(lit)->data;
//            break;
//        default:
//            todo();
//    }
//
//    size_t literal_width = data.count + 1 - get_count_excape_seq(data);
//
//    string_extend_cstr(&a_main, literals, "@.");
//    ir_extend_name(literals, key);
//    string_extend_cstr(&a_main, literals, " = private unnamed_addr constant [ ");
//    string_extend_size_t(&a_main, literals, literal_width);
//    string_extend_cstr(&a_main, literals, " x i8] c\"");
//    string_extend_strv_eval_escapes(&a_main, literals, data);
//    string_extend_cstr(&a_main, literals, "\\00\", align 1");
//    string_extend_cstr(&a_main, literals, "\n");
//}
//
//void emit_llvm_from_tree(const Ir_block* root) {
//    String struct_defs = {0};
//    String output = {0};
//    String literals = {0};
//
//    Alloca_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
//    Ir* curr = NULL;
//    while (ir_tbl_iter_next(&curr, &iter)) {
//        emit_out_of_line(&struct_defs, &output, &literals, curr);
//    }
//
//    emit_block(&struct_defs, &output, &literals, root);
//
//    FILE* file = fopen("test.ll", "w");
//    if (!file) {
//        msg(
//            DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN, "could not open file "FMT": %s\n",
//            strv_print(params.input_file_path), strerror(errno)
//        );
//        exit(EXIT_CODE_FAIL);
//    }
//
//    file_extend_strv(file, string_to_strv(struct_defs));
//    file_extend_strv(file, string_to_strv(output));
//    file_extend_strv(file, string_to_strv(literals));
//
//    msg(
//        DIAG_FILE_BUILT, POS_BUILTIN, "file "FMT" built\n",
//        strv_print(params.input_file_path)
//    );
//
//    fclose(file);
//}
//

void emit_llvm_from_tree(const Ir_block* root) {
    (void) root;
    todo();
}
