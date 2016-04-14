#include "jzon.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

#if __GNUC__ >= 3
#define __noinline __attribute__((noinline))
#define __used __attribute__((used))
#define __unused __attribute__((unused))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define __noinline
#define __used
#define __unused
#define likely(x) (x)
#define unlikely(x) (x)
#endif

int main(int __unused argc, char __unused **argv) {
    double v[] = {0, 1.0, -1.0, 1.0 / 3.0, -1.0 / 3.0, 1e308, -1e308, INFINITY, -INFINITY, NAN,
                  jzon::value{0xDEADBEEF, jzon::string_tag}.number,
                  jzon::value{0xBADCAB1E, jzon::string_tag}.number,
                  jzon::value{true}.number,
                  jzon::value{false}.number,
                  jzon::value{~1u, jzon::array_tag}.number,
                  jzon::value{0.0}.number};
    printf("%10s %5s %5s %8s %8s\n", "", "==", "isnan", "tag", "payload");
    for (auto x : v) {
        jzon::value z;
        z.number = x;
        printf("%10g %5d %5d %8x %8lu\n", x, x == x, isnan(x), z.tag(), z.integer);
    }
    return 0;
}
