#ifndef NODES_H
#define NODES_H

#include <stddef.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "node.h"
#include "assert.h"
#include "vector.h"

void nodes_log_tree_internal(LOG_LEVEL log_level, const Node* root, const char* file, int line);

#ifdef NDEBUG
#define nodes_assert_tree_linkage_is_consistant(root)
#else
void nodes_assert_tree_linkage_is_consistant(const Node* root);
#endif // NDEBUG

#define log_tree(log_level, root) \
    do { \
        nodes_assert_tree_linkage_is_consistant(root); \
        log_file_new(log_level, __FILE__, __LINE__, "tree:\n"); \
        nodes_log_tree_internal(log_level, root, __FILE__, __LINE__); \
        log_file_new(log_level, __FILE__, __LINE__, "\n"); \
    } while(0)

// return prev if prev is non-null, or parent otherwise
static inline Node* nodes_prev_or_parent(Node* node) {
    if (!node->prev) {
        return node->parent;
    }
    return node->prev;
}

static inline void set_left_child(Node* parent, Node* left_child) {
    (void) parent;
    (void) left_child;
    switch (parent->type) {
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
            return;
        case NODE_SYMBOL_TYPED:
            unreachable("");
            return;
        case NODE_LABEL:
            unreachable("");
            return;
        case NODE_LITERAL:
            node_unwrap_literal(parent)->child = left_child;
            return;
        case NODE_FUNCTION_CALL:
            node_unwrap_function_call(parent)->child = left_child;
            return;
        case NODE_LANG_TYPE:
            unreachable("");
            return;
        case NODE_OPERATOR:
            node_unwrap_operator(parent)->child = left_child;
            return;
        case NODE_STRUCT_LITERAL:
            node_unwrap_struct_literal(parent)->child = left_child;
            return;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            unreachable("");
            return;
        case NODE_VARIABLE_DEFINITION:
            unreachable("");
            return;
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
            return;
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
            return;
        case NODE_STRUCT_DEFINITION:
            node_unwrap_struct_def(parent)->child = left_child;
            return;
        case NODE_FUNCTION_PARAMETERS:
            node_unwrap_function_params(parent)->child = left_child;
            return;
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("should not assign this in this way");
            return;
        case NODE_FUNCTION_DECLARATION:
            node_unwrap_function_declaration(parent)->child = left_child;
            return;
        case NODE_FUNCTION_DEFINITION:
            node_unwrap_function_definition(parent)->child = left_child;
            return;
        case NODE_FOR_LOOP:
            node_unwrap_for_loop(parent)->child = left_child;
            return;
        case NODE_FOR_VARIABLE_DEF:
            node_unwrap_for_variable_def(parent)->child = left_child;
            return;
        case NODE_FOR_LOWER_BOUND:
            node_unwrap_for_lower_bound(parent)->child = left_child;
            return;
        case NODE_FOR_UPPER_BOUND:
            node_unwrap_for_upper_bound(parent)->child = left_child;
            return;
        case NODE_BREAK:
            node_unwrap_break(parent)->child = left_child;
            return;
        case NODE_RETURN_STATEMENT:
            node_unwrap_return_statement(parent)->child = left_child;
            return;
        case NODE_ASSIGNMENT:
            unreachable("");
            return;
        case NODE_IF_STATEMENT:
            node_unwrap_if(parent)->child = left_child;
            return;
        case NODE_IF_CONDITION:
            node_unwrap_if_condition(parent)->child = left_child;
            return;
        case NODE_BLOCK:
            node_unwrap_block(parent)->child = left_child;
            return;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            node_unwrap_struct_member_sym_untyped(parent)->child = left_child;
            return;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            unreachable("");
            return;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(parent)->child = left_child;
            return;
        case NODE_GOTO:
            node_unwrap_goto(parent)->child = left_child;
            return;
        case NODE_ALLOCA:
            unreachable("");
            return;
        case NODE_LLVM_STORE_LITERAL:
            node_unwrap_llvm_store_literal(parent)->child = node_unwrap_literal(left_child);
            return;
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
            return;
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
            return;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            node_unwrap_llvm_store_struct_literal(parent)->child = left_child;
            return;
        default:
            unreachable(NODE_FMT, node_print(parent));
    }
}

static inline Node* get_left_child(Node* node) {
    switch (node->type) {
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_LABEL:
            unreachable("");
        case NODE_LITERAL:
            return node_unwrap_literal(node)->child;
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call(node)->child;
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_OPERATOR:
            return node_unwrap_operator(node)->child;
        case NODE_STRUCT_LITERAL:
            return node_unwrap_struct_literal(node)->child;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            return node_unwrap_struct_def(node)->child;
        case NODE_FUNCTION_PARAMETERS:
            return node_unwrap_function_params(node)->child;
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("should not do it this way");
        case NODE_FUNCTION_DECLARATION:
            return node_unwrap_function_declaration(node)->child;
        case NODE_FUNCTION_DEFINITION:
            return node_unwrap_function_definition(node)->child;
        case NODE_FOR_LOOP:
            return node_unwrap_for_loop(node)->child;
        case NODE_FOR_VARIABLE_DEF:
            return node_unwrap_for_variable_def(node)->child;
        case NODE_FOR_LOWER_BOUND:
            return node_unwrap_for_lower_bound(node)->child;
        case NODE_FOR_UPPER_BOUND:
            return node_unwrap_for_upper_bound(node)->child;
        case NODE_BREAK:
            return node_unwrap_break(node)->child;
        case NODE_RETURN_STATEMENT:
            return node_unwrap_return_statement(node)->child;
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_IF_STATEMENT:
            return node_unwrap_if(node)->child;
        case NODE_IF_CONDITION:
            return node_unwrap_if_condition(node)->child;
        case NODE_BLOCK:
            return node_unwrap_block(node)->child;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            return node_unwrap_struct_member_sym_untyped(node)->child;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            return node_unwrap_struct_member_sym_typed(node)->child;
        case NODE_COND_GOTO:
            return node_unwrap_cond_goto(node)->child;
        case NODE_GOTO:
            return node_unwrap_goto(node)->child;
        case NODE_ALLOCA:
            unreachable("");
        case NODE_LLVM_STORE_LITERAL:
            return node_wrap(node_unwrap_llvm_store_literal(node)->child);
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            return node_unwrap_llvm_store_struct_literal(node)->child;
        case NODE_PTR_BYVAL_SYM:
            return NULL;
        case NODE_LLVM_REGISTER_SYM:
            return NULL;
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}

static inline const Node* get_left_child_const(const Node* node) {
    return get_left_child((Node*)node);
}

#define nodes_foreach_child(child, parent) \
    for (Node* child = get_left_child((Node*)parent); (child); (child) = (child)->next)

#define nodes_foreach_from_curr(curr_node, start_node) \
    for (Node* curr_node = ((Node*)start_node); (curr_node); (curr_node) = (curr_node)->next)

#define nodes_foreach_from_curr_const(curr_node, start_node) \
    for (const Node* curr_node = ((const Node*)start_node); (curr_node); (curr_node) = (curr_node)->next)

#define nodes_foreach_from_curr_rev(curr_node, start_node) \
    for (Node* curr_node = ((Node*)start_node); (curr_node); (curr_node) = (curr_node)->prev)

#define nodes_foreach_from_curr_rev_const(curr_node, start_node) \
    for (const Node* curr_node = ((const Node*)start_node); (curr_node); (curr_node) = (curr_node)->prev)

static inline Node* nodes_get_local_leftmost(Node* start_node) {
    assert(start_node);

    Node* first;
    nodes_foreach_from_curr_rev(curr_node, start_node) {
        first = curr_node;
    }
    return first;
}

static inline Node* nodes_get_local_rightmost(Node* start_node) {
    assert(start_node);

    Node* rightmost = NULL;
    nodes_foreach_from_curr(curr_node, start_node) {
        rightmost = curr_node;
    }
    return rightmost;
}

#define nodes_foreach_child_rev(child, parent) \
    nodes_foreach_from_curr_rev(child, nodes_get_local_rightmost((parent)->left_child))

// do not use this function to remove node from tree (use node_remove instead for that)
static inline void nodes_reset_links_of_self_only(Node* node, bool keep_children) {
    if (!keep_children) {
        set_left_child(node, NULL);
    }
    node->next = NULL;
    node->prev = NULL;
    node->parent = NULL;
}

static inline Node* node_new(Pos pos, NODE_TYPE node_type) {
    Node* new_node = arena_alloc(&a_main, sizeof(*new_node));
    if (!root_of_tree) {
        // it is assumed that the first node created is the root
        root_of_tree = new_node;
    }
    new_node->pos = pos;
    new_node->type = node_type;
    return new_node;
}

static inline void nodes_establish_siblings(Node* curr, Node* next) {
    curr->next = next;
    next->prev = curr;
}

static inline size_t nodes_count_children(const Node* parent) {
    size_t count = 0;
    nodes_foreach_child(child, parent) {
        count++;
    }
    return count;
}

static inline Node* nodes_get_leftmost_sibling(Node* node) {
    Node* result = node;
    while (result->prev) {
        result = result->prev;
    }
    return result;
}

static inline void nodes_establish_parent_left_child(Node* parent, Node* child) {
    assert(parent && child);

    nodes_foreach_from_curr(curr_node, nodes_get_local_leftmost(child)) {
        child->parent = parent;
    }
    set_left_child(parent, child);
}

static inline void nodes_insert_after(Node* curr, Node* node_to_insert) {
    assert(curr && node_to_insert);
    assert(!node_to_insert->next);
    assert(!node_to_insert->prev);
    assert(!node_to_insert->parent);

    Node* old_next = curr->next;
    nodes_establish_siblings(curr, node_to_insert);
    if (old_next) {
        nodes_establish_siblings(node_to_insert, old_next);
    }
    node_to_insert->parent = curr->parent;
}

static inline void nodes_insert_before(Node* node_to_insert_before, Node* node_to_insert) {
    assert(node_to_insert_before && node_to_insert);
    // this function must not be used for appending nodes that have next, parent, or prev node(s) attached
    assert(!node_to_insert->next);
    assert(!node_to_insert->prev);
    assert(!node_to_insert->parent);

    Node* new_prev = node_to_insert_before->prev;
    Node* new_next = node_to_insert_before;
    if (new_prev) {
        nodes_establish_siblings(new_prev, node_to_insert);
    }

    assert(new_next);
    nodes_establish_siblings(node_to_insert, new_next);
    assert(node_to_insert == node_to_insert_before->prev);
    assert(node_to_insert->next == node_to_insert_before);

    Node* parent = node_to_insert_before->parent;
    if (get_left_child(parent) == node_to_insert_before) {
        set_left_child(parent, node_to_insert);
    }
    node_to_insert->parent = parent;
}

static inline void nodes_append_child(Node* parent, Node* child) {
    // this function must not be used for appending nodes that have next, parent, or prev node(s) attached
    assert(!child->next);
    assert(!child->prev);
    assert(!child->parent);

    if (!get_left_child(parent)) {
        set_left_child(parent, child);
        child->parent = parent;
        return;
    }

    Node* curr_node = get_left_child(parent);
    while (curr_node->next) {
        curr_node = curr_node->next;
    }
    nodes_insert_after(curr_node, child);
}

// when node only has one child
static inline Node* nodes_single_child(Node* node) {
    assert(nodes_count_children(node) == 1);
    return get_left_child(node);
}

static inline const Node* nodes_single_child_const(const Node* node) {
    assert(nodes_count_children(node) == 1);
    return get_left_child_const(node);
}

static inline void nodes_remove_siblings(Node* node) {
    assert(node);

    Node* prev = node->prev;
    if (prev) {
        prev->next = NULL;
    }

    Node* next = node->next;
    if (next) {
        next->next = NULL;
    }

    node->next = NULL;
    node->prev = NULL;
}

static inline void nodes_remove_siblings_and_parent(Node* node) {
    assert(node);

    Node* parent_left_child = node->parent ? (get_left_child(node->parent)) : NULL;
    if (parent_left_child == node) {
        parent_left_child = node->next;
    }

    nodes_remove_siblings(node);

    if (node->parent) {
        set_left_child(node->parent, parent_left_child);
        node->parent = NULL;
    }
}

static inline Node* nodes_get_child_of_type(Node* parent, NODE_TYPE node_type) {
    if (!get_left_child(parent)) {
        todo();
    }

    nodes_foreach_child(child, parent) {
        if (child->type == node_type) {
            return child;
        }
    }

    log_tree(LOG_VERBOSE, parent);
    unreachable("node_type not found");
}

static inline const Node* nodes_get_child_of_type_const(const Node* parent, NODE_TYPE node_type) {
    return nodes_get_child_of_type((Node*)parent, node_type);
}

static inline Node* nodes_get_sibling_of_type(Node* node, NODE_TYPE node_type) {
    Node* curr_node = nodes_get_leftmost_sibling(node);

    while (!curr_node) {
        if (curr_node->type == node_type) {
            return curr_node;
        }
        curr_node = curr_node->next;
    }

    log_tree(LOG_VERBOSE, node);
    unreachable("");
}

static inline Node* nodes_get_child(Node* parent, size_t idx) {
    size_t curr_idx = 0;
    nodes_foreach_child(child, parent) {
        if (curr_idx == idx) {
            return child;
        }

        curr_idx++;
    }

    if (curr_idx > 0) {
        unreachable("idx %zu is out of bounds (max index is %zu [inclusive])", idx, curr_idx - 1);
    }
    unreachable("no children");
}

static inline const Node* nodes_get_child_const(const Node* parent, size_t idx) {
    return nodes_get_child((Node*)parent, idx);
}

static inline bool nodes_try_get_last_child_of_type(Node** result, Node* parent, NODE_TYPE node_type) {
    Node* curr_child = NULL;
    nodes_foreach_child(child, parent) {
        curr_child = child;
    }
    if (!curr_child) {
        return false;
    }

    nodes_foreach_from_curr_rev(curr, parent) {
        if (curr->type == node_type) {
            *result = curr;
            return true;
        }
    }

    return false;
}

static inline Node* nodes_get_last_child_of_type(Node* parent, NODE_TYPE node_type) {
    Node* result;
    try(nodes_try_get_last_child_of_type(&result, parent, node_type));
    return result;
}

// remove node_to_remove (and its children if present and keep_children is true) from tree. 
// children of node_to_remove will stay attached to node_to_remove if keep_children is true
static inline void nodes_remove(Node* node_to_remove, bool keep_children) {
    assert(node_to_remove);

    Node* next = node_to_remove->next;
    Node* prev = node_to_remove->prev;
    Node* parent = node_to_remove->parent;

    if (next) {
        next->prev = prev;
    }

    if (prev) {
        prev->next = next;
    }

    if (parent && get_left_child(parent) == node_to_remove) {
        set_left_child(parent, next);
    }

    if (!keep_children && get_left_child(node_to_remove)) {
        unreachable("");
    }

    nodes_reset_links_of_self_only(node_to_remove, keep_children);
}

static inline void nodes_extend_children(Node* parent, Node* start_of_nodes_to_extend) {
    Node* curr_node = start_of_nodes_to_extend;
    Node* next_node;
    while (1) {
        if (!curr_node) {
            break;
        }
        next_node = curr_node->next;

        nodes_remove(curr_node, true);
        nodes_append_child(parent, curr_node);

        curr_node = next_node;
    }
}

static inline Node* node_clone(const Node* node_to_clone) {
    Node* new_node = node_new(node_to_clone->pos, node_to_clone->type);
    *new_node = *node_to_clone;
    nodes_reset_links_of_self_only(new_node, false);
    return new_node;
}

static inline void nodes_move_back_one(Node* node) {
    assert(node->prev);

    Node* prev = node->prev;
    nodes_remove(node, true);
    nodes_insert_before(prev, node);
}

static inline Node* nodes_clone_self_and_children(const Node* node_to_clone) {
    Node* new_node = node_clone(node_to_clone);

    nodes_foreach_child(child, node_to_clone) {
        nodes_append_child(new_node, nodes_clone_self_and_children(child));
    }

    return new_node;
}

static inline void nodes_replace(Node* node_to_replace, Node* replacement_node) {
    (void) node_to_replace;
    (void) replacement_node;
    assert(!replacement_node->next);
    assert(!replacement_node->prev);
    assert(!replacement_node->parent);
    todo();
}

#endif // NODES_H
