#include <ir.h>
#include <util.h>
#include <symbol_iter.h>
#include <msg.h>
#include <errno.h>
#include <ir_utils.h>
#include <ir_lang_type.h>
#include <ir_lang_type_after.h>
#include <codegen_common.h>
#include <ir_lang_type_print.h>
#include <sizeof.h>
#include <strv_vec.h>
#include <subprocess.h>
#include <file.h>
#include <str_and_num_utils.h>
#include <ast_msg.h>

static void emit_c_extend_name(String* output, Ir_name name) {
#   ifndef NDEBUG
        Ir* result = NULL;
        if (ir_lookup(&result, name) && result->type == IR_EXPR && ir_expr_unwrap(result)->type == IR_LITERAL) {
            unreachable("emit_c_expr_piece should probably be used here instead of emit_c_extend_name");
        }
#   endif // NDEBUG

    ir_extend_name(output, name);
}

// TODO: avoid casting from void* to function pointer if possible (for standards compliance)
// TODO: look into using #line to make linker messages print location of .own source code?
typedef struct {
    String struct_defs;
    String output;
    String literals;
    String forward_decls;
} Emit_c_strs;

static void emit_c_block(Emit_c_strs* strs, const Ir_block* block);

static void emit_c_expr_piece(Emit_c_strs* strs, Ir_name child);

static void emit_c_array_access(Emit_c_strs* strs, const Ir_array_access* access);

static void emit_c_loc(String* output, Loc loc, Pos pos) {
    string_extend_cstr(&a_pass, output, "/* loc: ");
    string_extend_cstr(&a_pass, output, loc.file);
    string_extend_cstr(&a_pass, output, ":");
    string_extend_int64_t(&a_pass, output, loc.line);
    string_extend_cstr(&a_pass, output, " */\n");
    string_extend_cstr(&a_pass, output, "/* pos: ");
    string_extend_strv(&a_pass, output, pos.file_path);
    string_extend_cstr(&a_pass, output, ":");
    string_extend_int64_t(&a_pass, output, pos.line);
    string_extend_cstr(&a_pass, output, " */\n");
}

// TODO: see if this can be merged with extend_type_call_str in emit_llvm.c in some way
static void c_extend_type_call_str(String* output, Ir_lang_type ir_lang_type, bool opaque_ptr, bool is_from_fn) {
    if (opaque_ptr && ir_lang_type_get_pointer_depth(ir_lang_type) != 0) {
        string_extend_cstr(&a_pass, output, "void*");
        return;
    }

    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_FN: {
            Ir_lang_type_fn fn = ir_lang_type_fn_const_unwrap(ir_lang_type);
            if (!is_from_fn) {
                string_extend_cstr(&a_pass, output, "(");
            }
            c_extend_type_call_str(output, *fn.return_type, opaque_ptr, true);
            string_extend_cstr(&a_pass, output, "(*)(");
            for (size_t idx = 0; idx < fn.params.ir_lang_types.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_pass, output, ", ");
                }
                c_extend_type_call_str(output, vec_at(fn.params.ir_lang_types, idx), opaque_ptr, true);
            }
            string_extend_cstr(&a_pass, output, ")");
            if (!is_from_fn) {
                string_extend_cstr(&a_pass, output, ")");
            }
            return;
        }
        case IR_LANG_TYPE_TUPLE:
            unreachable("");
        case IR_LANG_TYPE_STRUCT:
            emit_c_extend_name(output, ir_lang_type_struct_const_unwrap(ir_lang_type).atom.str);
            for (size_t idx = 0; idx < (size_t)ir_lang_type_struct_const_unwrap(ir_lang_type).atom.pointer_depth; idx++) {
                string_extend_cstr(&a_pass, output, "*");
            }
            return;
        case IR_LANG_TYPE_VOID:
            ir_lang_type = ir_lang_type_void_const_wrap(ir_lang_type_void_new(ir_lang_type_get_pos(ir_lang_type), 0));
            string_extend_strv(&a_pass, output, sv("void"));
            return;
        case IR_LANG_TYPE_PRIMITIVE:
            extend_ir_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_C, ir_lang_type);
            return;
    }
    unreachable("");
}

static void emit_c_function_params(String* output, const Ir_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_pass, output, ", ");
        }

        if (vec_at(params->params, idx)->is_variadic) {
            string_extend_cstr(&a_pass, output, "... ");
            unwrap(idx + 1 == params->params.info.count && "only last parameter may be variadic at this point");
            return;
        }

        c_extend_type_call_str(output, vec_at(params->params, idx)->lang_type, true, false);
        string_extend_cstr(&a_pass, output, " ");
        emit_c_extend_name(output, vec_at(params->params, idx)->name_self);
    }
}

// TODO: remove IR_LANG_TYPE_TUPLE
static void emit_c_function_decl_internal(String* output, const Ir_function_decl* decl) {
    emit_c_loc(output, ir_get_loc(decl), decl->pos);
    switch (decl->return_type.type) {
        case IR_LANG_TYPE_FN:
            todo();
        case IR_LANG_TYPE_VOID:
            break;
        case IR_LANG_TYPE_STRUCT:
            break;
        case IR_LANG_TYPE_PRIMITIVE:
            break;
        case IR_LANG_TYPE_TUPLE:
            todo();
    }
    c_extend_type_call_str(output, decl->return_type, true, false);
    string_extend_cstr(&a_pass, output, " ");
    emit_c_extend_name(output, decl->name);

    vec_append(&a_pass, output, '(');
    if (decl->params->params.info.count < 1) {
        string_extend_cstr(&a_pass, output, "void");
    } else {
        emit_c_function_params(output, decl->params);
    }
    string_extend_cstr(&a_pass, output, ")");
}

static void emit_c_function_def(Emit_c_strs* strs, const Ir_function_def* fun_def) {
    emit_c_loc(&strs->output, ir_get_loc(fun_def), fun_def->pos);
    emit_c_function_decl_internal(&strs->forward_decls, fun_def->decl);
    string_extend_cstr(&a_pass, &strs->forward_decls, ";\n");

    emit_c_function_decl_internal(&strs->output, fun_def->decl);
    string_extend_cstr(&a_pass, &strs->output, " {\n");
    emit_c_block(strs, fun_def->body);
    string_extend_cstr(&a_pass, &strs->output, "}\n");
}

static void emit_c_function_decl(Emit_c_strs* strs, const Ir_function_decl* decl) {
    emit_c_loc(&strs->output, ir_get_loc(decl), decl->pos);
    //if (strv_is_equal(decl->name.base, sv("own_printf"))) {
    //    vec_foreach(idx, Ir_variable_def*, param, decl->params->params) {
    //        if (idx == 0) {
    //            log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, vec_at(ir_lang_type_struct_const_unwrap(param->lang_type).atom.str.a_genrgs, 0)));
    //        }
    //        log(LOG_DEBUG, FMT"\n", ir_lang_type_print(LANG_TYPE_MODE_LOG, param->lang_type));
    //    }
    //    log(LOG_DEBUG, FMT"\n", ir_print(decl));
    //    todo();
    //}
    emit_c_function_decl_internal(&strs->forward_decls, decl);
    string_extend_cstr(&a_pass, &strs->forward_decls, ";\n");
}

static void emit_c_struct_def(Emit_c_strs* strs, const Ir_struct_def* def) {
    emit_c_loc(&strs->output, ir_get_loc(def), def->pos);
    String buf = {0};
    string_extend_cstr(&a_pass, &buf, "typedef struct {\n");

    for (size_t idx = 0; idx < def->base.members.info.count; idx++) {
        Ir_struct_memb_def* curr = vec_at(def->base.members, idx);
        string_extend_cstr(&a_pass, &buf, "    ");
        Ir_lang_type ir_lang_type = {0};
        if (llvm_is_struct_like(vec_at(def->base.members, idx)->lang_type.type)) {
            Ir_name ori_name = ir_lang_type_get_str(LANG_TYPE_MODE_LOG, vec_at(def->base.members, idx)->lang_type);
            Ir_name* struct_to_use = NULL;
            if (!c_forward_struct_tbl_lookup(&struct_to_use, ori_name)) {
                Ir* child_def_  = NULL;
                unwrap(ir_lookup(&child_def_, ori_name));
                Ir_struct_def* child_def = ir_struct_def_unwrap(ir_def_unwrap(child_def_));
                struct_to_use = arena_alloc(&a_pass, sizeof(*struct_to_use));
                *struct_to_use = util_literal_ir_name_new();
                Ir_struct_def* new_def = ir_struct_def_new(def->pos, ((Ir_struct_def_base) {
                    .members = child_def->base.members,
                    .name = *struct_to_use
                }));
                unwrap(c_forward_struct_tbl_add(struct_to_use, ori_name));
                emit_c_struct_def(strs, new_def);
            }
            ir_lang_type = ir_lang_type_struct_const_wrap(ir_lang_type_struct_new(
                curr->pos,
                ir_lang_type_atom_new(*struct_to_use, ir_lang_type_get_pointer_depth(curr->lang_type))
            ));
        } else {
            ir_lang_type = curr->lang_type;
        }
        c_extend_type_call_str(&buf, ir_lang_type, true, false);
        string_extend_cstr(&a_pass, &buf, " ");
        emit_c_extend_name(&buf, curr->name_self);
        assert(curr->count > 0);
        if (curr->count > 1) {
            string_extend_cstr(&a_pass, &buf, "[");
            string_extend_size_t(&a_pass, &buf, curr->count);
            string_extend_cstr(&a_pass, &buf, "]");
        }
        string_extend_cstr(&a_pass, &buf, ";\n");
    }

    string_extend_cstr(&a_pass, &buf, "} ");
    emit_c_extend_name(&buf, def->base.name);
    string_extend_cstr(&a_pass, &buf, ";\n");

    string_extend_strv(&a_pass, &strs->struct_defs, string_to_strv(buf));
}

static void emit_c_def_out_of_line(Emit_c_strs* strs, const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            emit_c_function_def(strs, ir_function_def_const_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            return;
        case IR_FUNCTION_DECL:
            emit_c_function_decl(strs, ir_function_decl_const_unwrap(def));
            return;
        case IR_LABEL:
            return;
        case IR_STRUCT_DEF:
            emit_c_struct_def(strs, ir_struct_def_const_unwrap(def));
            return;
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void emit_c_import_path(Emit_c_strs* strs, const Ir_import_path* ir) {
    emit_c_block(strs, ir->block);
}

static void emit_c_out_of_line(Emit_c_strs* strs, const Ir* ir) {
    switch (ir->type) {
        case IR_DEF:
            emit_c_def_out_of_line(strs, ir_def_const_unwrap(ir));
            return;
        case IR_BLOCK:
            return;
        case IR_EXPR:
            return;
        case IR_LOAD_ELEMENT_PTR:
            return;
        case IR_FUNCTION_PARAMS:
            unreachable("");
        case IR_RETURN:
            unreachable("");
        case IR_GOTO:
            unreachable("");
        case IR_ALLOCA:
            return;
        case IR_COND_GOTO:
            unreachable("");
        case IR_STORE_ANOTHER_IR:
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_ARRAY_ACCESS:
            return;
        case IR_IMPORT_PATH:
            emit_c_import_path(strs, ir_import_path_const_unwrap(ir));
            return;
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
        case IR_REMOVED:
            return;
    }
    unreachable("");
}

static void emit_c_function_call(Emit_c_strs* strs, const Ir_function_call* fun_call) {
    emit_c_loc(&strs->output, ir_get_loc(fun_call), fun_call->pos);
    //unwrap(fun_call->ir_id == 0);

    // start of actual function call
    string_extend_cstr(&a_pass, &strs->output, "    ");
    if (fun_call->lang_type.type != IR_LANG_TYPE_VOID) {
        c_extend_type_call_str(&strs->output, fun_call->lang_type, true, false);
        string_extend_cstr(&a_pass, &strs->output, " ");
        emit_c_extend_name(&strs->output, fun_call->name_self);
        string_extend_cstr(&a_pass, &strs->output, " = ");
    } else {
        //unwrap(!strv_cstr_is_equal(ir_lang_type_get_str(LANG_TYPE_MODE_EMIT_C, fun_call->lang_type).base, "void"));
    }

    Ir* callee = NULL;
    unwrap(ir_lookup(&callee, fun_call->callee));
    string_extend_cstr(&a_pass, &strs->output, "(");
    if (callee->type != IR_EXPR) {
        c_extend_type_call_str(&strs->output, lang_type_from_ir_name(fun_call->callee), false, false);
    }
    string_extend_cstr(&a_pass, &strs->output, "(");
    emit_c_expr_piece(strs, fun_call->callee);
    string_extend_cstr(&a_pass, &strs->output, ")");
    string_extend_cstr(&a_pass, &strs->output, ")");

    // arguments
    string_extend_cstr(&a_pass, &strs->output, "(");
    if (fun_call->args.info.count > 0) {
        for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_pass, &strs->output, ", ");
            }
            emit_c_expr_piece(strs, vec_at(fun_call->args, idx));
        }
    }
    string_extend_cstr(&a_pass, &strs->output, ");\n");
}

static void emit_c_unary_operator(Emit_c_strs* strs, IR_UNARY_TYPE unary_type, Ir_lang_type cast_to) {
    (void) strs;
    // TODO: replace Ir_unary with Ir_cast_to to simplify codegen (this may not be doable if new ir unary operations are added)
    switch (unary_type) {
        case IR_UNARY_UNSAFE_CAST:
            string_extend_cstr(&a_pass, &strs->output, "(");
            c_extend_type_call_str(&strs->output, cast_to, true, false);
            string_extend_cstr(&a_pass, &strs->output, ")");
            return;
    }
    unreachable("");
}

static void emit_c_binary_operator(Emit_c_strs* strs, IR_BINARY_TYPE bin_type) {
    (void) strs;
    switch (bin_type) {
        case IR_BINARY_SUB:
            string_extend_cstr(&a_pass, &strs->output, " - ");
            return;
        case IR_BINARY_ADD:
            string_extend_cstr(&a_pass, &strs->output, " + ");
            return;
        case IR_BINARY_MULTIPLY:
            string_extend_cstr(&a_pass, &strs->output, " * ");
            return;
        case IR_BINARY_DIVIDE:
            string_extend_cstr(&a_pass, &strs->output, " / ");
            return;
        case IR_BINARY_MODULO:
            string_extend_cstr(&a_pass, &strs->output, " % ");
            return;
        case IR_BINARY_LESS_THAN:
            string_extend_cstr(&a_pass, &strs->output, " < ");
            return;
        case IR_BINARY_LESS_OR_EQUAL:
            string_extend_cstr(&a_pass, &strs->output, " <= ");
            return;
        case IR_BINARY_GREATER_OR_EQUAL:
            string_extend_cstr(&a_pass, &strs->output, " >= ");
            return;
        case IR_BINARY_GREATER_THAN:
            string_extend_cstr(&a_pass, &strs->output, " > ");
            return;
        case IR_BINARY_DOUBLE_EQUAL:
            string_extend_cstr(&a_pass, &strs->output, " == ");
            return;
        case IR_BINARY_NOT_EQUAL:
            string_extend_cstr(&a_pass, &strs->output, " != ");
            return;
        case IR_BINARY_BITWISE_XOR:
            string_extend_cstr(&a_pass, &strs->output, " ^ ");
            return;
        case IR_BINARY_BITWISE_AND:
            string_extend_cstr(&a_pass, &strs->output, " & ");
            return;
        case IR_BINARY_BITWISE_OR:
            string_extend_cstr(&a_pass, &strs->output, " | ");
            return;
        case IR_BINARY_SHIFT_LEFT:
            string_extend_cstr(&a_pass, &strs->output, " << ");
            return;
        case IR_BINARY_SHIFT_RIGHT:
            string_extend_cstr(&a_pass, &strs->output, " >> ");
            return;
        case IR_BINARY_COUNT:
            unreachable("");
    }
    unreachable("");
}

static void emit_c_operator(Emit_c_strs* strs, const Ir_operator* oper) {
    string_extend_cstr(&a_pass, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, ir_operator_get_lang_type(oper), true, false);
    string_extend_cstr(&a_pass, &strs->output, " ");
    emit_c_extend_name(&strs->output, ir_operator_get_name(oper));
    string_extend_cstr(&a_pass, &strs->output, " = ");

    switch (oper->type) {
        case IR_UNARY: {
            const Ir_unary* unary = ir_unary_const_unwrap(oper);
            emit_c_unary_operator(strs, unary->token_type, unary->lang_type);
            emit_c_expr_piece(strs, unary->child);
            break;
        }
        case IR_BINARY: {
            const Ir_binary* bin = ir_binary_const_unwrap(oper);
            emit_c_expr_piece(strs, bin->lhs);
            emit_c_binary_operator(strs, bin->token_type);
            emit_c_expr_piece(strs, bin->rhs);
            break;
        }
        default:
            unreachable("");
    }

    string_extend_cstr(&a_pass, &strs->output, ";\n");
}

static void emit_c_expr(Emit_c_strs* strs, const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            emit_c_operator(strs, ir_operator_const_unwrap(expr));
            return;
        case IR_FUNCTION_CALL:
            emit_c_function_call(strs, ir_function_call_const_unwrap(expr));
            return;
        case IR_LITERAL:
            todo();
            //extend_literal_decl(&strs->output, literals, ir_literal_const_unwrap(expr), true);
            return;
    }
    unreachable("");
}

static void extend_c_literal(Emit_c_strs* strs, const Ir_literal* lit) {
    switch (lit->type) {
        case IR_STRING:
            string_extend_cstr(&a_pass, &strs->output, "\"");
            string_extend_strv(&a_pass, &strs->output, ir_string_const_unwrap(lit)->data);
            string_extend_cstr(&a_pass, &strs->output, "\"");
            return;
        case IR_INT:
            string_extend_int64_t(&a_pass, &strs->output, ir_int_const_unwrap(lit)->data);
            return;
        case IR_FLOAT:
            string_extend_double(&a_pass, &strs->output, ir_float_const_unwrap(lit)->data);
            return;
        case IR_VOID:
            return;
        case IR_FUNCTION_NAME:
            emit_c_extend_name(&strs->output, ir_function_name_const_unwrap(lit)->fun_name);
            return;
    }
    unreachable("");
}

static void emit_c_expr_piece_expr(Emit_c_strs* strs, const Ir_expr* expr) {
    switch (expr->type) {
        case IR_LITERAL: {
            extend_c_literal(strs, ir_literal_const_unwrap(expr));
            return;
        }
        case IR_OPERATOR:
            fallthrough;
        case IR_FUNCTION_CALL:
            emit_c_extend_name(&strs->output, ir_expr_get_name(expr));
            return;
    }
    unreachable("");
}

// use this for expressions that may be literal
static void emit_c_expr_piece(Emit_c_strs* strs, Ir_name child) {
    Ir* result = NULL;
    unwrap(ir_lookup(&result, child));

    switch (result->type) {
        case IR_EXPR:
            emit_c_expr_piece_expr(strs, ir_expr_unwrap(result));
            return;
        case IR_ARRAY_ACCESS:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_LOAD_ELEMENT_PTR:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_STORE_ANOTHER_IR:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_LOAD_ANOTHER_IR:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_FUNCTION_PARAMS:
            unreachable("");
        case IR_DEF:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_RETURN:
            unreachable("");
        case IR_GOTO:
            unreachable("");
        case IR_ALLOCA:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_COND_GOTO:
            unreachable("");
        case IR_BLOCK:
            emit_c_extend_name(&strs->output, ir_get_name(result));
            return;
        case IR_IMPORT_PATH:
            unreachable("");
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
        case IR_REMOVED:
            return;
    }
    unreachable("");
}

static void emit_c_return(Emit_c_strs* strs, const Ir_return* rtn) {
    emit_c_loc(&strs->output, ir_get_loc(rtn), rtn->pos);
    string_extend_cstr(&a_pass, &strs->output, "    return ");
    emit_c_expr_piece(strs, rtn->child);
    string_extend_cstr(&a_pass, &strs->output, ";\n");
}

static void emit_c_alloca(String* output, const Ir_alloca* lang_alloca) {
    emit_c_loc(output, ir_get_loc(lang_alloca), lang_alloca->pos);
    Ir_name storage_loc = util_literal_ir_name_new();

    string_extend_cstr(&a_pass, output, "    ");
    c_extend_type_call_str(output, ir_lang_type_pointer_depth_dec(lang_alloca->lang_type), true, false);
    string_extend_cstr(&a_pass, output, " ");
    emit_c_extend_name(output, storage_loc);
    string_extend_cstr(&a_pass, output, ";\n");

    string_extend_cstr(&a_pass, output, "    ");
    string_extend_cstr(&a_pass, output, "void* ");
    emit_c_extend_name(output, lang_alloca->name);
    string_extend_cstr(&a_pass, output, " = (void*)&");
    emit_c_extend_name(output, storage_loc);
    string_extend_cstr(&a_pass, output, ";\n");
}

static void emit_c_label(Emit_c_strs* strs, const Ir_label* def) {
    emit_c_loc(&strs->output, ir_get_loc(def), def->pos);
    emit_c_extend_name(&strs->output, def->name);
    string_extend_cstr(&a_pass, &strs->output, ":\n");
    // supress c compiler warnings and allow non-c23 compilers
    string_extend_cstr(&a_pass, &strs->output, "    dummy = 0;\n");
}

static void emit_c_def_inline(Emit_c_strs* strs, const Ir_def* def) {
    (void) strs;
    switch (def->type) {
        case IR_FUNCTION_DEF:
            todo();
            //emit_function_def(struct_defs, output, literals, ir_function_def_const_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            return;
        case IR_FUNCTION_DECL:
            todo();
            //emit_function_decl(output, ir_function_decl_const_unwrap(def));
            return;
        case IR_LABEL:
            emit_c_label(strs, ir_label_const_unwrap(def));
            return;
        case IR_STRUCT_DEF:
            todo();
            //emit_struct_def(struct_defs, ir_struct_def_const_unwrap(def));
            return;
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void emit_c_store_another_ir(Emit_c_strs* strs, const Ir_store_another_ir* store) {
    emit_c_loc(&strs->output, ir_get_loc(store), store->pos);
    Ir* src = NULL;
    unwrap(ir_lookup(&src, store->ir_src));

    string_extend_cstr(&a_pass, &strs->output, "    *((");
    c_extend_type_call_str(&strs->output, store->lang_type, true, false);
    string_extend_cstr(&a_pass, &strs->output, "*)");
    emit_c_extend_name(&strs->output, store->ir_dest);
    string_extend_cstr(&a_pass, &strs->output, ") = ");

    emit_c_expr_piece(strs, store->ir_src);
    string_extend_cstr(&a_pass, &strs->output, ";\n");
}

static void emit_c_load_another_ir(Emit_c_strs* strs, const Ir_load_another_ir* load) {
    emit_c_loc(&strs->output, ir_get_loc(load), load->pos);
    string_extend_cstr(&a_pass, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, load->lang_type, true, false);
    string_extend_cstr(&a_pass, &strs->output, " ");
    emit_c_extend_name(&strs->output, load->name);
    string_extend_cstr(&a_pass, &strs->output, " = ");

    string_extend_cstr(&a_pass, &strs->output, "*((");
    c_extend_type_call_str(&strs->output, load->lang_type, true, false);
    string_extend_cstr(&a_pass, &strs->output, "*)");
    emit_c_extend_name(&strs->output, load->ir_src);

    string_extend_cstr(&a_pass, &strs->output, ");\n");
}

static void emit_c_load_element_ptr(Emit_c_strs* strs, const Ir_load_element_ptr* load) {
    emit_c_loc(&strs->output, ir_get_loc(load), load->pos);
    Ir* struct_def_ = NULL;
    unwrap(ir_lookup(&struct_def_, ir_lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type_from_ir_name(load->ir_src))));

    string_extend_cstr(&a_pass, &strs->output, "    void* ");
    emit_c_extend_name(&strs->output, load->name_self);
    string_extend_cstr(&a_pass, &strs->output, " = ");

    string_extend_cstr(&a_pass, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, lang_type_from_ir_name(load->ir_src), false, false);
    string_extend_cstr(&a_pass, &strs->output, ")");
    emit_c_extend_name(&strs->output, load->ir_src);
    string_extend_cstr(&a_pass, &strs->output, ")->");
    emit_c_extend_name(&strs->output, vec_at(ir_struct_def_unwrap(ir_def_unwrap(struct_def_))->base.members, load->memb_idx)->name_self);
    string_extend_cstr(&a_pass, &strs->output, ");\n");
}

static void emit_c_array_access(Emit_c_strs* strs, const Ir_array_access* access) {
    emit_c_loc(&strs->output, ir_get_loc(access), access->pos);
    string_extend_cstr(&a_pass, &strs->output, "    void* ");
    emit_c_extend_name(&strs->output, access->name_self);
    string_extend_cstr(&a_pass, &strs->output, " = ");

    string_extend_cstr(&a_pass, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, access->lang_type, false, false);
    string_extend_cstr(&a_pass, &strs->output, "*)");
    emit_c_extend_name(&strs->output, access->callee);
    string_extend_cstr(&a_pass, &strs->output, ")[");
    emit_c_expr_piece(strs, access->index);
    string_extend_cstr(&a_pass, &strs->output, "]);\n");
}

static void emit_c_goto_internal(Emit_c_strs* strs, Ir_name label) {
    string_extend_cstr(&a_pass, &strs->output, "    goto ");
    emit_c_extend_name(&strs->output, label);
    string_extend_cstr(&a_pass, &strs->output, ";\n");
}

static void emit_c_cond_goto(Emit_c_strs* strs, const Ir_cond_goto* cond_goto) {
    emit_c_loc(&strs->output, ir_get_loc(cond_goto), cond_goto->pos);
    string_extend_cstr(&a_pass, &strs->output, "    if (");
    emit_c_extend_name(&strs->output, cond_goto->condition);
    string_extend_cstr(&a_pass, &strs->output, ") {\n");
    emit_c_goto_internal(strs, cond_goto->if_true);
    string_extend_cstr(&a_pass, &strs->output, "    } else {\n");
    emit_c_goto_internal(strs, cond_goto->if_false);
    string_extend_cstr(&a_pass, &strs->output, "    }\n");
}

static void emit_c_goto(Emit_c_strs* strs, const Ir_goto* lang_goto) {
    emit_c_loc(&strs->output, ir_get_loc(lang_goto), lang_goto->pos);
    emit_c_goto_internal(strs, lang_goto->label);
}

static void emit_c_block(Emit_c_strs* strs, const Ir_block* block) {
    emit_c_loc(&strs->output, ir_get_loc(block), block->pos);
    vec_foreach(idx, Ir*, stmt, block->children) {
        switch (stmt->type) {
            case IR_EXPR:
                emit_c_expr(strs, ir_expr_const_unwrap(stmt));
                break;
            case IR_DEF:
                emit_c_def_inline(strs, ir_def_const_unwrap(stmt));
                break;
            case IR_RETURN:
                emit_c_return(strs, ir_return_const_unwrap(stmt));
                break;
            case IR_BLOCK:
                emit_c_block(strs, ir_block_const_unwrap(stmt));
                break;
            case IR_COND_GOTO:
                emit_c_cond_goto(strs, ir_cond_goto_const_unwrap(stmt));
                break;
            case IR_GOTO:
                emit_c_goto(strs, ir_goto_const_unwrap(stmt));
                break;
            case IR_ALLOCA:
                emit_c_alloca(&strs->output, ir_alloca_const_unwrap(stmt));
                break;
            case IR_LOAD_ELEMENT_PTR:
                emit_c_load_element_ptr(strs, ir_load_element_ptr_const_unwrap(stmt));
                break;
            case IR_ARRAY_ACCESS:
                emit_c_array_access(strs, ir_array_access_const_unwrap(stmt));
                break;
            case IR_LOAD_ANOTHER_IR:
                emit_c_load_another_ir(strs, ir_load_another_ir_const_unwrap(stmt));
                break;
            case IR_STORE_ANOTHER_IR:
                emit_c_store_another_ir(strs, ir_store_another_ir_const_unwrap(stmt));
                break;
            case IR_REMOVED:
                break;
            case IR_IMPORT_PATH:
                unreachable("");
            case IR_STRUCT_MEMB_DEF:
                unreachable("");
            case IR_FUNCTION_PARAMS:
                unreachable("");
            default:
                log(LOG_ERROR, FMT"\n", string_print(strs->output));
                ir_printf(stmt);
                todo();
        }
    }

    Alloca_iter iter = ir_tbl_iter_new(block->scope_id);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        emit_c_out_of_line(strs, curr);
    }
}

void emit_c_from_tree(void) {
    Strv test_output = strv_from_f(
        &a_temp,
        FMT"/"FMT".c",
        strv_print(params.build_dir),
        strv_print(params.output_file_path)
    );
    if (params.stop_after == STOP_AFTER_BACKEND_IR) {
        test_output = params.output_file_path;
    }

    if (params.compile_own) {
        String header = {0};
        Emit_c_strs strs = {0};

        string_extend_cstr(&a_pass, &header, "static int dummy = 0;\n");
        string_extend_cstr(&a_pass, &header, "#include <stddef.h>\n");
        string_extend_cstr(&a_pass, &header, "#include <stdint.h>\n");
        string_extend_cstr(&a_pass, &header, "#include <stdbool.h>\n");
#       ifndef NDEBUG
            string_extend_cstr(&a_pass, &header, "#include <assert.h>\n");
#       endif // NDEBUG

        Alloca_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
        Ir* curr = NULL;
        while (ir_tbl_iter_next(&curr, &iter)) {
            emit_c_out_of_line(&strs, curr);
        }
        if (env.error_count > 0) {
            return;
        }

        FILE* file = fopen(strv_dup(&a_pass, test_output), "w");
        if (!file) {
            msg(
                DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN, "could not open file "FMT" %s\n",
                strv_print(params.input_file_path), strerror(errno)
            );
            local_exit(EXIT_CODE_FAIL);
        }
        
        file_extend_strv(file, string_to_strv(header));
        file_extend_strv(file, string_to_strv(strs.struct_defs));
        file_extend_strv(file, string_to_strv(strs.forward_decls));
        file_extend_strv(file, string_to_strv(strs.literals));
        file_extend_strv(file, string_to_strv(strs.output));

        msg(DIAG_FILE_BUILT, POS_BUILTIN, "file "FMT" built\n", strv_print(params.input_file_path));
        fclose(file);
    }

    if (params.stop_after == STOP_AFTER_BACKEND_IR) {
        return;
    }

    {
        static_assert(
            PARAMETERS_COUNT == 32,
            "exhausive handling of params (not all parameters are explicitly handled)"
        );

        Strv path_c_compiler = {0};
        if (params.is_path_c_compiler) {
            path_c_compiler = params.path_c_compiler;
        } else {
            if (!target_triplet_is_equal(params.target_triplet, get_default_target_triplet())) {
                msg_todo("cross compiling without explicit \"path-c-compiler\" passed to the command line", POS_BUILTIN);
                return;
            }
            path_c_compiler = sv("cc");
        }

        Strv_vec cmd = {0};
        vec_append(&a_pass, &cmd, path_c_compiler);
        vec_append(&a_pass, &cmd, sv("-std=c99"));

        // supress clang warnings
        vec_append(&a_pass, &cmd, sv("-Wno-override-module"));
        vec_append(&a_pass, &cmd, sv("-Wno-incompatible-library-redeclaration"));
        vec_append(&a_pass, &cmd, sv("-Wno-builtin-requires-header"));
        vec_append(&a_pass, &cmd, sv("-Wno-unknown-warning-option"));

        // supress gcc warnings
        vec_append(&a_pass, &cmd, sv("-Wno-builtin-declaration-mismatch"));

#       ifdef NDEBUG
            vec_append(&a_pass, &cmd, sv("-Wno-unused-command-line-argument"));
#       endif // NDEBUG

        static_assert(OPT_LEVEL_COUNT == 2, "exhausive handling of opt types");
        switch (params.opt_level) {
            case OPT_LEVEL_O0:
                vec_append(&a_pass, &cmd, sv("-O0"));
                break;
            case OPT_LEVEL_O2:
                vec_append(&a_pass, &cmd, sv("-O2"));
                break;
            case OPT_LEVEL_COUNT:
                unreachable("");
            default:
                unreachable("");
        }

        vec_append(&a_pass, &cmd, sv("-g"));

        // output step
        static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop after states");
        switch (params.stop_after) {
            case STOP_AFTER_RUN:
                break;
            case STOP_AFTER_BIN:
                break;
            case STOP_AFTER_UPPER_S:
                vec_append(&a_pass, &cmd, sv("-S"));
                break;
            case STOP_AFTER_OBJ:
                vec_append(&a_pass, &cmd, sv("-c"));
                break;
            case STOP_AFTER_NONE:
                unreachable("");
            case STOP_AFTER_IR:
                unreachable("");
            case STOP_AFTER_BACKEND_IR:
                unreachable("");
            case STOP_AFTER_COUNT:
                unreachable("");
            default:
                unreachable("");
        }

        if (params.is_output_file_path) {
            vec_append(&a_pass, &cmd, sv("-o"));
            vec_append(&a_pass, &cmd, params.output_file_path);
        }

        // .own file, compiled to .c
        if (params.compile_own) {
            vec_append(&a_pass, &cmd, test_output);
        }

        // non-.own files to build
        vec_extend(&a_pass, &cmd, &params.static_libs);
        vec_extend(&a_pass, &cmd, &params.c_input_files);
        vec_extend(&a_pass, &cmd, &params.object_files);
        vec_extend(&a_pass, &cmd, &params.lower_s_files);
        vec_extend(&a_pass, &cmd, &params.upper_s_files);
        vec_extend(&a_pass, &cmd, &params.dynamic_libs);
        for (size_t idx = 0; idx < params.l_flags.info.count; idx++) {
            vec_append(&a_pass, &cmd, sv("-l"));
            vec_append(&a_pass, &cmd, vec_at(params.l_flags, idx));
        }

        int status = subprocess_call(cmd);
        if (status != 0) {
            msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process for the c backend returned exit code %d\n", status);
            msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_pass, cmd)));
            local_exit(EXIT_CODE_FAIL);
        }
    }
}
