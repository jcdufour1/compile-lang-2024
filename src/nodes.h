#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"
#include "assert.h"
#include "vector.h"

#define NODES_DEFAULT_CAPACITY 512

typedef struct {
    Vec_base info;
    Node* buf;
} Nodes;

extern Nodes nodes;

static inline void nodes_reserve(size_t minimum_count_empty_slots) {
    vector_reserve(&nodes, sizeof(nodes.buf[0]), minimum_count_empty_slots, NODES_DEFAULT_CAPACITY);
}

static inline Node* nodes_at(Node_id idx) {
    if (idx >= nodes.info.count) {
        if (idx == NODE_IDX_NULL) {
            log(LOG_FETAL, "index is NODE_IDX_NULL\n");
        } else {
            log(LOG_FETAL,  "index %zu out of bounds", idx);
        }
        abort();
    }
    return &nodes.buf[idx];
}

static inline Node_id node_new() {
    nodes_reserve(1);
    memset(&nodes.buf[nodes.info.count], 0, sizeof(nodes.buf[0]));
    Node_id idx_node_created = nodes.info.count;
    nodes.info.count++;

    Node* new_node = nodes_at(idx_node_created);
    new_node->next = NODE_IDX_NULL;
    new_node->prev = NODE_IDX_NULL;
    new_node->left_child = NODE_IDX_NULL;

    return idx_node_created;
}

static inline void nodes_set_left_child(Node_id parent, Node_id child) {
    assert(parent != NODE_IDX_NULL && child != NODE_IDX_NULL);
    nodes_at(parent)->left_child = child;
}

static inline void nodes_set_next(Node_id curr, Node_id next) {
    assert(curr != NODE_IDX_NULL && next != NODE_IDX_NULL);
    nodes_at(curr)->next = next;
    nodes_at(next)->prev = curr;
}

static inline void nodes_append_child(Node_id parent, Node_id child) {
    if (nodes_at(parent)->left_child == NODE_IDX_NULL) {
        nodes_at(parent)->left_child = child;
        return;
    }

    Node_id curr_node = nodes_at(parent)->left_child;
    while (nodes_at(curr_node)->next != NODE_IDX_NULL) {
        curr_node = nodes_at(curr_node)->next;
    }
    nodes_set_next(curr_node, child);
}

static inline Node_id nodes_get_local_leftmost(Node_id node) {
    Node_id result = node;
    while (nodes_at(result)->prev != NODE_IDX_NULL) {
        result = nodes_at(result)->prev;
    }
    return result;
}

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_id root, const char* file, int line);

#define log_tree(log_level, root) \
    nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__);


#endif // NODES_H
