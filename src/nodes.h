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

void nodes_assert_tree_linkage_is_consistant(Node_id root);

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

static inline void nodes_reserve(Nodes* curr_nodes, size_t minimum_count_empty_slots) {
    vector_reserve(curr_nodes, sizeof(nodes.buf[0]), minimum_count_empty_slots, NODES_DEFAULT_CAPACITY);
}

static inline size_t node_unwrap(Node_id node) {
    return node.id;
}

static inline bool node_is_null(Node_id node) {
    return node_unwrap(node) == NODE_IDX_NULL;
}

static inline Node* nodes_at(Node_id idx) {
    if (node_unwrap(idx) >= nodes.info.count) {
        if (node_is_null(idx)) {
            log(LOG_FETAL, "index is NODE_IDX_NULL\n");
        } else {
            log(LOG_FETAL,  "index %zu out of bounds", node_unwrap(idx));
        }
        abort();
    }
    return &nodes.buf[node_unwrap(idx)];
}

static inline Node_id nodes_next(Node_id idx) {
    return nodes_at(idx)->next;
}

static inline Node_id nodes_prev(Node_id idx) {
    return nodes_at(idx)->prev;
}

static inline Node_id nodes_parent(Node_id idx) {
    return nodes_at(idx)->parent;
}

static inline Node_id nodes_left_child(Node_id idx) {
    return nodes_at(idx)->left_child;
}

static inline void nodes_set_next(Node_id base, Node_id next) {
    nodes_at(base)->next = next;
}

static inline void nodes_set_prev(Node_id base, Node_id prev) {
    nodes_at(base)->prev = prev;
}

static inline void nodes_set_left_child_tmp(Node_id base, Node_id left_child) {
    nodes_at(base)->left_child = left_child;
}

static inline void nodes_set_parent(Node_id base, Node_id parent) {
    nodes_at(base)->parent = parent;
}

static inline Node_id node_id_from(size_t idx) {
    Node_id node = {.id = idx};
    return node;
}

#define nodes_foreach_child(child, parent) \
    for (Node_id child = nodes_left_child(parent); !node_is_null(child); (child) = nodes_next(child))

#define nodes_foreach_from_curr(curr_node, start_node) \
    for (Node_id curr_node = (start_node); !node_is_null(curr_node); (curr_node) = nodes_next(curr_node))

#define nodes_foreach_from_curr_rev(curr_node, start_node) \
    for (Node_id curr_node = (start_node); !node_is_null(curr_node); (curr_node) = nodes_prev(curr_node))

// do not use this function to remove node from tree (use node_remove instead for that)
static inline void nodes_reset_links_of_self_only(Node_id node, bool keep_children) {
    if (!keep_children) {
        nodes_set_left_child_tmp(node, node_id_from(NODE_IDX_NULL));
    }
    nodes_set_next(node, node_id_from(NODE_IDX_NULL));
    nodes_set_prev(node, node_id_from(NODE_IDX_NULL));
    nodes_set_parent(node, node_id_from(NODE_IDX_NULL));
}

static inline Node_id node_new() {
    nodes_reserve(&nodes, 1);
    memset(&nodes.buf[nodes.info.count], 0, sizeof(nodes.buf[0]));
    Node_id idx_node_created = node_id_from(nodes.info.count);
    nodes.info.count++;

    nodes_reset_links_of_self_only(idx_node_created, false);

    return idx_node_created;
}

static inline void nodes_establish_siblings(Node_id curr, Node_id next) {
    nodes_at(curr)->next = next;
    nodes_at(next)->prev = curr;
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
    while (!node_is_null(nodes_prev(result))) {
        result = nodes_prev(result);
    }
    return result;
}

static inline void nodes_establish_parent_left_child(Node_id parent, Node_id child) {
    assert(!node_is_null(parent) && !node_is_null(child));
    nodes_set_left_child_tmp(parent, child);
    nodes_set_parent(child, parent);
}

static inline void nodes_insert_after(Node_id curr, Node_id node_to_insert) {
    assert(!node_is_null(curr) && !node_is_null(node_to_insert));
    Node_id old_next = nodes_next(curr);
    nodes_establish_siblings(curr, node_to_insert);
    if (!node_is_null(old_next)) {
        nodes_establish_siblings(node_to_insert, old_next);
    }
    nodes_at(node_to_insert)->parent = nodes_at(curr)->parent;
}

static inline bool node_ids_equal(Node_id a, Node_id b) {
    return node_unwrap(a) == node_unwrap(b);
}

static inline void nodes_insert_before(Node_id node_to_insert_before, Node_id node_to_insert) {
    assert(!node_is_null(node_to_insert_before) && !node_is_null(node_to_insert));

    Node_id new_prev = nodes_at(node_to_insert_before)->prev;
    Node_id new_next = node_to_insert_before;
    if (!node_is_null(new_prev)) {
        nodes_establish_siblings(new_prev, node_to_insert);
    }

    assert(!node_is_null(new_next));
    nodes_establish_siblings(node_to_insert, new_next);

    Node_id parent = nodes_at(node_to_insert_before)->parent;
    if (node_ids_equal(nodes_left_child(parent), node_to_insert_before)) {
        nodes_at(parent)->left_child = node_to_insert;
    }
    nodes_at(node_to_insert)->parent = parent;

    log_tree(LOG_DEBUG, node_id_from(0));
    log_tree(LOG_DEBUG, parent);
    log_tree(LOG_DEBUG, nodes_at(parent)->left_child);
    log_tree(LOG_DEBUG, node_to_insert);
}

static inline void nodes_append_child(Node_id parent, Node_id child) {
    if (node_is_null(nodes_left_child(parent))) {
        nodes_at(parent)->left_child = child;
        nodes_at(child)->parent = parent;
        return;
    }

    Node_id curr_node = nodes_at(parent)->left_child;
    while (!node_is_null(nodes_next(curr_node))) {
        curr_node = nodes_at(curr_node)->next;
    }
    nodes_insert_after(curr_node, child);
}

static inline Node_id nodes_single_child(Node_id node) {
    assert(nodes_count_children(node) == 1);
    return nodes_at(node)->left_child;
}

// child of src will be kept
// there should not be next, prev, or parent attached to src
// note: src will be modified and should probably not be used again
static inline void nodes_replace(Node_id node_to_replace, Node_id src) {
    assert(node_is_null(nodes_next(src)));
    assert(node_is_null(nodes_prev(src)));
    assert(node_is_null(nodes_parent(src)));
    assert(!node_is_null(node_to_replace));

    if (node_is_null(nodes_prev(node_to_replace))) {
        nodes_establish_parent_left_child(nodes_at(node_to_replace)->parent, src);
    } else {
        nodes_establish_siblings(nodes_at(node_to_replace)->prev, src);
    }

    nodes_assert_tree_linkage_is_consistant(node_id_from(0));

    if (!node_is_null(nodes_next(node_to_replace))) {
        nodes_establish_siblings(src, nodes_at(node_to_replace)->next);
    }

    nodes_assert_tree_linkage_is_consistant(node_id_from(0));
}

static inline void nodes_extend_children(Node_id parent, Node_id parent_of_nodes_to_extend) {
    nodes_foreach_from_curr(curr_node, parent_of_nodes_to_extend) {
        nodes_append_child(parent, curr_node);
    }
}

static inline void nodes_remove_siblings(Node_id node) {
    assert(!node_is_null(node));

    Node_id prev = nodes_at(node)->prev;
    if (!node_is_null(prev)) {
        nodes_set_next(prev, node_id_from(NODE_IDX_NULL));
    }

    Node_id next = nodes_at(node)->next;
    if (!node_is_null(next)) {
        nodes_set_next(next, node_id_from(NODE_IDX_NULL));
    }

    nodes_set_next(node, node_id_from(NODE_IDX_NULL));
    nodes_set_prev(node, node_id_from(NODE_IDX_NULL));
}

static inline Node_id nodes_get_child_of_type(Node_id parent, NODE_TYPE node_type) {
    if (node_is_null(nodes_left_child(parent))) {
        todo();
    }

    nodes_foreach_child(child, parent) {
        if (nodes_at(child)->type == node_type) {
            return child;
        }
    }

    log_tree(LOG_VERBOSE, parent);
    unreachable("");
}

static inline Node_id nodes_get_sibling_of_type(Node_id node, NODE_TYPE node_type) {
    Node_id curr_node = nodes_get_leftmost_sibling(node);

    while (node_is_null(curr_node)) {
        if (nodes_at(curr_node)->type == node_type) {
            return curr_node;
        }
        curr_node = nodes_at(curr_node)->next;
    }

    log_tree(LOG_VERBOSE, node);
    unreachable("");
}

static inline Node_id nodes_get_child(Node_id parent, size_t idx) {
    size_t curr_idx = 0;
    nodes_foreach_child(child, parent) {
        if (curr_idx == idx) {
            return child;
        }

        curr_idx++;
    }

    unreachable("");
}

static inline bool nodes_try_get_last_child_of_type(Node_id* result, Node_id parent, NODE_TYPE node_type) {
    Node_id curr_child = node_id_from(NODE_IDX_NULL);
    nodes_foreach_child(child, parent) {
        curr_child = child;
    }
    if (node_is_null(curr_child)) {
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

// remove node_to_remove (and its children if present and keep_children is true) from tree. 
// children of node_to_remove will stay attached to node_to_remove if keep_children is true
static inline void nodes_remove(Node_id node_to_remove, bool keep_children) {
    assert(!node_is_null(node_to_remove));

    Node_id next = nodes_at(node_to_remove)->next;
    Node_id prev = nodes_at(node_to_remove)->prev;
    Node_id parent = nodes_at(node_to_remove)->parent;
    Node_id left_child = nodes_at(node_to_remove)->left_child;

    if (!node_is_null(next)) {
        nodes_at(next)->prev = prev;
    }

    if (!node_is_null(prev)) {
        nodes_at(prev)->next = next;
    }

    if (node_unwrap(nodes_left_child(parent)) == node_unwrap(node_to_remove)) {
        nodes_at(parent)->left_child = next;
    }

    if (!keep_children && !node_is_null(left_child)) {
        unreachable("");
    }

    nodes_reset_links_of_self_only(node_to_remove, keep_children);
}

#endif // NODES_H
