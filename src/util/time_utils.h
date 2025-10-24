#include <sys/time.h>
#include <stdint.h>
#include <strv.h>

static uint64_t get_time_milliseconds(void) {
    struct timeval time_now = {0};
    gettimeofday(&time_now, NULL);
    return time_now.tv_usec + time_now.tv_sec*1000000;
}

static Strv milliseconds_print_internal(uint64_t mills) {
    String buf = {0};

    string_extend_int64_t(&a_print, &buf, mills/1000000);
    string_extend_cstr(&a_print, &buf, ".");
    string_extend_int64_t(&a_print, &buf, mills%1000000);
    string_extend_cstr(&a_print, &buf, "sec");

    return string_to_strv(buf);
}

#define milliseconds_print(mills) \
    strv_print(milliseconds_print_internal(mills))

