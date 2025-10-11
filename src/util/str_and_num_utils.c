#include <str_and_num_utils.h>
#include <msg.h>
#include <msg_todo.h>
#include <errno.h>
#include <string_vec.h>

size_t get_count_excape_seq(Strv strv) {
    size_t count_excapes = 0;
    while (strv.count > 0) {
        if (strv_consume(&strv) == '\\') {
            if (strv.count < 1) {
                unreachable("invalid excape sequence");
            }

            strv_consume(&strv); // important in case of // excape sequence
            count_excapes++;
        }
    }
    return count_excapes;
}

// \n excapes are actually stored as is in tokens and irs, but should be printed as \0a
void string_extend_strv_eval_escapes(Arena* arena, String* string, Strv strv) {
    while (strv.count > 0) {
        char front_char = strv_consume(&strv);
        if (front_char == '\\') {
            vec_append(arena, string, '\\');
            switch (strv_consume(&strv)) {
                case 'n':
                    string_extend_hex_2_digits(arena, string, 0x0a);
                    break;
                default:
                    unreachable("");
            }
        } else {
            vec_append(arena, string, front_char);
        }
    }
}

static bool isdigit_no_underscore(char prev, char curr) {
    (void) prev;
    return isdigit(curr);
}

bool try_strv_octal_after_0o_to_int64_t(int64_t* result, const Pos pos, Strv strv) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < strv.count; idx++) {
        char curr_char = strv_at(strv, idx);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char < '0' || curr_char > '7') {
            msg(DIAG_INVALID_OCTAL, pos, "invalid octal literal\n");
            return false;
        }

        *result *= 8;
        *result += curr_char - '0';
    }

    assert(idx > 0);
    return true;
}

bool try_strv_hex_after_0x_to_int64_t(int64_t* result, const Pos pos, Strv strv) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < strv.count; idx++) {
        char curr_char = strv_at(strv, idx);
        if (curr_char == '_') {
            continue;
        }

        int increment = 0;
        if (isdigit(curr_char)) {
            increment = curr_char - '0';
        } else if (curr_char >= 'a' && curr_char <= 'f') {
            increment = (curr_char - 'a') + 10;
        } else if (curr_char >= 'A' && curr_char <= 'F') {
            increment = (curr_char - 'A') + 10;
        } else {
            msg(DIAG_INVALID_HEX, pos, "invalid hex literal\n");
            return false;
        }

        *result *= 16;
        *result += increment;
    }

    assert(idx > 0);
    return true;
}

bool try_strv_bin_after_0b_to_int64_t(int64_t* result, const Pos pos, Strv strv) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < strv.count; idx++) {
        char curr_char = strv_at(strv, idx);
        if (curr_char == '_') {
            continue;
        }

        if (curr_char != '0' && curr_char != '1') {
            msg(DIAG_INVALID_BIN, pos, "invalid bin literal\n");
            return false;
        }

        *result *= 2;
        *result += curr_char - '0';
    }

    assert(idx > 0);
    return true;
}

bool try_strv_to_int64_t(int64_t* result, const Pos pos, Strv strv) {
    *result = 0;
    size_t idx = 0;
    bool first_is_zero = false;
    for (idx = 0; idx < strv.count; idx++) {
        char curr_char = strv_at(strv, idx);
        if (curr_char == '_') {
            continue;
        }

        if (idx == 0) {
            first_is_zero = curr_char == '0';
        }

        if (first_is_zero && idx == 1 && isdigit(strv_at(strv, 1))) {
            msg(DIAG_INVALID_OCTAL, pos, "invalid octal literal; octal numbers must use `0o` prefix\n");
            return false;
        }

        if (isalpha(curr_char)) {
            // we are looking at x in 0x56, b in 0b10, etc.
            if (!first_is_zero || idx != 1) {
                msg(DIAG_INVALID_DECIMAL_LIT, pos, "invalid decimal literal\n");
                return false;
            }

            if (curr_char == 'x') {
                return try_strv_hex_after_0x_to_int64_t(
                    result,
                    pos,
                    strv_slice(strv, 2, strv.count - 2)
                );
            }

            if (curr_char == 'b') {
                return try_strv_bin_after_0b_to_int64_t(
                    result,
                    pos,
                    strv_slice(strv, 2, strv.count - 2)
                );
            }

            if (curr_char == 'o') {
                return try_strv_octal_after_0o_to_int64_t(
                    result,
                    pos,
                    strv_slice(strv, 2, strv.count - 2)
                );
            }

            msg(DIAG_INVALID_DECIMAL_LIT/* TODO */, pos, "invalid literal prefix `0%c`\n", curr_char);
            return false;
        }

        if (!isdigit(curr_char)) {
            break;
        }

        *result *= 10;
        *result += curr_char - '0';
    }

    return idx > 0;
}

bool try_strv_to_double(double* result, const Pos pos, Strv strv) {
    static Arena a_temp = {0};
    bool status = true;

    String buf = {0};
    string_extend_strv(&a_temp, &buf, strv);
    string_extend_cstr(&a_temp, &buf, "\0");

    char* buf_after = NULL;
    errno = 0;
    *result = strtod(buf.buf, &buf_after);
    if (errno != 0) {
        msg(DIAG_INVALID_FLOAT_LIT, pos, "%s\n", strerror(errno));
        status = false;
        goto error;
    }
    if ((buf_after - buf.buf) != (ptrdiff_t)strv.count) {
        // conversion unsuccessful
        msg_todo("actual error message for this problem", pos);
        status = false;
        goto error;
    }

    // conversion successful

error:
    arena_reset(&a_temp);
    return status;
}

bool try_strv_to_char(char* result, const Pos pos, Strv strv) {
    if (!strv_try_consume(&strv, '\\')) {
        if (strv.count != 1) {
            msg(
                DIAG_INVALID_CHAR_LIT, pos,
                "expected exactly one character in char literal without excapes, but got %zu\n",
                strv.count
            );
            return false;
        }
        *result = strv_front(strv);
        return true;
    }

    char esc_char = strv_consume(&strv);

    switch (esc_char) {
        case 'a':
            // fallthrough
        case 'b':
            // fallthrough
        case 'f':
            // fallthrough
        case 'n':
            // fallthrough
        case 'r':
            // fallthrough
        case 't':
            // fallthrough
        case 'v':
            // fallthrough
        case '\\':
            // fallthrough
        case '\'':
            // fallthrough
        case '"':
            // fallthrough
        case '?':
            // fallthrough
        case '0':
            if (strv.count != 0) {
                msg(
                    DIAG_INVALID_CHAR_LIT, pos,
                    "expected 0 characters in char literal after `\\%c`, but got %zu\n",
                    esc_char, strv.count
                );
                return false;
            }
            break;
        case 'x':
            // hexadecimal
            if (strv.count != 2) {
                msg(
                    DIAG_INVALID_CHAR_LIT, pos,
                    "expected exactly 2 characters in char literal after `\\x` excape, but got %zu\n",
                    strv.count + 1
                );
                return false;
            }
            break;
        case 'o':
            // octal
            if (strv.count != 3) {
                msg(
                    DIAG_INVALID_CHAR_LIT, pos,
                    "expected exactly 3 characters in char literal after `\\o` excape, but got %zu\n",
                    strv.count + 1
                );
                return false;
            }
            break;
        default: {
            String buf = {0};
            string_extend_cstr(&a_main, &buf, "excape sequence `\\");
            vec_append(&a_main, &buf, esc_char);
            string_extend_cstr(&a_main, &buf, "`");
            msg_todo_strv(string_to_strv(buf), pos);
            return false;
        }
    }

    switch (esc_char) {
        case 'a':
            *result = '\a';
            return true;
        case 'b':
            *result = '\b';
            return true;
        case 'f':
            *result = '\f';
            return true;
        case 'n':
            *result = '\n';
            return true;
        case 'r':
            *result = '\r';
            return true;
        case 't':
            *result = '\t';
            return true;
        case 'v':
            *result = '\v';
            return true;
        case '\\':
            *result = '\\';
            return true;
        case '\'':
            *result = '\'';
            return true;
        case '"':
            *result = '"';
            return true;
        case '?':
            *result = '?';
            return true;
        case '0':
            *result = '\0';
            return true;
        case 'x': {
            // hexadecimal
            int64_t result_ = 0;
            if (!try_strv_hex_after_0x_to_int64_t(&result_, pos, strv)) {
                return false;
            }
            assert(result_ <= 0xFF && "this should have been caught in the previous switch");
            *result = (char)result_;
            return true;
        }
        case 'o': {
            // hexadecimal
            int64_t result_ = 0;
            if (!try_strv_octal_after_0o_to_int64_t(&result_, pos, strv)) {
                return false;
            }
            assert(result_ <= 0xFF && "this should have been caught in the previous switch");
            *result = (char)result_;
            return true;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

bool try_strv_to_size_t(size_t* result, Strv strv) {
    *result = 0;
    size_t idx = 0;
    for (idx = 0; idx < strv.count; idx++) {
        char curr_char = strv_at(strv, idx);
        if (!isdigit(curr_char)) {
            break;
        }

        *result *= 10;
        *result += curr_char - '0';
    }

    if (idx < 1) {
        return false;
    }
    return true;
}

bool try_strv_consume_size_t(size_t* result, Strv* strv, bool ignore_underscore) {
    assert(!ignore_underscore && "not implemented");
    Strv num = strv_consume_while(strv, isdigit_no_underscore);
    return try_strv_to_size_t(result, num);
}

int64_t strv_to_int64_t(const Pos pos, Strv strv) {
    int64_t result = INT64_MAX;

    if (!try_strv_to_int64_t(&result,  pos, strv)) {
        unreachable(FMT, strv_print(strv));
    }
    return result;
}

Strv util_literal_strv_new_internal(const char* file, int line, Strv debug_prefix) {
    (void) debug_prefix;
    (void) file;
    (void) line;
    static size_t count = 0;

    String var_name = {0};
    string_extend_cstr(&a_main, &var_name, "str");

    // TODO: use better solution for debugging
    //string_extend_cstr(&a_main, &var_name, "file____");
    //string_extend_strv(&a_main, &var_name, serialize_name(name_new(sv(file), (Strv) {0}, (Ulang_type_vec) {0}, 0)));
    //string_extend_cstr(&a_main, &var_name, "_");
    //string_extend_size_t(&a_main, &var_name, line);
    //string_extend_cstr(&a_main, &var_name, "_");

#ifndef NDEBUG
    string_extend_cstr(&a_main, &var_name, "__");
    string_extend_strv(&a_main, &var_name, debug_prefix);
    string_extend_cstr(&a_main, &var_name, "__");
#endif // NDEBUG

    string_extend_size_t(&a_main, &var_name, count);
    
    count++;

    return string_to_strv(var_name);
}

Name util_literal_name_new_prefix_scope_internal(const char* file, int line, Strv debug_prefix, Scope_id scope_id) {
    return name_new(MOD_PATH_BUILTIN, util_literal_strv_new_internal(file, line, debug_prefix), (Ulang_type_vec) {0}, scope_id);
}
