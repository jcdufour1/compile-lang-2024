fn println(String string1, String string2) i32 {
    puts(string1);
    puts(string2);
    return 1;
}

fn main() {
    println("hello", "world");
    println("bye world", "bye again");
    return 0;
}
