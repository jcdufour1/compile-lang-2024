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
#include <sizeof.h>
#include <str_view_vec.h>
#include <subprocess.h>

static const char* TEST_OUTPUT = "test.c";

// TODO: avoid casting from void* to function pointer if possible (for standards compliance)
typedef struct {
    String struct_defs;
    String output;
    String literals;
    String forward_decls;
} Emit_c_strs;

static void emit_c_block(Emit_c_strs* strs,  const Llvm_block* block);

static void emit_c_expr_piece(Emit_c_strs* strs, Name child);

// TODO: see if this can be merged with extend_type_call_str in emit_llvm.c in some way
static void c_extend_type_call_str(String* output, Lang_type lang_type, bool opaque_ptr) {
    if (opaque_ptr && lang_type_get_pointer_depth(lang_type) != 0) {
        string_extend_cstr(&a_main, output, "void*");
        return;
    }

    switch (lang_type.type) {
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            string_extend_cstr(&a_main, output, "(");
            c_extend_type_call_str(output, *fn.return_type, opaque_ptr);
            string_extend_cstr(&a_main, output, "(*)(");
            for (size_t idx = 0; idx < fn.params.lang_types.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_main, output, ", ");
                }
                c_extend_type_call_str(output, vec_at(&fn.params.lang_types, idx), opaque_ptr);
            }
            string_extend_cstr(&a_main, output, "))");
            return;
        }
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_STRUCT:
            llvm_extend_name(output, lang_type_struct_const_unwrap(lang_type).atom.str);
            for (size_t idx = 0; idx < (size_t)lang_type_struct_const_unwrap(lang_type).atom.pointer_depth; idx++) {
                string_extend_cstr(&a_main, output, "*");
            }
            return;
        case LANG_TYPE_RAW_UNION:
            llvm_extend_name(output, lang_type_raw_union_const_unwrap(lang_type).atom.str);
            for (size_t idx = 0; idx < (size_t)lang_type_raw_union_const_unwrap(lang_type).atom.pointer_depth; idx++) {
                string_extend_cstr(&a_main, output, "*");
            }
            return;
        case LANG_TYPE_VOID:
            lang_type = lang_type_void_const_wrap(lang_type_void_new(lang_type_get_pos(lang_type)));
            string_extend_strv(&a_main, output, serialize_lang_type(lang_type));
            return;
        case LANG_TYPE_PRIMITIVE:
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_C, lang_type);
            return;
        case LANG_TYPE_ENUM:
            llvm_extend_name(output, lang_type_enum_const_unwrap(lang_type).atom.str);
            return;
    }
    unreachable("");
}

static void emit_c_function_params(String* output, const Llvm_function_params* params) {
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
        llvm_extend_name(output, vec_at(&params->params, idx)->name_self);
    }
}

static void emit_c_function_decl_internal(String* output, const Llvm_function_decl* decl) {
    c_extend_type_call_str(output, decl->return_type, true);
    string_extend_cstr(&a_main, output, " ");
    llvm_extend_name(output, decl->name);

    vec_append(&a_main, output, '(');
    if (decl->params->params.info.count < 1) {
        string_extend_cstr(&a_main, output, "void");
    } else {
        emit_c_function_params(output, decl->params);
    }
    string_extend_cstr(&a_main, output, ")");
}

static void emit_c_function_def(Emit_c_strs* strs, const Llvm_function_def* fun_def) {
    emit_c_function_decl_internal(&strs->forward_decls, fun_def->decl);
    string_extend_cstr(&a_main, &strs->forward_decls, ";\n");

    emit_c_function_decl_internal(&strs->output, fun_def->decl);
    string_extend_cstr(&a_main, &strs->output, " {\n");
    emit_c_block(strs, fun_def->body);
    string_extend_cstr(&a_main, &strs->output, "}\n");
}

static void emit_c_function_decl(Emit_c_strs* strs, const Llvm_function_decl* decl) {
    emit_c_function_decl_internal(&strs->forward_decls, decl);
    string_extend_cstr(&a_main, &strs->forward_decls, ";\n");
}

static void emit_c_struct_def(Emit_c_strs* strs, const Llvm_struct_def* def) {
    String buf = {0};
    Arena a_temp = {0};
    string_extend_cstr(&a_temp, &buf, "typedef struct {\n");

    for (size_t idx = 0; idx < def->base.members.info.count; idx++) {
        Llvm_variable_def* curr = vec_at(&def->base.members, idx);
        string_extend_cstr(&a_temp, &buf, "    ");
        Lang_type lang_type = {0};
        if (is_struct_like(vec_at(&def->base.members, idx)->lang_type.type)) {
            Name ori_name = lang_type_get_str(LANG_TYPE_MODE_LOG, vec_at(&def->base.members, idx)->lang_type);
            Name* struct_to_use = NULL;
            if (!c_forward_struct_tbl_lookup(&struct_to_use, ori_name)) {
                Llvm* child_def_  = NULL;
                unwrap(alloca_lookup(&child_def_, ori_name));
                Llvm_struct_def* child_def = llvm_struct_def_unwrap(llvm_def_unwrap(child_def_));
                struct_to_use = arena_alloc(&a_main, sizeof(*struct_to_use));
                *struct_to_use = util_literal_name_new2();
                Llvm_struct_def* new_def = llvm_struct_def_new(def->pos, (Llvm_struct_def_base) {
                    .members = child_def->base.members,
                    .name = *struct_to_use
                });
                unwrap(c_forward_struct_tbl_add(struct_to_use, ori_name));
                emit_c_struct_def(strs, new_def);
            }
            lang_type = lang_type_struct_const_wrap(lang_type_struct_new(
                curr->pos,
                lang_type_atom_new(*struct_to_use, lang_type_get_pointer_depth(curr->lang_type))
            ));
        } else {
            lang_type = curr->lang_type;
        }
        c_extend_type_call_str(&buf, lang_type, true);
        string_extend_cstr(&a_temp, &buf, " ");
        llvm_extend_name(&buf, curr->name_self);
        string_extend_cstr(&a_temp, &buf, ";\n");
    }

    string_extend_cstr(&a_temp, &buf, "} ");
    llvm_extend_name(&buf, def->base.name);
    string_extend_cstr(&a_temp, &buf, ";\n");

    string_extend_strv(&a_main, &strs->struct_defs, string_to_strv(buf));
    arena_free(&a_temp);
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
            emit_c_function_decl(strs, llvm_function_decl_const_unwrap(def));
            return;
        case LLVM_LABEL:
            return;
        case LLVM_STRUCT_DEF:
            emit_c_struct_def(strs, llvm_struct_def_const_unwrap(def));
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
        case LLVM_ARRAY_ACCESS:
            return;
    }
    unreachable("");
}

static void emit_c_function_call(Emit_c_strs* strs, const Llvm_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, &strs->output, "    ");
    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
        c_extend_type_call_str(&strs->output, fun_call->lang_type, true);
        string_extend_cstr(&a_main, &strs->output, " ");
        llvm_extend_name(&strs->output, fun_call->name_self);
        string_extend_cstr(&a_main, &strs->output, " = ");
    } else {
        assert(!str_view_cstr_is_equal(lang_type_get_str(LANG_TYPE_MODE_EMIT_C, fun_call->lang_type).base, "void"));
    }

    Llvm* callee = NULL;
    unwrap(alloca_lookup(&callee, fun_call->callee));
    string_extend_cstr(&a_main, &strs->output, "(");
    if (callee->type != LLVM_EXPR) {
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
    //emit_function_call_arguments(&strs->output, literals, fun_call);
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_unary_operator(Emit_c_strs* strs, UNARY_TYPE unary_type, Lang_type cast_to) {
    (void) strs;
    switch (unary_type) {
        case UNARY_DEREF:
            todo();
        case UNARY_REFER:
            todo();
        case UNARY_UNSAFE_CAST:
            string_extend_cstr(&a_main, &strs->output, "(");
            c_extend_type_call_str(&strs->output, cast_to, true);
            string_extend_cstr(&a_main, &strs->output, ")");
            return;
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

static void emit_c_operator(Emit_c_strs* strs, const Llvm_operator* oper) {
    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, llvm_operator_get_lang_type(oper), true);
    string_extend_cstr(&a_main, &strs->output, " ");
    llvm_extend_name(&strs->output, llvm_operator_get_name(oper));
    string_extend_cstr(&a_main, &strs->output, " = ");

    switch (oper->type) {
        case LLVM_UNARY: {
            const Llvm_unary* unary = llvm_unary_const_unwrap(oper);
            emit_c_unary_operator(strs, unary->token_type, unary->lang_type);
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

static void extend_c_literal(Emit_c_strs* strs, const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_STRING:
            string_extend_cstr(&a_main, &strs->output, "\"");
            string_extend_strv(&a_main, &strs->output, llvm_string_const_unwrap(lit)->data);
            string_extend_cstr(&a_main, &strs->output, "\"");
            return;
        case LLVM_INT:
            string_extend_int64_t(&a_main, &strs->output, llvm_int_const_unwrap(lit)->data);
            return;
        case LLVM_FLOAT:
            string_extend_double(&a_main, &strs->output, llvm_float_const_unwrap(lit)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_FUNCTION_NAME:
            llvm_extend_name(&strs->output, llvm_function_name_const_unwrap(lit)->fun_name);
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
        case LLVM_ARRAY_ACCESS:
            todo();
        case LLVM_LOAD_ELEMENT_PTR:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_DEF:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            llvm_extend_name(&strs->output, llvm_tast_get_name(result));
            return;
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
    Name storage_loc = util_literal_name_new2();

    string_extend_cstr(&a_main, output, "    ");
    c_extend_type_call_str(output, alloca->lang_type, true);
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

static void emit_c_label(Emit_c_strs* strs, const Llvm_label* def) {
    llvm_extend_name(&strs->output, def->name);
    string_extend_cstr(&a_main, &strs->output, ":\n");
    // supress c compiler warnings and allow non-c23 compilers
    string_extend_cstr(&a_main, &strs->output, "    dummy = 0;\n");
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
            emit_c_label(strs, llvm_label_const_unwrap(def));
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

static void emit_c_store_another_llvm(Emit_c_strs* strs, const Llvm_store_another_llvm* store) {
    Llvm* src = NULL;
    unwrap(alloca_lookup(&src, store->llvm_src));

    if (true /*src->type == LLVM_EXPR && llvm_expr_const_unwrap(src)->type == LLVM_LITERAL*/) {
        string_extend_cstr(&a_main, &strs->output, "    *((");
        c_extend_type_call_str(&strs->output, store->lang_type, true);
        string_extend_cstr(&a_main, &strs->output, "*)");
        llvm_extend_name(&strs->output, store->llvm_dest);
        string_extend_cstr(&a_main, &strs->output, ") = ");

        emit_c_expr_piece(strs, store->llvm_src);
        string_extend_cstr(&a_main, &strs->output, ";\n");
        return;
    }

    string_extend_cstr(&a_main, &strs->output, "    memcpy(");
    emit_c_expr_piece(strs, store->llvm_dest);
    string_extend_cstr(&a_main, &strs->output, ", &");

    emit_c_expr_piece(strs, store->llvm_src);
    string_extend_cstr(&a_main, &strs->output, ", ");
    string_extend_size_t(&a_main, &strs->output, sizeof_lang_type(store->lang_type));
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_load_another_llvm(Emit_c_strs* strs, const Llvm_load_another_llvm* load) {
    string_extend_cstr(&a_main, &strs->output, "    ");
    c_extend_type_call_str(&strs->output, load->lang_type, true);
    string_extend_cstr(&a_main, &strs->output, " ");
    llvm_extend_name(&strs->output, load->name);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "*((");
    c_extend_type_call_str(&strs->output, load->lang_type, true);
    string_extend_cstr(&a_main, &strs->output, "*)");
    llvm_extend_name(&strs->output, load->llvm_src);

    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_load_element_ptr(Emit_c_strs* strs, const Llvm_load_element_ptr* load) {
    Llvm* struct_def_ = NULL;
    unwrap(alloca_lookup(&struct_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type_from_get_name(load->llvm_src))));

    string_extend_cstr(&a_main, &strs->output, "    void* ");
    llvm_extend_name(&strs->output, load->name_self);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, lang_type_from_get_name(load->llvm_src), false);
    // TODO: remove this if statement, and fix the actual issue (this if statement is a temporary hack)
    if (lang_type_get_pointer_depth(lang_type_from_get_name(load->llvm_src)) < 1) {
        string_extend_cstr(&a_main, &strs->output, "*");
    }
    string_extend_cstr(&a_main, &strs->output, ")");
    llvm_extend_name(&strs->output, load->llvm_src);
    string_extend_cstr(&a_main, &strs->output, ")->");
    llvm_extend_name(&strs->output, vec_at(&llvm_struct_def_unwrap(llvm_def_unwrap(struct_def_))->base.members, load->memb_idx)->name_self);
    string_extend_cstr(&a_main, &strs->output, ");\n");
}

static void emit_c_array_access(Emit_c_strs* strs, const Llvm_array_access* access) {
    string_extend_cstr(&a_main, &strs->output, "    void* ");
    llvm_extend_name(&strs->output, access->name_self);
    string_extend_cstr(&a_main, &strs->output, " = ");

    string_extend_cstr(&a_main, &strs->output, "&(((");
    c_extend_type_call_str(&strs->output, lang_type_from_get_name(access->callee), false);
    string_extend_cstr(&a_main, &strs->output, ")");
    llvm_extend_name(&strs->output, access->callee);
    string_extend_cstr(&a_main, &strs->output, ")[");
    llvm_extend_name(&strs->output, access->index);
    string_extend_cstr(&a_main, &strs->output, "]);\n");
}

static void emit_c_goto_internal(Emit_c_strs* strs, Name name) {
    string_extend_cstr(&a_main, &strs->output, "    goto ");
    llvm_extend_name(&strs->output, name);
    string_extend_cstr(&a_main, &strs->output, ";\n");
}

static void emit_c_cond_goto(Emit_c_strs* strs, const Llvm_cond_goto* cond_goto) {
    string_extend_cstr(&a_main, &strs->output, "    if (");
    llvm_extend_name(&strs->output, cond_goto->condition);
    string_extend_cstr(&a_main, &strs->output, ") {\n");
    emit_c_goto_internal(strs, cond_goto->if_true);
    string_extend_cstr(&a_main, &strs->output, "    } else {\n");
    emit_c_goto_internal(strs, cond_goto->if_false);
    string_extend_cstr(&a_main, &strs->output, "    }\n");
}

static void emit_c_goto(Emit_c_strs* strs, const Llvm_goto* lang_goto) {
    emit_c_goto_internal(strs, lang_goto->name);
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
                emit_c_block(strs, llvm_block_const_unwrap(stmt));
                break;
            case LLVM_COND_GOTO:
                emit_c_cond_goto(strs, llvm_cond_goto_const_unwrap(stmt));
                break;
            case LLVM_GOTO:
                emit_c_goto(strs, llvm_goto_const_unwrap(stmt));
                break;
            case LLVM_ALLOCA:
                emit_c_alloca(&strs->output, llvm_alloca_const_unwrap(stmt));
                break;
            case LLVM_LOAD_ELEMENT_PTR:
                emit_c_load_element_ptr(strs, llvm_load_element_ptr_const_unwrap(stmt));
                break;
            case LLVM_ARRAY_ACCESS:
                emit_c_array_access(strs, llvm_array_access_const_unwrap(stmt));
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

void emit_c_from_tree(const Llvm_block* root) {
    String header = {0};
    Emit_c_strs strs = {0};

    string_extend_cstr(&a_main, &header, "static int dummy = 0;\n");
    string_extend_cstr(&a_main, &header, "#include <stddef.h>\n");
    string_extend_cstr(&a_main, &header, "#include <stdint.h>\n");
    string_extend_cstr(&a_main, &header, "#include <stdbool.h>\n");
    string_extend_cstr(&a_main, &header, "#include <string.h>\n");

    Alloca_iter iter = all_tbl_iter_new(0);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        emit_c_sometimes(&strs, curr);
    }

    emit_c_block(&strs, root);

    FILE* file = fopen(TEST_OUTPUT, "w");
    if (!file) {
        msg(
            DIAG_FILE_COULD_NOT_OPEN, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }
    
    // TODO: make function for for loop to compress code
    for (size_t idx = 0; idx < header.info.count; idx++) {
        if (EOF == fputc(vec_at(&header, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.struct_defs.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.struct_defs, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.forward_decls.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.forward_decls, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.literals.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.literals, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < strs.output.info.count; idx++) {
        if (EOF == fputc(vec_at(&strs.output, idx), file)) {
            todo();
        }
    }

    msg(DIAG_FILE_BUILT, dummy_pos, "file %s built\n", params.input_file_name);

    fclose(file);

    Str_view_vec cmd = {0};
    vec_append(&a_main, &cmd, str_view_from_cstr("clang"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-std=c99"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-Wno-override-module"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-Wno-incompatible-library-redeclaration"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-Wno-builtin-requires-header"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-O2"));
    vec_append(&a_main, &cmd, str_view_from_cstr("-o"));
    vec_append(&a_main, &cmd, str_view_from_cstr("test"));
    vec_append(&a_main, &cmd, str_view_from_cstr(TEST_OUTPUT));
    int status = subprocess_call(str_view_from_cstr("clang"), cmd);
    if (status != 0) {
        msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process returned exit code %d\n", status);
        String cmd_str = {0};
        for (size_t idx = 0; idx < cmd.info.count; idx++) {
            if (idx > 0) {
                string_extend_cstr(&a_main, &cmd_str, " ");
            }
            // TODO: consider arguments that contain spaces, etc.
            string_extend_strv(&a_main, &cmd_str, vec_at(&cmd, idx));
        }
        msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"STR_VIEW_FMT"`\n", string_print(cmd_str));
        exit(EXIT_CODE_FAIL);
    }
}
