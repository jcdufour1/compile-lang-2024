#include <auto_gen_vecs.h>
#include <auto_gen_util.h>
#include <auto_gen_tast.h>
#include <auto_gen_uast.h>
#include <auto_gen_ir.h>
#include <auto_gen_ulang_type.h>
#include <auto_gen_lang_type.h>
#include <auto_gen_ir_lang_type.h>

static const char* get_path(const char* build_dir, const char* file_name_in_dir) {
    String path = {0};

    string_extend_cstr(&a_gen, &path, build_dir);
    string_extend_cstr(&a_gen, &path, "/"); // TODO: do not hardcode this separator?
    string_extend_cstr(&a_gen, &path, file_name_in_dir);
    string_extend_cstr(&a_gen, &path, "\0");

    return path.buf;
}

int main(int argc, char** argv) {
    unwrap(argc == 2 && "invalid count of arguments provided");

    gen_ulang_type(get_path(argv[1], "ulang_type.h"));
    unwrap(!global_output);

    gen_lang_type(get_path(argv[1], "lang_type.h"));
    unwrap(!global_output);

    gen_ir_lang_type(get_path(argv[1], "ir_lang_type.h"));
    unwrap(!global_output);

    gen_all_tasts(get_path(argv[1], "tast_forward_decl.h"), false);
    unwrap(!global_output);
    gen_all_uasts(get_path(argv[1], "uast_forward_decl.h"), false);
    unwrap(!global_output);
    gen_all_irs(get_path(argv[1], "ir_forward_decl.h"), false);
    unwrap(!global_output);

    gen_all_vecs(get_path(argv[1], "vecs.h"));
    unwrap(!global_output);

    gen_all_tasts(get_path(argv[1], "tast.h"), true);
    unwrap(!global_output);

    gen_all_uasts(get_path(argv[1], "uast.h"), true);
    unwrap(!global_output);

    gen_all_irs(get_path(argv[1], "ir.h"), true);
    unwrap(!global_output);
}

