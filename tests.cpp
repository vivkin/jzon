#include "jzon.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

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

void pvalue(jzon::value v) {
    printf("%2d %6d %4x %15lu %13g\n", v.number == v.number, v.is_nan(), v.tag, v.payload, v.number);
}

int main(int __unused argc, char __unused **argv) {
    printf("%2s %6s %4s %15s %13s\n", "==", "is_nan", "tag", "payload", "number");

    double numbers[] = {0.0, 1.0, 1.0 / 3.0, 5.45, 7.62, 1e40, DBL_MIN, DBL_EPSILON, DBL_MAX, INFINITY, NAN};
    for (auto n : numbers) {
        pvalue(-n);
        pvalue(n);
    }

    pvalue({0, jzon::null_tag});
    pvalue({false, jzon::bool_tag});
    pvalue({true, jzon::bool_tag});
    pvalue({0xDEADBEEF, jzon::string_tag});
    pvalue({0xBADCAB1E, jzon::string_tag});
    pvalue({~1ul, jzon::array_tag});
    pvalue({0xFFFFFFFFul, jzon::object_tag});

    return 0;
}
