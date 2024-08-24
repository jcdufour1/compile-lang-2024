
extern("c") fn puts(String str_to_print);

fn println(String string1, i32 num2) i32 {
    puts(string1);
    return num2;
}

fn main() {
    println("hello", 54);
    return println("bye world", 55);
}
