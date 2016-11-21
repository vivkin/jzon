#include "gason2.h"
#include "gason2dump.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_SUITE("my");

void pbox(gason2::box v) {
    printf("%2d %6d %8x %10u %13g\n", v.number == v.number, v.is_nan(), v.tag.bits, v.payload, v.number);
}

TEST_CASE("boxing") {
    printf("%2s %6s %4s %15s %13s\n", "==", "is_nan", "tag", "payload", "number");

    double numbers[] = {0.0, 1.0, 1.0 / 3.0, 5.45, 7.62, 1e40, DBL_MIN, DBL_EPSILON, DBL_MAX, INFINITY, NAN};

    for (auto n : numbers) {
        pbox(-n);
        pbox(n);
    }

    pbox(gason2::error::expecting_value);
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
    CHECK(doc.error_code() == gason2::error::unexpected_character);

    CHECK(doc.parse(u8R"json(1234567890)json"));
    CHECK(doc.is_number());
    CHECK(doc.to_number() == 1234567890);

    CHECK_FALSE(doc.parse(u8R"json({42: "member name must be string")json"));
    CHECK(doc.error_code() == gason2::error::expecting_string);
}

TEST_SUITE_END();

TEST_SUITE("JSON_checker");

TEST_CASE("fail") {
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

TEST_CASE("pass") {
    gason2::document doc;
    CHECK(doc.parse(u8R"json([
    "JSON Test Pattern pass1",
    {"object with 1 member":["array with 1 element"]},
    {},
    [],
    -42,
    true,
    false,
    null,
    {
        "integer": 1234567890,
        "real": -9876.543210,
        "e": 0.123456789e-12,
        "E": 1.234567890E+34,
        "":  23456789012E66,
        "zero": 0,
        "one": 1,
        "space": " ",
        "quote": "\"",
        "backslash": "\\",
        "controls": "\b\f\n\r\t",
        "slash": "/ & \/",
        "alpha": "abcdefghijklmnopqrstuvwyz",
        "ALPHA": "ABCDEFGHIJKLMNOPQRSTUVWYZ",
        "digit": "0123456789",
        "0123456789": "digit",
        "special": "`1~!@#$%^&*()_+-={':[,]}|;.</>?",
        "hex": "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
        "true": true,
        "false": false,
        "null": null,
        "array":[  ],
        "object":{  },
        "address": "50 St. James Street",
        "url": "http://www.JSON.org/",
        "comment": "// /* <!-- --",
        "# -- --> */": " ",
        " s p a c e d " :[1,2 , 3

,

4 , 5        ,          6           ,7        ],"compact":[1,2,3,4,5,6,7],
        "jsontext": "{\"object with 1 member\":[\"array with 1 element\"]}",
        "quotes": "&#34; \u0022 %22 0x22 034 &#x22;",
        "\/\\\"\uCAFE\uBABE\uAB98\uFCDE\ubcda\uef4A\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?"
: "A key can be any string"
    },
    0.5 ,98.6
,
99.44
,

1066,
1e1,
0.1e1,
1e-1,
1e00,2e+00,2e-00
,"rosebud"])json"));
    CHECK(doc.parse(u8R"json([[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]])json"));
    CHECK(doc.parse(u8R"json({
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
})json"));
}

TEST_SUITE_END();

TEST_SUITE("nativejson-benchmark");

bool parse_double(const char *json, double expect) {
    gason2::document doc;
    if (doc.parse(json) && doc.is_array() && doc.size() == 1 && doc[(size_t)0].is_number()) {
        double actual = doc[(size_t)0].to_number();
        if (actual == expect)
            return true;
        unsigned long expect_ul;
        memcpy(&expect_ul, &expect, sizeof(expect_ul));
        unsigned long actual_ul;
        memcpy(&actual_ul, &actual, sizeof(actual_ul));
        fprintf(stderr, "\n    expect: %24.17g (0x%016lx)\n    actual: %24.17g (0x%016lx)\n\n", expect, expect_ul, actual, actual_ul);
    }
    return false;
}

TEST_CASE("doubles") {
    CHECK(parse_double("[0.0]", 0.0));
    CHECK(parse_double("[-0.0]", -0.0));
    CHECK(parse_double("[1.0]", 1.0));
    CHECK(parse_double("[-1.0]", -1.0));
    CHECK(parse_double("[1.5]", 1.5));
    CHECK(parse_double("[-1.5]", -1.5));
    CHECK(parse_double("[3.1416]", 3.1416));
    CHECK(parse_double("[1E10]", 1E10));
    CHECK(parse_double("[1e10]", 1e10));
    CHECK(parse_double("[1E+10]", 1E+10));
    CHECK(parse_double("[1E-10]", 1E-10));
    CHECK(parse_double("[-1E10]", -1E10));
    CHECK(parse_double("[-1e10]", -1e10));
    CHECK(parse_double("[-1E+10]", -1E+10));
    CHECK(parse_double("[-1E-10]", -1E-10));
    CHECK(parse_double("[1.234E+10]", 1.234E+10));
    CHECK(parse_double("[1.234E-10]", 1.234E-10));
    CHECK(parse_double("[1.79769e+308]", 1.79769e+308));
    CHECK(parse_double("[2.22507e-308]", 2.22507e-308));
    CHECK(parse_double("[-1.79769e+308]", -1.79769e+308));
    CHECK(parse_double("[-2.22507e-308]", -2.22507e-308));
    CHECK(parse_double("[4.9406564584124654e-324]", 4.9406564584124654e-324)); // minimum denormal
    CHECK(parse_double("[2.2250738585072009e-308]", 2.2250738585072009e-308)); // Max subnormal double
    CHECK(parse_double("[2.2250738585072014e-308]", 2.2250738585072014e-308)); // Min normal positive double
    CHECK(parse_double("[1.7976931348623157e+308]", 1.7976931348623157e+308)); // Max double
    CHECK(parse_double("[1e-10000]", 0.0));                                    // must underflow
    CHECK(parse_double("[18446744073709551616]", 18446744073709551616.0));     // 2^64 (max of uint64_t + 1, force to use double))
    CHECK(parse_double("[-9223372036854775809]", -9223372036854775809.0));     // -2^63 - 1(min of int64_t + 1, force to use double))
    CHECK(parse_double("[0.9868011474609375]", 0.9868011474609375));           // https://github.com/miloyip/rapidjson/issues/120
    CHECK(parse_double("[123e34]", 123e34));                                   // Fast Path Cases In Disguise
    CHECK(parse_double("[45913141877270640000.0]", 45913141877270640000.0));
    CHECK(parse_double("[2.2250738585072011e-308]", 2.2250738585072011e-308)); // http://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
    //CHECK(parse_double("[1e-00011111111111]", 0.0));
    //CHECK(parse_double("[-1e-00011111111111]", -0.0));
    CHECK(parse_double("[1e-214748363]", 0.0));
    CHECK(parse_double("[1e-214748364]", 0.0));
    //CHECK(parse_double("[1e-21474836311]", 0.0));
    CHECK(parse_double("[0.017976931348623157e+310]", 1.7976931348623157e+308)); // Max double in another form

    // Since
    // abs((2^-1022 - 2^-1074)) - 2.2250738585072012e-308)) = 3.109754131239141401123495768877590405345064751974375599... ¡Á 10^-324
    // abs((2^-1022)) - 2.2250738585072012e-308)) = 1.830902327173324040642192159804623318305533274168872044... ¡Á 10 ^ -324
    // So 2.2250738585072012e-308 should round to 2^-1022 = 2.2250738585072014e-308
    CHECK(parse_double("[2.2250738585072012e-308]", 2.2250738585072014e-308)); // http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/

    // More closer to normal/subnormal boundary
    // boundary = 2^-1022 - 2^-1075 = 2.225073858507201136057409796709131975934819546351645648... ¡Á 10^-308
    CHECK(parse_double("[2.22507385850720113605740979670913197593481954635164564e-308]", 2.2250738585072009e-308));
    CHECK(parse_double("[2.22507385850720113605740979670913197593481954635164565e-308]", 2.2250738585072014e-308));

    // 1.0 is in (1.0 - 2^-54, 1.0 + 2^-53))
    // 1.0 - 2^-54 = 0.999999999999999944488848768742172978818416595458984375
    CHECK(parse_double("[0.999999999999999944488848768742172978818416595458984375]", 1.0));                 // round to even
    CHECK(parse_double("[0.999999999999999944488848768742172978818416595458984374]", 0.99999999999999989)); // previous double
    CHECK(parse_double("[0.999999999999999944488848768742172978818416595458984376]", 1.0));                 // next double
    // 1.0 + 2^-53 = 1.00000000000000011102230246251565404236316680908203125
    CHECK(parse_double("[1.00000000000000011102230246251565404236316680908203125]", 1.0));                 // round to even
    CHECK(parse_double("[1.00000000000000011102230246251565404236316680908203124]", 1.0));                 // previous double
    CHECK(parse_double("[1.00000000000000011102230246251565404236316680908203126]", 1.00000000000000022)); // next double

    // Numbers from https://github.com/floitsch/double-conversion/blob/master/test/cctest/test-strtod.cc

    CHECK(parse_double("[72057594037927928.0]", 72057594037927928.0));
    CHECK(parse_double("[72057594037927936.0]", 72057594037927936.0));
    CHECK(parse_double("[72057594037927932.0]", 72057594037927936.0));
    CHECK(parse_double("[7205759403792793199999e-5]", 72057594037927928.0));
    CHECK(parse_double("[7205759403792793200001e-5]", 72057594037927936.0));

    CHECK(parse_double("[9223372036854774784.0]", 9223372036854774784.0));
    CHECK(parse_double("[9223372036854775808.0]", 9223372036854775808.0));
    CHECK(parse_double("[9223372036854775296.0]", 9223372036854775808.0));
    CHECK(parse_double("[922337203685477529599999e-5]", 9223372036854774784.0));
    CHECK(parse_double("[922337203685477529600001e-5]", 9223372036854775808.0));

    CHECK(parse_double("[10141204801825834086073718800384]", 10141204801825834086073718800384.0));
    CHECK(parse_double("[10141204801825835211973625643008]", 10141204801825835211973625643008.0));
    CHECK(parse_double("[10141204801825834649023672221696]", 10141204801825835211973625643008.0));
    CHECK(parse_double("[1014120480182583464902367222169599999e-5]", 10141204801825834086073718800384.0));
    CHECK(parse_double("[1014120480182583464902367222169600001e-5]", 10141204801825835211973625643008.0));

    CHECK(parse_double("[5708990770823838890407843763683279797179383808]", 5708990770823838890407843763683279797179383808.0));
    CHECK(parse_double("[5708990770823839524233143877797980545530986496]", 5708990770823839524233143877797980545530986496.0));
    CHECK(parse_double("[5708990770823839207320493820740630171355185152]", 5708990770823839524233143877797980545530986496.0));
    CHECK(parse_double("[5708990770823839207320493820740630171355185151999e-3]", 5708990770823838890407843763683279797179383808.0));
    CHECK(parse_double("[5708990770823839207320493820740630171355185152001e-3]", 5708990770823839524233143877797980545530986496.0));

    {
        char n1e308[312]; // '1' followed by 308 '0'
        n1e308[0] = '[';
        n1e308[1] = '1';
        for (int j = 2; j < 310; j++)
            n1e308[j] = '0';
        n1e308[310] = ']';
        n1e308[311] = '\0';
        CHECK(parse_double(n1e308, 1E308));
    }

    // Cover trimming
    CHECK(parse_double(
        "[2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
        "7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
        "9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
        "6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
        "1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
        "5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
        "2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
        "7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
        "e-308]",
        2.2250738585072014e-308));
}

template <size_t N>
bool parse_string(const char *json, const char (&expect)[N]) {
    gason2::document doc;
    if (doc.parse(json) && doc.is_array() && doc.size() == 1 && doc[(size_t)0].is_string()) {
        const char *actual = doc[(size_t)0].to_string();
        if ((strlen(actual) == N - 1) && (memcmp(actual, expect, N - 1) == 0))
            return true;
        gason2::vector<char> buffer;
        gason2::dump::stringify(buffer, doc[(size_t)0]);
        buffer.push_back('\0');
        buffer.pop_back();
        fprintf(stderr, "\n    expect: %s (%zd)\n    actual: %s (%zd)\n\n", expect, N - 1, buffer.begin(), strlen(actual));
    }
    return false;
}

TEST_CASE("strings") {
    CHECK(parse_string("[\"\"]", ""));
    CHECK(parse_string("[\"Hello\"]", "Hello"));
    CHECK(parse_string("[\"Hello\\nWorld\"]", "Hello\nWorld"));
    CHECK(parse_string("[\"Hello\\u0000World\"]", "Hello\0World"));
    CHECK(parse_string("[\"\\\"\\\\/\\b\\f\\n\\r\\t\"]", "\"\\/\b\f\n\r\t"));
    CHECK(parse_string("[\"\\u0024\"]", "\x24"));                    // Dollar sign U+0024
    CHECK(parse_string("[\"\\u00A2\"]", "\xC2\xA2"));                // Cents sign U+00A2
    CHECK(parse_string("[\"\\u20AC\"]", "\xE2\x82\xAC"));            // Euro sign U+20AC
    CHECK(parse_string("[\"\\uD834\\uDD1E\"]", "\xF0\x9D\x84\x9E")); // G clef sign U+1D11E
}

TEST_SUITE_END();
