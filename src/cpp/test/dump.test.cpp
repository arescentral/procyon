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

#include <procyon.h>

#include <gmock/gmock.h>

#include "./matchers.hpp"

using DumpTest = ::testing::Test;
using ::testing::Eq;

namespace pntest {

pn::value dump(const pn_value_t* x) {
    pn::value out;
    pn_set(out.c_obj(), 's', "");
    pn_file_t* f = pn_open_string(&out.c_obj()->s, "w");
    EXPECT_THAT(pn_dump(f, PN_DUMP_DEFAULT, 'x', x), Eq(true));
    fclose(f);
    return out;
}

template <typename... Args>
pn::value dump(char format, const Args&... args) {
    pn::value out;
    pn_set(out.c_obj(), 's', "");
    pn_file_t* f = pn_open_string(&out.c_obj()->s, "w");
    EXPECT_THAT(pn_dump(f, PN_DUMP_DEFAULT, format, args...), Eq(true));
    fclose(f);
    return out;
}

template <typename... Args>
pn::value dumpv(const char* format, const Args&... args) {
    pn::value x;
    pn_setv(x.c_obj(), format, args...);
    return dump(x.c_obj());
}

template <typename... Args>
pn::value dumpkv(const char* format, const Args&... args) {
    pn::value x;
    pn_setkv(x.c_obj(), format, args...);
    return dump(x.c_obj());
}

TEST_F(DumpTest, Named) {
    EXPECT_THAT(dump(&pn_null), IsString("null\n"));
    EXPECT_THAT(dump(&pn_inf), IsString("inf\n"));
    EXPECT_THAT(dump(&pn_true), IsString("true\n"));
    EXPECT_THAT(dump(&pn_false), IsString("false\n"));
    EXPECT_THAT(dump(&pn_neg_inf), IsString("-inf\n"));
    EXPECT_THAT(dump(&pn_nan), IsString("nan\n"));
    EXPECT_THAT(dump(&pn_zero), IsString("0\n"));
    EXPECT_THAT(dump(&pn_zerof), IsString("0.0\n"));
    EXPECT_THAT(dump(&pn_dataempty), IsString("$\n"));
    EXPECT_THAT(dump(&pn_strempty), IsString("\"\"\n"));
    EXPECT_THAT(dump(&pn_arrayempty), IsString("[]\n"));
    EXPECT_THAT(dump(&pn_mapempty), IsString("{}\n"));
}

TEST_F(DumpTest, Scalar) {
    EXPECT_THAT(dump('n'), IsString("null\n"));
    EXPECT_THAT(dump('?', true), IsString("true\n"));
    EXPECT_THAT(dump('?', false), IsString("false\n"));
    EXPECT_THAT(dump('i', 1), IsString("1\n"));
    EXPECT_THAT(dump('i', -1), IsString("-1\n"));
    EXPECT_THAT(dump('q', INT64_MAX), IsString("9223372036854775807\n"));
    EXPECT_THAT(dump('q', INT64_MIN), IsString("-9223372036854775808\n"));
    EXPECT_THAT(dump('d', 5.0), IsString("5.0\n"));
    EXPECT_THAT(dump('d', 0.5), IsString("0.5\n"));
}

TEST_F(DumpTest, FloatRounding) {
    struct {
        double      d;
        const char* repr;
    } tests[] = {
            {-0.0000000000000020e-308, "-2e-323\n"},    // 4th-least negative denormal
            {-0.0000000000000015e-308, "-1.5e-323\n"},  // 3rd-least negative denormal
            {-0.0000000000000010e-308, "-1e-323\n"},    // 2nd-least negative denormal
            {-0.0000000000000005e-308, "-5e-324\n"},    // least negative denormal
            {+0.0000000000000000e+000, "0.0\n"},        // zero
            {+0.0000000000000005e-308, "5e-324\n"},     // least positive denormal
            {+0.0000000000000010e-308, "1e-323\n"},     // 2nd-least positive denormal
            {+0.0000000000000015e-308, "1.5e-323\n"},   // 3rd-least positive denormal
            {+0.0000000000000020e-308, "2e-323\n"},     // 4th-least positive denormal

            {+2.2250738585072004e-308, "2.2250738585072004e-308\n"},  // 2nd-most positive denormal
            {+2.2250738585072009e-308, "2.225073858507201e-308\n"},   // most positive denormal
            {+2.2250738585072014e-308, "2.2250738585072014e-308\n"},  // least positive normal
            {+2.2250738585072019e-308, "2.225073858507202e-308\n"},   // 2nd-least positive normal

            {+1.9999999999999998e-001, "0.19999999999999998\n"},  // previous value before 0.2
            {+2.0000000000000000e-001, "0.2\n"},                  // 0.2
            {+2.0000000000000001e-001, "0.2\n"},                  // More accurate repr of 0.2
            {+2.0000000000000004e-001, "0.20000000000000004\n"},  // next value after 0.2

            {-5.0000000000000000e-001, "-0.5\n"},
            {+5.0000000000000000e-001, "0.5\n"},

            {+9.9999999999999964e+000, "9.999999999999996\n"},   // two values before 10
            {+9.9999999999999982e+000, "9.999999999999998\n"},   // previous value before 10
            {+1.0000000000000000e+001, "10.0\n"},                // 10
            {+1.0000000000000002e+001, "10.000000000000002\n"},  // next value after 10

            {+1.0000000000000000e-025, "1e-25\n"},               // 10 ^ -25
            {+1.0000000000000000e-024, "1e-24\n"},               // 10 ^ -24
            {+1.0000000000000000e-023, "1e-23\n"},               // 10 ^ -23
            {+1.0000000000000000e-022, "1e-22\n"},               // 10 ^ -22
            {+1.0000000000000000e-021, "1e-21\n"},               // 10 ^ -21
            {+1.0000000000000000e-020, "1e-20\n"},               // 10 ^ -20
            {+1.0000000000000000e-019, "1e-19\n"},               // 10 ^ -19
            {+1.0000000000000000e-018, "1e-18\n"},               // 10 ^ -18
            {+1.0000000000000000e-017, "1e-17\n"},               // 10 ^ -17
            {+1.0000000000000000e-016, "1e-16\n"},               // 10 ^ -16
            {+1.0000000000000000e-015, "1e-15\n"},               // 10 ^ -15
            {+1.0000000000000000e-014, "1e-14\n"},               // 10 ^ -14
            {+1.0000000000000000e-013, "1e-13\n"},               // 10 ^ -13
            {+1.0000000000000000e-012, "1e-12\n"},               // 10 ^ -12
            {+1.0000000000000000e-011, "1e-11\n"},               // 10 ^ -11
            {+1.0000000000000000e-010, "1e-10\n"},               // 10 ^ -10
            {+1.0000000000000000e-009, "1e-09\n"},               // 10 ^ -9
            {+1.0000000000000000e-008, "1e-08\n"},               // 10 ^ -8
            {+1.0000000000000000e-007, "1e-07\n"},               // 10 ^ -7
            {+1.0000000000000000e-006, "1e-06\n"},               // 10 ^ -6
            {+1.0000000000000000e-005, "1e-05\n"},               // 10 ^ -5
            {+1.0000000000000000e-004, "0.0001\n"},              // 10 ^ -4
            {+1.0000000000000000e-003, "0.001\n"},               // 10 ^ -3
            {+1.0000000000000000e-002, "0.01\n"},                // 10 ^ -2
            {+1.0000000000000000e-001, "0.1\n"},                 // 10 ^ -1
            {+1.0000000000000000e+000, "1.0\n"},                 // 10 ^ 0
            {+1.0000000000000000e+001, "10.0\n"},                // 10 ^ 1
            {+1.0000000000000000e+002, "100.0\n"},               // 10 ^ 2
            {+1.0000000000000000e+003, "1000.0\n"},              // 10 ^ 3
            {+1.0000000000000000e+004, "10000.0\n"},             // 10 ^ 4
            {+1.0000000000000000e+005, "100000.0\n"},            // 10 ^ 5
            {+1.0000000000000000e+006, "1000000.0\n"},           // 10 ^ 6
            {+1.0000000000000000e+007, "10000000.0\n"},          // 10 ^ 7
            {+1.0000000000000000e+008, "100000000.0\n"},         // 10 ^ 8
            {+1.0000000000000000e+009, "1000000000.0\n"},        // 10 ^ 9
            {+1.0000000000000000e+010, "10000000000.0\n"},       // 10 ^ 10
            {+1.0000000000000000e+011, "100000000000.0\n"},      // 10 ^ 11
            {+1.0000000000000000e+012, "1000000000000.0\n"},     // 10 ^ 12
            {+1.0000000000000000e+013, "10000000000000.0\n"},    // 10 ^ 13
            {+1.0000000000000000e+014, "100000000000000.0\n"},   // 10 ^ 14
            {+1.0000000000000000e+015, "1000000000000000.0\n"},  // 10 ^ 15
            {+1.0000000000000000e+016, "1e+16\n"},               // 10 ^ 16
            {+1.0000000000000000e+017, "1e+17\n"},               // 10 ^ 17
            {+1.0000000000000000e+018, "1e+18\n"},               // 10 ^ 18
            {+1.0000000000000000e+019, "1e+19\n"},               // 10 ^ 19
            {+1.0000000000000000e+020, "1e+20\n"},               // 10 ^ 20
            {+1.0000000000000000e+021, "1e+21\n"},               // 10 ^ 21
            {+1.0000000000000000e+022, "1e+22\n"},               // 10 ^ 22
            {+1.0000000000000000e+023, "1e+23\n"},               // 10 ^ 23
            {+1.0000000000000000e+024, "1e+24\n"},               // 10 ^ 24
            {+1.0000000000000000e+025, "1e+25\n"},               // 10 ^ 25

            {+9.9999999999999990e-006, "9.999999999999999e-06\n"},  // almost 10 ^ -5
            {+9.9999999999999990e-005, "9.999999999999999e-05\n"},  // almost 10 ^ -4
            {+9.9999999999999990e-004, "0.0009999999999999998\n"},  // almost 10 ^ -3
            {+9.9999999999999990e-003, "0.009999999999999998\n"},   // almost 10 ^ -2
            {+9.9999999999999990e-002, "0.09999999999999999\n"},    // almost 10 ^ -1
            {+9.9999999999999990e-001, "0.9999999999999999\n"},     // almost 10 ^ 0
            {+9.9999999999999990e+000, "9.999999999999998\n"},      // almost 10 ^ 1
            {+9.9999999999999990e+001, "99.99999999999999\n"},      // almost 10 ^ 2
            {+9.9999999999999990e+002, "999.9999999999999\n"},      // almost 10 ^ 3
            {+9.9999999999999990e+003, "9999.999999999998\n"},      // almost 10 ^ 4
            {+9.9999999999999990e+004, "99999.99999999999\n"},      // almost 10 ^ 5
            {+9.9999999999999990e+005, "999999.9999999999\n"},      // almost 10 ^ 6
            {+9.9999999999999990e+006, "9999999.999999998\n"},      // almost 10 ^ 7

            {+1.7976931348623155e+308, "1.7976931348623155e+308\n"},  // previous value before max
            {+1.7976931348623157e+308, "1.7976931348623157e+308\n"},  // float max
            {+INFINITY, "inf\n"},                                     // next value after float max

            {1.1, "1.1\n"},
            {1.1 * 3, "3.3000000000000003\n"},
            {1023.9999999999995, "1023.9999999999995\n"},
            {1023.9999999999997, "1023.9999999999997\n"},
            {1023.9999999999998, "1023.9999999999998\n"},
            {1023.9999999999999, "1023.9999999999999\n"},
            {1024.0000000000000, "1024.0\n"},
            {1024.0000000000002, "1024.0000000000002\n"},
            {1024.0000000000005, "1024.0000000000005\n"},
    };

    for (auto test : tests) {
        // First check: is the test valid? Does the representation given
        // actually encode the double value?
        double          d;
        pn_error_code_t error;
        pn_strtod(test.repr, strcspn(test.repr, "\n"), &d, &error);
        EXPECT_THAT(d, Eq(test.d));

        // Second check: do we print the given representation?
        EXPECT_THAT(dump('d', test.d), IsString(test.repr));
    }
}

TEST_F(DumpTest, Data) {
    EXPECT_THAT(dump('$', "", static_cast<size_t>(0)), IsString("$\n"));
    EXPECT_THAT(dump('$', "\x01\x02", static_cast<size_t>(2)), IsString("$0102\n"));
    EXPECT_THAT(dump('$', "\xff\x7f\x00\xff", static_cast<size_t>(4)), IsString("$ff7f00ff\n"));

    EXPECT_THAT(
            dump('$', "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff",
                 static_cast<size_t>(16)),
            IsString("$\t00112233 44556677 8899aabb ccddeeff\n"));

    EXPECT_THAT(
            dump('$',
                 "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"
                 "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"
                 "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99",
                 static_cast<size_t>(42)),
            IsString("$\t00112233 44556677 8899aabb ccddeeff 00112233 44556677 8899aabb ccddeeff\n"
                     "$\t00112233 44556677 8899\n"));
}

TEST_F(DumpTest, String) {
    EXPECT_THAT(dump('s', ""), IsString("\"\"\n"));
    EXPECT_THAT(dump('S', "\0", static_cast<size_t>(1)), IsString("\"\\u0000\"\n"));
    EXPECT_THAT(dump('c', '\1'), IsString("\"\\u0001\"\n"));
    EXPECT_THAT(dump('c', 'i'), IsString("\"i\"\n"));
    EXPECT_THAT(dump('s', "\177"), IsString("\"\\u007f\"\n"));
    EXPECT_THAT(dump('s', "procyon"), IsString("\"procyon\"\n"));
    EXPECT_THAT(dump('s', "procyon\n"), IsString(">\tprocyon\n"));

    EXPECT_THAT(
            dump('s',
                 "Four score and seven years ago our fathers brought forth on "
                 "this continent a new nation, conceived in liberty, and "
                 "dedicated to the proposition that all men are created equal."),
            IsString(">\tFour score and seven years ago our fathers brought forth on this\n"
                     ">\tcontinent a new nation, conceived in liberty, and dedicated to the\n"
                     ">\tproposition that all men are created equal.\n"
                     "!\n"));

    EXPECT_THAT(
            dump('s',
                 "Four score and seven years ago our fathers brought forth on this "
                 "continent a new nation, conceived in liberty, and dedicated to the "
                 "proposition that all men are created equal.\n"),
            IsString(">\tFour score and seven years ago our fathers brought forth on this\n"
                     ">\tcontinent a new nation, conceived in liberty, and dedicated to the\n"
                     ">\tproposition that all men are created equal.\n"));

    EXPECT_THAT(
            dump('s',
                 "Space: the final frontier.\n"
                 "\n"
                 "These are the voyages of the starship Enterprise. Its five-year "
                 "mission: to explore strange new worlds, to seek out new life and new "
                 "civilizations, to boldly go where no man has gone before.\n"),
            IsString(
                    ">\tSpace: the final frontier.\n"
                    ">\n"
                    ">\tThese are the voyages of the starship Enterprise. Its five-year mission:\n"
                    ">\tto explore strange new worlds, to seek out new life and new\n"
                    ">\tcivilizations, to boldly go where no man has gone before.\n"));

    pn::string s;
    for (int i = 0; i < 72; ++i) {
        s += "…";
    }
    s += " \n";
    EXPECT_THAT(
            dump('s', s.c_str()), IsString(">\t………………………………………………………………………………………………"
                                           "……………………………………………………………………………………………… \n"));
}

TEST_F(DumpTest, List) {
    EXPECT_THAT(dumpv("n"), IsString("[null]\n"));
    EXPECT_THAT(dumpv("??", true, false), IsString("[true, false]\n"));
    EXPECT_THAT(dumpv("iii", 1, 2, 3), IsString("[1, 2, 3]\n"));
    EXPECT_THAT(dumpv("dd", 0.1, 0.2), IsString("[0.1, 0.2]\n"));
    EXPECT_THAT(dumpv("n?id", true, 1, 1.0), IsString("[null, true, 1, 1.0]\n"));

    EXPECT_THAT(dumpv("s", "hello"), IsString("*\t\"hello\"\n"));
    EXPECT_THAT(
            dumpv("sss", "one", "two", "three"), IsString("*\t\"one\"\n"
                                                          "*\t\"two\"\n"
                                                          "*\t\"three\"\n"));
    EXPECT_THAT(
            dumpv("s", "one\ntwo\nthree\n"), IsString("*\t>\tone\n"
                                                      "\t|\ttwo\n"
                                                      "\t|\tthree\n"));

    pn_value_t inner;
    pn_setv(&inner, "n");
    EXPECT_THAT(dumpv("X", &inner), IsString("*\t[null]\n"));
    pn_setv(&inner, "s", "s");
    EXPECT_THAT(dumpv("X", &inner), IsString("*\t*\t\"s\"\n"));
    pn_setv(&inner, "ss", "a", "b");
    EXPECT_THAT(
            dumpv("xX", &inner, &inner), IsString("*\t*\t\"a\"\n"
                                                  "\t*\t\"b\"\n"
                                                  "*\t*\t\"a\"\n"
                                                  "\t*\t\"b\"\n"));
}

TEST_F(DumpTest, Map) {
    EXPECT_THAT(dumpkv("sn", "null"), IsString("{null: null}\n"));
    EXPECT_THAT(dumpkv("s?s?", "t", true, "f", false), IsString("{t: true, f: false}\n"));
    EXPECT_THAT(
            dumpkv("sisisi", "one", 1, "two", 2, "three", 3),
            IsString("{one: 1, two: 2, three: 3}\n"));
    EXPECT_THAT(dumpkv("sdsd", "less", 0.1, "more", 0.2), IsString("{less: 0.1, more: 0.2}\n"));
    EXPECT_THAT(
            dumpkv("sns?sisd", "null", "bool", true, "int", 1, "float", 1.0),
            IsString("{null: null, bool: true, int: 1, float: 1.0}\n"));

    EXPECT_THAT(dumpkv("ss", "hello", "world"), IsString("hello:  \"world\"\n"));
    EXPECT_THAT(
            dumpkv("ssssss", "1", "one", "2", "two", "3", "three"), IsString("1:  \"one\"\n"
                                                                             "2:  \"two\"\n"
                                                                             "3:  \"three\"\n"));
    EXPECT_THAT(
            dumpkv("ss", "n", "one\ntwo\nthree\n"), IsString("n:\n"
                                                             "\t>\tone\n"
                                                             "\t|\ttwo\n"
                                                             "\t|\tthree\n"));

    EXPECT_THAT(
            dumpkv("ssss", "one", "a\nb\n", "two", "c\nd\n"), IsString("one:\n"
                                                                       "\t>\ta\n"
                                                                       "\t|\tb\n"
                                                                       "two:\n"
                                                                       "\t>\tc\n"
                                                                       "\t|\td\n"));

    EXPECT_THAT(
            dumpkv("sssisssi", "one", "one\n", "two", 2, "three", "three\n", "four", 4),
            IsString("one:\n"
                     "\t>\tone\n"
                     "two:   2\n"
                     "three:\n"
                     "\t>\tthree\n"
                     "four:  4\n"));
}

TEST_F(DumpTest, Composite) {
    pn_value_t * usa, *chile, *cuba, *colors, *nicknames;
    pn::value    x;
    const size_t _3 = 3;
    pn_setkv(x.c_obj(), "sNsNsN", "us", &usa, "cl", &chile, "cu", &cuba);

    pn_setkv(
            usa, "sssdsisisNsN", "name", "United States of America", "ratio", 1.9, "stars", 50,
            "stripes", 13, "colors", &colors, "nicknames", &nicknames);
    pn_setv(colors, "$$$", "\xb2\x22\x34", _3, "\xff\xff\xff", _3, "\x3c\x3b\x6e", _3);
    pn_setv(nicknames, "sss", "The Stars and Stripes", "Old Glory", "The Star-Spangled Banner");

    pn_setkv(
            chile, "sssdsisisN", "name", "Republic of Chile", "ratio", 1.5, "stars", 1, "stripes",
            2, "colors", &colors);
    pn_setv(colors, "$$$", "\xda\x29\x1c", _3, "\xff\xff\xff", _3, "\x00\x33\xa0", _3);

    pn_setkv(
            cuba, "sssdsisisN", "name", "Republic of Cuba", "ratio", 2.0, "stars", 1, "stripes", 5,
            "colors", &colors);
    pn_setv(colors, "$$$", "\xcb\x15\x15", _3, "\xff\xff\xff", _3, "\x00\x2a\x8f", _3);

    EXPECT_THAT(
            dump(x.c_obj()), IsString("us:\n"
                                      "\tname:     \"United States of America\"\n"
                                      "\tratio:    1.9\n"
                                      "\tstars:    50\n"
                                      "\tstripes:  13\n"
                                      "\tcolors:\n"
                                      "\t\t*\t$b22234\n"
                                      "\t\t*\t$ffffff\n"
                                      "\t\t*\t$3c3b6e\n"
                                      "\tnicknames:\n"
                                      "\t\t*\t\"The Stars and Stripes\"\n"
                                      "\t\t*\t\"Old Glory\"\n"
                                      "\t\t*\t\"The Star-Spangled Banner\"\n"
                                      "cl:\n"
                                      "\tname:     \"Republic of Chile\"\n"
                                      "\tratio:    1.5\n"
                                      "\tstars:    1\n"
                                      "\tstripes:  2\n"
                                      "\tcolors:\n"
                                      "\t\t*\t$da291c\n"
                                      "\t\t*\t$ffffff\n"
                                      "\t\t*\t$0033a0\n"
                                      "cu:\n"
                                      "\tname:     \"Republic of Cuba\"\n"
                                      "\tratio:    2.0\n"
                                      "\tstars:    1\n"
                                      "\tstripes:  5\n"
                                      "\tcolors:\n"
                                      "\t\t*\t$cb1515\n"
                                      "\t\t*\t$ffffff\n"
                                      "\t\t*\t$002a8f\n"));
}

TEST_F(DumpTest, AllCTypes) {
    EXPECT_THAT(dump('\0'), IsString("null\n"));
    EXPECT_THAT(dump('n'), IsString("null\n"));
    EXPECT_THAT(dump('N'), IsString("null\n"));

    EXPECT_THAT(dump('?', true), IsString("true\n"));
    EXPECT_THAT(dump('?', false), IsString("false\n"));

    EXPECT_THAT(dump<int>('i', -1), IsString("-1\n"));
    EXPECT_THAT(dump<unsigned int>('I', 1), IsString("1\n"));
    EXPECT_THAT(dump<int8_t>('b', -8), IsString("-8\n"));
    EXPECT_THAT(dump<uint8_t>('B', 8), IsString("8\n"));
    EXPECT_THAT(dump<int16_t>('h', -16), IsString("-16\n"));
    EXPECT_THAT(dump<uint16_t>('H', 16), IsString("16\n"));
    EXPECT_THAT(dump<int32_t>('l', -32), IsString("-32\n"));
    EXPECT_THAT(dump<uint32_t>('L', 32), IsString("32\n"));
    EXPECT_THAT(dump<int64_t>('q', -64), IsString("-64\n"));
    EXPECT_THAT(dump<uint64_t>('Q', 64), IsString("64\n"));
    EXPECT_THAT(dump<intptr_t>('p', -128), IsString("-128\n"));
    EXPECT_THAT(dump<uintptr_t>('P', 128), IsString("128\n"));
    EXPECT_THAT(dump<size_t>('z', -256), IsString("-256\n"));
    EXPECT_THAT(dump<ssize_t>('Z', 256), IsString("256\n"));

    EXPECT_THAT(dump<float>('f', 1.5), IsString("1.5\n"));
    EXPECT_THAT(dump<double>('d', 2.5), IsString("2.5\n"));

    EXPECT_THAT(dump('a', *pn::array{"one"}.c_obj()), IsString("*\t\"one\"\n"));
    EXPECT_THAT(dump('A', *pn::array{"two"}.c_obj()), IsString("*\t\"two\"\n"));
    EXPECT_THAT(dump('m', *pn::map{{"3", "three"}}.c_obj()), IsString("3:  \"three\"\n"));
    EXPECT_THAT(dump('M', *pn::map{{"4", "four"}}.c_obj()), IsString("4:  \"four\"\n"));
    EXPECT_THAT(dump('x', pn::value{"five"}.c_obj()), IsString("\"five\"\n"));
    EXPECT_THAT(dump('X', pn::value{"six"}.c_obj()), IsString("\"six\"\n"));

    EXPECT_THAT(dump('s', "s"), IsString("\"s\"\n"));
    EXPECT_THAT((dump<const char*, size_t>('S', "S\0S", 3)), IsString("\"S\\u0000S\"\n"));
    EXPECT_THAT((dump<const char*, size_t>('$', "\x12\x34", 2)), IsString("$1234\n"));
    EXPECT_THAT(dump('c', 'c'), IsString("\"c\"\n"));
    EXPECT_THAT(dump('C', 0x1f600), IsString("\"\xf0\x9f\x98\x80\"\n"));
}

TEST_F(DumpTest, AllCppTypes) {
    EXPECT_THAT(pn::dump(nullptr), IsString("null\n"));
    EXPECT_THAT(pn::dump(true), IsString("true\n"));
    EXPECT_THAT(pn::dump(2), IsString("2\n"));
    EXPECT_THAT(pn::dump(3.14), IsString("3.14\n"));

    pn::data d;
    EXPECT_THAT(pn::dump(d), IsString("$\n"));
    EXPECT_THAT(pn::dump(d.copy()), IsString("$\n"));
    EXPECT_THAT(pn::dump(pn::data_ref{d}), IsString("$\n"));
    EXPECT_THAT(pn::dump(pn::data_view{d}), IsString("$\n"));

    pn::string s;
    EXPECT_THAT(pn::dump(s), IsString("\"\"\n"));
    EXPECT_THAT(pn::dump(s.copy()), IsString("\"\"\n"));
    EXPECT_THAT(pn::dump(pn::string_ref{s}), IsString("\"\"\n"));
    EXPECT_THAT(pn::dump(std::string("")), IsString("\"\"\n"));

    pn::value x{true};
    EXPECT_THAT(pn::dump(x), IsString("true\n"));
    EXPECT_THAT(pn::dump(x.copy()), IsString("true\n"));
    EXPECT_THAT(pn::dump(pn::value_ref{x}), IsString("true\n"));
    EXPECT_THAT(pn::dump(pn::value_cref{x}), IsString("true\n"));
}

TEST_F(DumpTest, CFailure) {
    pn::string_view ro = "";
    EXPECT_THAT(pn_dump(ro.open().c_obj(), 0, 'n'), Eq(false));
    EXPECT_THAT(pn_dump(ro.open().c_obj(), 0, '?', true), Eq(false));
    EXPECT_THAT(pn_dump(ro.open().c_obj(), 0, '?', false), Eq(false));
    EXPECT_THAT(pn_dump(ro.open().c_obj(), 0, 'i', 1), Eq(false));
    EXPECT_THAT(pn_dump(ro.open().c_obj(), 0, 'f', 1.0), Eq(false));
}

TEST_F(DumpTest, CppFailure) {
    pn::string_view ro = "";
    EXPECT_THAT(pn::dump(ro.open(), nullptr, pn::dump_default), Eq(false));
    EXPECT_THAT(pn::dump(ro.open(), true, pn::dump_default), Eq(false));
    EXPECT_THAT(pn::dump(ro.open(), false, pn::dump_default), Eq(false));
    EXPECT_THAT(pn::dump(ro.open(), 1, pn::dump_default), Eq(false));
    EXPECT_THAT(pn::dump(ro.open(), 1.0, pn::dump_default), Eq(false));
}

}  // namespace pntest
