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

#include <pn/array>

#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {

array::array() : _c_obj{vector_new<pn_array_t>(0)} {}

array::array(std::initializer_list<value> a) : _c_obj{vector_new<pn_array_t>(a.size())} {
    pn_value_t* out = _c_obj->values;
    for (const value& in : a) {
        pn_set(out++, 'x', in.c_obj());
    }
}

array::~array() { pn_arrayfree(_c_obj); }

int array::compare(array_cref other) const { return pn_arraycmp(*c_obj(), *other.c_obj()); }
int array_ref::compare(array_cref other) const { return pn_arraycmp(*c_obj(), *other.c_obj()); }
int array_cref::compare(array_cref other) const { return pn_arraycmp(*c_obj(), *other.c_obj()); }

static_assert(sizeof(array) == sizeof(pn_array_t*), "array size wrong");
static_assert(sizeof(array_ref) == sizeof(pn_array_t**), "array_ref size wrong");
static_assert(sizeof(array_cref) == sizeof(pn_array_t const**), "array_cref size wrong");

}  // namespace pn
