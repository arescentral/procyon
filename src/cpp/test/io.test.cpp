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
#include <pn/input>
#include <pn/output>

#include "./matchers.hpp"

using IoTest = ::testing::Test;
using ::testing::Eq;

namespace pntest {

template <typename T>
class IoTest : public T {
  public:
    template <typename Arg>
    pn::string write(const Arg arg) {
        pn::string data;
        pn::output out = this->output();
        EXPECT_THAT(out.write(arg), Eq(true));
        return this->output_result();
    }
};

class ViewIoTest : public testing::Test {
  public:
    pn::input input(pn::string_view s) {
        _data = s.as_data().copy();
        return _data.input();
    }
    pn::input input(const char* data, int size) { return input(pn::string{data, size}); }

    pn::output output() {
        _data.resize(0);
        return _data.output();
    };

    pn::string output_result() { return _data.as_string().copy(); }

  private:
    pn::data _data;
};

class FileIoTest : public testing::Test {
  public:
    FileIoTest() : _file{nullptr} {}

    pn::input input(pn::string_view s) {
        _file = tmpfile();
        fwrite(s.data(), 1, s.size(), _file);
        rewind(_file);
        return pn::input{_file};
    }
    pn::input input(const char* data, int size) { return input(pn::string{data, size}); }

    pn::output output() {
        _file = tmpfile();
        return pn::output{_file};
    };

    pn::string output_result() {
        pn::data d;
        d.resize(ftell(_file));
        rewind(_file);
        fread(d.data(), 1, d.size(), _file);
        return d.as_string().copy();
    }

  private:
    FILE* _file;
};

using IoTests = ::testing::Types<ViewIoTest, FileIoTest>;
TYPED_TEST_SUITE(IoTest, IoTests);

TYPED_TEST(IoTest, ReadC) {
    pn::input in = this->input(
            "\001\002\003\004\005\006\007\010"
            "\011\012\013\014\015\016\017\020"
            "\021\022\023\024\025\026\027\030"
            "\031\032\033\034\035\036\037\040"
            "\041\042\043\044");
    EXPECT_THAT(pn_read(in.c_obj(), ""), Eq(true));

    int8_t  i8 = 0;
    uint8_t u8 = 0;
    EXPECT_THAT(pn_read(in.c_obj(), "bB", &i8, &u8), Eq(true));
    EXPECT_THAT(i8, Eq(0x01));
    EXPECT_THAT(u8, Eq(0x02u));

    int16_t  i16 = 0;
    uint16_t u16 = 0;
    EXPECT_THAT(pn_read(in.c_obj(), "hH", &i16, &u16), Eq(true));
    EXPECT_THAT(i16, Eq(0x0304));
    EXPECT_THAT(u16, Eq(0x0506u));

    int32_t  i32 = 0;
    uint32_t u32 = 0;
    EXPECT_THAT(pn_read(in.c_obj(), "lL", &i32, &u32), Eq(true));
    EXPECT_THAT(i32, Eq(0x0708090a));
    EXPECT_THAT(u32, Eq(0x0b0c0d0eu));

    int64_t  i64 = 0;
    uint64_t u64 = 0;
    EXPECT_THAT(pn_read(in.c_obj(), "qQ", &i64, &u64), Eq(true));
    EXPECT_THAT(i64, Eq(INT64_C(0x0f10111213141516)));
    EXPECT_THAT(u64, Eq(UINT64_C(0x1718191a1b1c1d1e)));

    bool b = false;
    EXPECT_THAT(pn_read(in.c_obj(), "?", &b), Eq(true));
    EXPECT_THAT(b, Eq(true));

    EXPECT_THAT(pn_read(in.c_obj(), "#", 2), Eq(true));

    char c = false;
    EXPECT_THAT(pn_read(in.c_obj(), "c", &c), Eq(true));
    EXPECT_THAT(c, Eq('\042'));

    EXPECT_THAT(pn_input_eof(in.c_obj()), Eq(false));
    EXPECT_THAT(pn_read(in.c_obj(), "q", &i64), Eq(false));
    EXPECT_THAT(pn_input_eof(in.c_obj()), Eq(true));
}

TYPED_TEST(IoTest, ReadCpp) {
    pn::input in = this->input(
            "\001\002\003\004\005\006\007\010"
            "\011\012\013\014\015\016\017\020"
            "\021\022\023\024\025\026\027\030"
            "\031\032\033\034\035\036\037\040"
            "\041\042\043\044");
    EXPECT_THAT(in.read(), Eq(true));

    int8_t  i8 = 0;
    uint8_t u8 = 0;
    EXPECT_THAT(in.read(&i8, &u8), Eq(true));
    EXPECT_THAT(i8, Eq(0x01));
    EXPECT_THAT(u8, Eq(0x02u));

    int16_t  i16 = 0;
    uint16_t u16 = 0;
    EXPECT_THAT(in.read(&i16, &u16), Eq(true));
    EXPECT_THAT(i16, Eq(0x0304));
    EXPECT_THAT(u16, Eq(0x0506u));

    int32_t  i32 = 0;
    uint32_t u32 = 0;
    EXPECT_THAT(in.read(&i32, &u32), Eq(true));
    EXPECT_THAT(i32, Eq(0x0708090a));
    EXPECT_THAT(u32, Eq(0x0b0c0d0eu));

    int64_t  i64 = 0;
    uint64_t u64 = 0;
    EXPECT_THAT(in.read(&i64, &u64), Eq(true));
    EXPECT_THAT(i64, Eq(INT64_C(0x0f10111213141516)));
    EXPECT_THAT(u64, Eq(UINT64_C(0x1718191a1b1c1d1e)));

    bool b = false;
    EXPECT_THAT(in.read(&b), Eq(true));
    EXPECT_THAT(b, Eq(true));

    EXPECT_THAT(in.read(pn::pad(2)), Eq(true));

    char c = false;
    EXPECT_THAT(in.read(&c), Eq(true));
    EXPECT_THAT(c, Eq('\042'));

    EXPECT_THAT(in.eof(), Eq(false));
    EXPECT_THAT(in.read(&i64), Eq(false));
    EXPECT_THAT(in.eof(), Eq(true));
}

TYPED_TEST(IoTest, ReadExtrema) {
    char charmin, charmax;
    EXPECT_THAT(this->input("\200\177", 2).read(&charmin, &charmax).operator bool(), Eq(true));
    EXPECT_THAT(charmin, Eq(-128));
    EXPECT_THAT(charmax, Eq(127));

    int8_t i8min, i8max;
    EXPECT_THAT(this->input("\200\177", 2).read(&i8min, &i8max).operator bool(), Eq(true));
    EXPECT_THAT(i8min, Eq(-128));
    EXPECT_THAT(i8max, Eq(127));

    uint8_t u8min, u8max;
    EXPECT_THAT(this->input("\000\377", 2).read(&u8min, &u8max).operator bool(), Eq(true));
    EXPECT_THAT(u8min, Eq(0u));
    EXPECT_THAT(u8max, Eq(255u));

    int16_t i16min, i16max;
    EXPECT_THAT(
            this->input("\200\000\177\377", 4).read(&i16min, &i16max).operator bool(), Eq(true));
    EXPECT_THAT(i16min, Eq(-32768));
    EXPECT_THAT(i16max, Eq(32767));

    uint16_t u16min, u16max;
    EXPECT_THAT(
            this->input("\000\000\377\377", 4).read(&u16min, &u16max).operator bool(), Eq(true));
    EXPECT_THAT(u16min, Eq(0u));
    EXPECT_THAT(u16max, Eq(65535u));

    int32_t i32min, i32max;
    EXPECT_THAT(
            this->input("\200\000\000\000\177\377\377\377", 8)
                    .read(&i32min, &i32max)
                    .
                    operator bool(),
            Eq(true));
    EXPECT_THAT(i32min, Eq(-2147483648));
    EXPECT_THAT(i32max, Eq(2147483647));

    uint32_t u32min, u32max;
    EXPECT_THAT(
            this->input("\000\000\000\000\377\377\377\377", 8)
                    .read(&u32min, &u32max)
                    .
                    operator bool(),
            Eq(true));
    EXPECT_THAT(u32min, Eq(0u));
    EXPECT_THAT(u32max, Eq(4294967295u));

    int64_t i64min, i64max;
    EXPECT_THAT(
            this->input("\200\000\000\000\000\000\000\000\177\377\377\377\377\377\377\377", 16)
                    .read(&i64min, &i64max)
                    .
                    operator bool(),
            Eq(true));
    EXPECT_THAT(i64min, Eq(std::numeric_limits<int64_t>::min()));
    EXPECT_THAT(i64max, Eq(std::numeric_limits<int64_t>::max()));

    uint64_t u64min, u64max;
    EXPECT_THAT(
            this->input("\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377", 16)
                    .read(&u64min, &u64max)
                    .
                    operator bool(),
            Eq(true));
    EXPECT_THAT(u64min, Eq(std::numeric_limits<uint64_t>::min()));
    EXPECT_THAT(u64max, Eq(std::numeric_limits<uint64_t>::max()));
}

TYPED_TEST(IoTest, WriteC) {
    pn::output out = this->output();
    EXPECT_THAT(pn_write(out.c_obj(), ""), Eq(true));

    EXPECT_THAT(pn_write(out.c_obj(), "n"), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(""));
    EXPECT_THAT(pn_write(out.c_obj(), "nnn"), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(""));

    EXPECT_THAT(
            pn_write(
                    out.c_obj(), "cbB", static_cast<char>(1), static_cast<int8_t>(2),
                    static_cast<uint8_t>(3)),
            Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3"));

    EXPECT_THAT(
            pn_write(
                    out.c_obj(), "hH", static_cast<int16_t>(0x0405),
                    static_cast<uint16_t>(0x0607)),
            Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3\4\5\6\7"));

    EXPECT_THAT(
            pn_write(
                    out.c_obj(), "lL", static_cast<int32_t>(0x01010101),
                    static_cast<uint32_t>(0x02020202)),
            Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3\4\5\6\7\1\1\1\1\2\2\2\2"));

    EXPECT_THAT(pn_write(out.c_obj(), "#", static_cast<size_t>(2)), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(std::string("\1\2\3\4\5\6\7\1\1\1\1\2\2\2\2\0\0", 17)));
}

TYPED_TEST(IoTest, WriteCpp) {
    pn::output out = this->output();
    EXPECT_THAT(pn_write(out.c_obj(), ""), Eq(true));

    EXPECT_THAT(out.write(nullptr), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(""));
    EXPECT_THAT(out.write(nullptr, nullptr, nullptr), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(""));

    EXPECT_THAT(
            out.write(static_cast<char>(1), static_cast<int8_t>(2), static_cast<uint8_t>(3)),
            Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3"));

    EXPECT_THAT(out.write(static_cast<int16_t>(0x0405), static_cast<uint16_t>(0x0607)), Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3\4\5\6\7"));

    EXPECT_THAT(
            out.write(static_cast<int32_t>(0x01010101), static_cast<uint32_t>(0x02020202)),
            Eq(true));
    EXPECT_THAT(this->output_result(), Eq("\1\2\3\4\5\6\7\1\1\1\1\2\2\2\2"));

    EXPECT_THAT(out.write(pn::pad(2)), Eq(true));
    EXPECT_THAT(this->output_result(), Eq(std::string("\1\2\3\4\5\6\7\1\1\1\1\2\2\2\2\0\0", 17)));
}

TYPED_TEST(IoTest, WriteLimits) {
    EXPECT_THAT(this->write(static_cast<int8_t>(-128)), Eq(std::string("\200")));
    EXPECT_THAT(this->write(static_cast<uint8_t>(255)), Eq(std::string("\377")));
    EXPECT_THAT(this->write(static_cast<int16_t>(-32768)), Eq(std::string("\200\000", 2)));
    EXPECT_THAT(this->write(static_cast<uint16_t>(65535)), Eq(std::string("\377\377", 2)));
    EXPECT_THAT(this->write(static_cast<int32_t>(-2147483648)), Eq(std::string("\200\0\0\0", 4)));
    EXPECT_THAT(this->write(static_cast<uint32_t>(4294967295)), Eq(std::string(4, 0377)));
    EXPECT_THAT(
            this->write(INT64_C(-9223372036854775808u)), Eq(std::string("\200\0\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(UINT64_C(18446744073709551615)), Eq(std::string(8, 0377)));

    using f = std::numeric_limits<float>;
    EXPECT_THAT(this->write(-f::infinity()), Eq(std::string("\377\200\0\0", 4)));
    EXPECT_THAT(this->write(-f::max()), Eq(std::string("\377\177\377\377", 4)));
    EXPECT_THAT(this->write(-f::min()), Eq(std::string("\200\200\0\0", 4)));
    EXPECT_THAT(this->write(-f::denorm_min()), Eq(std::string("\200\0\0\1", 4)));
    EXPECT_THAT(this->write(-0.0f), Eq(std::string("\200\0\0\0", 4)));
    EXPECT_THAT(this->write(0.0f), Eq(std::string("\0\0\0\0", 4)));
    EXPECT_THAT(this->write(f::denorm_min()), Eq(std::string("\0\0\0\1", 4)));
    EXPECT_THAT(this->write(f::min()), Eq(std::string("\0\200\0\0", 4)));
    EXPECT_THAT(this->write(f::max()), Eq(std::string("\177\177\377\377", 4)));
    EXPECT_THAT(this->write(f::infinity()), Eq(std::string("\177\200\0\0", 4)));
    EXPECT_THAT(this->write(f::quiet_NaN()), Eq(std::string("\177\300\0\0", 4)));

    using d = std::numeric_limits<double>;
    EXPECT_THAT(this->write(-d::infinity()), Eq(std::string("\377\360\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(-d::max()), Eq(std::string("\377\357\377\377\377\377\377\377", 8)));
    EXPECT_THAT(this->write(-d::min()), Eq(std::string("\200\020\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(-d::denorm_min()), Eq(std::string("\200\0\0\0\0\0\0\1", 8)));
    EXPECT_THAT(this->write(-0.0), Eq(std::string("\200\0\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(0.0), Eq(std::string("\0\0\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(d::denorm_min()), Eq(std::string("\0\0\0\0\0\0\0\1", 8)));
    EXPECT_THAT(this->write(d::min()), Eq(std::string("\0\020\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(d::max()), Eq(std::string("\177\357\377\377\377\377\377\377", 8)));
    EXPECT_THAT(this->write(d::infinity()), Eq(std::string("\177\360\0\0\0\0\0\0", 8)));
    EXPECT_THAT(this->write(d::quiet_NaN()), Eq(std::string("\177\370\0\0\0\0\0\0", 8)));
}

TYPED_TEST(IoTest, ReadData) {
    pn::input in = this->input("\001\002\003\004\005\006");

    char d[2];
    EXPECT_THAT(pn_read(in.c_obj(), "$", (void*)d, (size_t)2), Eq(true));
    EXPECT_THAT(d[0], Eq(1));
    EXPECT_THAT(d[1], Eq(2));

    pn::data d2;
    d2.resize(2);
    EXPECT_THAT(in.read(&d2), Eq(true));
    EXPECT_THAT(d2, Eq(pn::string_view{"\003\004"}.as_data()));
}

TYPED_TEST(IoTest, WriteData) {
    pn::output out = this->output();

    EXPECT_THAT(pn_write(out.c_obj(), "$", "\001\002", (size_t)2), Eq(true));
    EXPECT_THAT(out.write(pn::string{"\003\004"}.as_data()), Eq(true));

    EXPECT_THAT(this->output_result(), Eq("\001\002\003\004"));
}

TYPED_TEST(IoTest, WriteString) {
    pn::output out = this->output();

    EXPECT_THAT(pn_write(out.c_obj(), "S", "\001\002", (size_t)2), Eq(true));
    EXPECT_THAT(pn_write(out.c_obj(), "s", "\003\004"), Eq(true));
    EXPECT_THAT(out.write(pn::string_view{"\005\006"}), Eq(true));

    EXPECT_THAT(this->output_result(), Eq("\001\002\003\004\005\006"));
}

TYPED_TEST(IoTest, ReadAllDataC) {
    pn::data  d;
    uint8_t   b;
    pn::input in = this->input("\1\2\3");
    EXPECT_THAT(pn_read(in.c_obj(), "B*", &b, d.c_obj()), Eq(true));
    EXPECT_THAT(pn_input_eof(in.c_obj()), Eq(true));
    EXPECT_THAT(b, Eq(1));
    EXPECT_THAT(d.as_string(), Eq("\2\3"));
}

TYPED_TEST(IoTest, ReadAllDataCpp) {
    pn::data  result;
    pn::input in = this->input("");
    in.read(all(result));
    EXPECT_THAT(in.operator bool(), Eq(false));
    EXPECT_THAT(in.eof(), Eq(true));
    EXPECT_THAT(in.error(), Eq(false));
    EXPECT_THAT(result.as_string(), Eq(""));

    pn::string s;
    for (int i = 0; i < 2000; ++i) {
        s += "\1\2\3\4\5\6\7";
    }
    result.resize(0);
    in = this->input(s);
    in.read(all(result));
    EXPECT_THAT(in.operator bool(), Eq(false));
    EXPECT_THAT(in.eof(), Eq(true));
    EXPECT_THAT(in.error(), Eq(false));
    perror("ReadAllDataCpp");
    EXPECT_THAT(result.as_string(), Eq(pn::string_view{s}));
}

TYPED_TEST(IoTest, ReadAllStringC) {
    pn::string s;
    uint8_t    b;
    pn::input  in = this->input("\1\2\3");
    EXPECT_THAT(pn_read(in.c_obj(), "B+", &b, s.c_obj()), Eq(true));
    EXPECT_THAT(pn_input_eof(in.c_obj()), Eq(true));
    EXPECT_THAT(b, Eq(1));
    EXPECT_THAT(s, Eq("\2\3"));
}

TYPED_TEST(IoTest, ReadAllStringCpp) {
    pn::string result;
    pn::input  in = this->input("");
    in.read(all(result));
    EXPECT_THAT(in.operator bool(), Eq(false));
    EXPECT_THAT(in.eof(), Eq(true));
    EXPECT_THAT(in.error(), Eq(false));
    perror("ReadAllStringCpp");
    EXPECT_THAT(result, Eq(""));

    pn::string s;
    for (int i = 0; i < 2000; ++i) {
        s += "\1\2\3\4\5\6\7";
    }
    result = "";
    in     = this->input(s);
    in.read(all(result));
    EXPECT_THAT(in.operator bool(), Eq(false));
    EXPECT_THAT(in.eof(), Eq(true));
    EXPECT_THAT(in.error(), Eq(false));
    EXPECT_THAT(result, Eq(pn::string_view{s}));
}

}  // namespace pntest
