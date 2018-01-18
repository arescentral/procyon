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

#include <pn/file>

#include <gmock/gmock.h>
#include <array>
#include <limits>
#include <vector>

#include "./matchers.hpp"

namespace pntest {

using ValueppTest = ::testing::Test;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::InSequence;
using ::testing::Le;
using ::testing::Lt;
using ::testing::MockFunction;
using ::testing::Ne;
using ::testing::Not;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::StrictMock;
using ::testing::TypedEq;

TEST_F(ValueppTest, ScalarConstruction) {
    EXPECT_THAT(pn::value{nullptr}.c_obj()->type, Eq(PN_NULL));
    EXPECT_THAT(pn::value{nullptr}, IsNull());
    EXPECT_THAT(pn::value{nullptr}, Not(IsBool(false)));
    EXPECT_THAT(pn::value{nullptr}, Not(IsInt(0)));
    EXPECT_THAT(pn::value{nullptr}, Not(IsFloat(0.0)));
    EXPECT_THAT(pn::value{nullptr}, Not(IsString("")));
    EXPECT_THAT(pn::value{nullptr}, Not(IsList()));
    EXPECT_THAT(pn::value{nullptr}, Not(IsMap()));

    EXPECT_THAT(pn::value{true}.c_obj()->type, Eq(PN_BOOL));
    EXPECT_THAT(pn::value{true}.c_obj()->b, Eq(true));
    EXPECT_THAT(pn::value{true}, Not(IsNull()));
    EXPECT_THAT(pn::value{true}, IsBool(true));
    EXPECT_THAT(pn::value{true}, Not(IsInt(0)));
    EXPECT_THAT(pn::value{true}, Not(IsFloat(0.0)));
    EXPECT_THAT(pn::value{true}, Not(IsString("")));
    EXPECT_THAT(pn::value{true}, Not(IsList()));
    EXPECT_THAT(pn::value{true}, Not(IsMap()));

    EXPECT_THAT(pn::value{2}.c_obj()->type, Eq(PN_INT));
    EXPECT_THAT(pn::value{2}.c_obj()->i, Eq(2));
    EXPECT_THAT(pn::value{2}, Not(IsNull()));
    EXPECT_THAT(pn::value{2}, Not(IsBool(false)));
    EXPECT_THAT(pn::value{2}, IsInt(2));
    EXPECT_THAT(pn::value{2}, Not(IsFloat(2.2)));
    EXPECT_THAT(pn::value{2}, Not(IsString("")));
    EXPECT_THAT(pn::value{2}, Not(IsList()));
    EXPECT_THAT(pn::value{2}, Not(IsMap()));

    EXPECT_THAT(pn::value{M_PI}.c_obj()->type, Eq(PN_FLOAT));
    EXPECT_THAT(pn::value{M_PI}.c_obj()->f, Eq(M_PI));
    EXPECT_THAT(pn::value{M_PI}, Not(IsNull()));
    EXPECT_THAT(pn::value{M_PI}, Not(IsBool(false)));
    EXPECT_THAT(pn::value{M_PI}, Not(IsInt(0)));
    EXPECT_THAT(pn::value{M_PI}, IsFloat(M_PI));
    EXPECT_THAT(pn::value{M_PI}, Not(IsString("")));
    EXPECT_THAT(pn::value{M_PI}, Not(IsList()));
    EXPECT_THAT(pn::value{M_PI}, Not(IsMap()));
}

TEST_F(ValueppTest, Assignment) {
    pn::value x;
    EXPECT_THAT(x = nullptr, IsNull());

    EXPECT_THAT(x = true, IsBool(true));
    EXPECT_THAT(x = false, IsBool(false));

    EXPECT_THAT(x = 0, IsInt(0));
    EXPECT_THAT(x = INT64_C(0), IsInt(0));

    EXPECT_THAT(x = 0.0, IsFloat(0.0));
    EXPECT_THAT(x = 0.0f, IsFloat(0.0));

    EXPECT_THAT(x = "", IsString(""));
    EXPECT_THAT(x = std::string(""), IsString(""));
}

TEST_F(ValueppTest, IsAs) {
    pn::data  d{(uint8_t[]){0x1f}, 1};
    pn::array a{3};
    pn::map   m{{"3.5", 4}};

    EXPECT_THAT(pn::value{nullptr}.is_null(), Eq(true));
    for (pn::value_cref x : pn::array{true, 1, 1.5, d.copy(), "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_null(), Eq(false));
    }

    EXPECT_THAT(pn::value{false}.is_bool(), Eq(true));
    EXPECT_THAT(pn::value{false}.as_bool(), Eq(false));
    EXPECT_THAT(pn::value{true}.is_bool(), Eq(true));
    EXPECT_THAT(pn::value{true}.as_bool(), Eq(true));
    for (pn::value_cref x : pn::array{nullptr, 1, 1.5, d.copy(), "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_bool(), Eq(false));
        EXPECT_THAT(x.as_bool(), Eq(false));
    }

    EXPECT_THAT(pn::value{1}.is_int(), Eq(true));
    EXPECT_THAT(pn::value{1}.as_int(), Eq(1));
    for (pn::value_cref x : pn::array{nullptr, true, 1.5, d.copy(), "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_int(), Eq(false));
        EXPECT_THAT(x.as_int(), Eq(0));
    }

    EXPECT_THAT(pn::value{1.5}.is_float(), Eq(true));
    EXPECT_THAT(pn::value{1.5}.as_float(), Eq(1.5));
    for (pn::value_cref x : pn::array{nullptr, true, 1, d.copy(), "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_float(), Eq(false));
        EXPECT_THAT(x.as_float(), Eq(0.0));
    }

    EXPECT_THAT(pn::value{1}.is_number(), Eq(true));
    EXPECT_THAT(pn::value{1}.as_number(), Eq(1));
    EXPECT_THAT(pn::value{1.5}.is_number(), Eq(true));
    EXPECT_THAT(pn::value{1.5}.as_number(), Eq(1.5));
    for (pn::value_cref x : pn::array{nullptr, true, d.copy(), "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_number(), Eq(false));
        EXPECT_THAT(x.as_number(), Eq(0));
    }

    EXPECT_THAT(pn::value{d.copy()}.is_data(), Eq(true));
    EXPECT_THAT(pn::value{d.copy()}.as_data().size(), Eq(1));
    EXPECT_THAT(pn::value{d.copy()}.as_data()[0], Eq('\x1f'));
    for (pn::value_cref x : pn::array{nullptr, true, 1, 1.5, "2", a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_data(), Eq(false));
        EXPECT_THAT(x.as_data().size(), Eq(0));
    }

    EXPECT_THAT(pn::value{"2"}.is_string(), Eq(true));
    EXPECT_THAT(pn::value{"2"}.as_string().cpp_str(), Eq("2"));
    for (pn::value_cref x : pn::array{nullptr, true, 1, 1.5, d.copy(), a.copy(), m.copy()}) {
        EXPECT_THAT(x.is_string(), Eq(false));
        EXPECT_THAT(x.as_string().cpp_str(), Eq(""));
    }

    EXPECT_THAT(pn::value{a.copy()}.is_array(), Eq(true));
    EXPECT_THAT(pn::value{a.copy()}.as_array().size(), Eq(1));
    EXPECT_THAT(pn::value{a.copy()}.as_array()[0].as_int(), Eq(3));
    for (pn::value_cref x : pn::array{nullptr, true, 1, 1.5, d.copy(), "2", m.copy()}) {
        EXPECT_THAT(x.is_array(), Eq(false));
        EXPECT_THAT(x.as_array().size(), Eq(0));
    }

    EXPECT_THAT((pn::value{m.copy()}.is_map()), Eq(true));
    EXPECT_THAT((pn::value{m.copy()}.as_map().size()), Eq(1));
    EXPECT_THAT((pn::value{m.copy()}.as_map().get("3.5")).as_int(), Eq(4));
    for (pn::value_cref x : pn::array{nullptr, true, 1, 1.5, d.copy(), "2", a.copy()}) {
        EXPECT_THAT(x.is_map(), Eq(false));
        EXPECT_THAT(x.as_map().size(), Eq(0));
    }
}

TEST_F(ValueppTest, List) {
    pn::array empty;
    EXPECT_THAT(pn::value(empty.copy()), IsList());
    EXPECT_THAT(empty.size(), Eq(0));
    EXPECT_THAT(empty.empty(), Eq(true));

    pn::array a{nullptr, true, 2, M_PI, "four"};
    EXPECT_THAT(pn::value(a.copy()), IsList(nullptr, true, 2, M_PI, "four"));
    EXPECT_THAT(a.size(), Eq(5));
    EXPECT_THAT(a.empty(), Eq(false));
    EXPECT_THAT(a.front(), IsNull());
    EXPECT_THAT(a[0], IsNull());
    EXPECT_THAT(a[1], IsBool(true));
    EXPECT_THAT(a[2], IsInt(2));
    EXPECT_THAT(a[3], IsFloat(M_PI));
    EXPECT_THAT(a[4], IsString("four"));
    EXPECT_THAT(a.back(), IsString("four"));

    StrictMock<MockFunction<void(pn::value_cref)>> fn;
    {
        InSequence s;
        EXPECT_CALL(fn, Call(IsNull()));
        EXPECT_CALL(fn, Call(IsBool(true)));
        EXPECT_CALL(fn, Call(IsInt(2)));
        EXPECT_CALL(fn, Call(IsFloat(M_PI)));
        EXPECT_CALL(fn, Call(IsString("four")));
    }
    for (const auto& x : a) {
        fn.Call(x);
    }
}

TEST_F(ValueppTest, Map) {
    pn::map empty;
    EXPECT_THAT(pn::value(empty.copy()), IsMap());
    EXPECT_THAT(empty.size(), Eq(0));
    EXPECT_THAT(empty.empty(), Eq(true));

    pn::map m{{"0", nullptr}, {"1", true}, {"2", 2}, {"3", M_PI}, {"4", "four"}};
    EXPECT_THAT(
            pn::value(m.copy()), IsMap("0", nullptr, "1", true, "2", 2, "3", M_PI, "4", "four"));
    EXPECT_THAT(m.size(), Eq(5));
    EXPECT_THAT(m.empty(), Eq(false));
    EXPECT_THAT(m["0"], IsNull());
    EXPECT_THAT(m["1"], IsBool(true));
    EXPECT_THAT(m["2"], IsInt(2));
    EXPECT_THAT(m["3"], IsFloat(M_PI));
    EXPECT_THAT(m["4"], IsString("four"));

    StrictMock<MockFunction<void(pn::string_view, pn::value_cref)>> fn;
    {
        InSequence s;
        EXPECT_CALL(fn, Call(IsString("0"), IsNull()));
        EXPECT_CALL(fn, Call(IsString("1"), IsBool(true)));
        EXPECT_CALL(fn, Call(IsString("2"), IsInt(2)));
        EXPECT_CALL(fn, Call(IsString("3"), IsFloat(M_PI)));
        EXPECT_CALL(fn, Call(IsString("4"), IsString("four")));
    }
    for (const auto& x : m) {
        fn.Call(x.key(), x.value());
    }
}

TEST_F(ValueppTest, Nested) {
    // * name: "Jane Doe"
    //   number: "555-1234"
    //   born: [1980, 10, 10]
    // * name: "John Doe"
    //   number: "555-6789"
    //   born: [1981, 2, 3]

    auto matches =
            IsList(IsMap("name", "Jane Doe", "number", "555-1234", "born", IsList(1980, 10, 10)),
                   IsMap("name", "John Doe", "number", "555-6789", "born", IsList(1981, 2, 3)));

    {
        pn::array born{1980, 10, 10};
        pn::map   jane{{"name", "Jane Doe"}, {"number", "555-1234"}, {"born", std::move(born)}};
        born = pn::array{1981, 2, 3};
        pn::map   john{{"name", "John Doe"}, {"number", "555-6789"}, {"born", std::move(born)}};
        pn::array x{std::move(jane), std::move(john)};
        EXPECT_THAT(x, matches);
    }

    {
        pn::value x;
        x.to_array().push_back(nullptr);
        pn::value_ref jane      = x.to_array().back();
        jane.to_map()["name"]   = "Jane Doe";
        jane.to_map()["number"] = "555-1234";
        jane.to_map()["born"]   = pn::array{1980, 10, 10};
        x.to_array().push_back(nullptr);
        pn::value_ref john      = x.to_array().back();
        john.to_map()["name"]   = "John Doe";
        john.to_map()["number"] = "555-6789";
        john.to_map()["born"]   = pn::array{1981, 2, 3};
        EXPECT_THAT(x, matches);
    }

    EXPECT_THAT(
            pn::value(pn::array{pn::map{{"name", "Jane Doe"},
                                        {"number", "555-1234"},
                                        {"born", pn::array{1980, 10, 10}}},
                                pn::map{{"name", "John Doe"},
                                        {"number", "555-6789"},
                                        {"born", pn::array{1981, 2, 3}}}}),
            matches);

    pn::value orig = pn::array{pn::map{{"name", "Jane Doe"},
                                       {"number", "555-1234"},
                                       {"born", pn::array{1980, 10, 10}}},
                               pn::map{{"name", "John Doe"},
                                       {"number", "555-6789"},
                                       {"born", pn::array{1981, 2, 3}}}};
    pn::value copy = orig.copy();
    orig           = nullptr;
    EXPECT_THAT(copy, matches);
}

TEST_F(ValueppTest, String) {
    pn::string x{""};
    EXPECT_THAT(pn::value(x.copy()), IsString(""));
    x.append(nullptr, 0);
    x += "";
    EXPECT_THAT(pn::value(x.copy()), IsString(""));
    x += "a";
    EXPECT_THAT(pn::value(x.copy()), IsString("a"));
    x += "bcdefghi";
    EXPECT_THAT(pn::value(x.copy()), IsString("abcdefghi"));
    x += pn::string{"jklmnopqrstuvwxyz"};
    EXPECT_THAT(pn::value(x.copy()), IsString("abcdefghijklmnopqrstuvwxyz"));
    x += std::string("\0\0\0\0", 4);
    EXPECT_THAT(
            pn::value(x.copy()), IsString(std::string("abcdefghijklmnopqrstuvwxyz\0\0\0\0", 30)));

    x = "";
    EXPECT_THAT(pn::value(x.copy()), IsString(""));
}

TEST_F(ValueppTest, ListModify) {
    pn::array x{1, 2, 3};
    EXPECT_THAT(x, IsList(1, 2, 3));

    x.push_back(4);
    std::array<int64_t, 2> a1 = {{5, 6}};
    std::vector<int>       a2{7, 8, 9, 10};
    std::copy(a1.begin(), a1.end(), std::back_inserter(x));
    std::copy(a2.begin(), a2.end(), std::back_inserter(x));
    EXPECT_THAT(x, IsList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    x.erase(x.begin() + 9);  // 1 2 3 4 5 6 7 8 9
    x.erase(x.begin() + 7);  // 1 2 3 4 5 6 7 9
    x.erase(x.begin() + 1);  // 1 3 4 5 6 7 9
    x.erase(x.begin() + 2);  // 1 3 5 6 7 9
    x.erase(x.begin() + 3);  // 1 3 5 7 9
    EXPECT_THAT(x, IsList(1, 3, 5, 7, 9));

    x.insert(x.begin() + 1, 1);           // 1 1 3 5 7 9
    x.insert(x.begin() + 2, 2);           // 1 1 2 3 5 7 9
    x.erase(x.begin() + 5);               // 1 1 2 3 5 9
    x.insert(x.begin() + 5, 8);           // 1 1 2 3 5 8 9
    EXPECT_THAT(x.pop_back(), IsInt(9));  // 1 1 2 3 5 8
    EXPECT_THAT(x, IsList(1, 1, 2, 3, 5, 8));
}

TEST_F(ValueppTest, MapModify) {
    pn::map x{{"one", 1}, {"two", 2}, {"three", 3}};
    EXPECT_THAT(x, IsMap("one", 1, "two", 2, "three", 3));
    EXPECT_THAT(x, Not(IsMap("one", 1, "three", 3, "two", 2)));
    EXPECT_THAT(x.has("two"), Eq(true));
    EXPECT_THAT(x.has("four"), Eq(false));

    x.set("four", 4);
    EXPECT_THAT(x, IsMap("one", 1, "two", 2, "three", 3, "four", 4));
    EXPECT_THAT(x.has("two"), Eq(true));
    EXPECT_THAT(x.has("four"), Eq(true));

    x.del("two");
    EXPECT_THAT(x, IsMap("one", 1, "three", 3, "four", 4));
    EXPECT_THAT(x.has("two"), Eq(false));
    EXPECT_THAT(x.has("four"), Eq(true));

    pn::value one;
    EXPECT_THAT(x.pop("one", one), Eq(true));
    EXPECT_THAT(one, IsInt(1));
    EXPECT_THAT(x.size(), Eq(2));
    EXPECT_THAT(x.has("one"), Eq(false));
    EXPECT_THAT(x.has("two"), Eq(false));
    EXPECT_THAT(x.has("three"), Eq(true));
    EXPECT_THAT(x.has("four"), Eq(true));

    x.clear();
    EXPECT_THAT(x, IsMap());
    EXPECT_THAT(x.size(), Eq(0));
    EXPECT_THAT(x.has("one"), Eq(false));
    EXPECT_THAT(x.has("two"), Eq(false));
    EXPECT_THAT(x.has("three"), Eq(false));
    EXPECT_THAT(x.has("four"), Eq(false));
}

TEST_F(ValueppTest, Partition) {
    pn::string_view s = "http://arescentral.org/antares/contributing/";

    pn::string_view scheme;
    ASSERT_THAT(partition(scheme, "//", s), Eq(true));
    EXPECT_THAT(scheme, Eq("http:"));
    EXPECT_THAT(s, Eq("arescentral.org/antares/contributing/"));

    pn::string_view host;
    ASSERT_THAT(partition(host, "/", s), Eq(true));
    EXPECT_THAT(host, Eq("arescentral.org"));
    EXPECT_THAT(s, Eq("antares/contributing/"));

    pn::string_view path;
    ASSERT_THAT(partition(path, "#", s), Eq(false));
    EXPECT_THAT(path, Eq("antares/contributing/"));
    EXPECT_THAT(s, Eq(""));
}

TEST_F(ValueppTest, Replace) {
    pn::string s{"0123456789"};
    s.replace(0, 2, "");
    EXPECT_THAT(s, Eq(pn::string_view{"23456789"}));
    s.replace(4, 4, "abcdef");
    EXPECT_THAT(s, Eq(pn::string_view{"2345abcdef"}));
    s.replace(0, 10, "!");
    EXPECT_THAT(s, Eq(pn::string_view{"!"}));
}

TEST_F(ValueppTest, Rune) {
    EXPECT_THAT(pn::rune{}, Eq(pn::string_view{"\000", 1}));
    EXPECT_THAT(pn::rune{0}, Eq(pn::string_view{"\000", 1}));
    EXPECT_THAT(pn::rune{1}, Eq(pn::string_view{"\001"}));
    EXPECT_THAT(pn::rune{0x7f}, Eq(pn::string_view{"\177"}));
    EXPECT_THAT(pn::rune{0x80}, Eq(pn::string_view{"\302\200"}));
    EXPECT_THAT(pn::rune{0x7ff}, Eq(pn::string_view{"\337\277"}));
    EXPECT_THAT(pn::rune{0x800}, Eq(pn::string_view{"\340\240\200"}));
    EXPECT_THAT(pn::rune{0xd7ff}, Eq(pn::string_view{"\355\237\277"}));
    EXPECT_THAT(pn::rune{0xe000}, Eq(pn::string_view{"\356\200\200"}));
    EXPECT_THAT(pn::rune{0xffff}, Eq(pn::string_view{"\357\277\277"}));
    EXPECT_THAT(pn::rune{0x10000}, Eq(pn::string_view{"\360\220\200\200"}));
    EXPECT_THAT(pn::rune{0x10ffff}, Eq(pn::string_view{"\364\217\277\277"}));

    EXPECT_THAT(pn::rune{0xd800}, Eq(pn::string_view{"\357\277\275"}));
    EXPECT_THAT(pn::rune{0xdfff}, Eq(pn::string_view{"\357\277\275"}));
    EXPECT_THAT(pn::rune{0x110000}, Eq(pn::string_view{"\357\277\275"}));
    EXPECT_THAT(pn::rune{0xffffffffu}, Eq(pn::string_view{"\357\277\275"}));
}

TEST_F(ValueppTest, StrCompare) {
    EXPECT_THAT(pn::string{"b"}, Eq(pn::string_view{"b"}));
    EXPECT_THAT(pn::string{"b"}, Ne(pn::string_view{"x"}));
    EXPECT_THAT(pn::string{"b"}, Lt(pn::string_view{"c"}));
    EXPECT_THAT(pn::string{"b"}, Le(pn::string_view{"b"}));
    EXPECT_THAT(pn::string{"b"}, Gt(pn::string_view{"a"}));
    EXPECT_THAT(pn::string{"b"}, Ge(pn::string_view{"b"}));
}

}  // namespace pntest
