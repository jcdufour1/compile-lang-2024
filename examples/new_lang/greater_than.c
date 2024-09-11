extern("c") fn puts(str_to_print: ptr) i32;

fn main() i32 {
    if 8 < 1 {
        puts("1");
    }

    if 8 < 8 {
        puts("2");
    }

    if 8 < 9 {
        puts("3");
    }

    if 8 > 1 {
        puts("4");
    }

    if 8 > 8 {
        puts("5");
    }

    if 8 > 9 {
        puts("6");
    }
    return 0;
}
