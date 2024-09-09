extern("c") fn printf(str_to_print: ptr, args: any...) i32;

fn main() i32 {
    for let num1: i32 in {0..10} {
        // define and assign variables
        let add_result: i32 = num1 + 3;
        let sub_result: i32 = num1 - 3;
        let mul_result: i32 = num1*3;
        let div_result: i32 = num1/3;
        printf("%d + 3 = %d\n", num1, add_result);
        printf("%d - 3 = %d\n", num1, sub_result);
        printf("%d*3 = %d\n", num1, mul_result);
        printf("%d/3 = %d\n", num1, div_result);
    }

    return 0;
}

