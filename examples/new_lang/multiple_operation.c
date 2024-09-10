
extern("c") fn printf(format_string: ptr, args: any...) i32;

fn main() i32 {
    let num1: i32 = 2;
    let num2: i32 = 93;
    let result: i32;
    let num3: i32 = 3;
    let num4: i32 = 9;

    result = num2*num3 + num4/num1;
    printf("%d\n", result);
    return 0;
}
