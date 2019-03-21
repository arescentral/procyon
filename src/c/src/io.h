// Copyright 2019 The Procyon Authors
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

#ifndef PROCYON_IO_H_
#define PROCYON_IO_H_

#include <pn/procyon.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

bool pn_raw_read(pn_file_t f, void* data, size_t size);
bool pn_raw_write(pn_file_t f, const void* data, size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_IO_H_
