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

using FormatTest = ::testing::Test;
using ::testing::Eq;

namespace pntest {

TEST_F(FormatTest, BracketsOnly) {
    EXPECT_THAT(pn::format(""), IsString(""));
    EXPECT_THAT(pn::format("{"), IsString("{"));
    EXPECT_THAT(pn::format("{{"), IsString("{"));
    EXPECT_THAT(pn::format("}"), IsString("}"));
    EXPECT_THAT(pn::format("}}"), IsString("}"));
    EXPECT_THAT(pn::format("{}"), IsString("null"));
    EXPECT_THAT(pn::format("}{"), IsString("}{"));
}

TEST_F(FormatTest, Invalid) {
    EXPECT_THAT(pn::format("{0"), IsString("{0"));
    EXPECT_THAT(pn::format("{unclosed"), IsString("{unclosed"));
    EXPECT_THAT(pn::format("{non-number}"), IsString("{non-number}"));

    EXPECT_THAT(pn::format("{-1}"), IsString("{-1}"));
}

TEST_F(FormatTest, Basic) {
    static constexpr double inf = std::numeric_limits<double>::infinity();
    static constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_THAT(pn::format("format"), IsString("format"));
    EXPECT_THAT(pn::format("format: {0}", nullptr), IsString("format: null"));
    EXPECT_THAT(pn::format("format: {0}", false), IsString("format: false"));
    EXPECT_THAT(pn::format("format: {0}", true), IsString("format: true"));
    EXPECT_THAT(pn::format("format: {0}", inf), IsString("format: inf"));
    EXPECT_THAT(pn::format("format: {0}", -inf), IsString("format: -inf"));
    EXPECT_THAT(pn::format("format: {0}", nan), IsString("format: nan"));
}

TEST_F(FormatTest, ImplicitPosition) {
    EXPECT_THAT(pn::format("{}"), IsString("null"));

    EXPECT_THAT(pn::format("{}", 0), IsString("0"));
    EXPECT_THAT(pn::format("{} {}", 0, 1), IsString("0 1"));
    EXPECT_THAT(pn::format("{} {} {}", 0, 1, 2), IsString("0 1 2"));
    EXPECT_THAT(pn::format("{} {} {} {}", 0, 1), IsString("0 1 1 1"));

    EXPECT_THAT(pn::format("{2} {0} {}", 0, 1, 2), IsString("2 0 1"));
    EXPECT_THAT(pn::format("{0} {2} {}", 0, 1, 2), IsString("0 2 2"));

    EXPECT_THAT(pn::format("{0} {}"), IsString("null null"));
    EXPECT_THAT(pn::format("{0} {1} {}", 0), IsString("0 null 0"));
    EXPECT_THAT(pn::format("{0} {3} {}", 0, 1, 2), IsString("0 null 2"));
}

TEST_F(FormatTest, Scalar) {
    EXPECT_THAT(pn::format("format: {0}", '!'), IsString("format: !"));
    EXPECT_THAT(pn::format("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", 1.0), IsString("format: 1.0"));

    EXPECT_THAT(pn::format("format: {0}", static_cast<int8_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<uint8_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<int16_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<uint16_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<int32_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<uint32_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<int64_t>(1)), IsString("format: 1"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<uint64_t>(1)), IsString("format: 1"));

    EXPECT_THAT(pn::format("format: {0}", static_cast<float>(1.0)), IsString("format: 1.0"));
    EXPECT_THAT(pn::format("format: {0}", static_cast<double>(1.0)), IsString("format: 1.0"));

    EXPECT_THAT(pn::format<int8_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<uint8_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<int16_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<uint16_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<int32_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<uint32_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<int64_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<uint64_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<intptr_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<uintptr_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<size_t>("format: {0}", 1), IsString("format: 1"));
    EXPECT_THAT(pn::format<ssize_t>("format: {0}", 1), IsString("format: 1"));

    EXPECT_THAT(pn::format<float>("format: {0}", 1.0), IsString("format: 1.0"));
    EXPECT_THAT(pn::format<double>("format: {0}", 1.0), IsString("format: 1.0"));
}

TEST_F(FormatTest, Vector) {
    pn::data d{reinterpret_cast<const uint8_t*>("\xff"), 1};
    EXPECT_THAT(pn::format("format: {0}", d), IsString("format: $ff"));
    EXPECT_THAT(pn::format("format: {0}", pn::data_ref{d}), IsString("format: $ff"));
    EXPECT_THAT(pn::format("format: {0}", pn::data_view{d}), IsString("format: $ff"));
    EXPECT_THAT(pn::format("format: {0}", pn::value{d.copy()}), IsString("format: $ff"));
    EXPECT_THAT(pn::format("format: {0}", pn::value_cref{d.copy()}), IsString("format: $ff"));

    pn::string s{"#"};
    EXPECT_THAT(pn::format("format: {0}", s), IsString("format: #"));
    EXPECT_THAT(pn::format("format: {0}", s.c_str()), IsString("format: #"));
    EXPECT_THAT(pn::format("format: {0}", std::string{s.c_str()}), IsString("format: #"));
    EXPECT_THAT(pn::format("format: {0}", pn::string_view{s}), IsString("format: #"));
    EXPECT_THAT(pn::format("format: {0}", pn::value{s.copy()}), IsString("format: #"));
    EXPECT_THAT(pn::format("format: {0}", pn::value_cref{s.copy()}), IsString("format: #"));

    pn::array a;
    EXPECT_THAT(pn::format("format: {0}", a), IsString("format: []"));
    EXPECT_THAT(pn::format("format: {0}", pn::array_ref{a}), IsString("format: []"));
    EXPECT_THAT(pn::format("format: {0}", pn::array_cref{a}), IsString("format: []"));
    EXPECT_THAT(pn::format("format: {0}", pn::value{a.copy()}), IsString("format: []"));
    EXPECT_THAT(pn::format("format: {0}", pn::value_cref{a.copy()}), IsString("format: []"));

    pn::map m;
    EXPECT_THAT(pn::format("format: {0}", m), IsString("format: {}"));
    EXPECT_THAT(pn::format("format: {0}", pn::map_ref{m}), IsString("format: {}"));
    EXPECT_THAT(pn::format("format: {0}", pn::map_cref{m}), IsString("format: {}"));
    EXPECT_THAT(pn::format("format: {0}", pn::value{m.copy()}), IsString("format: {}"));
    EXPECT_THAT(pn::format("format: {0}", pn::value_cref{m.copy()}), IsString("format: {}"));
}

TEST_F(FormatTest, Indexed) {
    EXPECT_THAT(
            pn::format("The {} of {} is bald", "king", "France"),
            IsString("The king of France is bald"));
    EXPECT_THAT(
            pn::format("The {0} of {1} is bald", "king", "France"),
            IsString("The king of France is bald"));
    EXPECT_THAT(
            pn::format("The {1} of {0} is bald", "France", "king"),
            IsString("The king of France is bald"));

    EXPECT_THAT(
            pn::format("The {0} of {1} is bald", "king"), IsString("The king of null is bald"));
    EXPECT_THAT(pn::format("The {0} of {1} is bald"), IsString("The null of null is bald"));
}

TEST_F(FormatTest, ArraySubscript) {
    EXPECT_THAT(
            pn::format("The {0[0]} of {0[1]} is bald", pn::array{"king", "France"}),
            IsString("The king of France is bald"));
    EXPECT_THAT(
            pn::format("The {[0]} of {[1]} is bald", pn::array{"king", "France"}),
            IsString("The king of France is bald"));
    EXPECT_THAT(
            pn::format("The {[0]} of {[1]} is bald", pn::value_cref{pn::array{"king", "France"}}),
            IsString("The king of France is bald"));

    EXPECT_THAT(
            pn::format("The {[0]} of {[1]} is bald", pn::array{"king"}),
            IsString("The king of null is bald"));
    EXPECT_THAT(
            pn::format("The {[0]} of {[1]} is bald", pn::array{}),
            IsString("The null of null is bald"));
    EXPECT_THAT(
            pn::format("The {[0]} of {[1]} is bald", ":)"), IsString("The null of null is bald"));

    EXPECT_THAT(
            pn::format("{[0]} {[0][0]} {[1][0]} {[1][1]}", pn::array{0, pn::array{10, 11}}),
            IsString("0 null 10 11"));

    EXPECT_THAT(pn::format("{[one]}", pn::array{0}), IsString("null"));
}

TEST_F(FormatTest, MapSubscript) {
    pn::map m{{"title", "Prof."}, {"given", "Otto"}, {"family", "Lidenbrock"}};
    EXPECT_THAT(pn::format("{[title]} {[family]}", m), IsString("Prof. Lidenbrock"));
    EXPECT_THAT(
            pn::format("{[title]} {[family]}", pn::value_cref{m.copy()}),
            IsString("Prof. Lidenbrock"));
    EXPECT_THAT(
            pn::format("{[name][title]} {[name][family]}", pn::map{{"name", m.copy()}}),
            IsString("Prof. Lidenbrock"));

    EXPECT_THAT(pn::format("{[title]} {[family]}", pn::map{}), IsString("null null"));
    EXPECT_THAT(pn::format("{[title]} {[family]}", nullptr), IsString("null null"));
}

}  // namespace pntest
