fn println(String string1, String string2) i32 {
    let string_hello: String = "new string 23";
    puts(string_hello);
    puts(string1);
    puts(string2);
    return 4;
}

fn main() {
    println("hello", "world");
    return println("bye world", "bye again");
}
