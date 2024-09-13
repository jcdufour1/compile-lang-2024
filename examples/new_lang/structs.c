
extern("c") fn printf(format_string: ptr, args: any...) i32;

struct Div {
    numerator: i32;
    denominator: i32;
}

fn main() i32 {
    let div: Div;
    div = {.numerator = 9, .denominator = 2};
    printf("%d/%d\n", div.numerator, div.denominator);
    div.numerator = 11;
    printf("%d/%d\n", div.numerator, div.denominator);
    return 0;
}
