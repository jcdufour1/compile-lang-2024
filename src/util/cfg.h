#ifndef CFG_H
#define CFG_H

#include <util.h>
#include <vector.h>
#include <size_t_vec.h>

#define CFG_NODE_START_OF_BLOCK SIZE_MAX

typedef struct {
    Size_t_vec preds;
    Size_t_vec succs;
    Name label_name; // should be {0} if pos_in_block is at start of block
    size_t pos_in_block;
} Cfg_node;

typedef struct {
    Vec_base info;
    Cfg_node* buf;
} Cfg_node_vec;

static inline Strv cfg_node_print_internal(Cfg_node node, int indent)  {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "cfg_node\n", indent);
    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&a_print, &buf, "preds: [", indent);
    vec_foreach(idx, size_t, curr, node.preds) {
        if (idx > 0) {
            string_extend_cstr(&a_print, &buf, ", ");
        }
        string_extend_size_t(&a_print, &buf, curr);
    }}
    string_extend_cstr(&a_print, &buf, "]\n");

    string_extend_cstr_indent(&a_print, &buf, "succs: [", indent);
    vec_foreach(idx, size_t, curr, node.succs) {
        if (idx > 0) {
            string_extend_cstr(&a_print, &buf, ", ");
        }
        string_extend_size_t(&a_print, &buf, curr);
    }}
    string_extend_cstr(&a_print, &buf, "]\n");

    string_extend_cstr_indent(&a_print, &buf, "name: ", indent);
    extend_name(NAME_LOG, &buf, node.label_name);
    string_extend_cstr(&a_print, &buf, "\n");

    string_extend_cstr_indent(&a_print, &buf, "pos_in_block: ", indent);
    string_extend_size_t(&a_print, &buf, node.pos_in_block);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

#define cfg_node_print(node) \
    strv_print(cfg_node_print_internal(node, 0))

#endif // CFG_H
