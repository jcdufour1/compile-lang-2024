#include <llvm.h>
#include <util.h>
#include <symbol_iter.h>
#include <msg.h>
#include <errno.h>
#include <llvm_utils.h>
#include <lang_type.h>
#include <lang_type_after.h>
#include <common.h>
#include <lang_type_get_pos.h>
#include <lang_type_print.h>
#include <lang_type_serialize.h>
#include <parser_utils.h>

typedef struct {
    String struct_defs;
    String output;
    String literals;
    String forward_decls;
} Emit_c_strs;

static void emit_c_block(Emit_c_strs* strs,  const Llvm_block* block);

static void emit_c_symbol_normal(String* literals, Name key, const Llvm_literal* lit);

static void emit_c_expr_piece(Emit_c_strs* strs, Name child);

// TODO: see if this can be merged with extend_type_call_str in emit_llvm.c in some way
static void c_extend_type_call_str(String* output, Lang_type lang_type) {
    if (lang_type_get_pointer_depth(lang_type) != 0) {
        string_extend_cstr(&a_main, output, "ptr");
        return;
    }

    switch (lang_type.type) {
        case LANG_TYPE_FN:
            unreachable("");
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_STRUCT:
            llvm_extend_name(output, lang_type_struct_const_unwrap(lang_type).atom.str);
            return;
        case LANG_TYPE_RAW_UNION:
            llvm_extend_name(output, lang_type_raw_union_const_unwrap(lang_type).atom.str);
            return;
        case LANG_TYPE_ENUM:
            lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(
                lang_type_signed_int_new(lang_type_get_pos(lang_type), 64, 0)
            )),
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lang_type);
            return;
        case LANG_TYPE_VOID:
            lang_type = lang_type_void_const_wrap(lang_type_void_new(lang_type_get_pos(lang_type)));
            string_extend_strv(&a_main, output, serialize_lang_type(lang_type));
            return;
        case LANG_TYPE_PRIMITIVE:
            if (lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_UNSIGNED_INT) {
                Lang_type_unsigned_int old_num = lang_type_unsigned_int_const_unwrap(lang_type_primitive_const_unwrap(lang_type));
                lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_get_pos(lang_type), old_num.bit_width, old_num.pointer_depth)));
            }
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_C, lang_type);
            return;
        case LANG_TYPE_SUM:
            llvm_extend_name(output, lang_type_sum_const_unwrap(lang_type).atom.str);
            return;
    }
    unreachable("");
}

static void emit_c_function_def_decl(String* output, const Llvm_function_decl* decl) {
    c_extend_type_call_str(output, decl->return_type);
    string_extend_cstr(&a_main, output, " ");
    llvm_extend_name(output, decl->name);

    vec_append(&a_main, output, '(');
    if (decl->params->params.info.count < 1) {
        string_extend_cstr(&a_main, output, "void");
    } else {
        todo();
        //emit_c_function_params(&strs->output, fun_def->decl->params);
    }
    vec_append(&a_main, output, ')');
}

static void emit_c_function_def(Emit_c_strs* strs, const Llvm_function_def* fun_def) {
    emit_c_function_def_decl(&strs->forward_decls, fun_def->decl);
    string_extend_cstr(&a_main, &strs->forward_decls, ";\n");

    emit_c_function_def_decl(&strs->output, fun_def->decl);
    string_extend_cstr(&a_main, &strs->output, " {\n");
    emit_c_block(strs, fun_def->body);
    string_extend_cstr(&a_main, &strs->output, "}\n");
}

// this is only intended for alloca_table, etc.
static void emit_c_def_sometimes(Emit_c_strs* strs, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            emit_c_function_def(strs, llvm_function_def_const_unwrap(def));
            return;
        case LLVM_VARIABLE_DEF:
            return;
        case LLVM_FUNCTION_DECL:
            todo();
            //emit_function_decl(output, llvm_function_decl_const_unwrap(def));
            return;
        case LLVM_LABEL:
            return;
        case LLVM_STRUCT_DEF:
            todo();
            //emit_struct_def(struct_defs, llvm_struct_def_const_unwrap(def));
            return;
        case LLVM_PRIMITIVE_DEF:
            todo();
        case LLVM_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void emit_c_sometimes(Emit_c_strs* strs, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            emit_c_def_sometimes(strs, llvm_def_const_unwrap(llvm));
            return;
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_EXPR:
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            return;
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
            return;
        case LLVM_GOTO:
            unreachable("");
            return;
        case LLVM_ALLOCA:
            return;
        case LLVM_COND_GOTO:
            unreachable("");
            return;
        case LLVM_STORE_ANOTHER_LLVM:
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            return;
    }
    unreachable("");
}

static void emit_c_function_call(Emit_c_strs* strs, const Llvm_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, &strs->output, "    ");
    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
        c_extend_type_call_str(&strs->output, fun_call->lang_type);
        string_extend_cstr(&a_main, &strs->output, " ");
        llvm_extend_name(&strs->output, fun_call->name_self);
        string_extend_cstr(&a_main, &strs->output, " = ");
    } else {
        assert(!str_view_cstr_is_equal(lang_type_get_str(LANG_TYPE_MODE_EMIT_C, fun_call->lang_type).base, "void"));
    }
    //extend_type_call_str(&strs->output, fun_call->lang_type);
    Llvm* callee = NULL;
    unwrap(alloca_lookup(&callee, fun_call->callee));

    switch (callee->type) {
        case LLVM_EXPR:
            llvm_extend_name(&strs->output, llvm_function_name_unwrap(llvm_literal_unwrap(llvm_expr_unwrap(((callee)))))->fun_name);
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_extend_name(&strs->output, llvm_load_another_llvm_const_unwrap(callee)->name);
            break;
        default:
            unreachable("");
    }
    //string_extend_strv(&a_main, &strs->output, fun_call->name_fun_to_call);

    // arguments
    string_extend_cstr(&a_main, &strs->output, "(");
    if (fun_call->args.info.count > 0) {
        todo();
    }
    //emit_function_call_arguments(&strs->output, literals, fun_call);
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_unary_operator(Emit_c_strs* strs, UNARY_TYPE unary_type) {
    (void) strs;
    switch (unary_type) {
        case UNARY_DEREF:
            todo();
        case UNARY_REFER:
            todo();
        case UNARY_UNSAFE_CAST:
            todo();
        case UNARY_NOT:
            todo();
    }
    unreachable("");
}

static void emit_c_binary_operator(Emit_c_strs* strs, BINARY_TYPE bin_type) {
    (void) strs;
    switch (bin_type) {
        case BINARY_SINGLE_EQUAL:
            unreachable("");
        case BINARY_SUB:
            string_extend_cstr(&a_main, &strs->output, " - ");
            return;
        case BINARY_ADD:
            string_extend_cstr(&a_main, &strs->output, " + ");
            return;
        case BINARY_MULTIPLY:
            string_extend_cstr(&a_main, &strs->output, " * ");
            return;
        case BINARY_DIVIDE:
            string_extend_cstr(&a_main, &strs->output, " / ");
            return;
        case BINARY_MODULO:
            todo();
        case BINARY_LESS_THAN:
            string_extend_cstr(&a_main, &strs->output, " < ");
            return;
        case BINARY_LESS_OR_EQUAL:
            string_extend_cstr(&a_main, &strs->output, " <= ");
            return;
        case BINARY_GREATER_OR_EQUAL:
            string_extend_cstr(&a_main, &strs->output, " >= ");
            return;
        case BINARY_GREATER_THAN:
            string_extend_cstr(&a_main, &strs->output, " > ");
            return;
        case BINARY_DOUBLE_EQUAL:
            string_extend_cstr(&a_main, &strs->output, " == ");
            return;
        case BINARY_NOT_EQUAL:
            string_extend_cstr(&a_main, &strs->output, " != ");
            return;
        case BINARY_BITWISE_XOR:
            string_extend_cstr(&a_main, &strs->output, " ^ ");
            return;
        case BINARY_BITWISE_AND:
            string_extend_cstr(&a_main, &strs->output, " & ");
            return;
        case BINARY_BITWISE_OR:
            string_extend_cstr(&a_main, &strs->output, " | ");
            return;
        case BINARY_LOGICAL_AND:
            string_extend_cstr(&a_main, &strs->output, " && ");
            return;
        case BINARY_LOGICAL_OR:
            string_extend_cstr(&a_main, &strs->output, " || ");
            return;
        case BINARY_SHIFT_LEFT:
            string_extend_cstr(&a_main, &strs->output, " << ");
            return;
        case BINARY_SHIFT_RIGHT:
            string_extend_cstr(&a_main, &strs->output, " >> ");
            return;
    }
    unreachable("");
}

static void emit_c_operator(Emit_c_strs* strs, const Llvm_operator* oper) {
    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, llvm_operator_get_lang_type(oper));
    string_extend_cstr(&a_main, &strs->output, " ");
    llvm_extend_name(&strs->output, llvm_operator_get_name(oper));
    string_extend_cstr(&a_main, &strs->output, " = ");

    switch (oper->type) {
        case LLVM_UNARY: {
            const Llvm_unary* unary = llvm_unary_const_unwrap(oper);
            emit_c_unary_operator(strs, unary->token_type);
            emit_c_expr_piece(strs, unary->child);
            break;
        }
        case LLVM_BINARY: {
            const Llvm_binary* bin = llvm_binary_const_unwrap(oper);
            emit_c_expr_piece(strs, bin->lhs);
            emit_c_binary_operator(strs, bin->token_type);
            emit_c_expr_piece(strs, bin->rhs);
            break;
        }
        default:
            unreachable("");
    }

    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_expr(Emit_c_strs* strs, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            emit_c_operator(strs, llvm_operator_const_unwrap(expr));
            return;
        case LLVM_FUNCTION_CALL:
            emit_c_function_call(strs, llvm_function_call_const_unwrap(expr));
            return;
        case LLVM_LITERAL:
            todo();
            //extend_literal_decl(&strs->output, literals, llvm_literal_const_unwrap(expr), true);
            return;
    }
    unreachable("");
}

static void extend_c_literal(Emit_c_strs* strs, const Llvm_literal* literal) {
    switch (literal->type) {
        case LLVM_STRING:
            string_extend_strv(&a_main, &strs->output, llvm_string_const_unwrap(literal)->data);
            return;
        case LLVM_NUMBER:
            string_extend_int64_t(&a_main, &strs->output, llvm_number_const_unwrap(literal)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_FUNCTION_NAME:
            llvm_extend_name(&strs->output, llvm_function_name_const_unwrap(literal)->fun_name);
            return;
    }
    unreachable("");
}

static void emit_c_expr_piece_expr(Emit_c_strs* strs, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_LITERAL: {
            extend_c_literal(strs, llvm_literal_const_unwrap(expr));
            return;
        }
        case LLVM_OPERATOR:
            // fallthrough
        case LLVM_FUNCTION_CALL:
            llvm_extend_name(&strs->output, llvm_expr_get_name(expr));
            return;
    }
    unreachable("");
}

static void emit_c_expr_piece(Emit_c_strs* strs, Name child) {
    Llvm* result = NULL;
    unwrap(alloca_lookup(&result, child));

    switch (result->type) {
        case LLVM_EXPR:
            emit_c_expr_piece_expr(strs, llvm_expr_unwrap(result));
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            todo();
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_DEF:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_BLOCK:
            unreachable("");
    }
    unreachable("");
}

static void emit_c_return(Emit_c_strs* strs, const Llvm_return* rtn) {
    string_extend_cstr(&a_main, &strs->output, "    return ");
    emit_c_expr_piece(strs, rtn->child);
    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_alloca(String* output, const Llvm_alloca* alloca) {
    Name storage_loc = name_new((Str_view) {0}, util_literal_name_new(), (Ulang_type_vec) {0}, SCOPE_BUILTIN);

    string_extend_cstr(&a_main, output, "    ");
    c_extend_type_call_str(output, alloca->lang_type);
    string_extend_cstr(&a_main, output, " ");
    llvm_extend_name(output, storage_loc);
    string_extend_cstr(&a_main, output, ";\n");

    string_extend_cstr(&a_main, output, "    ");
    string_extend_cstr(&a_main, output, "void* ");
    llvm_extend_name(output, alloca->name);
    string_extend_cstr(&a_main, output, " = (void*)&");
    llvm_extend_name(output, storage_loc);
    string_extend_cstr(&a_main, output, ";\n");
}

static void emit_c_def(Emit_c_strs* strs, const Llvm_def* def) {
    (void) strs;
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            todo();
            //emit_function_def(struct_defs, output, literals, llvm_function_def_const_unwrap(def));
            return;
        case LLVM_VARIABLE_DEF:
            return;
        case LLVM_FUNCTION_DECL:
            todo();
            //emit_function_decl(output, llvm_function_decl_const_unwrap(def));
            return;
        case LLVM_LABEL:
            todo();
            //emit_label(output, llvm_label_const_unwrap(def));
            return;
        case LLVM_STRUCT_DEF:
            todo();
            //emit_struct_def(struct_defs, llvm_struct_def_const_unwrap(def));
            return;
        case LLVM_PRIMITIVE_DEF:
            todo();
        case LLVM_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void emit_c_store_another_llvm_src_literal(String* output, const Llvm_literal* literal) {
    string_extend_cstr(&a_main, output, " ");

    switch (literal->type) {
        case LLVM_STRING:
            llvm_extend_name(output, llvm_string_const_unwrap(literal)->name);
            return;
        case LLVM_NUMBER:
            string_extend_int64_t(&a_main, output, llvm_number_const_unwrap(literal)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_FUNCTION_NAME:
            unreachable("");
    }
    unreachable("");
}

static void emit_c_store_another_llvm_src_expr(Emit_c_strs* strs, const Llvm_expr* expr) {
    (void) env;

    switch (expr->type) {
        case LLVM_LITERAL: {
            const Llvm_literal* lit = llvm_literal_const_unwrap(expr);
            if (lit->type == LLVM_STRING) {
                emit_c_symbol_normal(&strs->literals, llvm_literal_get_name(lit), lit);
            }
            emit_c_store_another_llvm_src_literal(&strs->output, lit);
            return;
        }
        case LLVM_FUNCTION_CALL:
            // fallthrough
        case LLVM_OPERATOR:
            llvm_extend_name(&strs->output, llvm_expr_get_name(expr));
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(expr)));
    }
}

static void emit_c_store_another_llvm(Emit_c_strs* strs, const Llvm_store_another_llvm* store) {
    Llvm* src = NULL;
    unwrap(alloca_lookup(&src, store->llvm_src));

    string_extend_cstr(&a_main, &strs->output, "    *((");
    c_extend_type_call_str(&strs->output, store->lang_type);
    string_extend_cstr(&a_main, &strs->output, "*)");
    llvm_extend_name(&strs->output, store->llvm_dest);
    string_extend_cstr(&a_main, &strs->output, ") = ");

    switch (src->type) {
        case LLVM_DEF: {
            todo();
            //const Llvm_def* src_def = llvm_def_const_unwrap(src);
            //const Llvm_variable_def* src_var_def = llvm_variable_def_const_unwrap(src_def);
            //(void) src_var_def;
            //string_extend_cstr(&a_main, output, " %");
            //llvm_extend_name(output, llvm_tast_get_name(src));
            break;
        }
        case LLVM_EXPR:
            emit_c_store_another_llvm_src_expr(strs, llvm_expr_const_unwrap(src));
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            todo();
            //string_extend_cstr(&a_main, output, "%");
            //llvm_extend_name(output, llvm_tast_get_name(src));
            break;
        case LLVM_ALLOCA:
            todo();
            //string_extend_cstr(&a_main, output, " %");
            //llvm_extend_name(output, llvm_tast_get_name(src));
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(src));
    }
    string_extend_cstr(&a_main, &strs->output, ";\n");
    return;
    //string_extend_cstr(&a_main, output, " %");
    //llvm_extend_name(output, llvm_tast_get_name(store->llvm_src.llvm));

    todo();
    //string_extend_cstr(&a_main, output, ", ptr %");
    //llvm_extend_name(output, store->llvm_dest);
    //string_extend_cstr(&a_main, output, ", align 8");
    //string_extend_cstr(&a_main, output, "\n");
}

static void emit_c_load_another_llvm(Emit_c_strs* strs, const Llvm_load_another_llvm* load) {
    log(LOG_DEBUG, TAST_FMT"\n", string_print(strs->output));

    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, load->lang_type);
    string_extend_cstr(&a_main, &strs->output, " ");
    llvm_extend_name(&strs->output, load->name);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "*((");
    c_extend_type_call_str(&strs->output, load->lang_type);
    string_extend_cstr(&a_main, &strs->output, "*)");
    llvm_extend_name(&strs->output, load->llvm_src);

    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_block(Emit_c_strs* strs, const Llvm_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        const Llvm* stmt = vec_at(&block->children, idx);
        switch (stmt->type) {
            case LLVM_EXPR:
                emit_c_expr(strs, llvm_expr_const_unwrap(stmt));
                break;
            case LLVM_DEF:
                emit_c_def(strs, llvm_def_const_unwrap(stmt));
                break;
            case LLVM_RETURN:
                emit_c_return(strs, llvm_return_const_unwrap(stmt));
                break;
            case LLVM_BLOCK:
                todo();
                //emit_block(struct_defs, output, literals, llvm_block_const_unwrap(stmt));
                break;
            case LLVM_COND_GOTO:
                todo();
                //emit_cond_goto(output, llvm_cond_goto_const_unwrap(stmt));
                break;
            case LLVM_GOTO:
                todo();
                //emit_goto(output, llvm_goto_const_unwrap(stmt));
                break;
            case LLVM_ALLOCA:
                emit_c_alloca(&strs->output, llvm_alloca_const_unwrap(stmt));
                break;
            case LLVM_LOAD_ELEMENT_PTR:
                todo();
                //emit_load_element_ptr(output, llvm_load_element_ptr_const_unwrap(stmt));
                break;
            case LLVM_LOAD_ANOTHER_LLVM:
                emit_c_load_another_llvm(strs, llvm_load_another_llvm_const_unwrap(stmt));
                break;
            case LLVM_STORE_ANOTHER_LLVM:
                emit_c_store_another_llvm(strs, llvm_store_another_llvm_const_unwrap(stmt));
                break;
            default:
                log(LOG_ERROR, STRING_FMT"\n", string_print(strs->output));
                llvm_printf(stmt);
                todo();
        }
    }

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        emit_c_sometimes(strs, curr);
    }
}

static void emit_c_symbol_normal(String* literals, Name key, const Llvm_literal* lit) {
    Str_view data = {0};
    switch (lit->type) {
        case LLVM_STRING:
            data = llvm_string_const_unwrap(lit)->data;
            break;
        default:
            todo();
    }

    string_extend_cstr(&a_main, literals, "static const ");
    llvm_extend_name(literals, key);
    string_extend_cstr(&a_main, literals, " = \"");
    string_extend_strv(&a_main, literals, data);
    string_extend_cstr(&a_main, literals, "\"\n");
}

void emit_c_from_tree(const Llvm_block* root) {
    String header = {0};
    Emit_c_strs strs = {0};

    string_extend_cstr(&a_main, &header, "#include <stddef.h>\n");
    string_extend_cstr(&a_main, &header, "#include <stdint.h>\n");

    Alloca_iter iter = all_tbl_iter_new(0);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        emit_c_sometimes(&strs, curr);
    }

    emit_c_block(&strs, root);

    FILE* file = fopen("test.c", "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < header.info.count; idx++) {
        if (EOF == fputc(vec_at(&header, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.forward_decls.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.forward_decls, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.struct_defs.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.struct_defs, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.output.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.output, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.literals.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.literals, idx), file)) {
            todo();
        }
    }

    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "file %s built\n",
        params.input_file_name
    );

    fclose(file);
}
