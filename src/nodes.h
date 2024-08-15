#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"

#define NODES_DEFAULT_CAPACITY 512

typedef struct {
    Node* buf;
    size_t count;
    size_t capacity;
} Nodes;

extern Nodes nodes;

static inline void Nodes_reserve(size_t minimum_count_empty_slots) {
    // nodes->capacity must be at least one greater than nodes->count for null termination
    while (nodes.count + minimum_count_empty_slots + 1 > nodes.capacity) {
        if (nodes.capacity < 1) {
            nodes.capacity = NODES_DEFAULT_CAPACITY;
            nodes.buf = safe_malloc(nodes.capacity);
        } else {
            nodes.capacity *= 2;
            nodes.buf = safe_realloc(nodes.buf, nodes.capacity);
        }
    }
}

static inline void Nodes_append(const Node* node) {
    Nodes_reserve(1);
    nodes.buf[nodes.count++] = *node;
}

static inline Node* Node_new() {
    Nodes_reserve(1);
    memset(&nodes.buf[nodes.count], 0, sizeof(nodes.buf[0]));
    Node* node_created = &nodes.buf[nodes.count];
    nodes.count++;
    return node_created;
}

void Nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, const Node* root, const char* file, int line);

#define log_tree(log_level, root) \
    Nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__);

#endif // NODES_H
