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

#ifndef PROCYON_VECTOR_H_
#define PROCYON_VECTOR_H_

#include <stdlib.h>

#ifdef __cplusplus
#include <type_traits>
#define VECTOR_CAST(V, M) reinterpret_cast<typename std::remove_reference<decltype(V)>::type>(M)
#else
#define VECTOR_CAST(V, M) M
#endif

#define VECTOR_INIT(V, N)                                                     \
    do {                                                                      \
        size_t __count = (N);                                                 \
        size_t needed  = sizeof(**(V)) + (__count * sizeof(*(*(V))->values)); \
        *(V)           = VECTOR_CAST(*(V), malloc(needed));                   \
        (*(V))->count  = __count;                                             \
        (*(V))->size   = needed;                                              \
    } while (false)

#define VECTOR_EXTEND(V, N)                                                        \
    do {                                                                           \
        size_t __count = (N);                                                      \
        (*(V))->count += __count;                                                  \
        size_t needed = sizeof(**(V)) + ((*(V))->count * sizeof(*(*(V))->values)); \
        while ((*(V))->size < needed) {                                            \
            (*(V))->size *= 2;                                                     \
        }                                                                          \
        *(V) = VECTOR_CAST(*(V), realloc(*(V), (*(V))->size));                     \
    } while (false)

#define VECTOR_FIRST(V) ((V)->values[0])
#define VECTOR_LAST(V) ((V)->values[(V)->count - 1])

#endif  // PROCYON_VECTOR_H_
