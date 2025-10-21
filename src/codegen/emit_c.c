#include <ir.h>
#include <util.h>
#include <symbol_iter.h>
#include <msg.h>
#include <errno.h>
#include <ir_utils.h>
#include <ir_lang_type.h>
#include <ir_lang_type_after.h>
#include <codegen_common.h>
#include <ir_lang_type_get_pos.h>
#include <ir_lang_type_print.h>
#include <sizeof.h>
#include <strv_vec.h>
#include <subprocess.h>
#include <file.h>
#include <str_and_num_utils.h>

// TODO: avoid casting from void* to function pointer if possible (for standards compliance)
// TODO: look into using #line to make linker messages print location of .own source code?
typedef struct {
    String struct_defs;
    String output;
    String literals;
    String forward_decls;
} Emit_c_strs;

static void emit_c_block(Emit_c_strs* strs, const Ir_block* block);

static void emit_c_expr_piece(Emit_c_strs* strs, Name child);

static void emit_c_array_access(Emit_c_strs* strs, const Ir_array_access* access);

static void emit_c_loc(String* output, Loc loc, Pos pos) {
    string_extend_cstr(&a_main, output, "/* loc: ");
    string_extend_cstr(&a_main, output, loc.file);
    string_extend_cstr(&a_main, output, ":");
    string_extend_int64_t(&a_main, output, loc.line);
    string_extend_cstr(&a_main, output, " */\n");
    string_extend_cstr(&a_main, output, "/* pos: ");
    string_extend_strv(&a_main, output, pos.file_path);
    string_extend_cstr(&a_main, output, ":");
    string_extend_int64_t(&a_main, output, pos.line);
    string_extend_cstr(&a_main, output, " */\n");
}

// TODO: see if this can be merged with extend_type_call_str in emit_ir.c in some way
static void c_extend_type_call_str(String* output, Ir_lang_type ir_lang_type, bool opaque_ptr) {
    if (opaque_ptr && ir_lang_type_get_pointer_depth(ir_lang_type) != 0) {
        string_extend_cstr(&a_main, output, "void*");
        return;
    }

    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_FN: {
            Ir_lang_type_fn fn = ir_lang_type_fn_const_unwrap(ir_lang_type);
            string_extend_cstr(&a_main, output, "(");
            c_extend_type_call_str(output, *fn.return_type, opaque_ptr);
            string_extend_cstr(&a_main, output, "(*)(");
            for (size_t idx = 0; idx < fn.params.ir_lang_types.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_main, output, ", ");
                }
                c_extend_type_call_str(output, vec_at(&fn.params.ir_lang_types, idx), opaque_ptr);
            }
            string_extend_cstr(&a_main, output, "))");
            return;
        }
        case IR_LANG_TYPE_TUPLE:
            unreachable("");
        case IR_LANG_TYPE_STRUCT:
            ir_extend_name(output, ir_lang_type_struct_const_unwrap(ir_lang_type).atom.str);
            for (size_t idx = 0; idx < (size_t)ir_lang_type_struct_const_unwrap(ir_lang_type).atom.pointer_depth; idx++) {
                string_extend_cstr(&a_main, output, "*");
            }
            return;
        case IR_LANG_TYPE_VOID:
            ir_lang_type = ir_lang_type_void_const_wrap(ir_lang_type_void_new(ir_lang_type_get_pos(ir_lang_type)));
            string_extend_strv(&a_main, output, sv("void"));
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
            string_extend_cstr(&a_main, output, ", ");
        }

        if (vec_at(&params->params, idx)->is_variadic) {
            string_extend_cstr(&a_main, output, "... ");
            assert(idx + 1 == params->params.info.count && "only last parameter may be variadic at this point");
            return;
        }

        c_extend_type_call_str(output, vec_at(&params->params, idx)->lang_type, true);
        string_extend_cstr(&a_main, output, " ");
        ir_extend_name(output, vec_at(&params->params, idx)->name_self);
    }
}

static void emit_c_function_decl_internal(String* output, const Ir_function_decl* decl) {
    emit_c_loc(output, decl->loc, decl->pos);
    c_extend_type_call_str(output, decl->return_type, true);
    string_extend_cstr(&a_main, output, " ");
    ir_extend_name(output, decl->name);

    vec_append(&a_main, output, '(');
    if (decl->params->params.info.count < 1) {
        string_extend_cstr(&a_main, output, "void");
    } else {
        emit_c_function_params(output, decl->params);
    }
    string_extend_cstr(&a_main, output, ")");
}

static void emit_c_function_def(Emit_c_strs* strs, const Ir_function_def* fun_def) {
    emit_c_loc(&strs->output, fun_def->loc, fun_def->pos);
    emit_c_function_decl_internal(&strs->forward_decls, fun_def->decl);
    string_extend_cstr(&a_main, &strs->forward_decls, ";\n");

    emit_c_function_decl_internal(&strs->output, fun_def->decl);
    string_extend_cstr(&a_main, &strs->output, " {\n");
    emit_c_block(strs, fun_def->body);
    string_extend_cstr(&a_main, &strs->output, "}\n");
}

static void emit_c_function_decl(Emit_c_strs* strs, const Ir_function_decl* decl) {
    emit_c_loc(&strs->output, decl->loc, decl->pos);
    emit_c_function_decl_internal(&strs->forward_decls, decl);
    string_extend_cstr(&a_main, &strs->forward_decls, ";\n");
}

static void emit_c_struct_def(Emit_c_strs* strs, const Ir_struct_def* def) {
    emit_c_loc(&strs->output, def->loc, def->pos);
    String buf = {0};
    Arena a_temp = {0};
    string_extend_cstr(&a_temp, &buf, "typedef struct {\n");

    for (size_t idx = 0; idx < def->base.members.info.count; idx++) {
        Ir_variable_def* curr = vec_at(&def->base.members, idx);
        string_extend_cstr(&a_temp, &buf, "    ");
        Ir_lang_type ir_lang_type = {0};
        if (llvm_is_struct_like(vec_at(&def->base.members, idx)->lang_type.type)) {
            Name ori_name = ir_lang_type_get_str(LANG_TYPE_MODE_LOG, vec_at(&def->base.members, idx)->lang_type);
            Name* struct_to_use = NULL;
            if (!c_forward_struct_tbl_lookup(&struct_to_use, ori_name)) {
                Ir* child_def_  = NULL;
                unwrap(ir_lookup(&child_def_, ori_name));
                Ir_struct_def* child_def = ir_struct_def_unwrap(ir_def_unwrap(child_def_));
                struct_to_use = arena_alloc(&a_main, sizeof(*struct_to_use));
                *struct_to_use = util_literal_name_new();
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
        c_extend_type_call_str(&buf, ir_lang_type, true);
        string_extend_cstr(&a_temp, &buf, " ");
        ir_extend_name(&buf, curr->name_self);
        string_extend_cstr(&a_temp, &buf, ";\n");
    }

    string_extend_cstr(&a_temp, &buf, "} ");
    ir_extend_name(&buf, def->base.name);
    string_extend_cstr(&a_temp, &buf, ";\n");

    string_extend_strv(&a_main, &strs->struct_defs, string_to_strv(buf));
    arena_free(&a_temp);
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
            return;
        case IR_GOTO:
            unreachable("");
            return;
        case IR_ALLOCA:
            return;
        case IR_COND_GOTO:
            unreachable("");
            return;
        case IR_STORE_ANOTHER_IR:
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_ARRAY_ACCESS:
            return;
        case IR_IMPORT_PATH:
            emit_c_import_path(strs, ir_import_path_const_unwrap(ir));
            return;
        case IR_REMOVED:
            return;
    }
    unreachable("");
}

static void emit_c_function_call(Emit_c_strs* strs, const Ir_function_call* fun_call) {
    emit_c_loc(&strs->output, fun_call->loc, fun_call->pos);
    //assert(fun_call->ir_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, &strs->output, "    ");
    if (fun_call->lang_type.type != IR_LANG_TYPE_VOID) {
        c_extend_type_call_str(&strs->output, fun_call->lang_type, true);
        string_extend_cstr(&a_main, &strs->output, " ");
        ir_extend_name(&strs->output, fun_call->name_self);
        string_extend_cstr(&a_main, &strs->output, " = ");
    } else {
        //assert(!strv_cstr_is_equal(ir_lang_type_get_str(LANG_TYPE_MODE_EMIT_C, fun_call->lang_type).base, "void"));
    }

    Ir* callee = NULL;
    unwrap(ir_lookup(&callee, fun_call->callee));
    string_extend_cstr(&a_main, &strs->output, "(");
    if (callee->type != IR_EXPR) {
        c_extend_type_call_str(&strs->output, lang_type_from_get_name(fun_call->callee), false);
    }
    string_extend_cstr(&a_main, &strs->output, "(");
    emit_c_expr_piece(strs, fun_call->callee);
    string_extend_cstr(&a_main, &strs->output, ")");
    string_extend_cstr(&a_main, &strs->output, ")");

    // arguments
    string_extend_cstr(&a_main, &strs->output, "(");
    if (fun_call->args.info.count > 0) {
        for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &strs->output, ", ");
            }
            emit_c_expr_piece(strs, vec_at(&fun_call->args, idx));
        }
    }
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_unary_operator(Emit_c_strs* strs, UNARY_TYPE unary_type, Ir_lang_type cast_to) {
    (void) strs;
    // TODO: replace Ir_unary with Ir_cast_to to simplify codegen (this may not be doable if new ir unary operations are added)
    switch (unary_type) {
        case UNARY_DEREF:
            unreachable("defer should not make it here");
        case UNARY_REFER:
            unreachable("refer should not make it here");
        case UNARY_UNSAFE_CAST:
            string_extend_cstr(&a_main, &strs->output, "(");
            c_extend_type_call_str(&strs->output, cast_to, true);
            string_extend_cstr(&a_main, &strs->output, ")");
            return;
        case UNARY_LOGICAL_NOT:
            unreachable("not should not make it here");
        case UNARY_SIZEOF:
            unreachable("sizeof should not make it here");
        case UNARY_COUNTOF:
            unreachable("countof should not make it here");
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
            string_extend_cstr(&a_main, &strs->output, " % ");
            return;
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

static void emit_c_operator(Emit_c_strs* strs, const Ir_operator* oper) {
    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, ir_operator_get_lang_type(oper), true);
    string_extend_cstr(&a_main, &strs->output, " ");
    ir_extend_name(&strs->output, ir_operator_get_name(oper));
    string_extend_cstr(&a_main, &strs->output, " = ");

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

    string_extend_cstr(&a_main, &strs->output, ";\n");
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
            string_extend_cstr(&a_main, &strs->output, "\"");
            string_extend_strv(&a_main, &strs->output, ir_string_const_unwrap(lit)->data);
            string_extend_cstr(&a_main, &strs->output, "\"");
            return;
        case IR_INT:
            string_extend_int64_t(&a_main, &strs->output, ir_int_const_unwrap(lit)->data);
            return;
        case IR_FLOAT:
            string_extend_double(&a_main, &strs->output, ir_float_const_unwrap(lit)->data);
            return;
        case IR_VOID:
            return;
        case IR_FUNCTION_NAME:
            ir_extend_name(&strs->output, ir_function_name_const_unwrap(lit)->fun_name);
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
            // fallthrough
        case IR_FUNCTION_CALL:
            ir_extend_name(&strs->output, ir_expr_get_name(expr));
            return;
    }
    unreachable("");
}

static void emit_c_expr_piece(Emit_c_strs* strs, Name child) {
    Ir* result = NULL;
    unwrap(ir_lookup(&result, child));

    switch (result->type) {
        case IR_EXPR:
            emit_c_expr_piece_expr(strs, ir_expr_unwrap(result));
            return;
        case IR_ARRAY_ACCESS:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_LOAD_ELEMENT_PTR:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_STORE_ANOTHER_IR:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_LOAD_ANOTHER_IR:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_FUNCTION_PARAMS:
            unreachable("");
        case IR_DEF:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_RETURN:
            unreachable("");
        case IR_GOTO:
            unreachable("");
        case IR_ALLOCA:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_COND_GOTO:
            unreachable("");
        case IR_BLOCK:
            ir_extend_name(&strs->output, ir_tast_get_name(result));
            return;
        case IR_IMPORT_PATH:
            unreachable("");
            return;
        case IR_REMOVED:
            return;
    }
    unreachable("");
}

static void emit_c_return(Emit_c_strs* strs, const Ir_return* rtn) {
    emit_c_loc(&strs->output, rtn->loc, rtn->pos);
    string_extend_cstr(&a_main, &strs->output, "    return ");
    emit_c_expr_piece(strs, rtn->child);
    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_alloca(String* output, const Ir_alloca* alloca) {
    emit_c_loc(output, alloca->loc, alloca->pos);
    Name storage_loc = util_literal_name_new();

    string_extend_cstr(&a_main, output, "    ");
    c_extend_type_call_str(output, ir_lang_type_pointer_depth_dec(alloca->lang_type), true);
    string_extend_cstr(&a_main, output, " ");
    ir_extend_name(output, storage_loc);
    string_extend_cstr(&a_main, output, ";\n");

    string_extend_cstr(&a_main, output, "    ");
    string_extend_cstr(&a_main, output, "void* ");
    ir_extend_name(output, alloca->name);
    string_extend_cstr(&a_main, output, " = (void*)&");
    ir_extend_name(output, storage_loc);
    string_extend_cstr(&a_main, output, ";\n");
}

static void emit_c_label(Emit_c_strs* strs, const Ir_label* def) {
    emit_c_loc(&strs->output, def->loc, def->pos);
    ir_extend_name(&strs->output, def->name);
    string_extend_cstr(&a_main, &strs->output, ":\n");
    // supress c compiler warnings and allow non-c23 compilers
    string_extend_cstr(&a_main, &strs->output, "    dummy = 0;\n");
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
    emit_c_loc(&strs->output, store->loc, store->pos);
    Ir* src = NULL;
    unwrap(ir_lookup(&src, store->ir_src));

    string_extend_cstr(&a_main, &strs->output, "    *((");
    c_extend_type_call_str(&strs->output, store->lang_type, true);
    string_extend_cstr(&a_main, &strs->output, "*)");
    ir_extend_name(&strs->output, store->ir_dest);
    string_extend_cstr(&a_main, &strs->output, ") = ");

    emit_c_expr_piece(strs, store->ir_src);
    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_load_another_ir(Emit_c_strs* strs, const Ir_load_another_ir* load) {
    emit_c_loc(&strs->output, load->loc, load->pos);
    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, load->lang_type, true);
    string_extend_cstr(&a_main, &strs->output, " ");
    ir_extend_name(&strs->output, load->name);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "*((");
    c_extend_type_call_str(&strs->output, load->lang_type, true);
    string_extend_cstr(&a_main, &strs->output, "*)");
    ir_extend_name(&strs->output, load->ir_src);

    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_load_element_ptr(Emit_c_strs* strs, const Ir_load_element_ptr* load) {
    emit_c_loc(&strs->output, load->loc, load->pos);
    Ir* struct_def_ = NULL;
    unwrap(ir_lookup(&struct_def_, ir_lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type_from_get_name(load->ir_src))));

    string_extend_cstr(&a_main, &strs->output, "    void* ");
    ir_extend_name(&strs->output, load->name_self);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, lang_type_from_get_name(load->ir_src), false);
    string_extend_cstr(&a_main, &strs->output, ")");
    ir_extend_name(&strs->output, load->ir_src);
    string_extend_cstr(&a_main, &strs->output, ")->");
    ir_extend_name(&strs->output, vec_at(&ir_struct_def_unwrap(ir_def_unwrap(struct_def_))->base.members, load->memb_idx)->name_self);
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_array_access(Emit_c_strs* strs, const Ir_array_access* access) {
    emit_c_loc(&strs->output, access->loc, access->pos);
    string_extend_cstr(&a_main, &strs->output, "    void* ");
    ir_extend_name(&strs->output, access->name_self);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, lang_type_from_get_name(access->callee), false);
    string_extend_cstr(&a_main, &strs->output, ")");
    ir_extend_name(&strs->output, access->callee);
    string_extend_cstr(&a_main, &strs->output, ")[");
    ir_extend_name(&strs->output, access->index);
    string_extend_cstr(&a_main, &strs->output, "]);\n");
}

static void emit_c_goto_internal(Emit_c_strs* strs, Name label) {
    string_extend_cstr(&a_main, &strs->output, "    goto ");
    ir_extend_name(&strs->output, label);
    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_cond_goto(Emit_c_strs* strs, const Ir_cond_goto* cond_goto) {
    emit_c_loc(&strs->output, cond_goto->loc, cond_goto->pos);
    string_extend_cstr(&a_main, &strs->output, "    if (");
    ir_extend_name(&strs->output, cond_goto->condition);
    string_extend_cstr(&a_main, &strs->output, ") {\n");
    emit_c_goto_internal(strs, cond_goto->if_true);
    string_extend_cstr(&a_main, &strs->output, "    } else {\n");
    emit_c_goto_internal(strs, cond_goto->if_false);
    string_extend_cstr(&a_main, &strs->output, "    }\n");
}

static void emit_c_goto(Emit_c_strs* strs, const Ir_goto* lang_goto) {
    emit_c_loc(&strs->output, lang_goto->loc, lang_goto->pos);
    emit_c_goto_internal(strs, lang_goto->label);
}

static void emit_c_block(Emit_c_strs* strs, const Ir_block* block) {
    emit_c_loc(&strs->output, block->loc, block->pos);
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
            default:
                log(LOG_ERROR, FMT"\n", string_print(strs->output));
                ir_printf(stmt);
                todo();
        }
    }}

    Alloca_iter iter = ir_tbl_iter_new(block->scope_id);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        emit_c_out_of_line(strs, curr);
    }
}

void emit_c_from_tree(void) {
    Strv test_output = sv("test.c");
    if (params.stop_after == STOP_AFTER_GEN_BACKEND_IR) {
        test_output = params.output_file_path;
    }

    if (params.compile_own) {
        String header = {0};
        Emit_c_strs strs = {0};

        string_extend_cstr(&a_main, &header, "static int dummy = 0;\n");
        string_extend_cstr(&a_main, &header, "#include <stddef.h>\n");
        string_extend_cstr(&a_main, &header, "#include <stdint.h>\n");
        string_extend_cstr(&a_main, &header, "#include <stdbool.h>\n");
#       ifndef NDEBUG
            string_extend_cstr(&a_main, &header, "#include <assert.h>\n");
#       endif // NDEBUG

        Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);
        Ir* curr = NULL;
        while (ir_tbl_iter_next(&curr, &iter)) {
            emit_c_out_of_line(&strs, curr);
        }

        FILE* file = fopen(strv_to_cstr(&a_main, test_output), "w");
        if (!file) {
            msg(
                DIAG_FILE_COULD_NOT_OPEN, POS_BUILTIN, "could not open file "FMT" %s\n",
                strv_print(params.input_file_path), strerror(errno)
            );
            exit(EXIT_CODE_FAIL);
        }
        
        file_extend_strv(file, string_to_strv(header));
        file_extend_strv(file, string_to_strv(strs.struct_defs));
        file_extend_strv(file, string_to_strv(strs.forward_decls));
        file_extend_strv(file, string_to_strv(strs.literals));
        file_extend_strv(file, string_to_strv(strs.output));

        msg(DIAG_FILE_BUILT, POS_BUILTIN, "file "FMT" built\n", strv_print(params.input_file_path));
        fclose(file);
    }

    if (params.stop_after == STOP_AFTER_GEN_BACKEND_IR) {
        return;
    }

    {
        static_assert(
            PARAMETERS_COUNT == 17,
            "exhausive handling of params (not all parameters are explicitly handled)"
        );

        Strv_vec cmd = {0};
        vec_append(&a_main, &cmd, sv("clang"));
        vec_append(&a_main, &cmd, sv("-std=c99"));
        vec_append(&a_main, &cmd, sv("-Wno-override-module"));
        vec_append(&a_main, &cmd, sv("-Wno-incompatible-library-redeclaration"));
        vec_append(&a_main, &cmd, sv("-Wno-builtin-requires-header"));
#       ifdef NDEBUG
            vec_append(&a_main, &cmd, sv("-Wno-unused-command-line-argument"));
#       endif // DNDEBUG

        static_assert(OPT_LEVEL_COUNT == 2, "exhausive handling of opt types");
        switch (params.opt_level) {
            case OPT_LEVEL_O0:
                vec_append(&a_main, &cmd, sv("-O0"));
                break;
            case OPT_LEVEL_O2:
                vec_append(&a_main, &cmd, sv("-O2"));
                break;
            default:
                unreachable("");
        }

        vec_append(&a_main, &cmd, sv("-g"));

        // output step
        static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop after states (not all are explicitly handled)");
        switch (params.stop_after) {
            case STOP_AFTER_RUN:
                break;
            case STOP_AFTER_BIN:
                break;
            case STOP_AFTER_LOWER_S:
                vec_append(&a_main, &cmd, sv("-S"));
                break;
            case STOP_AFTER_OBJ:
                vec_append(&a_main, &cmd, sv("-c"));
                break;
            default:
                unreachable("");
        }

        // output file
        vec_append(&a_main, &cmd, sv("-o"));
        vec_append(&a_main, &cmd, params.output_file_path);

        // .own file, compiled to .c
        if (params.compile_own) {
            vec_append(&a_main, &cmd, test_output);
        }

        // non-.own files to build
        for (size_t idx = 0; idx < params.l_flags.info.count; idx++) {
            vec_append(&a_main, &cmd, sv("-l"));
            vec_append(&a_main, &cmd, vec_at(&params.l_flags, idx));
        }
        for (size_t idx = 0; idx < params.static_libs.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.static_libs, idx));
        }
        for (size_t idx = 0; idx < params.dynamic_libs.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.dynamic_libs, idx));
        }
        for (size_t idx = 0; idx < params.c_input_files.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.c_input_files, idx));
        }
        for (size_t idx = 0; idx < params.object_files.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.object_files, idx));
        }
        for (size_t idx = 0; idx < params.lower_s_files.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.lower_s_files, idx));
        }
        for (size_t idx = 0; idx < params.upper_s_files.info.count; idx++) {
            vec_append(&a_main, &cmd, vec_at(&params.upper_s_files, idx));
        }

        int status = subprocess_call(cmd);
        if (status != 0) {
            msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process for the c backend returned exit code %d\n", status);
            msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_main, cmd)));
            exit(EXIT_CODE_FAIL);
        }
    }
}
