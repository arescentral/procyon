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

#include <pn/value>

#include <pn/array>
#include <pn/data>
#include <pn/map>
#include <pn/string>

#include "common.hpp"

namespace pn {

template <typename CppType>
static auto release_c_obj(CppType&& x) ->
        typename std::remove_reference<decltype(*x.c_obj())>::type {
    typename std::remove_reference<decltype(*x.c_obj())>::type y{};
    std::swap(*x.c_obj(), y);
    return y;
}

value::value(data d) : _c_obj{pn_value_t{PN_DATA, {.d = release_c_obj(d)}}} {}
value::value(const std::string& s) : value(string{s}) {}
value::value(const char* s) : value(string{s}) {}
value::value(string s) : _c_obj{pn_value_t{PN_STRING, {.s = release_c_obj(s)}}} {}
value::value(array a) : _c_obj{pn_value_t{PN_ARRAY, {.a = release_c_obj(a)}}} {}
value::value(map m) : _c_obj{pn_value_t{PN_MAP, {.m = release_c_obj(m)}}} {}

template <type t>
struct helper;

template <>
struct helper<PN_DATA> {
    template <typename value_api>
    static data_ref to(value_api& x) {
        return data_ref{&(x.is_data() ? x : (x = data{})).c_obj()->d};
    }
    template <typename value_api>
    static data_view as(const value_api& x) {
        return x.is_data() ? data_view{x.c_obj()->d->values, static_cast<int>(x.c_obj()->d->count)}
                           : data_view{};
    }
};

data_view value::as_data() const { return helper<PN_DATA>::as(*this); }
data_view value_ref::as_data() const { return helper<PN_DATA>::as(*this); }
data_view value_cref::as_data() const { return helper<PN_DATA>::as(*this); }
data_ref  value::to_data() { return helper<PN_DATA>::to(*this); }
data_ref  value_ref::to_data() { return helper<PN_DATA>::to(*this); }

template <>
struct helper<PN_STRING> {
    template <typename value_api>
    static string_ref to(value_api& x) {
        return string_ref{&(x.is_string() ? x : (x = string{})).c_obj()->s};
    }
    template <typename value_api>
    static string_view as(const value_api& x) {
        return x.is_string() ? string_view{x.c_obj()->s->values,
                                           static_cast<int>(x.c_obj()->s->count) - 1}
                             : string_view{};
    }
};

string_view value::as_string() const { return helper<PN_STRING>::as(*this); }
string_view value_ref::as_string() const { return helper<PN_STRING>::as(*this); }
string_view value_cref::as_string() const { return helper<PN_STRING>::as(*this); }
string_ref  value::to_string() { return helper<PN_STRING>::to(*this); }
string_ref  value_ref::to_string() { return helper<PN_STRING>::to(*this); }

template <>
struct helper<PN_ARRAY> {
    template <typename value_api>
    static array_ref to(value_api& x) {
        return array_ref{&(x.is_array() ? x : (x = array{})).c_obj()->a};
    }
    template <typename value_api>
    static array_cref as(const value_api& x) {
        return array_cref{x.is_array() ? &x.c_obj()->a : &pn_arrayempty.a};
    }
};

array_cref value::as_array() const { return helper<PN_ARRAY>::as(*this); }
array_cref value_ref::as_array() const { return helper<PN_ARRAY>::as(*this); }
array_cref value_cref::as_array() const { return helper<PN_ARRAY>::as(*this); }
array_ref  value::to_array() { return helper<PN_ARRAY>::to(*this); }
array_ref  value_ref::to_array() { return helper<PN_ARRAY>::to(*this); }

template <>
struct helper<PN_MAP> {
    template <typename value_api>
    static map_ref to(value_api& x) {
        return map_ref{&(x.is_map() ? x : (x = map{})).c_obj()->m};
    }
    template <typename value_api>
    static map_cref as(const value_api& x) {
        return map_cref{x.is_map() ? &x.c_obj()->m : &pn_mapempty.m};
    }
};

map_cref value::as_map() const { return helper<PN_MAP>::as(*this); }
map_cref value_ref::as_map() const { return helper<PN_MAP>::as(*this); }
map_cref value_cref::as_map() const { return helper<PN_MAP>::as(*this); }
map_ref  value::to_map() { return helper<PN_MAP>::to(*this); }
map_ref  value_ref::to_map() { return helper<PN_MAP>::to(*this); }

static_assert(sizeof(value) == sizeof(pn_value_t), "value size wrong");
static_assert(sizeof(value_ref) == sizeof(pn_value_t*), "value_ref size wrong");
static_assert(sizeof(value_cref) == sizeof(pn_value_t const*), "value_cref size wrong");

static_assert(std::is_nothrow_default_constructible<value>::value, "not default constructible");
static_assert(std::is_destructible<value>::value, "not destructible");
static_assert(!is_copyable<value>::value, "is copyable");
static_assert(is_nothrow_movable<value>::value, "not nothrow movable");

static_assert(conversion<value, std::nullptr_t>::is_nothrow, "no nothrow nullptr conversion");
static_assert(conversion<value, bool>::is_nothrow, "no nothrow bool conversion");
static_assert(conversion<value, int>::is_nothrow, "no nothrow int conversion");
static_assert(conversion<value, int16_t>::is_nothrow, "no nothrow int16_t conversion");
static_assert(conversion<value, uint16_t>::is_nothrow, "no nothrow uint16_t conversion");
static_assert(conversion<value, int32_t>::is_nothrow, "no nothrow int32_t conversion");
static_assert(conversion<value, int64_t>::is_nothrow, "no nothrow int64_t conversion");
static_assert(conversion<value, float>::is_nothrow, "no nothrow float conversion");
static_assert(conversion<value, double>::is_nothrow, "no nothrow double conversion");

static_assert(conversion<value, const char*>::can_throw, "bad const char* conversion");
static_assert(conversion<value, std::string>::can_throw, "bad std::string conversion");

static_assert(conversion<value, data>::can_throw, "bad pn::data conversion");
static_assert(conversion<value, string>::can_throw, "bad pn::string conversion");
static_assert(conversion<value, array>::can_throw, "bad pn::array conversion");
static_assert(conversion<value, map>::can_throw, "bad pn::map conversion");
static_assert(conversion<value, data_ref>::fails, "allows data copy conversion");
static_assert(conversion<value, string_ref>::fails, "allows string copy conversion");
static_assert(conversion<value, array_ref>::fails, "allows array copy conversion");
static_assert(conversion<value, map_ref>::fails, "allows map copy conversion");
static_assert(conversion<value, data_view>::fails, "allows data copy conversion");
static_assert(conversion<value, string_view>::fails, "allows string copy conversion");
static_assert(conversion<value, array_cref>::fails, "allows array copy conversion");
static_assert(conversion<value, map_cref>::fails, "allows map copy conversion");
static_assert(conversion<value, const data&>::fails, "allows data copy conversion");
static_assert(conversion<value, const string&>::fails, "allows string copy conversion");
static_assert(conversion<value, const array&>::fails, "allows array copy conversion");
static_assert(conversion<value, const map&>::fails, "allows map copy conversion");

static_assert(conversion<value, char>::fails, "allows char conversion");
static_assert(conversion<value, signed char>::fails, "allows signed char conversion");
static_assert(conversion<value, unsigned char>::fails, "allows unsigned char conversion");
static_assert(conversion<value, uint32_t>::fails, "allows uint32_t conversion");
static_assert(conversion<value, uint64_t>::fails, "allows uint64_t conversion");
static_assert(conversion<value, void*>::fails, "allows void* conversion");
static_assert(conversion<value, char*>::fails, "allows char* conversion");

}  // namespace pn
