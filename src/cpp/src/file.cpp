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

#include <errno.h>
#include <stdlib.h>
#include <system_error>

#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {

namespace internal {

static_assert(std::is_same<index_range<0>::type, indexes<>>::value, "");
static_assert(std::is_same<index_range<1>::type, indexes<0>>::value, "");
static_assert(std::is_same<index_range<2>::type, indexes<0, 1>>::value, "");
static_assert(std::is_same<index_range<3>::type, indexes<0, 1, 2>>::value, "");
static_assert(std::is_same<index_range<4>::type, indexes<0, 1, 2, 3>>::value, "");

}  // namespace internal

file& file::check() & {
    if (!c_obj() || error()) {
        throw std::system_error(errno, std::system_category());
    } else if (eof()) {
        throw std::runtime_error("unexpected eof");
    }
    return *this;
}

file_view file_view::check() {
    if (!c_obj() || error()) {
        throw std::system_error(errno, std::system_category());
    } else if (eof()) {
        throw std::runtime_error("unexpected eof");
    }
    return *this;
}

file open(string_view path, const char* mode) { return file{fopen(path.copy().c_str(), mode)}; }

bool parse(file_view in, value_ref out, pn_error_t* error) {
    return pn_parse(in.c_obj(), out.c_obj(), error);
}

}  // namespace pn
