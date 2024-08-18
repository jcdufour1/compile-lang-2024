#ifndef PARSE_ITEMS_H
#define PARSE_ITEMS_H

#include "node.h"
#include "nodes.h"
#include "token_view.h"
#include "token.h"
#include "parser.h"

#define PARSE_ITEMS_DEFAULT_CAPACITY 64

typedef struct {
    Node_id root_node; // node that tokens are being parsed into
    Tk_view tokens; // these tokens will be parsed into root_node
    PARSE_STATE parse_state;
} Parse_item;

typedef struct {
    Parse_item* buf;
    size_t count;
    size_t capacity;
} Parse_items;

extern Parse_items parse_items;

static inline void parse_items_reserve(size_t minimum_count_empty_slots) {
    while (parse_items.count + minimum_count_empty_slots > parse_items.capacity) {
        if (parse_items.capacity < 1) {
            parse_items.capacity = PARSE_ITEMS_DEFAULT_CAPACITY;
            parse_items.buf = safe_malloc(parse_items.capacity, sizeof(parse_items.buf[0]));
        } else {
            parse_items.capacity *= 2;
            parse_items.buf = safe_realloc(parse_items.buf, parse_items.capacity, sizeof(parse_items.buf[0]));
        }
    }
}

static inline void parse_items_append(Parse_item item) {
    parse_items_reserve(1);
    parse_items.buf[parse_items.count++] = item;
}

static inline Parse_item parse_items_pop() {
    if (parse_items.count < 1) {
        unreachable();
    }
    Parse_item item_to_return = parse_items.buf[parse_items.count - 1];
    memset(&parse_items.buf[parse_items.count - 1], 0, sizeof(parse_items.buf[0]));
    parse_items.count--;
    return item_to_return;
}

static inline void parse_items_set_to_zero_len() {
    if (parse_items.count < 1) {
        return;
    }
    memset(parse_items.buf, 0, sizeof(parse_items.buf[0])*parse_items.count);
    parse_items.count = 0;
}

static inline Str_view parse_item_print_internal(Parse_item item) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_string(&buf, node_print_internal(item.root_node));

    for (size_t idx = 0; idx < item.tokens.count; idx++) {
        string_extend_strv(&buf, token_print_internal(item.tokens.tokens[idx]));
    }

    string_extend_cstr(&buf, "parse_state:");
    string_add_int(&buf, item.parse_state);

    Str_view str_view = {.str = buf.buf, .count = buf.count};
    return str_view;
}

#define PARSE_ITEM_FMT STR_VIEW_FMT

#define parse_item_print(parse_item) str_view_print(parse_item_print_internal(parse_item))

#endif // PARSE_ITEMS_H
