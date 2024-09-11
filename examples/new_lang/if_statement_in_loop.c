extern("c") fn puts(str_to_print: ptr) i32;
extern("c") fn printf(str_to_print_2: ptr, args: any...) i32;

fn main() i32 {
    for num1: i32 in {0..7} {
        for num2: i32 in {0..num1} {
            let num_temp: i32;
            num_temp = num2;
            if num_temp < 5 {
                printf("%d ", num2);
            }
        }
        printf("\n");
    }

    for num11: i32 in {2..9} {
        for num12: i32 in {num11..9} {
            let num_temp_2: i32 = num12;
            if num_temp_2 < 6 {
                printf("%d ", num12);
            }
        }
        printf("\n");
    }

    return 0;
}
