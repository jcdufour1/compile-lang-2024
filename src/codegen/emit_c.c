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

static void emit_c_block(String* struct_defs, String* output, String* literals, const Llvm_block* block);

// TODO: see if this can be merged with extend_type_call_str in emit_llvm.c in some way
static void extend_type_call_str(String* output, Lang_type lang_type) {
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
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lang_type);
            return;
        case LANG_TYPE_SUM:
            llvm_extend_name(output, lang_type_sum_const_unwrap(lang_type).atom.str);
            return;
    }
    unreachable("");
}

static void emit_c_function_def(String* struct_defs, String* output, String* literals, const Llvm_function_def* fun_def) {
    extend_type_call_str(output, fun_def->decl->return_type);
    string_extend_cstr(&a_main, output, " ");
    llvm_extend_name(output, llvm_tast_get_name(llvm_def_const_wrap(llvm_function_def_const_wrap(fun_def))));

    vec_append(&a_main, output, '(');
    if (fun_def->decl->params->params.info.count < 1) {
        string_extend_cstr(&a_main, output, "void");
    } else {
        todo();
        //emit_c_function_params(output, fun_def->decl->params);
    }
    vec_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");
    emit_c_block(struct_defs, output, literals, fun_def->body);
    string_extend_cstr(&a_main, output, "}\n");
}

// this is only intended for alloca_table, etc.
static void emit_c_def_sometimes(String* struct_defs, String* output, String* literals, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            emit_c_function_def(struct_defs, output, literals, llvm_function_def_const_unwrap(def));
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

static void emit_c_sometimes(String* struct_defs, String* output, String* literals, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            emit_c_def_sometimes(struct_defs, output, literals, llvm_def_const_unwrap(llvm));
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

static void emit_c_function_call(String* output, String* literals, const Llvm_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, output, "    ");
    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
        extend_type_call_str(output, fun_call->lang_type);
        string_extend_cstr(&a_main, output, "%");
        llvm_extend_name(output, fun_call->name_self);
        string_extend_cstr(&a_main, output, " = ");
    } else {
        assert(!str_view_cstr_is_equal(lang_type_get_str(fun_call->lang_type).base, "void"));
    }
    //extend_type_call_str(output, fun_call->lang_type);
    Llvm* callee = NULL;
    unwrap(alloca_lookup(&callee, fun_call->callee));

    switch (callee->type) {
        case LLVM_EXPR:
            llvm_extend_name(output, llvm_function_name_unwrap(llvm_literal_unwrap(llvm_expr_unwrap(((callee)))))->fun_name);
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_extend_name(output, llvm_load_another_llvm_const_unwrap(callee)->name);
            break;
        default:
            unreachable("");
    }
    //string_extend_strv(&a_main, output, fun_call->name_fun_to_call);

    // arguments
    string_extend_cstr(&a_main, output, "(");
    if (fun_call->args.info.count > 0) {
        todo();
    }
    //emit_function_call_arguments(output, literals, fun_call);
    string_extend_cstr(&a_main, output, ");\n");
}

static void emit_c_expr(String* output, String* literals, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            todo();
            //emit_c_operator(output, llvm_operator_const_unwrap(expr));
            return;
        case LLVM_FUNCTION_CALL:
            emit_c_function_call(output, literals, llvm_function_call_const_unwrap(expr));
            return;
        case LLVM_LITERAL:
            todo();
            //extend_literal_decl(output, literals, llvm_literal_const_unwrap(expr), true);
            return;
    }
    unreachable("");
}

static void extend_c_literal(String* output, const Llvm_literal* literal) {
    switch (literal->type) {
        case LLVM_STRING:
            string_extend_strv(&a_main, output, llvm_string_const_unwrap(literal)->data);
            return;
        case LLVM_NUMBER:
            string_extend_int64_t(&a_main, output, llvm_number_const_unwrap(literal)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_FUNCTION_NAME:
            llvm_extend_name(output, llvm_function_name_const_unwrap(literal)->fun_name);
            return;
    }
    unreachable("");
}

static void emit_c_return_expr(String* output, const Llvm_expr* child) {
    switch (child->type) {
        case LLVM_LITERAL: {
            const Llvm_literal* literal = llvm_literal_const_unwrap(child);
            string_extend_cstr(&a_main, output, "    return ");
            extend_c_literal(output, literal);
            string_extend_cstr(&a_main, output, ";\n");
            break;
        }
        case LLVM_OPERATOR:
            // fallthrough
        case LLVM_FUNCTION_CALL:
            string_extend_cstr(&a_main, output, "    return ");
            llvm_extend_name(output, llvm_expr_get_name(child));
            string_extend_cstr(&a_main, output, ";\n");
            break;
        default:
            unreachable(TAST_FMT"\n", llvm_expr_print(child));
    }
}

static void emit_c_return(String* output, const Llvm_return* fun_return) {
    Llvm* sym_to_return = NULL;
    unwrap(alloca_lookup(&sym_to_return, fun_return->child));

    switch (sym_to_return->type) {
        case LLVM_EXPR:
            emit_c_return_expr(output, llvm_expr_const_unwrap(sym_to_return));
            return;
        case LLVM_LOAD_ANOTHER_LLVM: {
            const Llvm_load_another_llvm* load = llvm_load_another_llvm_const_unwrap(sym_to_return);
            string_extend_cstr(&a_main, output, "    return ");
            llvm_extend_name(output, load->name);
            string_extend_cstr(&a_main, output, ";\n");
            return;
        }
        default:
            unreachable(TAST_FMT"\n", llvm_print(sym_to_return));
    }
}

static void emit_c_block(String* struct_defs, String* output, String* literals, const Llvm_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        const Llvm* stmt = vec_at(&block->children, idx);
        switch (stmt->type) {
            case LLVM_EXPR:
                emit_c_expr(output, literals, llvm_expr_const_unwrap(stmt));
                break;
            case LLVM_DEF:
                todo();
                //emit_def(struct_defs, output, literals, llvm_def_const_unwrap(stmt));
                break;
            case LLVM_RETURN:
                emit_c_return(output, llvm_return_const_unwrap(stmt));
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
                todo();
                //emit_alloca(output, llvm_alloca_const_unwrap(stmt));
                break;
            case LLVM_LOAD_ELEMENT_PTR:
                todo();
                //emit_load_element_ptr(output, llvm_load_element_ptr_const_unwrap(stmt));
                break;
            case LLVM_LOAD_ANOTHER_LLVM:
                todo();
                //emit_load_another_llvm(output, llvm_load_another_llvm_const_unwrap(stmt));
                break;
            case LLVM_STORE_ANOTHER_LLVM:
                todo();
                //emit_store_another_llvm(output, literals, llvm_store_another_llvm_const_unwrap(stmt));
                break;
            default:
                log(LOG_ERROR, STRING_FMT"\n", string_print(*output));
                llvm_printf(stmt);
                todo();
        }
    }

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        emit_c_sometimes(struct_defs, output, literals, curr);
    }
}

void emit_c_from_tree(const Llvm_block* root) {
    String struct_defs = {0};
    String output = {0};
    String literals = {0};

    Alloca_iter iter = all_tbl_iter_new(0);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        emit_c_sometimes(&struct_defs, &output, &literals, curr);
    }

    emit_c_block(&struct_defs, &output, &literals, root);

    FILE* file = fopen("test.c", "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < struct_defs.info.count; idx++) {
        if (EOF == fputc(vec_at(&struct_defs, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < output.info.count; idx++) {
        if (EOF == fputc(vec_at(&output, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < literals.info.count; idx++) {
        if (EOF == fputc(vec_at(&literals, idx), file)) {
            todo();
        }
    }

    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, (File_path_to_text) {0}, dummy_pos, "file %s built\n",
        params.input_file_name
    );

    fclose(file);
}
