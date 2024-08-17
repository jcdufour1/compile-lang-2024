#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"
#include "assert.h"

#define NODES_DEFAULT_CAPACITY 512

typedef struct {
    Node* buf;
    size_t count;
    size_t capacity;
} Nodes;

extern Nodes nodes;

static inline void nodes_reserve(size_t minimum_count_empty_slots) {
    // nodes->capacity must be at least one greater than nodes->count for null termination
    while (nodes.count + minimum_count_empty_slots + 1 > nodes.capacity) {
        if (nodes.capacity < 1) {
            nodes.capacity = NODES_DEFAULT_CAPACITY;
            nodes.buf = safe_malloc(nodes.capacity, sizeof(nodes.buf[0]));
        } else {
            nodes.capacity *= 2;
            nodes.buf = safe_realloc(nodes.buf, nodes.capacity, sizeof(nodes.buf[0]));
        }
    }
}

static inline Node* nodes_at(Node_idx idx) {
    if (idx >= nodes.count) {
        if (idx == NODE_IDX_NULL) {
            log(LOG_FETAL, "index is NODE_IDX_NULL\n");
        } else {
            log(LOG_FETAL,  "index %zu out of bounds", idx);
        }
        abort();
    }
    return &nodes.buf[idx];
}

static inline Node_idx node_new() {
    nodes_reserve(1);
    memset(&nodes.buf[nodes.count], 0, sizeof(nodes.buf[0]));
    Node_idx idx_node_created = nodes.count;
    nodes.count++;

    Node* new_node = nodes_at(idx_node_created);
    new_node->next = NODE_IDX_NULL;
    new_node->prev = NODE_IDX_NULL;
    new_node->left_child = NODE_IDX_NULL;
    new_node->right_child = NODE_IDX_NULL;

    return idx_node_created;
}

static inline void nodes_set_right_child(Node_idx parent, Node_idx child) {
    assert(parent != NODE_IDX_NULL && child != NODE_IDX_NULL);
    nodes_at(parent)->right_child = child;
}

static inline void nodes_set_next(Node_idx curr, Node_idx next) {
    assert(curr != NODE_IDX_NULL && next != NODE_IDX_NULL);
    nodes_at(curr)->next = next;
    nodes_at(next)->prev = curr;
}

static inline Node_idx nodes_get_local_leftmost(Node_idx node) {
    Node_idx result = node;
    while (nodes_at(result)->prev != NODE_IDX_NULL) {
        result = nodes_at(result)->prev;
    }
    return result;
}

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_idx root, const char* file, int line);

#define log_tree(log_level, root) \
    nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__);


#endif // NODES_H
