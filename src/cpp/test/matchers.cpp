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

#include "./matchers.hpp"

#include <limits>

using ValueTest = ::testing::Test;
using ::testing::Eq;
using ::testing::MakeMatcher;
using ::testing::MakePolymorphicMatcher;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::Ne;
using ::testing::Not;
using ::testing::PolymorphicMatcher;
using ::testing::PrintToString;

std::ostream& operator<<(std::ostream& ostr, pn_type_t x) {
    switch (x) {
        case PN_NULL: return ostr << "null";
        case PN_BOOL: return ostr << "a bool";
        case PN_INT: return ostr << "an int";
        case PN_FLOAT: return ostr << "a float";
        case PN_DATA: return ostr << "data";
        case PN_STRING: return ostr << "a string";
        case PN_ARRAY: return ostr << "a array";
        case PN_MAP: return ostr << "a map";
    }
}

std::ostream& operator<<(std::ostream& ostr, const pn_value_t& x) {
    switch (x.type) {
        case PN_NULL: return ostr << "null";
        case PN_BOOL: return ostr << (x.b ? "true" : "false");
        case PN_INT: return ostr << x.i;
        case PN_FLOAT: return ostr << x.f;
        case PN_DATA:
            ostr << "$";
            for (size_t i = 0; i < x.d->count; ++i) {
                ostr << " " << std::hex << static_cast<int>(x.d->values[i]);
            }
            return ostr;
        case PN_STRING: return ostr << x.s;
        case PN_ARRAY:
            ostr << '[';
            for (size_t i = 0; i < x.a->count; ++i) {
                if (i != 0) {
                    ostr << ", ";
                }
                ostr << x.a->values[i];
            }
            return ostr << ']';
        case PN_MAP:
            ostr << '{';
            for (size_t i = 0; i < x.m->count; ++i) {
                if (i != 0) {
                    ostr << ", ";
                }
                std::string key(x.m->values[i].key->values, x.m->values[i].key->count - 1);
                ostr << PrintToString(key) << ": " << x.m->values[i].value;
            }
            return ostr << '}';
    }
    return ostr;
}

std::ostream& operator<<(std::ostream& ostr, const pn_string_t* s) {
    return ostr << PrintToString(std::string(s->values, s->count - 1));
}

std::ostream& operator<<(std::ostream& ostr, const pn_error_t& x) {
    return ostr << x.lineno << ":" << x.column << ": " << PrintToString(pn_strerror(x.code));
}

bool operator==(const pn_error_t& x, const pn_error_t& y) {
    return (x.column == y.column) && (x.lineno == y.lineno) && (x.code == y.code);
}

namespace pn {

std::ostream& operator<<(std::ostream& ostr, const value& x) { return ostr << *x.c_obj(); }
void          PrintTo(const string& s, std::ostream* ostr) { *ostr << value{s.copy()}; }
void          PrintTo(const string_view& s, std::ostream* ostr) { *ostr << value{s.copy()}; }
void          PrintTo(const rune& r, std::ostream* ostr) { PrintTo(string_view{r}, ostr); }

}  // namespace pn

namespace pntest {

Matcher<const pn_value_t&> match(std::nullptr_t) { return IsNull(); }

Matcher<const pn_value_t&> match(bool b) { return IsBool(b); }

Matcher<const pn_value_t&> match(int i) { return IsInt(i); }

Matcher<const pn_value_t&> match(int64_t i) { return IsInt(i); }

Matcher<const pn_value_t&> match(double f) { return IsFloat(f); }

Matcher<const pn_value_t&> match(const char* s) { return IsString(s); }

Matcher<const pn_value_t&> match(const Matcher<const pn_value_t&>& m) { return m; }

bool IsNullMatcher::MatchAndExplain(const pn_value_t& x, MatchResultListener* listener) const {
    if (x.type != PN_NULL) {
        *listener << "is " << x.type;
        return false;
    }
    return true;
}

PolymorphicMatcher<IsNullMatcher> IsNull() { return MakePolymorphicMatcher(IsNullMatcher()); }

bool IsDataMatcher::MatchAndExplain(
        const std::vector<uint8_t>& d, ::testing::MatchResultListener* listener) const {
    *listener << "is " << testing::PrintToString(d);
    return d == _d;
}

bool IsDataMatcher::MatchAndExplain(
        const pn_data_t* x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(std::vector<uint8_t>(x->values, x->values + x->count), listener);
}

bool IsDataMatcher::MatchAndExplain(
        const pn_value_t& x, ::testing::MatchResultListener* listener) const {
    if (x.type != PN_DATA) {
        *listener << "is " << x.type;
        return false;
    }
    return MatchAndExplain(x.d, listener);
}

bool IsDataMatcher::MatchAndExplain(
        pn::data_view x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(std::vector<uint8_t>(x.begin(), x.end()), listener);
}

bool IsDataMatcher::MatchAndExplain(
        pn::value_cref x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(*x.c_obj(), listener);
}

void IsDataMatcher::DescribeTo(::std::ostream* os) const {
    *os << "is " << ::testing::PrintToString(_d);
}

void IsDataMatcher::DescribeNegationTo(::std::ostream* os) const {
    *os << "is not " << ::testing::PrintToString(_d);
}

bool IsStringMatcher::MatchAndExplain(
        const std::string& s, ::testing::MatchResultListener* listener) const {
    *listener << "is " << testing::PrintToString(s);
    return s == _s;
}

bool IsStringMatcher::MatchAndExplain(
        const pn_string_t* x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(std::string(x->values, x->count - 1), listener);
}

bool IsStringMatcher::MatchAndExplain(
        const pn_value_t& x, ::testing::MatchResultListener* listener) const {
    if (x.type != PN_STRING) {
        *listener << "is " << x.type;
        return false;
    }
    return MatchAndExplain(x.s, listener);
}

bool IsStringMatcher::MatchAndExplain(
        pn::string_view x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(x.cpp_str(), listener);
}

bool IsStringMatcher::MatchAndExplain(
        pn::value_cref x, ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(*x.c_obj(), listener);
}

void IsStringMatcher::DescribeTo(::std::ostream* os) const {
    *os << "is " << ::testing::PrintToString(_s);
}

void IsStringMatcher::DescribeNegationTo(::std::ostream* os) const {
    *os << "is not " << ::testing::PrintToString(_s);
}

template <pn_type_t type, typename T, typename Eq>
bool SimpleMatcher<type, T, Eq>::MatchAndExplain(
        const T& x, ::testing::MatchResultListener* listener) const {
    *listener << "is " << x;
    return Eq()(x, _x);
}

template <pn_type_t type, typename T, typename Eq>
bool SimpleMatcher<type, T, Eq>::MatchAndExplain(
        const pn_value_t& x, MatchResultListener* listener) const {
    if (x.type != type) {
        *listener << "is " << x.type;
        return false;
    }
    const T y = x.*_property;
    return MatchAndExplain(y, listener);
}

template class SimpleMatcher<PN_BOOL, bool>;
template class SimpleMatcher<PN_INT, int64_t>;
template class SimpleMatcher<PN_FLOAT, double>;
template class SimpleMatcher<PN_FLOAT, double, is_nan>;

PolymorphicMatcher<SimpleMatcher<PN_BOOL, bool>> IsBool(bool b) {
    return MakePolymorphicMatcher(SimpleMatcher<PN_BOOL, bool>(b, &pn_value_t::b));
}

PolymorphicMatcher<SimpleMatcher<PN_INT, int64_t>> IsInt(int64_t i) {
    return MakePolymorphicMatcher(SimpleMatcher<PN_INT, int64_t>(i, &pn_value_t::i));
}

PolymorphicMatcher<SimpleMatcher<PN_FLOAT, double>> IsFloat(double f) {
    return MakePolymorphicMatcher(SimpleMatcher<PN_FLOAT, double>(f, &pn_value_t::f));
}

PolymorphicMatcher<SimpleMatcher<PN_FLOAT, double, is_nan>> IsNan() {
    return MakePolymorphicMatcher(SimpleMatcher<PN_FLOAT, double, is_nan>(
            std::numeric_limits<double>::quiet_NaN(), &pn_value_t::f));
}

::testing::PolymorphicMatcher<IsDataMatcher> IsData(const std::vector<uint8_t>& d) {
    return MakePolymorphicMatcher(IsDataMatcher(d));
}

::testing::PolymorphicMatcher<IsStringMatcher> IsString(const std::string& s) {
    return MakePolymorphicMatcher(IsStringMatcher(s));
}

bool IsValueMatcher::MatchAndExplain(const pn_value_t& x, MatchResultListener* listener) const {
    if (x.type != _x.c_obj()->type) {
        *listener << "is " << x.type;
        return false;
    }
    *listener << "is " << x;
    return pn_cmp(&x, _x.c_obj()) == 0;
}

PolymorphicMatcher<IsValueMatcher> IsValue(const pn_value_t& x) {
    return MakePolymorphicMatcher(IsValueMatcher(x));
}

IsListMatcher::IsListMatcher(const std::vector<Matcher<const pn_value_t&>>& matchers)
        : _matchers(matchers) {}

bool IsListMatcher::MatchAndExplain(const pn_array_t* a, MatchResultListener* listener) const {
    if (a->count != _matchers.size()) {
        *listener << "has " << a->count << " elements";
        return false;
    }
    for (size_t i = 0; i < _matchers.size(); ++i) {
        if (!_matchers[i].Matches(a->values[i])) {
            *listener << "where element " << i << " is " << PrintToString(a->values[i]);
            // TODO(sfiera): "...which", and only when an explanation is given.
            _matchers[i].MatchAndExplain(a->values[i], listener);
            return false;
        }
    }
    return true;
}

bool IsListMatcher::MatchAndExplain(const pn_value_t& x, MatchResultListener* listener) const {
    if (x.type != PN_ARRAY) {
        *listener << "is " << x.type;
        return false;
    }
    const pn_array_t* a = x.a;
    return MatchAndExplain(a, listener);
}

void IsListMatcher::DescribeTo(::std::ostream* os) const {
    *os << "is ";
    Describe(os);
}

void IsListMatcher::DescribeNegationTo(::std::ostream* os) const {
    *os << "is not ";
    Describe(os);
}

void IsListMatcher::Describe(::std::ostream* os) const {
    if (_matchers.empty()) {
        *os << "an empty array";
        return;
    }
    *os << "a " << _matchers.size() << "-element array where ";
    for (size_t i = 0; i < _matchers.size(); ++i) {
        if (i != 0) {
            if (_matchers.size() >= 3) {
                *os << ",";
            }
            if (i == _matchers.size() - 1) {
                *os << " and";
            }
            *os << " ";
        }
        *os << "element " << i << " ";
        _matchers[i].DescribeTo(os);
    }
}

IsMapMatcher::IsMapMatcher(
        const std::vector<std::pair<std::string, Matcher<const pn_value_t&>>>& matchers)
        : _matchers(matchers) {}

bool IsMapMatcher::MatchAndExplain(const pn_map_t* m, MatchResultListener* listener) const {
    if (m->count != _matchers.size()) {
        *listener << "has " << m->count << " elements";
        return false;
    }
    for (size_t i = 0; i < _matchers.size(); ++i) {
        std::string key(m->values[i].key->values, m->values[i].key->count - 1);
        if (_matchers[i].first != key) {
            *listener << "where element " << i << " value is " << PrintToString(m->values[i].key);
            return false;
        }
        if (!_matchers[i].second.Matches(m->values[i].value)) {
            *listener << "where element " << i << " value is "
                      << PrintToString(m->values[i].value);
            // TODO(sfiera): "...which", and only when an explanation is given.
            _matchers[i].second.MatchAndExplain(m->values[i].value, listener);
            return false;
        }
    }
    return true;
}

bool IsMapMatcher::MatchAndExplain(const pn_value_t& x, MatchResultListener* listener) const {
    if (x.type != PN_MAP) {
        *listener << "is a " << x.type;
        return false;
    }
    const pn_map_t* m = x.m;
    return MatchAndExplain(m, listener);
}

void IsMapMatcher::DescribeTo(::std::ostream* os) const {
    *os << "is ";
    Describe(os);
}

void IsMapMatcher::DescribeNegationTo(::std::ostream* os) const {
    *os << "is not ";
    Describe(os);
}

void IsMapMatcher::Describe(::std::ostream* os) const {
    if (_matchers.empty()) {
        *os << "an empty map";
        return;
    }
    *os << "a " << _matchers.size() << "-element map where ";
    for (size_t i = 0; i < _matchers.size(); ++i) {
        if (i != 0) {
            if (_matchers.size() >= 3) {
                *os << ",";
            }
            if (i == _matchers.size() - 1) {
                *os << " and";
            }
            *os << " ";
        }
        *os << "element " << i << " key is " << _matchers[i].first << " and value ";
        _matchers[i].second.DescribeTo(os);
    }
}

}  // namespace pntest
