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

#include "common.h"

#include <string.h>

#include "./vector.h"

pn_string_t* pn_string_new(const char* src, size_t len) {
    pn_string_t* s;
    VECTOR_INIT(&s, len + 1);
    if (len) {
        memcpy(&s->values, src, len);
    }
    s->values[len] = '\0';
    return s;
}

pn_data_t* pn_data_new(const uint8_t* src, size_t len) {
    pn_data_t* d;
    VECTOR_INIT(&d, len);
    if (len) {
        memcpy(&d->values, src, len);
    }
    return d;
}

int pn_memncmp(const void* data1, size_t size1, const void* data2, size_t size2) {
    if (size1 && size2) {
        int c = memcmp(data1, data2, (size1 < size2) ? size1 : size2);
        if (c) {
            return c;
        }
    }
    return PN_CMP(size1, size2);
}
