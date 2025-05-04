#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t b;
    uint8_t g;
} Color;

bool is_red(Color color) {
    return color.r == 255 && color.b == 0 && color.g == 0;
}

bool is_blue(Color color) {
    return color.r == 0 && color.b == 255 && color.g == 0;
}

bool is_green(Color color) {
    return color.r == 0 && color.b == 0 && color.g == 255;
}

