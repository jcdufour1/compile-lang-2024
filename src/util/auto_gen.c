#include <auto_gen_vecs.h>
#include <auto_gen_util.h>
#include <auto_gen_tast.h>
#include <auto_gen_uast.h>
#include <auto_gen_ir.h>
#include <auto_gen_lang_type.h>
#include <auto_gen_llvm_lang_type.h>

// TODO: move todos to somewhere else
//
// TODO: make print functions check for null and print <null> or something instead of seg fault
// TODO: figure out if I should de-duplilcate string literals, etc.

// TODO: strings should actually be their own type (not just u8*) (maybe slice with null termination)

// TODO: support alternative backend (such as qbe, cuik)

// TODO: fix printing for lang_type in msg to not put newline

// TODO: actually use newline to end statement depending on last token of line of line
// TODO: expected failure case for invalid type in extern "c" function
//
//
// TODO: expected failure test for 
// type Token enum {
//     string u8*
//     number i32
// }
//
// fn main() i32 {
//     let token Token = Token.number
//     return 0
// }

static const char* get_path(const char* build_dir, const char* file_name_in_dir) {
    String path = {0};

    string_extend_cstr(&gen_a, &path, build_dir);
    string_extend_cstr(&gen_a, &path, "/");
    string_extend_cstr(&gen_a, &path, file_name_in_dir);
    string_extend_cstr(&gen_a, &path, "\0");

    return path.buf;
}

int main(int argc, char** argv) {
    assert(argc == 2 && "invalid count of arguments provided");

    gen_lang_type(get_path(argv[1], "lang_type.h"), true);
    assert(!global_output);

    gen_llvm_lang_type(get_path(argv[1], "llvm_lang_type.h"), true);
    assert(!global_output);

    gen_all_tasts(get_path(argv[1], "tast_forward_decl.h"), false);
    assert(!global_output);
    gen_all_uasts(get_path(argv[1], "uast_forward_decl.h"), false);
    assert(!global_output);
    gen_all_irs(get_path(argv[1], "ir_forward_decl.h"), false);
    assert(!global_output);

    gen_all_vecs(get_path(argv[1], "vecs.h"));
    assert(!global_output);

    gen_all_tasts(get_path(argv[1], "tast.h"), true);
    assert(!global_output);

    gen_all_uasts(get_path(argv[1], "uast.h"), true);
    assert(!global_output);

    gen_all_irs(get_path(argv[1], "ir.h"), true);
    assert(!global_output);
}

