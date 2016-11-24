#include "doctest.h"
#include "gason2.h"

TEST_CASE("[JSON_checker] fail") {
    gason2::document doc;
    CHECK(doc.parse(u8R"json("A JSON payload should be an object or array, not a string.")json"));
    CHECK_FALSE(doc.parse(u8R"json(["Unclosed array")json"));
    CHECK_FALSE(doc.parse(u8R"json({unquoted_key: "keys must be quoted"})json"));
    CHECK_FALSE(doc.parse(u8R"json(["extra comma",])json"));
    CHECK_FALSE(doc.parse(u8R"json(["double extra comma",,])json"));
    CHECK_FALSE(doc.parse(u8R"json([   , "<-- missing value"])json"));
    CHECK_FALSE(doc.parse(u8R"json(["Comma after the close"],)json"));
    CHECK_FALSE(doc.parse(u8R"json(["Extra close"]])json"));
    CHECK_FALSE(doc.parse(u8R"json({"Extra comma": true,})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Extra value after close": true} "misplaced quoted value")json"));
    CHECK_FALSE(doc.parse(u8R"json({"Illegal expression": 1 + 2})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Illegal invocation": alert()})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Numbers cannot have leading zeroes": 013})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Numbers cannot be hex": 0x14})json"));
    CHECK_FALSE(doc.parse(u8R"json(["Illegal backslash escape: \x15"])json"));
    CHECK_FALSE(doc.parse(u8R"json([\naked])json"));
    CHECK_FALSE(doc.parse(u8R"json(["Illegal backslash escape: \017"])json"));
    CHECK(doc.parse(u8R"json([[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]])json"));
    CHECK_FALSE(doc.parse(u8R"json({"Missing colon" null})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Double colon":: null})json"));
    CHECK_FALSE(doc.parse(u8R"json({"Comma instead of colon", null})json"));
    CHECK_FALSE(doc.parse(u8R"json(["Colon instead of comma": false])json"));
    CHECK_FALSE(doc.parse(u8R"json(["Bad value", truth])json"));
    CHECK_FALSE(doc.parse(u8R"json(['single quote'])json"));
    CHECK_FALSE(doc.parse(u8R"json(["	tab	character	in	string	"])json"));
    CHECK_FALSE(doc.parse(u8R"json(["tab\   character\   in\  string\  "])json"));
    CHECK_FALSE(doc.parse(u8R"json(["line
break"])json"));
    CHECK_FALSE(doc.parse(u8R"json(["line\
break"])json"));
    CHECK_FALSE(doc.parse(u8R"json([0e])json"));
    CHECK_FALSE(doc.parse(u8R"json([0e+])json"));
    CHECK_FALSE(doc.parse(u8R"json([0e+-1])json"));
    CHECK_FALSE(doc.parse(u8R"json({"Comma instead if closing brace": true,)json"));
    CHECK_FALSE(doc.parse(u8R"json(["mismatch"})json"));
}
