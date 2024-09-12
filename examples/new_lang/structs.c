
extern("c") fn printf(format_string: ptr, args: any...) i32;

struct Div {
    numer: i32;
    denom: i32;
}

fn main() i32 {
    let div: Div;
    div = {.numer = 9, .denom = 2};
    printf("%d\n", div.numer);
    return 0;
}
