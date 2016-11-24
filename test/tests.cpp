#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "gason2.h"

TEST_CASE("[gason] parsing") {
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
    CHECK(doc["num"].to_string("haha") == "haha");
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
