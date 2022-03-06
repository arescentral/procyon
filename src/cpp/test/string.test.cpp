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

#include <pn/procyon.h>

#include <gmock/gmock.h>
#include <limits>

#include "../src/unicode.h"
#include "./matchers.hpp"

using StringTest = ::testing::Test;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Ne;

namespace pntest {

static std::vector<uint32_t> runes(pn::string_view s) {
    std::vector<uint32_t> runes;
    for (pn::rune r : s) {
        runes.push_back(r.value());
    }
    return runes;
}

static std::vector<uint32_t> rev_runes(pn::string_view s) {
    std::vector<uint32_t> runes;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        runes.push_back((*it).value());
    }
    std::reverse(runes.begin(), runes.end());
    return runes;
}

TEST_F(StringTest, AllRunes) {
    EXPECT_THAT(runes(""), ElementsAre());
    EXPECT_THAT(rev_runes(""), ElementsAre());

    EXPECT_THAT(runes("1"), ElementsAre('1'));
    EXPECT_THAT(rev_runes("1"), ElementsAre('1'));

    EXPECT_THAT(runes("ASCII"), ElementsAre('A', 'S', 'C', 'I', 'I'));
    EXPECT_THAT(rev_runes("ASCII"), ElementsAre('A', 'S', 'C', 'I', 'I'));

    EXPECT_THAT(runes("\343\201\213\343\201\252"), ElementsAre(0x304b, 0x306a));
    EXPECT_THAT(rev_runes("\343\201\213\343\201\252"), ElementsAre(0x304b, 0x306a));

    // Invalid: '\377' can never occur in UTF-8.
    EXPECT_THAT(runes("\377"), ElementsAre(0xFFFD));
    EXPECT_THAT(rev_runes("\377"), ElementsAre(0xFFFD));

    // Invalid: continuation bytes without headers.
    EXPECT_THAT(
            runes("\200\200\200\200\200"), ElementsAre(0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD));
    EXPECT_THAT(
            rev_runes("\200\200\200\200\200"),
            ElementsAre(0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD));

    // Invalid: overlong encoding of '\0'.
    EXPECT_THAT(runes("\300\200"), ElementsAre(0xFFFD, 0xFFFD));
    EXPECT_THAT(rev_runes("\300\200"), ElementsAre(0xFFFD, 0xFFFD));
}

TEST_F(StringTest, Iterator) {
    pn::string_view s = "\1\2\3\4";

    const auto begin = s.begin();
    const auto end   = s.end();
    EXPECT_THAT(begin.offset(), Eq(0));
    EXPECT_THAT(end.offset(), Eq(4));

    auto       it     = begin;
    const auto second = ++it;
    const auto third  = ++it;
    const auto fourth = ++it;
    EXPECT_THAT(*begin, Eq(pn::rune{1}));
    EXPECT_THAT(*second, Eq(pn::rune{2}));
    EXPECT_THAT(*third, Eq(pn::rune{3}));
    EXPECT_THAT(*fourth, Eq(pn::rune{4}));
    EXPECT_THAT(++it, Eq(end));

    EXPECT_THAT(second, Ne(begin));
    EXPECT_THAT(second, Gt(begin));
    EXPECT_THAT(second, Ge(begin));
    EXPECT_THAT(second, Ge(second));
    EXPECT_THAT(second, Eq(second));
    EXPECT_THAT(second, Le(second));
    EXPECT_THAT(second, Le(third));
    EXPECT_THAT(second, Lt(third));
    EXPECT_THAT(second, Ne(third));

    it = begin;
    EXPECT_THAT(it++, Eq(begin));
    EXPECT_THAT(++it, Eq(third));
    EXPECT_THAT(--it, Eq(second));
    EXPECT_THAT(it++, Eq(second));
}

TEST_F(StringTest, ReverseIterator) {
    pn::string_view s = "\4\3\2\1";

    const auto begin = s.rbegin();
    const auto end   = s.rend();
    EXPECT_THAT(begin.offset(), Eq(4));
    EXPECT_THAT(end.offset(), Eq(0));

    auto       it     = begin;
    const auto second = ++it;
    const auto third  = ++it;
    const auto fourth = ++it;
    EXPECT_THAT(*begin, Eq(pn::rune{1}));
    EXPECT_THAT(*second, Eq(pn::rune{2}));
    EXPECT_THAT(*third, Eq(pn::rune{3}));
    EXPECT_THAT(*fourth, Eq(pn::rune{4}));
    EXPECT_THAT(++it, Eq(end));

    EXPECT_THAT(second, Ne(begin));
    EXPECT_THAT(second, Gt(begin));
    EXPECT_THAT(second, Ge(begin));
    EXPECT_THAT(second, Ge(second));
    EXPECT_THAT(second, Eq(second));
    EXPECT_THAT(second, Le(second));
    EXPECT_THAT(second, Le(third));
    EXPECT_THAT(second, Lt(third));
    EXPECT_THAT(second, Ne(third));

    it = begin;
    EXPECT_THAT(it++, Eq(begin));
    EXPECT_THAT(++it, Eq(third));
    EXPECT_THAT(--it, Eq(second));
    EXPECT_THAT(it++, Eq(second));
}

TEST_F(StringTest, Unicode) {
    EXPECT_THAT(pn::string{"Рядок"}, IsString("Рядок"));
    EXPECT_THAT(pn::string{u"Рядок"}, IsString("Рядок"));
    EXPECT_THAT(pn::string{U"Рядок"}, IsString("Рядок"));
    EXPECT_THAT(pn::string{"Рядок"}.cpp_str(), Eq("Рядок"));
    EXPECT_THAT(pn::string{"Рядок"}.cpp_u16str(), Eq(u"Рядок"));
    EXPECT_THAT(pn::string{"Рядок"}.cpp_u32str(), Eq(U"Рядок"));

    EXPECT_THAT(pn::string{"文字列"}, IsString("文字列"));
    EXPECT_THAT(pn::string{u"文字列"}, IsString("文字列"));
    EXPECT_THAT(pn::string{U"文字列"}, IsString("文字列"));
    EXPECT_THAT(pn::string{"文字列"}.cpp_str(), Eq("文字列"));
    EXPECT_THAT(pn::string{"文字列"}.cpp_u16str(), Eq(u"文字列"));
    EXPECT_THAT(pn::string{"文字列"}.cpp_u32str(), Eq(U"文字列"));

    EXPECT_THAT(pn::string{"🧵"}, IsString("🧵"));
    EXPECT_THAT(pn::string{u"🧵"}, IsString("🧵"));
    EXPECT_THAT(pn::string{U"🧵"}, IsString("🧵"));
    EXPECT_THAT(pn::string{"🧵"}.cpp_str(), Eq("🧵"));
    EXPECT_THAT(pn::string{"🧵"}.cpp_u16str(), Eq(u"🧵"));
    EXPECT_THAT(pn::string{"🧵"}.cpp_u32str(), Eq(U"🧵"));

    EXPECT_THAT((pn::string{"\0", 1}), IsString(std::string("\0", 1)));
    EXPECT_THAT((pn::string{u"\0", 1}), IsString(std::string("\0", 1)));
    EXPECT_THAT((pn::string{U"\0", 1}), IsString(std::string("\0", 1)));

    EXPECT_THAT(pn::string{"\177"}, IsString("\177"));
    EXPECT_THAT(pn::string{u"\177"}, IsString("\177"));
    EXPECT_THAT(pn::string{U"\177"}, IsString("\177"));

    EXPECT_THAT(pn::string{"\200"}, IsString("\200"));  // non-validating
    EXPECT_THAT(pn::string{"\377"}, IsString("\377"));  // non-validating

    EXPECT_THAT(pn::string{"\u0080"}, IsString("\302\200"));
    EXPECT_THAT(pn::string{u"\u0080"}, IsString("\302\200"));
    EXPECT_THAT(pn::string{U"\u0080"}, IsString("\302\200"));

    EXPECT_THAT(pn::string{"\u0100"}, IsString("\304\200"));
    EXPECT_THAT(pn::string{u"\u0100"}, IsString("\304\200"));
    EXPECT_THAT(pn::string{U"\u0100"}, IsString("\304\200"));

    EXPECT_THAT(pn::string{"\u07ff"}, IsString("\337\277"));
    EXPECT_THAT(pn::string{u"\u07ff"}, IsString("\337\277"));
    EXPECT_THAT(pn::string{U"\u07ff"}, IsString("\337\277"));

    EXPECT_THAT(pn::string{"\u0800"}, IsString("\340\240\200"));
    EXPECT_THAT(pn::string{u"\u0800"}, IsString("\340\240\200"));
    EXPECT_THAT(pn::string{U"\u0800"}, IsString("\340\240\200"));

    EXPECT_THAT(pn::string{"\ufffd"}, IsString("\357\277\275"));
    EXPECT_THAT(pn::string{u"\ufffd"}, IsString("\357\277\275"));
    EXPECT_THAT(pn::string{U"\ufffd"}, IsString("\357\277\275"));

    EXPECT_THAT(pn::string{"\uffff"}, IsString("\357\277\277"));
    EXPECT_THAT(pn::string{u"\uffff"}, IsString("\357\277\277"));
    EXPECT_THAT(pn::string{U"\uffff"}, IsString("\357\277\277"));

    EXPECT_THAT(pn::string{"\U00010000"}, IsString("\360\220\200\200"));
    EXPECT_THAT(pn::string{u"\U00010000"}, IsString("\360\220\200\200"));
    EXPECT_THAT(pn::string{U"\U00010000"}, IsString("\360\220\200\200"));

    EXPECT_THAT(pn::string{"\U0010ffff"}, IsString("\364\217\277\277"));
    EXPECT_THAT(pn::string{u"\U0010ffff"}, IsString("\364\217\277\277"));
    EXPECT_THAT(pn::string{U"\U0010ffff"}, IsString("\364\217\277\277"));
}

}  // namespace pntest
