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

#ifndef PROCYON_COMMON_H_
#define PROCYON_COMMON_H_

#include <procyon.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define PN_CMP(x, y) (((x) < (y)) ? -1 : ((x) > (y)) ? 1 : 0)

pn_string_t* pn_string_new(const char* src, size_t len);
pn_data_t*   pn_data_new(const uint8_t* src, size_t len);

int pn_memncmp(const void* data1, size_t size1, const void* data2, size_t size2);

char* pn_dtoa(char* b, double x);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_COMMON_H_
