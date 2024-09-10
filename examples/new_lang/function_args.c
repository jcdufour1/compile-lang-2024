extern("c") fn puts(str_to_print: ptr) i32;

fn print_string(string_input: ptr) i32 {
    puts(string_input);
    return 0;
}

fn main() i32 {
    print_string("hello");
    return 0;
}
