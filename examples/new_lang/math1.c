extern("c") fn printf(str_to_print: ptr, args: any...) i32;

fn do_more_operations(curr_num: i32) {
    let result5: i32;
    result5 = 2*curr_num + 1;
    printf("2*%d + 1 = %d\n", curr_num, result5);
}

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

        do_more_operations(num1);
    }

    return 0;
}

