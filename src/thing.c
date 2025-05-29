#include <vector.h>
#include <env.h>

int main(void) {
    Defer_collection* defer_coll = vec_at_ref(&env.defered_collections, env.defered_collections.info.count - 1);
    vec_append(&a_main, &defer_coll->pairs, ((Defer_pair) {
        defer,
        tast_label_new(defer->pos, util_literal_name_new2())
    }));
    return 0;
}

