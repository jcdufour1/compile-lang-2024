#include <stdio.h>
#include <string.h>

int mul(int a, int b) {
    return a*b;
}

typedef struct {
    long num;
    char ch;
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
    printf("%d/%d %c\n", div.numerator, div.denominator, div.token.ch);
}

void do_thing_ptr(Div* div) {
}

void do_thing_member_ptr(int* numerator) {
    *numerator = 9;
}

int main(void) {
    int a = 8;
    Div div = {.numerator = 78, .denominator = 90, .extra_num = 7};
    printf("%d\n", div.extra_num);
    return 0;
}
