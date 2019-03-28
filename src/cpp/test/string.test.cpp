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

#include "../src/utf8.h"
#include "./matchers.hpp"

using StringTest = ::testing::Test;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ne;
using ::testing::Lt;
using ::testing::Le;
using ::testing::Gt;
using ::testing::Ge;

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

}  // namespace pntest
