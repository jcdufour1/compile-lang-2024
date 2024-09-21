#include <stdio.h>
#include <string.h>

int mul(int a, int b) {
    return a*b;
}

typedef struct {
    long num;
    char* string_data;
} Token;

typedef struct {
    int numerator;
    int denominator;
    int extra_num;
    int extra_num_2;
    int extra_num_3;
    Token token;
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
    printf("%d/%d %s\n", div.numerator, div.denominator, div.token.string_data);
}

void do_thing_ptr(Div* div) {
}

int main(void) {
    Token token = {.num = 89, .string_data = "djsf"};
    Div div = {.numerator = 78, .denominator = 90, .extra_num = 7};
    printf("%d\n", div.extra_num);
    div.extra_num = 12;
    printf("%d\n", div.extra_num);
    do_thing(div);
    do_thing_ptr(&div);
    return 0;
}
