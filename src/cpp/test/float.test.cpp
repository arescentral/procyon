// Copyright 2017 The Procyon Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>
#include <procyon.h>
#include <cmath>
#include <limits>
#include <pn/string>

#include "../src/common.h"

using StrToDTest = ::testing::Test;
using FloatTest  = ::testing::Test;
using ::testing::Eq;
using ::testing::Ne;

namespace pntest {

TEST_F(FloatTest, ParseZero) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("0.0", &d, &err), Eq(true)) << err;
    EXPECT_THAT(err, Eq(PN_OK));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_ZERO)) << d;
    EXPECT_THAT(std::signbit(d), Eq(0)) << d;
}

TEST_F(FloatTest, ParseMin) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("2.2250738585072014e-308", &d, &err), Eq(true)) << err;
    EXPECT_THAT(err, Eq(PN_OK));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_NORMAL)) << d;
    EXPECT_THAT(d, Eq(2.2250738585072014e-308));
}

TEST_F(FloatTest, ParseDenorm) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("1e-320", &d, &err), Eq(false)) << d;
    EXPECT_THAT(err, Eq(PN_ERROR_FLOAT_OVERFLOW));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_SUBNORMAL));
    EXPECT_THAT(d, Eq(1e-320));
}

TEST_F(FloatTest, ParseDenormMin) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("5e-324", &d, &err), Eq(false)) << d;
    EXPECT_THAT(err, Eq(PN_ERROR_FLOAT_OVERFLOW));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_SUBNORMAL));
    EXPECT_THAT(d, Eq(5e-324));
}

TEST_F(FloatTest, ParseTooSmall) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("5e-999", &d, &err), Eq(false)) << d;
    EXPECT_THAT(std::fpclassify(d), Eq(FP_ZERO));
    EXPECT_THAT(std::signbit(d), Eq(0));
}

TEST_F(FloatTest, ParseGoogol) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("1e100", &d, &err), Eq(true)) << err;
    EXPECT_THAT(err, Eq(PN_OK));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_NORMAL)) << d;
    EXPECT_THAT(d, Eq(1e100)) << d;
}

TEST_F(FloatTest, ParseMax) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("1.7976931348623157e308", &d, &err), Eq(true)) << err;
    EXPECT_THAT(err, Eq(PN_OK));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_NORMAL));
    EXPECT_THAT(d, Eq(1.7976931348623157e308));
}

TEST_F(FloatTest, ParseTooLarge) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    EXPECT_THAT(pn::strtod("1e999", &d, &err), Eq(false)) << d;
    EXPECT_THAT(err, Eq(PN_ERROR_FLOAT_OVERFLOW));
    EXPECT_THAT(std::fpclassify(d), Eq(FP_INFINITE));
}

TEST_F(FloatTest, ParseExcessiveExponent) {
    double          d   = 0.0;
    pn_error_code_t err = PN_OK;
    const char*     s =
            "1"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "e-512";
    EXPECT_THAT(pn::strtod(s, &d, &err), Eq(true)) << err;
    EXPECT_THAT(err, Eq(PN_OK));
    EXPECT_THAT(d, Eq(1.0));
}

TEST_F(FloatTest, ParseOne) {
    const char* tests[] = {
            "1",
            "001",
            "000000001",
            "0000000000000001",
            "00000000000000000000000000000000000000000000000000000000000000000000000000000001",
            "1.",
            "1.0",
            "1.00",
            "1.00000000",
            "1.000000000000000",
            "1.0000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "1e0",
            "1e0000",
            "1e0000000000000000000000000000000000",
            "10e-1",
            "100000000000000000000000000000000e-32",
            "10000000000000000000000000000000000000000000000000000000000000000e-64",
            "1.0e0",
            "0.1e1",
            "0.01e2",
            "0.00000000000000000000000000000001e32"};
    for (const auto& test : tests) {
        double          d   = 0.0;
        pn_error_code_t err = PN_OK;
        EXPECT_THAT(pn::strtod(test, &d, &err), Eq(true)) << test << ": " << err;
        EXPECT_THAT(err, Eq(PN_OK));
        EXPECT_THAT(std::fpclassify(d), Eq(FP_NORMAL)) << test << " -> " << d;
        EXPECT_THAT(d, Eq(1.0)) << test << " -> " << d;
    }
}

TEST_F(FloatTest, ParseSpecial) {
    struct {
        char        sign;
        int         fp_type;
        const char* s;
    } tests[] = {{'+', FP_ZERO, "0"},
                 {'+', FP_ZERO, "0.0"},
                 {'+', FP_ZERO, "0e0"},
                 {'+', FP_ZERO, "0.0e0"},
                 {'+', FP_ZERO, "0.00000000000000000000000"},
                 {'-', FP_ZERO, "-0"},
                 {'-', FP_ZERO, "-0.0"},
                 {'-', FP_ZERO, "-0e0"},
                 {'-', FP_ZERO, "-0.0e0"},
                 {'-', FP_ZERO, "-0.00000000000000000000000"},
                 {'+', FP_INFINITE, "inf"},
                 {'+', FP_INFINITE, "+inf"},
                 {'-', FP_INFINITE, "-inf"},
                 {'+', FP_NAN, "nan"}};
    for (const auto& test : tests) {
        double          d   = 0.0;
        pn_error_code_t err = PN_OK;
        EXPECT_THAT(pn::strtod(test.s, &d, &err), Eq(true)) << test.s << ": " << err;
        EXPECT_THAT(err, Eq(PN_OK));
        EXPECT_THAT(std::fpclassify(d), Eq(test.fp_type)) << test.s << " -> " << d;
        EXPECT_THAT(std::signbit(d) == 0 ? '+' : '-', Eq(test.sign)) << test.s << " -> " << d;
    }
}

TEST_F(FloatTest, ParseBadValues) {
    for (const char* s :
         {"",  "-",     "+",     ".",    " ",    " 0",    "0 ", " 0 ", "x",     "e1", "one",
          "∞", "++inf", "--inf", "+nan", "-nan", "0e0e0", "$1", "1f",  "0.00?", "1e∞"}) {
        double          d   = 0.0;
        pn_error_code_t err = PN_OK;
        EXPECT_THAT(pn::strtod(s, &d, &err), Eq(false)) << s << ": " << d;
        EXPECT_THAT(err, Eq(PN_ERROR_INVALID_FLOAT));
    }
}

pn::string dtoa(double d) {
    char buffer[32];
    return pn::string{pn_dtoa(buffer, d)};
}

TEST_F(FloatTest, PrintZero) {
    EXPECT_THAT(dtoa(+0.0), Eq("0.0"));
    EXPECT_THAT(dtoa(-0.0), Eq("-0.0"));
}

TEST_F(FloatTest, PrintOne) {
    EXPECT_THAT(dtoa(+1.0), Eq("1.0"));
    EXPECT_THAT(dtoa(-1.0), Eq("-1.0"));
}

TEST_F(FloatTest, PrintHalf) {
    EXPECT_THAT(dtoa(+0.5), Eq("0.5"));
    EXPECT_THAT(dtoa(-0.5), Eq("-0.5"));
}

TEST_F(FloatTest, PrintPi) {
    EXPECT_THAT(dtoa(+M_PI), Eq("3.141592653589793"));
    EXPECT_THAT(dtoa(-M_PI), Eq("-3.141592653589793"));
}

TEST_F(FloatTest, PrintSpecial) {
    EXPECT_THAT(dtoa(std::numeric_limits<double>::infinity()), Eq("inf"));
    EXPECT_THAT(dtoa(-std::numeric_limits<double>::infinity()), Eq("-inf"));
    EXPECT_THAT(dtoa(-std::numeric_limits<double>::quiet_NaN()), Eq("nan"));
}

TEST_F(FloatTest, PrintPowersOfTen) {
    EXPECT_THAT(dtoa(1e-308), Eq("1e-308"));
    EXPECT_THAT(dtoa(1e-300), Eq("1e-300"));
    EXPECT_THAT(dtoa(1e-200), Eq("1e-200"));
    EXPECT_THAT(dtoa(1e-100), Eq("1e-100"));
    EXPECT_THAT(dtoa(1e-50), Eq("1e-50"));
    EXPECT_THAT(dtoa(1e-25), Eq("1e-25"));
    EXPECT_THAT(dtoa(1e-24), Eq("1e-24"));
    EXPECT_THAT(dtoa(1e-23), Eq("1e-23"));
    EXPECT_THAT(dtoa(1e-22), Eq("1e-22"));
    EXPECT_THAT(dtoa(1e-21), Eq("1e-21"));

    EXPECT_THAT(dtoa(1e-20), Eq("1e-20"));
    EXPECT_THAT(dtoa(1e-19), Eq("1e-19"));
    EXPECT_THAT(dtoa(1e-18), Eq("1e-18"));
    EXPECT_THAT(dtoa(1e-17), Eq("1e-17"));
    EXPECT_THAT(dtoa(1e-16), Eq("1e-16"));
    EXPECT_THAT(dtoa(1e-15), Eq("1e-15"));
    EXPECT_THAT(dtoa(1e-14), Eq("1e-14"));
    EXPECT_THAT(dtoa(1e-13), Eq("1e-13"));
    EXPECT_THAT(dtoa(1e-12), Eq("1e-12"));
    EXPECT_THAT(dtoa(1e-11), Eq("1e-11"));

    EXPECT_THAT(dtoa(1e-10), Eq("1e-10"));
    EXPECT_THAT(dtoa(1e-9), Eq("1e-09"));
    EXPECT_THAT(dtoa(1e-8), Eq("1e-08"));
    EXPECT_THAT(dtoa(1e-7), Eq("1e-07"));
    EXPECT_THAT(dtoa(1e-6), Eq("1e-06"));
    EXPECT_THAT(dtoa(1e-5), Eq("1e-05"));
    EXPECT_THAT(dtoa(1e-4), Eq("0.0001"));
    EXPECT_THAT(dtoa(1e-3), Eq("0.001"));
    EXPECT_THAT(dtoa(1e-2), Eq("0.01"));
    EXPECT_THAT(dtoa(1e-1), Eq("0.1"));

    EXPECT_THAT(dtoa(1e0), Eq("1.0"));

    EXPECT_THAT(dtoa(1e+1), Eq("10.0"));
    EXPECT_THAT(dtoa(1e+2), Eq("100.0"));
    EXPECT_THAT(dtoa(1e+3), Eq("1000.0"));
    EXPECT_THAT(dtoa(1e+4), Eq("10000.0"));
    EXPECT_THAT(dtoa(1e+5), Eq("100000.0"));
    EXPECT_THAT(dtoa(1e+6), Eq("1000000.0"));
    EXPECT_THAT(dtoa(1e+7), Eq("10000000.0"));
    EXPECT_THAT(dtoa(1e+8), Eq("100000000.0"));
    EXPECT_THAT(dtoa(1e+9), Eq("1000000000.0"));
    EXPECT_THAT(dtoa(1e+10), Eq("10000000000.0"));

    EXPECT_THAT(dtoa(1e+11), Eq("100000000000.0"));
    EXPECT_THAT(dtoa(1e+12), Eq("1000000000000.0"));
    EXPECT_THAT(dtoa(1e+13), Eq("10000000000000.0"));
    EXPECT_THAT(dtoa(1e+14), Eq("100000000000000.0"));
    EXPECT_THAT(dtoa(1e+15), Eq("1000000000000000.0"));
    EXPECT_THAT(dtoa(1e+16), Eq("1e+16"));
    EXPECT_THAT(dtoa(1e+17), Eq("1e+17"));
    EXPECT_THAT(dtoa(1e+18), Eq("1e+18"));
    EXPECT_THAT(dtoa(1e+19), Eq("1e+19"));
    EXPECT_THAT(dtoa(1e+20), Eq("1e+20"));

    EXPECT_THAT(dtoa(1e+21), Eq("1e+21"));
    EXPECT_THAT(dtoa(1e+22), Eq("1e+22"));
    EXPECT_THAT(dtoa(1e+23), Eq("1e+23"));
    EXPECT_THAT(dtoa(1e+24), Eq("1e+24"));
    EXPECT_THAT(dtoa(1e+25), Eq("1e+25"));
    EXPECT_THAT(dtoa(1e+50), Eq("1e+50"));
    EXPECT_THAT(dtoa(1e+100), Eq("1e+100"));
    EXPECT_THAT(dtoa(1e+200), Eq("1e+200"));
    EXPECT_THAT(dtoa(1e+300), Eq("1e+300"));
    EXPECT_THAT(dtoa(1e+308), Eq("1e+308"));
}

TEST_F(FloatTest, PrintNines) {
    EXPECT_THAT(dtoa(9.00000000000000000000), Eq("9.0"));
    EXPECT_THAT(dtoa(99.0000000000000000000), Eq("99.0"));
    EXPECT_THAT(dtoa(999.000000000000000000), Eq("999.0"));
    EXPECT_THAT(dtoa(9999.00000000000000000), Eq("9999.0"));
    EXPECT_THAT(dtoa(99999.0000000000000000), Eq("99999.0"));
    EXPECT_THAT(dtoa(999999.000000000000000), Eq("999999.0"));
    EXPECT_THAT(dtoa(9999999.00000000000000), Eq("9999999.0"));
    EXPECT_THAT(dtoa(99999999.0000000000000), Eq("99999999.0"));
    EXPECT_THAT(dtoa(999999999.000000000000), Eq("999999999.0"));
    EXPECT_THAT(dtoa(9999999999.00000000000), Eq("9999999999.0"));
    EXPECT_THAT(dtoa(99999999999.0000000000), Eq("99999999999.0"));
    EXPECT_THAT(dtoa(999999999999.000000000), Eq("999999999999.0"));
    EXPECT_THAT(dtoa(9999999999999.00000000), Eq("9999999999999.0"));
    EXPECT_THAT(dtoa(99999999999999.0000000), Eq("99999999999999.0"));
    EXPECT_THAT(dtoa(999999999999999.000000), Eq("999999999999999.0"));
    EXPECT_THAT(dtoa(9999999999999999.00000), Eq("1e+16"));
    EXPECT_THAT(dtoa(99999999999999999.0000), Eq("1e+17"));
    EXPECT_THAT(dtoa(999999999999999999.000), Eq("1e+18"));
    EXPECT_THAT(dtoa(9999999999999999999.00), Eq("1e+19"));
    EXPECT_THAT(dtoa(99999999999999999999.0), Eq("1e+20"));

    EXPECT_THAT(dtoa(9.999999999999999), Eq("9.999999999999998"));  // 8!
    EXPECT_THAT(dtoa(99.99999999999999), Eq("99.99999999999999"));
    EXPECT_THAT(dtoa(999.9999999999999), Eq("999.9999999999999"));
    EXPECT_THAT(dtoa(9999.999999999999), Eq("9999.999999999998"));  // 8!
    EXPECT_THAT(dtoa(99999.99999999999), Eq("99999.99999999999"));
    EXPECT_THAT(dtoa(999999.9999999999), Eq("999999.9999999999"));
    EXPECT_THAT(dtoa(9999999.999999999), Eq("9999999.999999998"));  // 8!
    EXPECT_THAT(dtoa(99999999.99999999), Eq("99999999.99999999"));
    EXPECT_THAT(dtoa(999999999.9999999), Eq("999999999.9999999"));
    EXPECT_THAT(dtoa(9999999999.999999), Eq("9999999999.999998"));  // 8!
    EXPECT_THAT(dtoa(99999999999.99999), Eq("99999999999.99998"));  // 8!
    EXPECT_THAT(dtoa(999999999999.9999), Eq("999999999999.9999"));
    EXPECT_THAT(dtoa(9999999999999.999), Eq("9999999999999.998"));  // 8!
    EXPECT_THAT(dtoa(99999999999999.99), Eq("99999999999999.98"));  // 8!
    EXPECT_THAT(dtoa(999999999999999.9), Eq("999999999999999.9"));

    EXPECT_THAT(dtoa(00000000000000000000.9), Eq("0.9"));
    EXPECT_THAT(dtoa(0000000000000000000.99), Eq("0.99"));
    EXPECT_THAT(dtoa(000000000000000000.999), Eq("0.999"));
    EXPECT_THAT(dtoa(00000000000000000.9999), Eq("0.9999"));
    EXPECT_THAT(dtoa(0000000000000000.99999), Eq("0.99999"));
    EXPECT_THAT(dtoa(000000000000000.999999), Eq("0.999999"));
    EXPECT_THAT(dtoa(00000000000000.9999999), Eq("0.9999999"));
    EXPECT_THAT(dtoa(0000000000000.99999999), Eq("0.99999999"));
    EXPECT_THAT(dtoa(000000000000.999999999), Eq("0.999999999"));
    EXPECT_THAT(dtoa(00000000000.9999999999), Eq("0.9999999999"));
    EXPECT_THAT(dtoa(0000000000.99999999999), Eq("0.99999999999"));
    EXPECT_THAT(dtoa(000000000.999999999999), Eq("0.999999999999"));
    EXPECT_THAT(dtoa(00000000.9999999999999), Eq("0.9999999999999"));
    EXPECT_THAT(dtoa(0000000.99999999999999), Eq("0.99999999999999"));
    EXPECT_THAT(dtoa(000000.999999999999999), Eq("0.999999999999999"));
    EXPECT_THAT(dtoa(00000.9999999999999999), Eq("0.9999999999999999"));
    EXPECT_THAT(dtoa(0000.99999999999999999), Eq("1.0"));
    EXPECT_THAT(dtoa(000.999999999999999999), Eq("1.0"));
    EXPECT_THAT(dtoa(00.9999999999999999999), Eq("1.0"));
    EXPECT_THAT(dtoa(0.99999999999999999999), Eq("1.0"));
}

TEST_F(FloatTest, PrintImprecise) {
    // 9007199254740991 is the largest whole number that can be
    // represented exactly in the 53 bits of the significand. After
    // that, numbers should round towards multiples of 4.
    //
    // TODO(sfiera): switch to scientific notation when ULP > 1?
    // 9.007199254740992e+15 conveys to me imprecision in a way that
    // 9007199254740992 does not. The switch to scientific notation
    // otherwise will happen at 1e+16.
    EXPECT_THAT(dtoa(9007199254740990.0), Eq("9007199254740990.0"));
    EXPECT_THAT(dtoa(9007199254740991.0), Eq("9007199254740991.0"));
    EXPECT_THAT(dtoa(9007199254740992.0), Eq("9007199254740992.0"));
    EXPECT_THAT(dtoa(9007199254740993.0), Eq("9007199254740992.0"));
    EXPECT_THAT(dtoa(9007199254740994.0), Eq("9007199254740994.0"));
    EXPECT_THAT(dtoa(9007199254740995.0), Eq("9007199254740996.0"));
    EXPECT_THAT(dtoa(9007199254740996.0), Eq("9007199254740996.0"));
    EXPECT_THAT(dtoa(9007199254740997.0), Eq("9007199254740996.0"));
    EXPECT_THAT(dtoa(9007199254740998.0), Eq("9007199254740998.0"));
    EXPECT_THAT(dtoa(9007199254740999.0), Eq("9007199254741000.0"));
}

TEST_F(FloatTest, PrintExtrema) {
    EXPECT_THAT(dtoa(-1.7976931348623157e308), Eq("-1.7976931348623157e+308"));
    EXPECT_THAT(dtoa(-2.2250738585072014e-308), Eq("-2.2250738585072014e-308"));
    EXPECT_THAT(dtoa(-5e-324), Eq("-5e-324"));
    EXPECT_THAT(dtoa(5e-324), Eq("5e-324"));
    EXPECT_THAT(dtoa(2.2250738585072014e-308), Eq("2.2250738585072014e-308"));
    EXPECT_THAT(dtoa(1.7976931348623157e308), Eq("1.7976931348623157e+308"));
}

}  // namespace pntest
