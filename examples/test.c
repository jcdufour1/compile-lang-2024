
extern("c") fn puts(String str_to_print) i32;

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
    println("hello", 54);
    return println("bye world", 55);
}
