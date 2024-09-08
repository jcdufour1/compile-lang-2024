
extern("c") fn puts(str_to_print: ptr) i32;
extern("c") fn printf(str_to_print_2: ptr, args: any...) i32;

fn main() i32 {
    for let num1: i32 in {0..7} {
        for let num2: i32 in {num1..7} {
            printf("%d ", num2);
        }
        puts("");

        if num1 < 4 {
            puts("hello world");
        }
        if num1 < 5 {
            puts("bye everyone");
        }
    }

    return 0;
}
