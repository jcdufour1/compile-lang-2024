
extern("c") fn puts(String str_to_print);

fn println(String string1, String string2) i32 {
    let num1: i32 = 6;
    num1 = 8;
    puts(string1);
    puts(string2);
    return num1;
}

fn main() {
    println("hello", "world");
    return println("bye world", "bye again");
}
