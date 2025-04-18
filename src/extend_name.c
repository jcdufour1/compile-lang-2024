#include <extend_name.h>
#include <serialize_module_symbol_name.h>

void extend_name_llvm(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name( name));
}
