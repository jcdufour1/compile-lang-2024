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

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_id root, const char* file, int line);

#define log_tree(log_level, root) \
    do { \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"); \
        nodes_log_tree_rec(log_level, 0, root, __FILE__, __LINE__); \
        log_file_new(log_level, __FILE__, __LINE__, "\n"); \
    } while(0);

typedef struct {
    Vec_base info;
    Node* buf;
} Nodes;

extern Nodes nodes;

#define nodes_foreach_child(child, parent) \
    for (Node_id child = nodes_at(parent)->left_child; (child) != NODE_IDX_NULL; (child) = nodes_at(child)->next)

#define nodes_foreach_from_curr(curr_node, start_node) \
    for (Node_id curr_node = (start_node); (curr_node) != NODE_IDX_NULL; (curr_node) = nodes_at(curr_node)->next)

#define nodes_foreach_from_curr_rev(curr_node, start_node) \
    for (Node_id curr_node = (start_node); (curr_node) != NODE_IDX_NULL; (curr_node) = nodes_at(curr_node)->prev)

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

static inline size_t nodes_count_children(Node_id parent) {
    size_t count = 0;
    nodes_foreach_child(child, parent) {
        count++;
    }
    return count;
}

static inline Node_id nodes_get_leftmost_sibling(Node_id node) {
    Node_id result = node;
    while (nodes_at(result)->prev != NODE_IDX_NULL) {
        result = nodes_at(result)->prev;
    }
    return result;
}

static inline void nodes_set_left_child(Node_id parent, Node_id child) {
    assert(parent != NODE_IDX_NULL && child != NODE_IDX_NULL);
    nodes_at(parent)->left_child = child;
    nodes_at(child)->parent = parent;
}

static inline void nodes_establish_siblings(Node_id curr, Node_id next) {
    nodes_at(curr)->next = next;
    nodes_at(next)->prev = curr;
}

static inline void nodes_insert_after(Node_id curr, Node_id node_to_insert) {
    assert(curr != NODE_IDX_NULL && node_to_insert != NODE_IDX_NULL);
    Node_id old_next = nodes_at(curr)->next;
    nodes_establish_siblings(curr, node_to_insert);
    if (old_next != NODE_IDX_NULL) {
        nodes_establish_siblings(node_to_insert, old_next);
    }
    nodes_at(node_to_insert)->parent = nodes_at(curr)->parent;
}

static inline void nodes_insert_before(Node_id curr, Node_id node_to_insert) {
    assert(curr != NODE_IDX_NULL && node_to_insert != NODE_IDX_NULL);

    Node_id old_prev = nodes_at(curr)->prev;
    nodes_at(old_prev)->next = node_to_insert;
    nodes_at(node_to_insert)->prev = old_prev;

    nodes_at(curr)->prev = node_to_insert;
    nodes_at(node_to_insert)->next = curr;

    nodes_at(node_to_insert)->parent = nodes_at(curr)->parent;
    Node_id parent = nodes_at(curr)->parent;
    nodes_at(parent)->left_child = nodes_get_leftmost_sibling(curr);
}

static inline void nodes_append_child(Node_id parent, Node_id child) {
    if (nodes_at(parent)->left_child == NODE_IDX_NULL) {
        nodes_at(parent)->left_child = child;
        nodes_at(child)->parent = parent;
        return;
    }

    Node_id curr_node = nodes_at(parent)->left_child;
    while (nodes_at(curr_node)->next != NODE_IDX_NULL) {
        curr_node = nodes_at(curr_node)->next;
    }
    nodes_insert_after(curr_node, child);
}

static inline void nodes_reset_links(Node_id node) {
    nodes_at(node)->left_child = NODE_IDX_NULL;
    nodes_at(node)->next = NODE_IDX_NULL;
    nodes_at(node)->prev = NODE_IDX_NULL;
    nodes_at(node)->parent = NODE_IDX_NULL;
}

static inline Node_id nodes_single_child(Node_id node) {
    assert(nodes_count_children(node) == 1);
    return nodes_at(node)->left_child;
}

static inline void nodes_replace(Node_id node_to_replace, Node_id src) {
    Node new_node = *nodes_at(src);
    new_node.parent = nodes_at(node_to_replace)->parent;
    //new_node.left_child = nodes_at(node_to_replace)->left_child;
    //new_node.right_child = nodes_at(node_to_replace)->left_child;

    *nodes_at(node_to_replace) = new_node;
}

static inline void nodes_extend_children(Node_id parent, Node_id start_of_nodes_to_extend) {
    nodes_foreach_from_curr(curr_node, start_of_nodes_to_extend) {
        nodes_append_child(parent, curr_node);
    }
}

static inline void nodes_remove_siblings(Node_id node) {
    assert(node != NODE_IDX_NULL);

    Node_id prev = nodes_at(node)->prev;
    if (prev != NODE_IDX_NULL) {
        nodes_at(prev)->next = NODE_IDX_NULL;
    }

    Node_id next = nodes_at(node)->next;
    if (next != NODE_IDX_NULL) {
        nodes_at(next)->next = NODE_IDX_NULL;
    }

    nodes_at(node)->next = NODE_IDX_NULL;
    nodes_at(node)->prev = NODE_IDX_NULL;
}

static inline Node_id nodes_get_child_of_type(Node_id parent, NODE_TYPE node_type) {
    if (nodes_at(parent)->left_child == NODE_IDX_NULL) {
        todo();
    }

    nodes_foreach_child(child, parent) {
        if (nodes_at(child)->type == node_type) {
            return child;
        }
    }

    log_tree(LOG_VERBOSE, parent);
    unreachable();
}

static inline Node_id nodes_get_sibling_of_type(Node_id node, NODE_TYPE node_type) {
    Node_id curr_node = nodes_get_leftmost_sibling(node);

    while (curr_node != NODE_IDX_NULL) {
        if (nodes_at(curr_node)->type == node_type) {
            return curr_node;
        }
        curr_node = nodes_at(curr_node)->next;
    }

    log_tree(LOG_VERBOSE, node);
    unreachable();
}

static inline Node_id nodes_get_child(Node_id parent, size_t idx) {
    size_t curr_idx = 0;
    nodes_foreach_child(child, parent) {
        if (curr_idx == idx) {
            return child;
        }

        curr_idx++;
    }

    unreachable();
}

static inline bool nodes_try_get_last_child_of_type(Node_id* result, Node_id parent, NODE_TYPE node_type) {
    Node_id curr_child = NODE_IDX_NULL;
    nodes_foreach_child(child, parent) {
        curr_child = child;
    }
    if (curr_child == NODE_IDX_NULL) {
        return false;
    }

    nodes_foreach_from_curr_rev(curr, parent) {
        if (nodes_at(curr)->type == node_type) {
            *result = curr;
            return true;
        }
    }

    return false;
}

static inline Node_id nodes_get_last_child_of_type(Node_id parent, NODE_TYPE node_type) {
    Node_id result;
    try(nodes_try_get_last_child_of_type(&result, parent, node_type));
    return result;
}

#endif // NODES_H
