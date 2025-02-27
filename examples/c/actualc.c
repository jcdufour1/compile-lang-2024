#include <stdio.h>
#include <string.h>

int mul(int a, int b) {
    return a*b;
}

typedef struct {
    int type;
    int number_data;
    char* string_data;
    int extra;
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

typedef union {
    int num;
    char* cstr;
} Thing;

void print_token(Token token1) {
    printf("%d\n", token1.type);
}

static int get_num_1(void) {
    printf("get_num_1\n");
    return 7;
}

static int get_num_2(void) {
    printf("get_num_2\n");
    return 6;
}

static int get_num_3(void) {
    printf("get_num_3\n");
    return 5;
}

int main() {
    //if (get_num_1() < 4 && get_num_2() > 5 || get_num_3() < 6) {
    //    printf("yes\n");
    //}

    if (get_num_1() && get_num_2() && get_num_3() < 6) {
        printf("yes\n");
    }
    return 0;
}
