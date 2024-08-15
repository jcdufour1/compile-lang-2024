#include "newstring.h"
#include "util.h"
#include "str_view.h"
#include "node.h"

static const char* NODE_LITERAL_DESCRIPTION = "literal";
static const char* NODE_FUNCTION_CALL_DESCRIPTION = "fn_call";
static const char* NODE_FUNCTION_DEFINITION_DESCRIPTION = "fn_def";
static const char* NODE_FUNCTION_PARAMETERS_DESCRIPTION = "fn_params";
static const char* NODE_FUNCTION_RETURN_TYPES_DESCRIPTION = "fn_return_types";
static const char* NODE_FUNCTION_BODY_DESCRIPTION = "fn_body";

static Str_view node_type_get_strv(NODE_TYPE node_type) {
    Str_view buf = {0};

    switch (node_type) {
        case NODE_LITERAL:
            return str_view_from_cstr(NODE_LITERAL_DESCRIPTION);
        case NODE_FUNCTION_CALL:
            return str_view_from_cstr(NODE_FUNCTION_CALL_DESCRIPTION);
        case NODE_FUNCTION_DEFINITION:
            return str_view_from_cstr(NODE_FUNCTION_DEFINITION_DESCRIPTION);
        case NODE_FUNCTION_PARAMETERS:
            return str_view_from_cstr(NODE_FUNCTION_PARAMETERS_DESCRIPTION);
        case NODE_FUNCTION_RETURN_TYPES:
            return str_view_from_cstr(NODE_FUNCTION_RETURN_TYPES_DESCRIPTION);
        case NODE_FUNCTION_BODY:
            return str_view_from_cstr(NODE_FUNCTION_BODY_DESCRIPTION);
        default:
            todo();
    }
}

String node_print_internal(const Node* node, int pad_x) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    for (int idx = 0; idx < pad_x; idx++) {
        string_append(&buf, ' ');
    }

    string_extend_strv(&buf, node->name);

    string_append(&buf, ':');

    string_extend_strv(&buf, node_type_get_strv(node->type));

    string_append(&buf, '\n');

    return buf;
}

