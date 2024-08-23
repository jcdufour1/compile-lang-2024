
extern("c") fn puts(String str_to_print);

fn println(String string1, String string2) i32 {
    let string_hello: String = "new string 23";
    let num1: i32 = 24;
    puts(string_hello);
    puts(string1);
    puts(string2);
    return num1;
}

fn main() {
    return println("hello", "world");
    return println("bye world", "bye again");
}
