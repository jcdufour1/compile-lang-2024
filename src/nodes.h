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
            nodes.buf = safe_malloc(sizeof(nodes.buf[0])*nodes.capacity);
        } else {
            nodes.capacity *= 2;
            nodes.buf = safe_realloc(nodes.buf, sizeof(nodes.buf[0])*nodes.capacity);
        }
    }
}

static inline Node* nodes_at(Node_idx idx) {
    assert(idx < nodes.count && "out of bounds");
    return &nodes.buf[idx];
}

static inline Node_idx node_new() {
    nodes_reserve(1);
    memset(&nodes.buf[nodes.count], 0, sizeof(nodes.buf[0]));
    Node_idx idx_node_created = nodes.count;
    nodes.count++;

    Node* new_node = nodes_at(idx_node_created);
    new_node->parameters = NODE_IDX_NULL;
    new_node->return_types = NODE_IDX_NULL;
    new_node->body = NODE_IDX_NULL;
    new_node->left = NODE_IDX_NULL;
    new_node->right = NODE_IDX_NULL;

    return idx_node_created;
}

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_idx root, const char* file, int line);

#define log_tree(log_level, root) \
    nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__);

#endif // NODES_H
