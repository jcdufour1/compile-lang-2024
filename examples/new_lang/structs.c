
extern("c") fn printf(format_string: ptr, args: any...) i32;

struct Div {
    numerator: i32;
    denominator: i32;
}

fn get_numerator(div: Div) i32 {
    div.numerator = 8;
    div.denominator = 7;
    printf("%d/%d\n", div.numerator, div.denominator);
    return div.numerator;
}

fn get_div(div: Div) Div {
    div.numerator = 14;
    div.denominator = 34;
    printf("%d/%d\n", div.numerator, div.denominator);
    return div;
}

fn div_ptr(div: Div*) i32 {
    div.numerator = 87;
    div.denominator = 23;
    printf("%d/%d\n", div.numerator, div.denominator);
    return 0;
}

fn main() i32 {
    let div: Div;
    div = {.numerator = 9, .denominator = 2};
    printf("%d/%d\n", div.numerator, div.denominator);
    div.numerator = 11;
    div.denominator = 10;
    let result: i32 = get_numerator(div);
    printf("%d/%d\n", div.numerator, div.denominator);
    printf("%d/%d\n", result, div.denominator);
    let new_div: Div = get_div(div);
    printf("%d/%d\n", new_div.numerator, new_div.denominator);
    div_ptr(new_div);
    printf("%d/%d\n", new_div.numerator, new_div.denominator);
    return 0;
}
