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

#include "../src/parse.h"
#include "./matchers.hpp"

using ParseTest = testing::Test;
using testing::PrintToString;

namespace pntest {
namespace {

std::pair<pn::value, pn_error_t> parse(const std::string& arg) {
    pn::value x;
    pn::value in;
    pn_set(in.c_obj(), 'S', arg.data(), arg.size());
    pn_error_t error;
    pn_file_t* f      = pn_open_string(&in.c_obj()->s, "r");
    bool       parsed = pn_parse(f, x.c_obj(), &error);
    fclose(f);
    if (parsed) {
        return std::make_pair(std::move(x), pn_error_t{PN_OK, 0, 0});
    }
    return std::make_pair(nullptr, error);
}

template <typename T>
bool value_matches(const pn_value_t* x, T y) {
    return pn_cmp(x, pn::value(y).c_obj()) == 0;
}

struct anything {
} any;
bool value_matches(const pn_value_t*, anything) { return true; }

bool value_matches(const pn_value_t* x, double f) {
    if (std::fpclassify(f) == FP_NAN) {
        return (x->type == PN_FLOAT) && (std::fpclassify(x->f) == FP_NAN);
    }
    return pn_cmp(x, set('d', f).c_obj()) == 0;
}

bool value_matches(const pn_value_t* x, const std::vector<uint8_t>& y) {
    return pn_cmp(x, set('$', y.data(), y.size()).c_obj()) == 0;
}

bool value_matches(const pn_value_t* x, pn_value_t* y) { return pn_cmp(x, y) == 0; }
bool value_matches(const pn_value_t* x, const pn_value_t* y) { return pn_cmp(x, y) == 0; }

template <typename T>
class ParseMatcher : public ::testing::MatcherInterface<const std::pair<pn::value, pn_error_t>&> {
  public:
    ParseMatcher(const T& expected, pn_error_t error) : _expected{expected}, _error{error} {}

    bool MatchAndExplain(
            const std::pair<pn::value, pn_error_t>& arg,
            ::testing::MatchResultListener*         listener) const override {
        if (arg.second.code != PN_OK) {
            *listener << "parse failed with " << ::testing::PrintToString(arg.second) << " ("
                      << arg.second.lineno << ":" << arg.second.column << ")";
            return (_error.code == arg.second.code) && (_error.lineno == arg.second.lineno) &&
                   (_error.column == arg.second.column);
        }
        *listener << "parses to " << arg.first;
        return (_error.code == arg.second.code) && value_matches(arg.first.c_obj(), _expected);
    }

    void DescribeTo(::std::ostream* os) const override {
        if (_error.code == PN_OK) {
            *os << "parses to " << ::testing::PrintToString(_expected);
        } else {
            *os << "fails to parse with " << _error.code << " (" << _error.lineno << ":"
                << _error.column << ")";
        }
    }

  private:
    T          _expected;
    pn_error_t _error;
};

template <typename T>
testing::Matcher<const std::pair<pn::value, pn_error_t>&> ParsesTo(T expected) {
    return ::testing::MakeMatcher(new ParseMatcher<T>(expected, pn_error_t{PN_OK, 0, 0}));
}

testing::Matcher<const std::pair<pn::value, pn_error_t>&> FailsToParse(
        pn_error_code_t code, size_t lineno, size_t column) {
    return ::testing::MakeMatcher(
            new ParseMatcher<std::nullptr_t>(nullptr, pn_error_t{code, lineno, column}));
}

TEST_F(ParseTest, Constants) {
    EXPECT_THAT(parse("null"), ParsesTo(&pn_null));
    EXPECT_THAT(parse("true"), ParsesTo(&pn_true));
    EXPECT_THAT(parse("false"), ParsesTo(&pn_false));
    EXPECT_THAT(parse("inf"), ParsesTo(&pn_inf));
    EXPECT_THAT(parse("+inf"), ParsesTo(&pn_inf));
    EXPECT_THAT(parse("-inf"), ParsesTo(&pn_neg_inf));
    EXPECT_THAT(parse("nan"), ParsesTo(std::numeric_limits<double>::quiet_NaN()));
    EXPECT_THAT(parse("floop"), FailsToParse(PN_ERROR_BADWORD, 1, 1));
}

TEST_F(ParseTest, Integer) {
    EXPECT_THAT(parse("0"), ParsesTo(0));
    EXPECT_THAT(parse("1"), ParsesTo(1));
    EXPECT_THAT(parse("9223372036854775807"), ParsesTo(INT64_MAX));
    EXPECT_THAT(parse("9223372036854775808"), FailsToParse(PN_ERROR_INT_OVERFLOW, 1, 1));
    EXPECT_THAT(parse("-9223372036854775808"), ParsesTo(INT64_MIN));
    EXPECT_THAT(parse("-9223372036854775809"), FailsToParse(PN_ERROR_INT_OVERFLOW, 1, 1));
}

TEST_F(ParseTest, Float) {
    EXPECT_THAT(parse("0.0"), ParsesTo(0.0));
    EXPECT_THAT(parse("0.5"), ParsesTo(0.5));
    EXPECT_THAT(parse("0e0"), ParsesTo(0.0));
    EXPECT_THAT(parse("0.5e0"), ParsesTo(0.5));
}

TEST_F(ParseTest, Data) {
    EXPECT_THAT(parse("$"), ParsesTo(std::vector<uint8_t>{}));
    EXPECT_THAT(parse("$0"), FailsToParse(PN_ERROR_PARTIAL, 1, 2));
    EXPECT_THAT(parse("$00"), ParsesTo(std::vector<uint8_t>{0}));
    EXPECT_THAT(parse("$ 00"), ParsesTo(std::vector<uint8_t>{0}));
    EXPECT_THAT(parse("$ 0 0"), FailsToParse(PN_ERROR_PARTIAL, 1, 3));
    EXPECT_THAT(parse("$00112233"), ParsesTo(std::vector<uint8_t>{0, 17, 34, 51}));

    EXPECT_THAT(parse("$ 00 $ 01"), FailsToParse(PN_ERROR_SUFFIX, 1, 6));
    EXPECT_THAT(
            parse("$ 00\n"
                  "$ 01\n"),
            ParsesTo(std::vector<uint8_t>{0, 1}));

    EXPECT_THAT(
            parse("# 00\n"
                  "$ 01\n"
                  "# 02\n"
                  "$ 03\n"
                  "# 04\n"),
            ParsesTo(std::vector<uint8_t>{1, 3}));

    EXPECT_THAT(
            parse("[$, $1f, $ffff, $ 0f 1e 2d 3c]"),
            ParsesTo(setv("$$$$", "", static_cast<size_t>(0), "\037", static_cast<size_t>(1),
                          "\377\377", static_cast<size_t>(2), "\017\036\055\074",
                          static_cast<size_t>(4))
                             .c_obj()));

    EXPECT_THAT(
            parse("[$abcd\n"
                  "$1234]\n"),
            FailsToParse(PN_ERROR_ARRAY_END, 1, 7));
}

TEST_F(ParseTest, String) {
    EXPECT_THAT(parse("\"\""), ParsesTo(""));
    EXPECT_THAT(parse("\"yo whaddup\""), ParsesTo("yo whaddup"));
    EXPECT_THAT(parse("\"\\/\\\"\\\\\\b\\f\\n\\r\\t\""), ParsesTo("/\"\\\b\f\n\r\t"));
    EXPECT_THAT(parse("\"\\v\""), FailsToParse(PN_ERROR_BADESC, 1, 2));

    EXPECT_THAT(parse("\"\\u000\""), FailsToParse(PN_ERROR_BADUESC, 1, 2));
    EXPECT_THAT(parse("\"\\u0001\""), ParsesTo("\001"));
    EXPECT_THAT(parse("\"\\u0012\""), ParsesTo("\022"));
    EXPECT_THAT(parse("\"\\u0123\""), ParsesTo("\304\243"));
    EXPECT_THAT(parse("\"\\u1234\""), ParsesTo("\341\210\264"));
}

TEST_F(ParseTest, XString) {
    EXPECT_THAT(parse(">"), ParsesTo("\n"));
    EXPECT_THAT(parse("|"), ParsesTo("\n"));
    EXPECT_THAT(parse("!"), ParsesTo(""));
    EXPECT_THAT(parse("> "), ParsesTo("\n"));
    EXPECT_THAT(parse("| "), ParsesTo("\n"));
    EXPECT_THAT(parse("! "), ParsesTo(""));
    EXPECT_THAT(parse(">\t"), ParsesTo("\n"));
    EXPECT_THAT(parse("|\t"), ParsesTo("\n"));
    EXPECT_THAT(parse("!\t"), ParsesTo(""));
    EXPECT_THAT(parse(">>"), ParsesTo(">\n"));
    EXPECT_THAT(parse("||"), ParsesTo("|\n"));
    EXPECT_THAT(parse("!!"), FailsToParse(PN_ERROR_BANG_SUFFIX, 1, 2));

    EXPECT_THAT(parse("!\n>\n"), FailsToParse(PN_ERROR_BANG_LAST, 2, 1));
    EXPECT_THAT(parse("!\n|\n"), FailsToParse(PN_ERROR_BANG_LAST, 2, 1));
    EXPECT_THAT(parse("!\n!\n"), FailsToParse(PN_ERROR_BANG_LAST, 2, 1));

    EXPECT_THAT(parse("> one"), ParsesTo("one\n"));
    EXPECT_THAT(parse("| one"), ParsesTo("one\n"));
    EXPECT_THAT(parse("! one"), FailsToParse(PN_ERROR_BANG_SUFFIX, 1, 3));
    EXPECT_THAT(
            parse("| one\n"
                  "| two"),
            ParsesTo("one\ntwo\n"));
    EXPECT_THAT(
            parse("| one\n"
                  "> two\n"
                  "!\n"),
            ParsesTo("one two"));

    EXPECT_THAT(
            parse("| one\n"
                  "!\n"
                  "# two\n"),
            ParsesTo("one"));

    EXPECT_THAT(
            parse(">\n"
                  "> Line two\n"
                  "> of three.\n"
                  ">\n"),
            ParsesTo("\n"
                     "Line two of three.\n"
                     "\n"));

    EXPECT_THAT(
            parse(">\n"
                  ">\n"
                  "> Line three\n"
                  "> of five.\n"
                  ">\n"
                  ">\n"),
            ParsesTo("\n\n"
                     "Line three of five.\n"
                     "\n\n"));

    EXPECT_THAT(
            parse("> Paragraph\n"
                  "> one.\n"
                  ">\n"
                  "> Paragraph\n"
                  "> two.\n"),
            ParsesTo("Paragraph one.\n"
                     "\n"
                     "Paragraph two.\n"));

    EXPECT_THAT(
            parse("> One.\n"
                  ">\n"
                  "> Two.\n"
                  "!\n"),
            ParsesTo("One.\n"
                     "\n"
                     "Two."));

    EXPECT_THAT(
            parse("| Four score and seven years ago our fathers brought forth on this\n"
                  "> continent a new nation, conceived in liberty, and dedicated to the\n"
                  "> proposition that all men are created equal.\n"
                  "!\n"),
            ParsesTo("Four score and seven years ago our fathers brought forth on "
                     "this continent a new nation, conceived in liberty, and "
                     "dedicated to the proposition that all men are created equal."));

    EXPECT_THAT(
            parse("| Space: the final frontier.\n"
                  ">\n"
                  "| These are the voyages of the starship Enterprise. Its five-year mission:\n"
                  "> to explore strange new worlds, to seek out new life and new\n"
                  "> civilizations, to boldly go where no man has gone before.\n"),
            ParsesTo("Space: the final frontier.\n"
                     "\n"
                     "These are the voyages of the starship Enterprise. Its five-year "
                     "mission: to explore strange new worlds, to seek out new life and new "
                     "civilizations, to boldly go where no man has gone before.\n"));
}

TEST_F(ParseTest, List) {
    EXPECT_THAT(parse("[]"), ParsesTo(setv("").c_obj()));
    EXPECT_THAT(parse("[0]"), ParsesTo(setv("i", 0).c_obj()));
    EXPECT_THAT(
            parse("[[[0]]]"),
            ParsesTo(setv("x", setv("x", setv("i", 0).c_obj()).c_obj()).c_obj()));
    EXPECT_THAT(parse("[1, 2, 3]"), ParsesTo(setv("iii", 1, 2, 3).c_obj()));

    EXPECT_THAT(
            parse("[1, [2, [3]]]"),
            ParsesTo(setv("ix", 1, setv("ix", 2, setv("i", 3).c_obj()).c_obj()).c_obj()));

    EXPECT_THAT(parse("["), FailsToParse(PN_ERROR_SHORT, 1, 2));
    EXPECT_THAT(parse("[1"), FailsToParse(PN_ERROR_ARRAY_END, 1, 3));
    EXPECT_THAT(parse("[1,"), FailsToParse(PN_ERROR_SHORT, 1, 4));

    EXPECT_THAT(parse("[}"), FailsToParse(PN_ERROR_SHORT, 1, 2));
    EXPECT_THAT(parse("[1}"), FailsToParse(PN_ERROR_ARRAY_END, 1, 3));
    EXPECT_THAT(parse("[1, }"), FailsToParse(PN_ERROR_SHORT, 1, 5));
}

TEST_F(ParseTest, XList) {
    EXPECT_THAT(parse("* 0"), ParsesTo(setv("i", 0).c_obj()));
    EXPECT_THAT(
            parse("* * * 0"),
            ParsesTo(setv("x", setv("x", setv("i", 0).c_obj()).c_obj()).c_obj()));
    EXPECT_THAT(
            parse("* 1\n"
                  "* 2\n"
                  "* 3\n"),
            ParsesTo(setv("iii", 1, 2, 3).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "* * 2\n"
                  "  * * 3\n"),
            ParsesTo(setv("ix", 1, setv("ix", 2, setv("i", 3).c_obj()).c_obj()).c_obj()));
    EXPECT_THAT(
            parse("*\n"
                  "  1\n"
                  "*\n"
                  "  *\n"
                  "    2\n"
                  "  *\n"
                  "    *\n"
                  "      3\n"),
            ParsesTo(setv("ix", 1, setv("ix", 2, setv("i", 3).c_obj()).c_obj()).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "# :)\n"
                  "* 2\n"
                  "  # :(\n"
                  "* 3\n"
                  "# :|\n"),
            ParsesTo(setv("iii", 1, 2, 3).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "  * 2\n"
                  "    * 3\n"),
            FailsToParse(PN_ERROR_SIBLING, 2, 3));
    EXPECT_THAT(
            parse("* * 1\n"
                  " * 2\n"),
            FailsToParse(PN_ERROR_OUTDENT, 2, 2));

    EXPECT_THAT(parse("*"), FailsToParse(PN_ERROR_LONG, 1, 2));
}

TEST_F(ParseTest, Map) {
    EXPECT_THAT(parse("{}"), ParsesTo(setkv("").c_obj()));
    EXPECT_THAT(parse("{0: false}"), ParsesTo(setkv("s?", "0", false).c_obj()));
    EXPECT_THAT(
            parse("{0: {1: {2: 3}}}"),
            ParsesTo(setkv("sx", "0", setkv("sx", "1", setkv("si", "2", 3).c_obj()).c_obj())
                             .c_obj()));
    EXPECT_THAT(
            parse("{one: 1, two: 2, three: 3}"),
            ParsesTo(setkv("sisisi", "one", 1, "two", 2, "three", 3).c_obj()));

    EXPECT_THAT(
            parse("{one: 1, and: {two: 2, and: {three: 3}}}"),
            ParsesTo(setkv("sisx", "one", 1, "and",
                           setkv("sisx", "two", 2, "and", setkv("si", "three", 3).c_obj()).c_obj())
                             .c_obj()));

    EXPECT_THAT(parse("{"), FailsToParse(PN_ERROR_MAP_KEY, 1, 2));
    EXPECT_THAT(parse("{1"), FailsToParse(PN_ERROR_MAP_KEY, 1, 2));
    EXPECT_THAT(parse("{1,"), FailsToParse(PN_ERROR_MAP_KEY, 1, 2));
    EXPECT_THAT(parse("{1:"), FailsToParse(PN_ERROR_SHORT, 1, 4));
    EXPECT_THAT(parse("{1: 1"), FailsToParse(PN_ERROR_MAP_END, 1, 6));
    EXPECT_THAT(parse("{1: 1,"), FailsToParse(PN_ERROR_MAP_KEY, 1, 7));

    EXPECT_THAT(parse("{]"), FailsToParse(PN_ERROR_MAP_KEY, 1, 2));
    EXPECT_THAT(parse("{1: ]"), FailsToParse(PN_ERROR_SHORT, 1, 5));
    EXPECT_THAT(parse("{1: 1 ]"), FailsToParse(PN_ERROR_MAP_END, 1, 7));
    EXPECT_THAT(parse("{1: 1, ]"), FailsToParse(PN_ERROR_MAP_KEY, 1, 8));
}

TEST_F(ParseTest, XMap) {
    EXPECT_THAT(parse(": null"), ParsesTo(setkv("sn", "").c_obj()));
    EXPECT_THAT(parse("zero: 0"), ParsesTo(setkv("si", "zero", 0).c_obj()));
    EXPECT_THAT(
            parse("one:\n"
                  "  two:\n"
                  "    three: 0"),
            ParsesTo(
                    setkv("sx", "one", setkv("sx", "two", setkv("si", "three", 0).c_obj()).c_obj())
                            .c_obj()));
    EXPECT_THAT(
            parse("one: 1\n"
                  "two: 2\n"
                  "three: 3\n"),
            ParsesTo(setkv("sisisi", "one", 1, "two", 2, "three", 3).c_obj()));

    EXPECT_THAT(
            parse("one: 1\n"
                  "and:\n"
                  "  two: 2\n"
                  "  and:\n"
                  "    three: 3\n"),
            ParsesTo(setkv("sisx", "one", 1, "and",
                           setkv("sisx", "two", 2, "and", setkv("si", "three", 3).c_obj()).c_obj())
                             .c_obj()));
    EXPECT_THAT(
            parse("one:\n"
                  "  1\n"
                  "and:\n"
                  "  two:\n"
                  "    2\n"
                  "  and:\n"
                  "    three:\n"
                  "      3\n"),
            ParsesTo(setkv("sisx", "one", 1, "and",
                           setkv("sisx", "two", 2, "and", setkv("si", "three", 3).c_obj()).c_obj())
                             .c_obj()));
    EXPECT_THAT(
            parse("one:\n"
                  "\n"
                  "  1\n"
                  "two:\n"
                  "  \n"
                  "  2\n"
                  "three:\n"
                  "\t\n"
                  "  3\n"),
            ParsesTo(setkv("sisisi", "one", 1, "two", 2, "three", 3).c_obj()));

    EXPECT_THAT(
            parse("one: 1\n"
                  "  two: 2\n"
                  "    three: 3\n"),
            FailsToParse(PN_ERROR_CHILD, 2, 3));
    EXPECT_THAT(
            parse("one: 1\n"
                  "# :)\n"
                  "two: 2\n"
                  "     # :(\n"
                  "three: 3\n"),
            ParsesTo(setkv("sisisi", "one", 1, "two", 2, "three", 3).c_obj()));
    EXPECT_THAT(
            parse("one: 1\n"
                  "# :)\n"
                  "two: 2\n"
                  "  # :)\n"  // doesn't match indentation of '2'
                  "three: 3\n"),
            ParsesTo(setkv("sisisi", "one", 1, "two", 2, "three", 3).c_obj()));

    EXPECT_THAT(
            parse("\"\": \"\"\n"
                  "\":\": \":\"\n"),
            ParsesTo(setkv("ssss", "", "", ":", ":").c_obj()));

    EXPECT_THAT(
            parse("\"\\u0001\": $01\n"
                  "\"\\n\": $0a\n"
                  "\"\\u007f\": $7f\n"
                  "\"\\u0080\": $c280\n"
                  "\"\\u72ac\\u524d\": $e78aac e5898d\n"),
            ParsesTo(setkv("s$s$s$s$s$", "\1", "\1", static_cast<size_t>(1), "\n", "\n",
                           static_cast<size_t>(1), "\177", "\177", static_cast<size_t>(1),
                           "\302\200", "\302\200", static_cast<size_t>(2),
                           "\347\212\254\345\211\215", "\347\212\254\345\211\215",
                           static_cast<size_t>(6))
                             .c_obj()));
}

TEST_F(ParseTest, Equivalents) {
    EXPECT_THAT(parse("!").first, IsValue(*parse("\"\"").first.c_obj()));
    EXPECT_THAT(parse("|\n!").first, IsValue(*parse("\"\"").first.c_obj()));
    EXPECT_THAT(parse("|").first, IsValue(*parse("\"\\n\"").first.c_obj()));
    EXPECT_THAT(parse("|\n>\n!").first, IsValue(*parse("\"\\n\"").first.c_obj()));

    EXPECT_THAT(parse("{1: 2}").first, IsValue(*parse("1: 2").first.c_obj()));

    EXPECT_THAT(parse("[1]").first, IsValue(*parse("* 1").first.c_obj()));
}

TEST_F(ParseTest, Composite) {
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
            parse("us:\n"
                  "  name:     \"United States of America\"\n"
                  "  ratio:    1.9\n"
                  "  stars:    50\n"
                  "  stripes:  13\n"
                  "  colors:   [$b22234, $ffffff, $3c3b6e]\n"
                  "  nicknames:\n"
                  "    * \"The Stars and Stripes\"\n"
                  "    * \"Old Glory\"\n"
                  "    * \"The Star-Spangled Banner\"\n"
                  "cl:\n"
                  "  name:     \"Republic of Chile\"\n"
                  "  ratio:    1.5\n"
                  "  stars:    1\n"
                  "  stripes:  2\n"
                  "  colors:   [$da291c, $ffffff, $0033a0]\n"
                  "cu:\n"
                  "  name:     \"Republic of Cuba\"\n"
                  "  ratio:    2.0\n"
                  "  stars:    1\n"
                  "  stripes:  5\n"
                  "  colors:   [$cb1515, $ffffff, $002a8f]\n"),
            ParsesTo(x.c_obj()));
}

TEST_F(ParseTest, Comment) {
    // Missing values
    EXPECT_THAT(parse("# comment"), FailsToParse(PN_ERROR_LONG, 1, 10));
    EXPECT_THAT(parse("* # comment"), FailsToParse(PN_ERROR_LONG, 1, 12));

    EXPECT_THAT(parse("true# comment"), ParsesTo(true));
    EXPECT_THAT(parse("true # comment"), ParsesTo(true));
    EXPECT_THAT(parse("true\n# comment"), ParsesTo(true));
    EXPECT_THAT(parse("1# comment"), ParsesTo(1));
    EXPECT_THAT(parse("1 # comment"), ParsesTo(1));
    EXPECT_THAT(parse("1\n# comment"), ParsesTo(1));
    EXPECT_THAT(parse("\"\"# comment"), ParsesTo(""));
    EXPECT_THAT(parse("\"\" # comment"), ParsesTo(""));
    EXPECT_THAT(parse("\"\"\n# comment"), ParsesTo(""));
    EXPECT_THAT(parse("$00# comment"), ParsesTo(std::vector<uint8_t>{0}));
    EXPECT_THAT(parse("$00 # comment"), ParsesTo(std::vector<uint8_t>{0}));
    EXPECT_THAT(parse("$00\n# comment"), ParsesTo(std::vector<uint8_t>{0}));
    EXPECT_THAT(parse("># comment"), ParsesTo("# comment\n"));
    EXPECT_THAT(parse("> # comment"), ParsesTo("# comment\n"));
    EXPECT_THAT(parse(">\n# comment"), ParsesTo("\n"));

    EXPECT_THAT(
            parse("* # comment\n"
                  "  1\n"),
            ParsesTo(setv("i", 1).c_obj()));

    EXPECT_THAT(
            parse("* # comment\n"
                  "  # etc\n"
                  "  1\n"),
            ParsesTo(setv("i", 1).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "  # comment\n"),
            ParsesTo(setv("i", 1).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "  # comment\n"
                  "  # etc\n"),
            ParsesTo(setv("i", 1).c_obj()));

    EXPECT_THAT(
            parse("* 1\n"
                  "# parent\n"
                  "  # child\n"),
            ParsesTo(setv("i", 1).c_obj()));
}

TEST_F(ParseTest, SameLine) {
    EXPECT_THAT(parse("1 1"), FailsToParse(PN_ERROR_SUFFIX, 1, 3));
    EXPECT_THAT(parse("1\n1"), FailsToParse(PN_ERROR_SIBLING, 2, 1));
}

TEST_F(ParseTest, Bad) {
    EXPECT_THAT(parse("&"), FailsToParse(PN_ERROR_BADCHAR, 1, 1));

    EXPECT_THAT(parse(""), FailsToParse(PN_ERROR_LONG, 1, 1));
    EXPECT_THAT(parse("]"), FailsToParse(PN_ERROR_LONG, 1, 1));
}

TEST_F(ParseTest, StackSmash) {
    EXPECT_THAT(parse(std::string(63, '*') + "null"), ParsesTo(any));
    EXPECT_THAT(parse(std::string(64, '*') + "null"), FailsToParse(PN_ERROR_RECURSION, 1, 64));
    EXPECT_THAT(parse(std::string(100, '*') + "null"), FailsToParse(PN_ERROR_RECURSION, 1, 64));
    EXPECT_THAT(
            parse(std::string(512 * 512, '*') + "null"), FailsToParse(PN_ERROR_RECURSION, 1, 64));
}

}  // namespace
}  // namespace pntest
