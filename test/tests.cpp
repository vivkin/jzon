#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "gason2.h"
#include "gason2dump.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

void pbox(gason2::value v) {
    printf("%2d %6d %8x %10u %13g\n", v.number == v.number, v.is_nan(), static_cast<unsigned int>(v.tag.type), v.payload, v.number);
}

TEST_CASE("boxing") {
    printf("%2s %6s %4s %15s %13s\n", "==", "is_nan", "tag", "payload", "number");

    double numbers[] = {0.0, 1.0, 1.0 / 3.0, 5.45, 7.62, 1e40, DBL_MIN, DBL_EPSILON, DBL_MAX, INFINITY, NAN};

    for (auto n : numbers) {
        pbox(-n);
        pbox(n);
    }

    pbox(gason2::error::expecting_value);
    pbox(gason2::error::unexpected_character);
    pbox({gason2::type::null, 0});
    pbox({gason2::type::boolean, false});
    pbox({gason2::type::boolean, true});
    pbox({gason2::type::string, 0xDEADBEEF});
    pbox({gason2::type::string, 0xBADCAB1E});
    pbox({gason2::type::array, ~1ul});
    pbox({gason2::type::object, 0xFFFFFFFFul});
}

TEST_CASE("obvious") {
    gason2::document doc;

    CHECK_FALSE(doc.parse(""));
    CHECK(doc.error_code() == gason2::error::expecting_value);

    CHECK(doc.parse(u8R"json(1234567890)json"));
    CHECK(doc.is_number());
    CHECK(doc.to_number() == 1234567890);

    CHECK_FALSE(doc.parse(u8R"json({42: "member name must be string")json"));
    CHECK(doc.error_code() == gason2::error::expecting_string);

    CHECK(doc.parse(u8R"json({
    "empty": {},
    "alpha": "abcdefghijklmnopqrstuvwyz",
    "num": 123456789,
    "literals": [false, true, null]
})json"));
    CHECK(doc.is_object());
    CHECK(doc.size() == 4);
    CHECK_EQ(doc["alpha"].to_string(), "abcdefghijklmnopqrstuvwyz");
    CHECK(doc["num"].to_string() == "abcdefghijklmnopqrstuvwyz");
    CHECK(doc["num"].to_number() == 123456789);
    CHECK(doc[100][500].to_number(100500) == 100500);
    CHECK(doc["empty"].is_object());
    CHECK(doc["empty"].size() == 0);
    CHECK(doc["empty"]["missing"].is_null());
    CHECK(doc["empty"]["missing"][111]["really"].to_bool(true));
    CHECK(doc["literals"].is_array());
    CHECK(doc["literals"].size() == 3);
    CHECK(doc["literals"][0].is_bool());
    CHECK(doc["literals"][1].to_bool());
    CHECK(doc["literals"][999].is_null());
    CHECK(doc["literals"]["missing"].is_null());
}
