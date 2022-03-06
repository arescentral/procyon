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

#include <pn/input>
#include <pn/output>

#include "../../c/src/common.h"
#include "../../c/src/unicode.h"
#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {
namespace {

template <typename char_type, int char_size = sizeof(char_type) * 8>
struct utf;

template <typename char_type>
struct utf<char_type, 16> {
    static constexpr int max_expansion = 3;

    static void init(pn_string** s, const char_type* data, int size) {
        VECTOR_INIT(s, (size * max_expansion) + 1);
        char*    out   = &(*s)->values[0];
        uint16_t state = 0;
        for (const char_type* end = data + size; data != end; ++data) {
            state = pn_decode_utf16(state, *data, &out);
        }
        pn_decode_utf16_done(state, &out);
        *(out++)    = '\0';
        (*s)->count = out - &(*s)->values[0];
    }

    static std::basic_string<char_type> str(const char* data, int size) {
        std::basic_string<char_type> out;
        for (int i = 0; i < size; i = pn_rune_next(data, size, i)) {
            uint16_t rune_data[2];
            size_t   rune_size;
            pn_encode_utf16(pn_rune(data, size, i), rune_data, &rune_size);
            out.append(rune_data, rune_data + rune_size);
        }
        return out;
    }
};

static void pn_unichr_advance(pn_rune_t rune, char** data) {
    size_t size;
    pn_unichr(rune, *data, &size);
    *data += size;
}

template <typename char_type>
struct utf<char_type, 32> {
    static constexpr int max_expansion = 4;

    static void init(pn_string** s, const char_type* data, int size) {
        VECTOR_INIT(s, (size * max_expansion) + 1);
        char* out = &(*s)->values[0];
        for (const char_type* end = data + size; data != end; ++data) {
            pn_unichr_advance(*data, &out);
        }
        *(out++)    = '\0';
        (*s)->count = out - &(*s)->values[0];
    }

    static std::basic_string<char_type> str(const char* data, int size) {
        std::basic_string<char_type> out;
        for (int i = 0; i < size; i = pn_rune_next(data, size, i)) {
            out.push_back(pn_rune(data, size, i));
        }
        return out;
    }
};

}  // namespace

string::string(const char* data, size_type size) : _c_obj{pn_string_new(data, size)} {}
string::string(const char16_t* data, size_type size) { utf<char16_t>::init(&_c_obj, data, size); }
string::string(const char32_t* data, size_type size) { utf<char32_t>::init(&_c_obj, data, size); }
string::string(const wchar_t* data, size_type size) { utf<wchar_t>::init(&_c_obj, data, size); }

string::~string() { free(_c_obj); }

std::u16string string::cpp_u16str() const { return utf<char16_t>::str(data(), size()); }
std::u32string string::cpp_u32str() const { return utf<char32_t>::str(data(), size()); }
std::wstring   string::cpp_wstr() const { return utf<wchar_t>::str(data(), size()); }

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

bool partition(string_view* found, string_view separator, string_view* input) {
    string_view::size_type at = input->find(separator);
    if (at < 0) {
        *found = *input;
        *input = string_view{};
        return false;
    }
    *found = input->substr(0, at);
    *input = input->substr(at + separator.size());
    return true;
}

bool strtoll(string_view s, int64_t* i, pn_error_code_t* error) {
    return pn_strtoll(s.data(), s.size(), i, error);
}

bool strtod(string_view s, double* f, pn_error_code_t* error) {
    return pn_strtod(s.data(), s.size(), f, error);
}

input string::input() const { return check_c_obj(::pn::input{pn_view_input(data(), size())}); }
input string_ref::input() const { return check_c_obj(::pn::input{pn_view_input(data(), size())}); }
input string_view::input() const {
    return check_c_obj(::pn::input{pn_view_input(data(), size())});
}
output string::output() { return check_c_obj(::pn::output{pn_string_output(c_obj())}); }
output string_ref::output() const { return check_c_obj(::pn::output{pn_string_output(c_obj())}); }

}  // namespace pn
