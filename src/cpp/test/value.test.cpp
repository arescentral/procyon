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
#include <limits>

#include "./matchers.hpp"

using ValueTest = ::testing::Test;
using ::testing::Eq;
using ::testing::Not;

namespace pntest {

TEST_F(ValueTest, Predefined) {
    EXPECT_THAT(pn_null, IsNull());
    EXPECT_THAT(pn_null, Not(IsBool(false)));
    EXPECT_THAT(pn_null, Not(IsInt(0)));
    EXPECT_THAT(pn_null, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_null, Not(IsData({})));
    EXPECT_THAT(pn_null, Not(IsString("")));
    EXPECT_THAT(pn_null, Not(IsList()));
    EXPECT_THAT(pn_null, Not(IsMap()));

    EXPECT_THAT(pn_false, Not(IsNull()));
    EXPECT_THAT(pn_false, IsBool(false));
    EXPECT_THAT(pn_false, Not(IsInt(0)));
    EXPECT_THAT(pn_false, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_false, Not(IsData({})));
    EXPECT_THAT(pn_false, Not(IsString("")));
    EXPECT_THAT(pn_false, Not(IsList()));
    EXPECT_THAT(pn_false, Not(IsMap()));

    EXPECT_THAT(pn_zero, Not(IsNull()));
    EXPECT_THAT(pn_zero, Not(IsBool(false)));
    EXPECT_THAT(pn_zero, IsInt(0));
    EXPECT_THAT(pn_zero, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_zero, Not(IsData({})));
    EXPECT_THAT(pn_zero, Not(IsString("")));
    EXPECT_THAT(pn_zero, Not(IsList()));
    EXPECT_THAT(pn_zero, Not(IsMap()));

    EXPECT_THAT(pn_zerof, Not(IsNull()));
    EXPECT_THAT(pn_zerof, Not(IsBool(false)));
    EXPECT_THAT(pn_zerof, Not(IsInt(0)));
    EXPECT_THAT(pn_zerof, IsFloat(0.0));
    EXPECT_THAT(pn_zerof, Not(IsData({})));
    EXPECT_THAT(pn_zerof, Not(IsString("")));
    EXPECT_THAT(pn_zerof, Not(IsList()));
    EXPECT_THAT(pn_zerof, Not(IsMap()));

    EXPECT_THAT(pn_dataempty, Not(IsNull()));
    EXPECT_THAT(pn_dataempty, Not(IsBool(false)));
    EXPECT_THAT(pn_dataempty, Not(IsInt(0)));
    EXPECT_THAT(pn_dataempty, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_dataempty, IsData({}));
    EXPECT_THAT(pn_dataempty, Not(IsString("")));
    EXPECT_THAT(pn_dataempty, Not(IsList()));
    EXPECT_THAT(pn_dataempty, Not(IsMap()));

    EXPECT_THAT(pn_strempty, Not(IsNull()));
    EXPECT_THAT(pn_strempty, Not(IsBool(false)));
    EXPECT_THAT(pn_strempty, Not(IsInt(0)));
    EXPECT_THAT(pn_strempty, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_strempty, Not(IsData({})));
    EXPECT_THAT(pn_strempty, IsString(""));
    EXPECT_THAT(pn_strempty, Not(IsList()));
    EXPECT_THAT(pn_strempty, Not(IsMap()));

    EXPECT_THAT(pn_arrayempty, Not(IsNull()));
    EXPECT_THAT(pn_arrayempty, Not(IsBool(false)));
    EXPECT_THAT(pn_arrayempty, Not(IsInt(0)));
    EXPECT_THAT(pn_arrayempty, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_arrayempty, Not(IsData({})));
    EXPECT_THAT(pn_arrayempty, Not(IsString("")));
    EXPECT_THAT(pn_arrayempty, IsList());
    EXPECT_THAT(pn_arrayempty, Not(IsMap()));

    EXPECT_THAT(pn_mapempty, Not(IsNull()));
    EXPECT_THAT(pn_mapempty, Not(IsBool(false)));
    EXPECT_THAT(pn_mapempty, Not(IsInt(0)));
    EXPECT_THAT(pn_mapempty, Not(IsFloat(0.0)));
    EXPECT_THAT(pn_mapempty, Not(IsData({})));
    EXPECT_THAT(pn_mapempty, Not(IsString("")));
    EXPECT_THAT(pn_mapempty, Not(IsList()));
    EXPECT_THAT(pn_mapempty, IsMap());

    EXPECT_THAT(pn_true, IsBool(true));
    EXPECT_THAT(pn_inf, IsFloat(std::numeric_limits<double>::infinity()));
    EXPECT_THAT(pn_neg_inf, IsFloat(-std::numeric_limits<double>::infinity()));
    EXPECT_THAT(pn_nan, IsNan());
}

TEST_F(ValueTest, Set) {
    EXPECT_THAT(set('n'), IsNull());

    {
        pn_value_t* out = nullptr;
        pn::value   x   = set('N', &out);
        EXPECT_THAT(x, IsNull());
        EXPECT_THAT(out, Eq(x.c_obj()));
    }

    EXPECT_THAT(set('?', false), IsBool(false));
    EXPECT_THAT(set('?', true), IsBool(true));

    EXPECT_THAT(set<int>('i', (1)), IsInt(1));
    EXPECT_THAT(set<unsigned>('I', 2), IsInt(2));
    EXPECT_THAT(set<int8_t>('b', 3), IsInt(3));
    EXPECT_THAT(set<uint8_t>('B', 4), IsInt(4));
    EXPECT_THAT(set<int64_t>('h', 5), IsInt(5));
    EXPECT_THAT(set<uint16_t>('H', 6), IsInt(6));
    EXPECT_THAT(set<int32_t>('l', 7), IsInt(7));
    EXPECT_THAT(set<uint32_t>('L', 8), IsInt(8));
    EXPECT_THAT(set<int64_t>('q', 9), IsInt(9));
    EXPECT_THAT(set<uint64_t>('Q', 10), IsInt(10));
    EXPECT_THAT(set<intptr_t>('p', 11), IsInt(11));
    EXPECT_THAT(set<uintptr_t>('P', 12), IsInt(12));
    EXPECT_THAT(set<size_t>('z', 13), IsInt(13));
    EXPECT_THAT(set<ssize_t>('Z', 14), IsInt(14));

    EXPECT_THAT(set<float>('f', M_PI), IsFloat(static_cast<float>(M_PI)));
    EXPECT_THAT(set<double>('d', M_PI), IsFloat(static_cast<double>(M_PI)));

    EXPECT_THAT(set('c', 'c'), IsString("c"));
    EXPECT_THAT(set('C', 'c'), IsString("c"));
    EXPECT_THAT(set('s', "string"), IsString("string"));

    {
        pn_value_t source;
        pn_set(&source, 's', "source");
        pn::value x = set('x', &source);
        EXPECT_THAT(x, IsString("source"));
        EXPECT_THAT(source, IsString("source"));
        EXPECT_THAT(source.s, Not(Eq(x.c_obj()->s)));

        EXPECT_THAT(set('X', &source), IsString("source"));
        EXPECT_THAT(source, IsNull());
    }
}

TEST_F(ValueTest, Char) {
    auto IsInvalid = IsString("\357\277\275");

    EXPECT_THAT(set('c', '\0'), IsString(std::string("\0", 1)));
    EXPECT_THAT(set('c', '\1'), IsString("\1"));
    EXPECT_THAT(set('c', 'c'), IsString("c"));
    EXPECT_THAT(set('c', '~'), IsString("~"));
    EXPECT_THAT(set('c', '\177'), IsString("\177"));
    EXPECT_THAT(set('c', '\200'), IsInvalid);
    EXPECT_THAT(set('c', '\377'), IsInvalid);

    EXPECT_THAT(set('C', '\0'), IsString(std::string("\0", 1)));
    EXPECT_THAT(set('C', '\1'), IsString("\1"));
    EXPECT_THAT(set('C', 'c'), IsString("c"));
    EXPECT_THAT(set('C', '~'), IsString("~"));
    EXPECT_THAT(set('C', '\177'), IsString("\177"));  // DELETE
    EXPECT_THAT(set('C', 0x80), IsString("\302\200"));
    EXPECT_THAT(set('C', 0xbd), IsString("\302\275"));   // VULGAR FRACTION ONE HALF
    EXPECT_THAT(set('C', 0x3c0), IsString("\317\200"));  // GREEK SMALL LETTER PI
    EXPECT_THAT(set('C', 0x7ff), IsString("\337\277"));
    EXPECT_THAT(set('C', 0x800), IsString("\340\240\200"));
    EXPECT_THAT(set('C', 0x2191), IsString("\342\206\221"));  // UPWARDS ARROW
    EXPECT_THAT(set('C', 0x306d), IsString("\343\201\255"));  // HIRAGANA LETTER NE
    EXPECT_THAT(set('C', 0xd7ff), IsString("\355\237\277"));
    EXPECT_THAT(set('C', 0xd800), IsInvalid);  // Surrogate code point
    EXPECT_THAT(set('C', 0xdfff), IsInvalid);  // Surrogate code point
    EXPECT_THAT(set('C', 0xe000), IsString("\356\200\200"));
    EXPECT_THAT(set('C', 0xfffd), IsInvalid);  // REPLACEMENT CHARACTER
    EXPECT_THAT(set('C', 0xffff), IsString("\357\277\277"));
    EXPECT_THAT(set('C', 0x10000), IsString("\360\220\200\200"));
    EXPECT_THAT(set('C', 0x1f6e4), IsString("\360\237\233\244"));  // RAILWAY TRACK
    EXPECT_THAT(set('C', 0x100000), IsString("\364\200\200\200"));
    EXPECT_THAT(set('C', 0x10ffff), IsString("\364\217\277\277"));
    EXPECT_THAT(set('C', 0x110000), IsInvalid);
    EXPECT_THAT(set('C', 0xffffffffu), IsInvalid);
}

TEST_F(ValueTest, Setv) {
    pn_value_t x;

    EXPECT_THAT(setv(""), IsList());
    pn_clear(&x);

    EXPECT_THAT(setv("n?ifs", true, 2, M_PI, "four"), IsList(nullptr, true, 2, M_PI, "four"));
    pn_clear(&x);
}

TEST_F(ValueTest, Setkv) {
    EXPECT_THAT(
            setkv("sns?sisfss", "0", "1", true, "2", 2, "3", M_PI, "4", "four"),
            IsMap("0", nullptr, "1", true, "2", 2, "3", M_PI, "4", "four"));
}

TEST_F(ValueTest, Nested) {
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
        pn::value born = setv("iii", 1980, 10, 10);
        pn::value jane =
                setkv("sssssX", "name", "Jane Doe", "number", "555-1234", "born", born.c_obj());
        born = setv("iii", 1981, 2, 3);
        pn::value john =
                setkv("sssssX", "name", "John Doe", "number", "555-6789", "born", born.c_obj());
        pn::value x = setv("XX", jane.c_obj(), john.c_obj());
        EXPECT_THAT(x, matches);
    }

    pn::value x;
    {
        pn_value_t* jane;
        pn_value_t* john;
        pn_value_t* born;

        x = setv("NN", &jane, &john);
        pn_setkv(jane, "sssssN", "name", "Jane Doe", "number", "555-1234", "born", &born);
        pn_setv(born, "iii", 1980, 10, 10);
        pn_setkv(john, "sssssN", "name", "John Doe", "number", "555-6789", "born", &born);
        pn_setv(born, "iii", 1981, 2, 3);
        EXPECT_THAT(x, matches);
    }

    pn::value copy;
    pn_set(copy.c_obj(), 'x', x.c_obj());
    pn_clear(x.c_obj());
    EXPECT_THAT(copy, matches);
}

TEST_F(ValueTest, String) {
    pn::value x = set('s', "");
    EXPECT_THAT(x, IsString(""));
    pn_strcat(&x.c_obj()->s, "");
    EXPECT_THAT(x, IsString(""));
    pn_strcat(&x.c_obj()->s, "a");
    EXPECT_THAT(x, IsString("a"));
    pn_strcat(&x.c_obj()->s, "bcdefghi");
    EXPECT_THAT(x, IsString("abcdefghi"));
    pn_strcat(&x.c_obj()->s, "jklmnopqrstuvwxyz");
    EXPECT_THAT(x, IsString("abcdefghijklmnopqrstuvwxyz"));
    pn_strncat(&x.c_obj()->s, "\0\0\0\0", 4);
    EXPECT_THAT(x, IsString(std::string("abcdefghijklmnopqrstuvwxyz\0\0\0\0", 30)));
}

TEST_F(ValueTest, List) {
    pn::value x = setv("iii", 1, 2, 3);
    EXPECT_THAT(x, IsList(1, 2, 3));

    pn_arrayext(&x.c_obj()->a, "");
    pn_arrayext(&x.c_obj()->a, "i", 4);
    pn_arrayext(&x.c_obj()->a, "ii", 5, 6);
    pn_arrayext(&x.c_obj()->a, "iiii", 7, 8, 9, 10);
    EXPECT_THAT(x, IsList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    pn_arraydel(&x.c_obj()->a, 9);  // 1 2 3 4 5 6 7 8 9
    pn_arraydel(&x.c_obj()->a, 7);  // 1 2 3 4 5 6 7 9
    pn_arraydel(&x.c_obj()->a, 1);  // 1 3 4 5 6 7 9
    pn_arraydel(&x.c_obj()->a, 2);  // 1 3 5 6 7 9
    pn_arraydel(&x.c_obj()->a, 3);  // 1 3 5 7 9
    EXPECT_THAT(x, IsList(1, 3, 5, 7, 9));

    pn_arrayins(&x.c_obj()->a, 1, 'i', 1);  // 1 1 3 5 7 9
    pn_arrayins(&x.c_obj()->a, 2, 'i', 2);  // 1 1 2 3 5 7 9
    pn_arraydel(&x.c_obj()->a, 5);          // 1 1 2 3 5 9
    pn_arrayins(&x.c_obj()->a, 5, 'i', 8);  // 1 1 2 3 5 8 9
    pn_arraydel(&x.c_obj()->a, 6);          // 1 1 2 3 5 8
    EXPECT_THAT(x, IsList(1, 1, 2, 3, 5, 8));
}

TEST_F(ValueTest, Map) {
    pn::value x = setkv("sisisi", "one", 1, "two", 2, "three", 3);
    EXPECT_THAT(x, IsMap("one", 1, "two", 2, "three", 3));
}

TEST_F(ValueTest, Invalid) {
    // Invalid formatting characters.
    EXPECT_THAT(set('\0'), IsNull());
    EXPECT_THAT(set('!'), IsNull());
    EXPECT_THAT(set('@'), IsNull());
    EXPECT_THAT(set('\277'), IsNull());

    EXPECT_THAT(setv("!@"), IsList());
    EXPECT_THAT(setv("n!n@n"), IsList(nullptr, nullptr, nullptr));

    EXPECT_THAT(setkv("s!", "!"), IsMap());
    EXPECT_THAT(setkv("sns!sn", "1", "2", "3"), IsMap("1", nullptr, "3", nullptr));

    // Not valid types for keys
    EXPECT_THAT(setkv("in!nsn", 1, "3"), IsMap("3", nullptr));
}

TEST_F(ValueTest, StrCompare) {
    EXPECT_THAT(pn_strcmp(set('s', "a").c_obj()->s, set('s', "b").c_obj()->s), Eq(-1));
    EXPECT_THAT(pn_strcmp(set('s', "b").c_obj()->s, set('s', "a").c_obj()->s), Eq(1));
    EXPECT_THAT(pn_strcmp(set('s', "a").c_obj()->s, set('s', "a").c_obj()->s), Eq(0));
}

}  // namespace pntest
