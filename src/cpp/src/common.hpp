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

#ifndef PROCYON_COMMON_HPP_
#define PROCYON_COMMON_HPP_

#include <system_error>

#include "../../c/src/vector.h"

namespace pn {

template <typename T>
static T* vector_new(size_t n) {
    T* x;
    VECTOR_INIT(&x, n);
    return x;
}

template <typename T>
T check_c_obj(T t) {
    if (!t.c_obj()) {
        throw std::system_error(errno, std::system_category());
    }
    return t;
}

template <typename T>
struct is_copyable {
    constexpr static bool value =
            std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value;
};

template <typename T>
struct is_nothrow_movable {
    constexpr static bool value = std::is_nothrow_move_constructible<T>::value &&
                                  std::is_nothrow_move_assignable<T>::value;
};

template <typename out, typename in>
struct conversion {
    constexpr static bool is_nothrow = std::is_nothrow_constructible<out, in>::value &&
                                       std::is_nothrow_assignable<out, in>::value;
    constexpr static bool can_throw = std::is_constructible<out, in>::value &&
                                      std::is_assignable<out, in>::value &&
                                      !std::is_nothrow_constructible<out, in>::value &&
                                      !std::is_nothrow_assignable<out, in>::value;
    constexpr static bool fails =
            !std::is_constructible<out, in>::value && !std::is_assignable<out, in>::value;
};

}  // namespace pn

#endif  // PROCYON_COMMON_HPP_
