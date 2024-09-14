
extern("c") fn printf(format_string: ptr, args: any...) i32;

struct Div {
    numerator: i32;
    denominator: i32;
}

fn get_numerator(div: Div) i32 {
    return div.numerator;
}

fn main() i32 {
    let div: Div;
    div = {.numerator = 9, .denominator = 2};
    printf("%d/%d\n", div.numerator, div.denominator);
    div.numerator = 11;
    div.denominator = 10;
    let result: i32 = get_numerator(div);
    printf("%d/%d\n", result, div.denominator);
    return 0;
}
