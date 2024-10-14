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
    int num1 = 8;
    int* ptr = &num1;
    return 0;
}
