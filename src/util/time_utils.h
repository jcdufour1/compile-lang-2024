
#ifdef _WIN32
#   include <winapi_wrappers.h>
#else
#   include <sys/time.h>
#   include <stdint.h>
#endif // _WIN32
       
#include <strv.h>

INLINE uint64_t get_time_milliseconds(void) {
#   ifdef _WIN32
        return winapi_GetTickCount64();
#   else
        struct timeval time_now = {0};
        gettimeofday(&time_now, NULL);
        return (uint64_t)time_now.tv_usec + (uint64_t)time_now.tv_sec*1000000;
#   endif // _WIN32
    unreachable("");
}

static Strv milliseconds_print_internal(uint64_t mills) {
    String buf = {0};

    string_extend_uint64_t(&a_temp, &buf, mills/1000000);
    string_extend_cstr(&a_temp, &buf, ".");
    char num_str[32];
    sprintf(num_str, "%.06"PRIu64, mills%1000000);
    string_extend_cstr(&a_temp, &buf, num_str);
    string_extend_cstr(&a_temp, &buf, "sec");

    return string_to_strv(buf);
}

#define milliseconds_print(mills) \
    strv_print(milliseconds_print_internal(mills))

