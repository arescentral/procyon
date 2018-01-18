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

#include <pn/map>

#include "../../c/src/vector.h"
#include "./common.hpp"

namespace pn {

namespace internal {

void map_set(pn_map_t** m, string key, value x) {
    pn_value_t k = {PN_STRING, {.s = nullptr}};
    std::swap(k.s, *key.c_obj());
    pn_mapset(m, 'X', 'X', &k, x.c_obj());
}

value_ref map_force(pn_map_t** m, string key) {
    pn_value_t* x = pn_mapget(*m, 'S', key.data(), key.size());
    if (!x) {
        pn_value_t k = {PN_STRING, {.s = nullptr}};
        std::swap(k.s, *key.c_obj());
        pn_mapset(m, 'X', 'N', &k, &x);
    }
    return value_ref{x};
}

value_ref map_force(pn_map_t** m, const char* data, int size) {
    pn_value_t* x = pn_mapget(*m, 'S', data, size);
    if (!x) {
        pn_mapset(m, 'S', 'N', data, size, &x);
    }
    return value_ref{x};
}

value_cref map_get(const pn_map_t* m, const char* data, int size) {
    const pn_value_t* x = pn_mapget_const(m, 'S', data, size);
    if (!x) {
        x = &pn_null;
    }
    return value_cref{x};
}

void map_clear(pn_map_t* m) {
    while (m->count > 0) {
        size_t index = --m->count;
        free(m->values[index].key);
        pn_clear(&m->values[index].value);
    }
}

}  // namespace internal

map::map() : _c_obj{vector_new<pn_map_t>(0)} {}

map::map(std::initializer_list<std::pair<string, value>> m)
        : _c_obj{vector_new<pn_map_t>(m.size())} {
    pn_kv_pair_t* out = _c_obj->values;
    for (const std::pair<string, value>& in : m) {
        pn_value_t k;
        pn_set(&k, 'S', in.first.data(), in.first.size());
        out->key = k.s;
        pn_set(&out++->value, 'x', in.second.c_obj());
    }
}

map::~map() { pn_mapfree(_c_obj); }

static_assert(sizeof(map) == sizeof(pn_map_t*), "map size wrong");
static_assert(sizeof(map_ref) == sizeof(pn_map_t**), "map_ref size wrong");
static_assert(sizeof(map_cref) == sizeof(pn_map_t const**), "map_cref size wrong");

}  // namespace pn
