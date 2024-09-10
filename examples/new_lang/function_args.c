extern("c") fn puts(str_to_print: ptr) i32;
extern("c") fn printf(str_to_print_2: ptr, args: any...) i32;

fn print_string(string_input: ptr, num1: i32) i32 {
    puts(string_input);
    printf("%d\n", num1);

    let num2: i32 = 3*num1 + 7;
    printf("%d\n", num2);

    return 0;
}

fn main() i32 {
    print_string("hello", 86);
    return 0;
}
