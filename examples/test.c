
extern("c") fn puts(String str_to_print);

fn println(String string1, i32 num2) i32 {
    let string_thing: String = "67";
    string_thing = "thing thing";
    puts(string1);
    puts(string_thing);
    return num2;
}

fn main() {
    println("hello", 54);
    return println("bye world", 55);
}
