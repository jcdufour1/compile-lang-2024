
extern("c") fn puts(str_to_print: ptr) i32
extern("c") fn printf(str_to_print_2: ptr, args: any...) i32

fn main() i32 {
    for num1: i32 in {0..7} {
        printf("outer for\n")
        for num2: i32 in {0..num1} {
            num1 = 8
            printf("%d ", num2)
        }
        printf("\n")
    }

    return 0
}
