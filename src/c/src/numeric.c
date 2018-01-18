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

#include <ctype.h>
#include <procyon.h>
#include <stdlib.h>
#include <string.h>

#define PN_MAX_DIGITS64 19
#define DIGITS(SUFFIX)                                                                          \
    {                                                                                           \
        0, INT64_C(1##SUFFIX), INT64_C(2##SUFFIX), INT64_C(3##SUFFIX), INT64_C(4##SUFFIX),      \
                INT64_C(5##SUFFIX), INT64_C(6##SUFFIX), INT64_C(7##SUFFIX), INT64_C(8##SUFFIX), \
                INT64_C(9##SUFFIX)                                                              \
    }

static int64_t digits[PN_MAX_DIGITS64][10] = {
        [0]  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        [1]  = DIGITS(0),
        [2]  = DIGITS(00),
        [3]  = DIGITS(000),
        [4]  = DIGITS(0000),
        [5]  = DIGITS(00000),
        [6]  = DIGITS(000000),
        [7]  = DIGITS(0000000),
        [8]  = DIGITS(00000000),
        [9]  = DIGITS(000000000),
        [10] = DIGITS(0000000000),
        [11] = DIGITS(00000000000),
        [12] = DIGITS(000000000000),
        [13] = DIGITS(0000000000000),
        [14] = DIGITS(00000000000000),
        [15] = DIGITS(000000000000000),
        [16] = DIGITS(0000000000000000),
        [17] = DIGITS(00000000000000000),
        [18] = DIGITS(000000000000000000),
};

// Doesn't handle overflow or non-digits.
static int64_t pos_strtoll(const char* data, size_t size) {
    int64_t(*table)[10] = digits + size - 1;
    int64_t result      = 0;
    for (; size; ++data, --size, --table) {
        result += (*table)[*data - '0'];
    }
    return result;
}

bool pn_strtoll(const char* data, size_t size, int64_t* i, pn_error_code_t* error) {
    pn_error_code_t local_error;
    if (!error) {
        error = &local_error;
    }

    bool negative = (size && (*data == '-'));
    bool positive = (size && (*data == '+'));
    if (negative || positive) {
        ++data, --size;
    }

    for (size_t i = 0; i < size; ++i) {
        if (!isdigit(data[i])) {
            return *error = PN_ERROR_INVALID_INT, false;
        }
    }

    if (size > PN_MAX_DIGITS64) {
        return *error = PN_ERROR_INT_OVERFLOW, false;
    } else if (size == PN_MAX_DIGITS64) {
        int64_t head = digits[PN_MAX_DIGITS64 - 1][*data - '0'];
        int64_t tail = pos_strtoll(data + 1, size - 1);
        if (negative) {
            if ((INT64_MIN + head) > -tail) {
                return *error = PN_ERROR_INT_OVERFLOW, false;
            }
            *i = -tail - head;
        } else {
            if ((INT64_MAX - head) < tail) {
                return *error = PN_ERROR_INT_OVERFLOW, false;
            }
            *i = tail + head;
        }
    } else if (size > 0) {
        *i = negative ? -pos_strtoll(data, size) : pos_strtoll(data, size);
    } else {
        return *error = PN_ERROR_INVALID_INT, false;
    }

    return true;
}
