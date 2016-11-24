#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include "doctest.h"
#include "gason2.h"

#define TEST_STRING(json, expect)        \
    doc.parse(json);                     \
    val = doc[0];                        \
    n = sizeof(expect) - 1;              \
    CHECK(val.to_string() == expect);    \
    CHECK(strlen(val.to_string()) == n); \
    CHECK_FALSE(memcmp(val.to_string(), expect, n))

TEST_CASE("[nativejson-benchmark] conformance string") {
    gason2::document doc;
    gason2::node val;
    size_t n;

    TEST_STRING("[\"\"]", "");
    TEST_STRING("[\"Hello\"]", "Hello");
    TEST_STRING("[\"Hello\\nWorld\"]", "Hello\nWorld");
    TEST_STRING("[\"Hello\\u0000World\"]", "Hello\0World");
    TEST_STRING("[\"\\\"\\\\/\\b\\f\\n\\r\\t\"]", "\"\\/\b\f\n\r\t");
    TEST_STRING("[\"\\u0024\"]", "\x24");                    // Dollar sign U+0024
    TEST_STRING("[\"\\u00A2\"]", "\xC2\xA2");                // Cents sign U+00A2
    TEST_STRING("[\"\\u20AC\"]", "\xE2\x82\xAC");            // Euro sign U+20AC
    TEST_STRING("[\"\\uD834\\uDD1E\"]", "\xF0\x9D\x84\x9E"); // G clef sign U+1D11E
}
