#include <cfg.h>

Strv cfg_node_print_internal(Cfg_node node, size_t idx, int indent)  {
    String buf = {0};

    string_extend_cstr_indent(&a_temp, &buf, "cfg_node:", indent);
    string_extend_size_t(&a_temp, &buf, idx);
    string_extend_cstr(&a_temp, &buf, "\n");
    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&a_temp, &buf, "preds: [", indent);
    vec_foreach(idx, size_t, pred, node.preds) {
        if (idx >= node.pred_backedge_start) {
            break;
        }
        if (idx > 0) {
            string_extend_cstr(&a_temp, &buf, ", ");
        }
        string_extend_size_t(&a_temp, &buf, pred);
    }
    string_extend_cstr(&a_temp, &buf, "]\n");

    string_extend_cstr_indent(&a_temp, &buf, "pred_backedges: [", indent);
    vec_foreach(idx, size_t, pred_back, node.preds) {
        if (idx < node.pred_backedge_start) {
            continue;
        }
        if (idx - node.pred_backedge_start > 0) {
            string_extend_cstr(&a_temp, &buf, ", ");
        }
        string_extend_size_t(&a_temp, &buf, pred_back);
    }
    string_extend_cstr(&a_temp, &buf, "]\n");

    string_extend_cstr_indent(&a_temp, &buf, "succs: [", indent);
    vec_foreach(idx, size_t, succ, node.succs) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, &buf, ", ");
        }
        string_extend_size_t(&a_temp, &buf, succ);
    }
    string_extend_cstr(&a_temp, &buf, "]\n");

    string_extend_cstr_indent(&a_temp, &buf, "name: ", indent);
    extend_ir_name(NAME_LOG, &buf, node.label_name);
    string_extend_cstr(&a_temp, &buf, "\n");

    string_extend_cstr_indent(&a_temp, &buf, "pos_in_block: ", indent);
    string_extend_size_t(&a_temp, &buf, node.pos_in_block);
    string_extend_cstr(&a_temp, &buf, "\n");

    return string_to_strv(buf);
}

