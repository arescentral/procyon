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

#ifndef PROCYON_UTF8_H_
#define PROCYON_UTF8_H_

#include <procyon.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void   pn_chr(uint8_t rune, char* data, size_t* size);
void   pn_unichr(uint32_t rune, char* data, size_t* size);
size_t pn_rune_width(uint32_t rune);
size_t pn_str_width(const char* data, size_t size);
bool   pn_isprint(uint32_t rune);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_UTF8_H_
