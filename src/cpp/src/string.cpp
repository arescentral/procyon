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

#include <pn/string>

#include <pn/file>

#include "../../c/src/common.h"
#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {

string::string(const char* data, size_type size) : _c_obj{pn_string_new(data, size)} {}

string::~string() { free(_c_obj); }

static_assert(sizeof(string) == sizeof(pn_string_t*), "string size wrong");
static_assert(sizeof(string_ref) == sizeof(pn_string_t**), "string_ref size wrong");

rune::size_type rune::count(string_view s) { return std::distance(s.begin(), s.end()); }

data_view string::as_data() const {
    return data_view{reinterpret_cast<const uint8_t*>(data()), size()};
}

data_view string_ref::as_data() const {
    return data_view{reinterpret_cast<const uint8_t*>(data()), size()};
}

data_view string_view::as_data() const {
    return data_view{reinterpret_cast<const uint8_t*>(data()), size()};
}

string_view rune::slice(string_view s, size_type offset) {
    auto it = s.begin();
    for (size_type i = 0; i < offset; ++i) {
        ++it;
    }
    return s.substr(it.offset());
}

string_view rune::slice(string_view s, size_type offset, size_type size) {
    s       = slice(s, offset);
    auto it = s.begin();
    for (size_type i = 0; i < size; ++i) {
        ++it;
    }
    return s.substr(0, it.offset());
}

string::size_type string::find(string_view needle, size_type offset) const {
    return string_view{*this}.find(needle, offset);
}
string_ref::size_type string_ref::find(string_view needle, size_type offset) const {
    return string_view{*this}.find(needle, offset);
}
string::size_type string::rfind(string_view needle, size_type offset) const {
    return string_view{*this}.rfind(needle, offset);
}
string_ref::size_type string_ref::rfind(string_view needle, size_type offset) const {
    return string_view{*this}.rfind(needle, offset);
}

string_view::size_type string_view::find(string_view needle, size_type offset) const {
    if (offset + needle.size() > size()) {
        return npos;
    }
    for (size_type i = offset; i < size() - needle.size() + 1; ++i) {
        if (substr(i, needle.size()) == needle) {
            return i;
        }
    }
    return npos;
}

string_view::size_type string_view::rfind(string_view needle, size_type offset) const {
    if (offset + needle.size() > size()) {
        return npos;
    }
    for (size_type i = size() - needle.size() + 1; i > offset; --i) {
        if (substr(i - 1, needle.size()) == needle) {
            return i - 1;
        }
    }
    return npos;
}

int string::compare(pn::string_view other) const {
    return pn_memncmp(data(), size(), other.data(), other.size());
}
int string_ref::compare(pn::string_view other) const {
    return pn_memncmp(data(), size(), other.data(), other.size());
}
int string_view::compare(pn::string_view other) const {
    return pn_memncmp(data(), size(), other.data(), other.size());
}

bool partition(string_view& found, string_view separator, string_view& input) {
    string_view::size_type at = input.find(separator);
    if (at < 0) {
        found = input;
        input = string_view{};
        return false;
    }
    found = input.substr(0, at);
    input = input.substr(at + separator.size());
    return true;
}

bool strtoll(string_view s, int64_t* i, pn_error_code_t* error) {
    return pn_strtoll(s.data(), s.size(), i, error);
}

bool strtod(string_view s, double* f, pn_error_code_t* error) {
    return pn_strtod(s.data(), s.size(), f, error);
}

file string::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file string_ref::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file string_view::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file string::open(const char* mode) { return check_c_obj(file{pn_open_string(c_obj(), mode)}); }
file string_ref::open(const char* mode) {
    return check_c_obj(file{pn_open_string(c_obj(), mode)});
}

}  // namespace pn
