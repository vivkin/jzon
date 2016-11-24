#include "doctest.h"
#include "gason2.h"
#include <float.h>
#include <math.h>

using namespace gason2;

#define TEST_NOTNAN(x)              \
    CHECK_FALSE(var_t{x}.is_nan()); \
    CHECK_FALSE(var_t{-x}.is_nan())

TEST_CASE("[gason] boxing double") {
    TEST_NOTNAN(0.0);
    TEST_NOTNAN(1.0);
    TEST_NOTNAN(1.0 / 3.0);
    TEST_NOTNAN(5.45);
    TEST_NOTNAN(7.62);
    TEST_NOTNAN(1e40);
    TEST_NOTNAN(M_E);
    TEST_NOTNAN(M_LOG2E);
    TEST_NOTNAN(M_LOG10E);
    TEST_NOTNAN(M_LN2);
    TEST_NOTNAN(M_LN10);
    TEST_NOTNAN(M_PI);
    TEST_NOTNAN(M_PI_2);
    TEST_NOTNAN(M_PI_4);
    TEST_NOTNAN(M_1_PI);
    TEST_NOTNAN(M_2_PI);
    TEST_NOTNAN(M_2_SQRTPI);
    TEST_NOTNAN(M_SQRT2);
    TEST_NOTNAN(M_SQRT1_2);
    TEST_NOTNAN(DBL_MIN);
    TEST_NOTNAN(DBL_EPSILON);
    TEST_NOTNAN(DBL_MAX);
    TEST_NOTNAN(INFINITY);
    TEST_NOTNAN(NAN);
}

#define TEST_TYPE(x)                  \
    CHECK(var_t{x}.is_nan());         \
    CHECK_FALSE(var_t{x}.is_error()); \
    CHECK(var_t{x}.tag.type == x)

TEST_CASE("[gason] boxing types") {
    TEST_TYPE(type::null);
    TEST_TYPE(type::boolean);
    TEST_TYPE(type::string);
    TEST_TYPE(type::array);
    TEST_TYPE(type::object);
}

#define TEST_ERROR(x)           \
    CHECK(var_t{x}.is_nan());   \
    CHECK(var_t{x}.is_error()); \
    CHECK(var_t{x}.tag.error == x)

TEST_CASE("[gason] boxing errors") {
    TEST_ERROR(error::expecting_string);
    TEST_ERROR(error::expecting_value);
    TEST_ERROR(error::invalid_literal_name);
    TEST_ERROR(error::invalid_number);
    TEST_ERROR(error::invalid_string_char);
    TEST_ERROR(error::invalid_string_escape);
    TEST_ERROR(error::invalid_surrogate_pair);
    TEST_ERROR(error::missing_colon);
    TEST_ERROR(error::missing_comma_or_bracket);
    TEST_ERROR(error::unexpected_character);
}
