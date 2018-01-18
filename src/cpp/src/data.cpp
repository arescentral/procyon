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

#include <pn/data>

#include <pn/file>
#include <pn/string>

#include "../../c/src/common.h"
#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {

string_view data::as_string() const {
    return string_view{reinterpret_cast<const char*>(data()), size()};
}

string_view data_ref::as_string() const {
    return string_view{reinterpret_cast<const char*>(data()), size()};
}

string_view data_view::as_string() const {
    return string_view{reinterpret_cast<const char*>(data()), size()};
}

data_::data_(const_pointer data, size_type size) : _c_obj{pn_data_new(data, size)} {}

data_::~data_() { free(_c_obj); }

static_assert(sizeof(data) == sizeof(pn_data_t*), "data size wrong");
static_assert(sizeof(data_ref) == sizeof(pn_data_t**), "data_ref size wrong");

static void resize_data(pn_data_t** d, data::size_type n) {
    if (n > static_cast<int>((*d)->count)) {
        int delta = n - (*d)->count;
        VECTOR_EXTEND(d, delta);
        memset((*d)->values + n - delta, 0x00, delta);
    } else {
        (*d)->count = n;
    }
}

void data::resize(size_type n) { resize_data(c_obj(), n); }
void data_ref::resize(size_type n) { resize_data(c_obj(), n); }

int data_view::compare(data_view other) const {
    return pn_memncmp(data(), size(), other.data(), other.size());
}

file data::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file data_ref::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file data_view::open() const { return check_c_obj(file{pn_open_view(data(), size())}); }
file data::open(const char* mode) { return check_c_obj(file{pn_open_data(c_obj(), mode)}); }
file data_ref::open(const char* mode) { return check_c_obj(file{pn_open_data(c_obj(), mode)}); }

}  // namespace pn
