
extern("c") fn puts(String str_to_print) i32;
extern("c") fn printf(String format_string, any... args) i32;

fn get_string() String {
    let string_thing_2: String = "new programming language";
    return string_thing_2;
}

fn println(String string1, i32 num2) i32 {
    let string_thing: String = "67";
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
    let string_thing_4: String = "bicycle";
    printf("%s: %d\n", string_thing_4, count_3);
    return println("bye world", 55);
}
