#include <stdio.h>
#include <string.h>

int mul(int a, int b) {
    return a*b;
}

typedef struct {
    int numerator;
    int denominator;
    int extra_num;
    int extra_num_2;
    int extra_num_3;
} Div;

Div print(const char* str, int num1, char* another_str) {
    int b = 1;
    puts(str);
    b = mul(3, 4);
    puts(str);

    Div div = {.numerator = 1, .denominator = 2};
    return div;
}

void do_thing(Div div) {
}

int main(void) {
    Div div = {.numerator = 78, .denominator = 90, .extra_num = 7};
    printf("%d\n", div.extra_num);
    div.extra_num = 12;
    printf("%d\n", div.extra_num);
    return 0;
}
