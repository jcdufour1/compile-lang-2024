
extern("c") fn puts(String str_to_print);

fn println(String string1, String string2) i32 {
    let num1: i32 = 9;
    let num2: i32 = 10;
    puts(string1);
    puts(string2);
    return num2;
}

fn main() {
    println("hello", "world");
    return println("bye world", "bye again");
}
