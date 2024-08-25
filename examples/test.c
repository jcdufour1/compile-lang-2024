
extern("c") fn puts(str_to_print: ptr) i32;
extern("c") fn printf(format_string: ptr, args: any...) i32;

fn get_string() ptr {
    let string_thing_2: ptr = "new programming language";
    return string_thing_2;
}

fn println(string1: ptr, num2: i32) i32 {
    let string_thing: ptr = "67";
    let num1: i32 = 1;
    num1 = 3;
    num1 = 5;
    string_thing = get_string();
    puts(string1);
    puts(string_thing);
    return num1;
}

fn main() i32 {
    let count_3: i32 = 89;
    let string_thing_4: ptr = get_string();
    string_thing_4 = "hello";
    printf("%s: %d\n", string_thing_4, count_3);
    return println("bye world", 55);
}
