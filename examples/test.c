
extern("c") fn puts(String str_to_print);

fn println(String string1, i32 num2) i32 {
    let string_thing: String = "67";
    let num1: i32 = 1;
    num1 = 3;
    num1 = 4;
    string_thing = "thing thing";
    puts(string1);
    puts(string_thing);
    return num1;
}

fn main() {
    println("hello", 54);
    return println("bye world", 55);
}
