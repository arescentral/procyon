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

#ifndef TEST_MATCHERS_HPP_
#define TEST_MATCHERS_HPP_

#include <procyon.h>
#include <cmath>
#include <pn/array>
#include <pn/data>
#include <pn/file>
#include <pn/map>
#include <pn/string>
#include <pn/value>

#include <gmock/gmock.h>
#include <limits>

std::ostream& operator<<(std::ostream& ostr, pn_type_t x);
std::ostream& operator<<(std::ostream& ostr, const pn_value_t& x);
std::ostream& operator<<(std::ostream& ostr, const pn_string_t* s);
std::ostream& operator<<(std::ostream& ostr, const pn_error_t& x);

bool operator==(const pn_error_t& x, const pn_error_t& y);

namespace pn {

std::ostream& operator<<(std::ostream& ostr, const value& x);
void          PrintTo(const string& s, std::ostream* ostr);
void          PrintTo(const string_view& s, std::ostream* ostr);
void          PrintTo(const rune& r, std::ostream* ostr);

}  // namespace pn

namespace pntest {

::testing::Matcher<const pn_value_t&> match(const ::testing::Matcher<const pn_value_t&>& m);
::testing::Matcher<const pn_value_t&> match(std::nullptr_t);
::testing::Matcher<const pn_value_t&> match(bool b);
::testing::Matcher<const pn_value_t&> match(int i);
::testing::Matcher<const pn_value_t&> match(int64_t i);
::testing::Matcher<const pn_value_t&> match(double f);
::testing::Matcher<const pn_value_t&> match(const char* s);

template <typename... Args>
pn::value set(char format, const Args&... args) {
    pn::value x;
    pn_set(x.c_obj(), format, args...);
    return x;
}

template <typename... Args>
pn::value setv(const char* format, const Args&... args) {
    pn::value x;
    pn_setv(x.c_obj(), format, args...);
    return x;
}

template <typename... Args>
pn::value setkv(const char* format, const Args&... args) {
    pn::value x;
    pn_setkv(x.c_obj(), format, args...);
    return x;
}

class IsNullMatcher {
  public:
    IsNullMatcher() {}
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    void DescribeTo(::std::ostream* os) const { *os << "is null"; }
    void DescribeNegationTo(::std::ostream* os) const { *os << "is not null"; }
    template <typename U>
    bool MatchAndExplain(const U& x, ::testing::MatchResultListener* listener) const {
        return MatchAndExplain(*x.c_obj(), listener);
    }
};

template <pn_type_t type, typename T, typename Eq = std::equal_to<T>>
class SimpleMatcher {
  public:
    SimpleMatcher(T x, T(pn_value_t::*property)) : _x(x), _property(property) {}
    bool MatchAndExplain(const T& x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    void DescribeTo(::std::ostream* os) const { *os << "is " << _x; }
    void DescribeNegationTo(::std::ostream* os) const { *os << "is not " << _x; }
    template <typename U>
    bool MatchAndExplain(const U& x, ::testing::MatchResultListener* listener) const {
        return MatchAndExplain(*x.c_obj(), listener);
    }

  private:
    const T _x;
    T(pn_value_t::*const _property);
};

struct is_nan {
    bool operator()(double x, double) { return std::fpclassify(x) == FP_NAN; }
};

class IsDataMatcher {
  public:
    IsDataMatcher(const std::vector<uint8_t>& d) : _d(d) {}

    bool MatchAndExplain(
            const std::vector<uint8_t>& d, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_data_t* x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(pn::data_view x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(pn::value_cref x, ::testing::MatchResultListener* listener) const;

    void DescribeTo(::std::ostream* os) const;
    void DescribeNegationTo(::std::ostream* os) const;

  private:
    const std::vector<uint8_t> _d;
};

class IsStringMatcher {
  public:
    IsStringMatcher(const std::string& s) : _s(s) {}

    bool MatchAndExplain(const std::string& s, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_string_t* x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(pn::string_view x, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(pn::value_cref x, ::testing::MatchResultListener* listener) const;

    void DescribeTo(::std::ostream* os) const;
    void DescribeNegationTo(::std::ostream* os) const;

  private:
    const std::string _s;
};

class IsValueMatcher {
  public:
    IsValueMatcher(const pn_value_t& x) : _x(set('x', &x)) {}
    IsValueMatcher(const IsValueMatcher& other) : _x(set('x', other._x.c_obj())) {}

    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;

    template <typename U>
    bool MatchAndExplain(const U& x, ::testing::MatchResultListener* listener) const {
        return MatchAndExplain(*x.c_obj(), listener);
    }

    void DescribeTo(::std::ostream* os) const { *os << "is null"; }
    void DescribeNegationTo(::std::ostream* os) const { *os << "is not null"; }

  private:
    const pn::value _x;
};

::testing::PolymorphicMatcher<IsNullMatcher>                           IsNull();
::testing::PolymorphicMatcher<SimpleMatcher<PN_BOOL, bool>>            IsBool(bool b);
::testing::PolymorphicMatcher<SimpleMatcher<PN_INT, int64_t>>          IsInt(int64_t i);
::testing::PolymorphicMatcher<SimpleMatcher<PN_FLOAT, double>>         IsFloat(double f);
::testing::PolymorphicMatcher<SimpleMatcher<PN_FLOAT, double, is_nan>> IsNan();
::testing::PolymorphicMatcher<IsDataMatcher>   IsData(const std::vector<uint8_t>& d);
::testing::PolymorphicMatcher<IsStringMatcher> IsString(const std::string& s);
::testing::PolymorphicMatcher<IsValueMatcher>  IsValue(const pn_value_t& x);

class IsListMatcher {
  public:
    IsListMatcher(const std::vector<::testing::Matcher<const pn_value_t&>>& matchers);
    bool MatchAndExplain(const pn_array_t* a, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    void DescribeTo(::std::ostream* os) const;
    void DescribeNegationTo(::std::ostream* os) const;
    template <typename U>
    bool MatchAndExplain(const U& x, ::testing::MatchResultListener* listener) const {
        return MatchAndExplain(*x.c_obj(), listener);
    }

  private:
    void                                               Describe(::std::ostream* os) const;
    std::vector<::testing::Matcher<const pn_value_t&>> _matchers;
};

template <typename... Args>
struct ListMatcherAdder;

template <>
struct ListMatcherAdder<> {
    static std::vector<::testing::Matcher<const pn_value_t&>> matchers() { return {}; };
};

template <typename T, typename... Args>
struct ListMatcherAdder<T, Args...> {
    static std::vector<::testing::Matcher<const pn_value_t&>> matchers(
            const T& t, const Args&... args) {
        auto tail = ListMatcherAdder<Args...>::matchers(args...);
        tail.insert(tail.begin(), match(t));
        return tail;
    };
};

template <typename... Args>
inline ::testing::PolymorphicMatcher<IsListMatcher> IsList(const Args&... args) {
    return ::testing::MakePolymorphicMatcher(
            IsListMatcher(ListMatcherAdder<Args...>::matchers(args...)));
}

class IsMapMatcher {
  public:
    IsMapMatcher(const std::vector<std::pair<std::string, ::testing::Matcher<const pn_value_t&>>>&
                         matchers);
    bool MatchAndExplain(const pn_map_t* m, ::testing::MatchResultListener* listener) const;
    bool MatchAndExplain(const pn_value_t& x, ::testing::MatchResultListener* listener) const;
    void DescribeTo(::std::ostream* os) const;
    void DescribeNegationTo(::std::ostream* os) const;
    template <typename U>
    bool MatchAndExplain(const U& x, ::testing::MatchResultListener* listener) const {
        return MatchAndExplain(*x.c_obj(), listener);
    }

  private:
    void Describe(::std::ostream* os) const;
    std::vector<std::pair<std::string, ::testing::Matcher<const pn_value_t&>>> _matchers;
};

template <typename... Args>
struct MapMatcherAdder;

template <>
struct MapMatcherAdder<> {
    static std::vector<std::pair<std::string, ::testing::Matcher<const pn_value_t&>>> matchers() {
        return {};
    };
};

template <typename Key, typename Value, typename... Args>
struct MapMatcherAdder<Key, Value, Args...> {
    static std::vector<std::pair<std::string, ::testing::Matcher<const pn_value_t&>>> matchers(
            const Key& key, const Value& value, const Args&... args) {
        auto tail = MapMatcherAdder<Args...>::matchers(args...);
        tail.insert(tail.begin(), {key, match(value)});
        return tail;
    };
};

template <typename... Args>
inline ::testing::PolymorphicMatcher<IsMapMatcher> IsMap(const Args&... args) {
    return ::testing::MakePolymorphicMatcher(
            IsMapMatcher(MapMatcherAdder<Args...>::matchers(args...)));
}

}  // namespace pntest

#endif  // TEST_MATCHERS_HPP_
