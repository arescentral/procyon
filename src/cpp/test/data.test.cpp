// Copyright 2019 The Procyon Authors
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
#include <pn/data>

#include "../src/utf8.h"
#include "./matchers.hpp"

using DataTest = ::testing::Test;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Ne;

namespace pntest {

TEST_F(DataTest, Iterator) {
    pn::string_view s = "\1\2\3\4";
    pn::data_view   d = s.as_data();

    const auto begin = d.begin();
    const auto end   = d.end();
    EXPECT_THAT(end - begin, Eq(4));

    const auto second = begin + 1;
    const auto third  = end - 2;
    const auto fourth = begin + 3;
    EXPECT_THAT(*begin, Eq(1));
    EXPECT_THAT(*second, Eq(2));
    EXPECT_THAT(*third, Eq(3));
    EXPECT_THAT(*fourth, Eq(4));

    EXPECT_THAT(second, Ne(begin));
    EXPECT_THAT(second, Gt(begin));
    EXPECT_THAT(second, Ge(begin));
    EXPECT_THAT(second, Ge(second));
    EXPECT_THAT(second, Eq(second));
    EXPECT_THAT(second, Le(second));
    EXPECT_THAT(second, Le(third));
    EXPECT_THAT(second, Lt(third));
    EXPECT_THAT(second, Ne(third));

    auto it = begin;
    EXPECT_THAT(it++, Eq(begin));
    EXPECT_THAT(++it, Eq(third));
    EXPECT_THAT(--it, Eq(second));
    EXPECT_THAT(it++, Eq(second));
    EXPECT_THAT(it += 2, Eq(end));
    EXPECT_THAT(it -= 4, Eq(begin));
}

TEST_F(DataTest, ReverseIterator) {
    pn::string_view s = "\4\3\2\1";
    pn::data_view   d = s.as_data();

    const auto begin = d.rbegin();
    const auto end   = d.rend();
    EXPECT_THAT(end - begin, Eq(4));

    const auto second = begin + 1;
    const auto third  = end - 2;
    const auto fourth = begin + 3;
    EXPECT_THAT(*begin, Eq(1));
    EXPECT_THAT(*second, Eq(2));
    EXPECT_THAT(*third, Eq(3));
    EXPECT_THAT(*fourth, Eq(4));

    EXPECT_THAT(second, Ne(begin));
    EXPECT_THAT(second, Gt(begin));
    EXPECT_THAT(second, Ge(begin));
    EXPECT_THAT(second, Ge(second));
    EXPECT_THAT(second, Eq(second));
    EXPECT_THAT(second, Le(second));
    EXPECT_THAT(second, Le(third));
    EXPECT_THAT(second, Lt(third));
    EXPECT_THAT(second, Ne(third));

    auto it = begin;
    EXPECT_THAT(it++, Eq(begin));
    EXPECT_THAT(++it, Eq(third));
    EXPECT_THAT(--it, Eq(second));
    EXPECT_THAT(it++, Eq(second));
    EXPECT_THAT(it += 2, Eq(end));
    EXPECT_THAT(it -= 4, Eq(begin));
}

}  // namespace pntest
