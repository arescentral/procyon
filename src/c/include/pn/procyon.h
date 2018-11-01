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

#ifndef PROCYON_H_
#define PROCYON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct pn_value   pn_value_t;
typedef struct pn_data    pn_data_t;
typedef struct pn_string  pn_string_t;
typedef struct pn_array   pn_array_t;
typedef struct pn_map     pn_map_t;
typedef struct pn_kv_pair pn_kv_pair_t;

typedef bool    pn_bool_t;
typedef int64_t pn_int_t;
typedef double  pn_float_t;
typedef FILE    pn_file_t;

typedef struct pn_error pn_error_t;

typedef enum {
    PN_OK = 0,
    PN_ERROR_INTERNAL,
    PN_ERROR_SYSTEM,

    PN_ERROR_OUTDENT,

    PN_ERROR_CHILD,
    PN_ERROR_SIBLING,
    PN_ERROR_SUFFIX,
    PN_ERROR_LONG,
    PN_ERROR_SHORT,
    PN_ERROR_ARRAY_END,
    PN_ERROR_MAP_KEY,
    PN_ERROR_MAP_END,

    PN_ERROR_CTRL,
    PN_ERROR_NONASCII,
    PN_ERROR_UTF8_HEAD,
    PN_ERROR_UTF8_TAIL,
    PN_ERROR_BADCHAR,
    PN_ERROR_DATACHAR,
    PN_ERROR_PARTIAL,
    PN_ERROR_BADWORD,
    PN_ERROR_BADESC,
    PN_ERROR_BADUESC,
    PN_ERROR_STREOL,
    PN_ERROR_BANG_SUFFIX,
    PN_ERROR_BANG_LAST,

    PN_ERROR_INT_OVERFLOW,
    PN_ERROR_INVALID_INT,

    PN_ERROR_FLOAT_OVERFLOW,
    PN_ERROR_INVALID_FLOAT,

    PN_ERROR_RECURSION,
} pn_error_code_t;

struct pn_error {
    pn_error_code_t code;
    size_t          lineno;
    size_t          column;
};

const char* pn_strerror(pn_error_code_t code);

typedef enum {
    PN_NULL   = 0,
    PN_BOOL   = 1,
    PN_INT    = 2,
    PN_FLOAT  = 3,
    PN_DATA   = 4,
    PN_STRING = 5,
    PN_ARRAY  = 6,
    PN_MAP    = 7,
} pn_type_t;

struct pn_value {
    pn_type_t type;
    union {
        pn_bool_t    b;
        pn_int_t     i;
        pn_float_t   f;
        pn_data_t*   d;
        pn_string_t* s;
        pn_array_t*  a;
        pn_map_t*    m;
    };
};

// Well-known constants.
extern const struct pn_value pn_null;        // null
extern const struct pn_value pn_true;        // true
extern const struct pn_value pn_false;       // false
extern const struct pn_value pn_inf;         // inf
extern const struct pn_value pn_neg_inf;     // -inf
extern const struct pn_value pn_nan;         // nan
extern const struct pn_value pn_zero;        // 0
extern const struct pn_value pn_zerof;       // 0.0
extern const struct pn_value pn_dataempty;   // $
extern const struct pn_value pn_strempty;    // ""
extern const struct pn_value pn_arrayempty;  // []
extern const struct pn_value pn_mapempty;    // {}

// Constructors.
//
// n: null (no argument) N: (set *(pn_value_t** arg) to dst
// ?: bool
// i: int I: unsigned int
// b: int8, B: uint8, h: int16, H: uint16, l: int32, L: uint32, q: int64, Q: uint64
// p: intptr_t, P: uintptr_t (or a pointer), z: size_t, Z: ssize_t
// f: float, d: double
// s: const char* (NUL-terminated), S: const char* and size_t
// c: char (becomes a 1-character string), C: uint32_t (becomes a 1-rune string)
// $: const uint8_t* and size_t, #: size_t (zeroed out data)
// a: const pn_array_t* (copy), A: pn_array_t* (move)
// m: const pn_map_t* (copy), M: pn_map_t* (move)
// x: const pn_value_t* (copy), X: pn_value_t* (move)
void pn_set(pn_value_t* dst, int format, ...);
void pn_setv(pn_value_t* dst, const char* format, ...);
void pn_setkv(pn_value_t* dst, const char* format, ...);

void pn_clear(pn_value_t* x);
void pn_swap(pn_value_t* x, pn_value_t* y);
int  pn_cmp(const pn_value_t* x, const pn_value_t* y);

// Sequence of bytes with no assigned interpretation.
struct pn_data {
    size_t  count;
    size_t  size;
    uint8_t values[];
};

pn_data_t* pn_datadup(const pn_data_t* d);
int        pn_datacmp(const pn_data_t* d1, const pn_data_t* d2);
void       pn_datacat(pn_data_t** d, const uint8_t* data, size_t size);
void       pn_dataresize(pn_data_t** d, size_t size);

// Sequence of unicode code points, stored as UTF-8.
struct pn_string {
    size_t count;
    size_t size;
    char   values[];
};

pn_string_t* pn_strdup(const pn_string_t* s);
int          pn_strcmp(const pn_string_t* s1, const pn_string_t* s2);
void         pn_strcat(pn_string_t** s, const char* src);
void         pn_strncat(pn_string_t** s, const char* src, size_t len);
bool         pn_strtoll(const char* data, size_t size, int64_t* i, pn_error_code_t* error);
bool         pn_strtod(const char* data, size_t size, double* f, pn_error_code_t* error);
void         pn_strresize(pn_string_t** s, size_t size);
void         pn_strreplace(
                pn_string_t** s, size_t at, size_t remove_size, const char* replace_data,
                size_t replace_size);

int32_t pn_rune(const char* data, size_t size, size_t index);
// Requires: 0 <= *index < size
size_t pn_rune_next(const char* data, size_t size, size_t index);
// Requires: 0 < *index <= size
size_t pn_rune_prev(const char* data, size_t size, size_t index);

// Sequence of array values.
struct pn_array {
    size_t     count;
    size_t     size;
    pn_value_t values[];
};

pn_array_t* pn_arraydup(const pn_array_t* a);
void        pn_arrayfree(pn_array_t* a);
int         pn_arraycmp(const pn_array_t* l1, const pn_array_t* l2);
void        pn_arrayext(pn_array_t** a, const char* format, ...);
void        pn_arrayins(pn_array_t** a, size_t index, int format, ...);
void        pn_arraydel(pn_array_t** a, size_t index);
void        pn_arrayresize(pn_array_t** a, size_t size);

struct pn_kv_pair {
    pn_string_t* key;
    pn_value_t   value;
};

// Sequence of (string key, value) pairs.
struct pn_map {
    size_t       count;
    size_t       size;
    pn_kv_pair_t values[];
};

pn_map_t* pn_mapdup(const pn_map_t* m);
void      pn_mapfree(pn_map_t* m);
int       pn_mapcmp(const pn_map_t* m1, const pn_map_t* m2);
// Returns NULL if not found (note: different from &pn_null).
pn_value_t*       pn_mapget(pn_map_t* m, int key_format, ...);
const pn_value_t* pn_mapget_const(const pn_map_t* m, int key_format, ...);
// Returns true if a new map entry was created, false if old entry was updated.
bool pn_mapset(pn_map_t** m, int key_format, int value_format, ...);
// Returns true if there was an element to delete.
bool pn_mapdel(pn_map_t** m, int key_format, ...);
// Returns true if there was an element to pop.
bool pn_mappop(pn_map_t** m, pn_value_t* x, int key_format, ...);

bool pn_parse(pn_file_t* file, pn_value_t* out, pn_error_t* error);

enum {
    PN_DUMP_DEFAULT = 0,
    PN_DUMP_SHORT   = 1,
};
bool pn_dump(pn_file_t* file, int flags, int format, ...);

// Opens a procyon string for stdio reading and writing.
// `d` or `s` must point to valid values of the given type.
//
// Supported modes:
//
//     mode  read  write  truncate  initial position
//     ----  ----  -----  --------  ----------------
//     r     Y     N      N         start of argument
//     r+    Y     Y      N         start of argument
//     w     N     Y      Y         start of argument
//     w+    Y     Y      Y         start of argument
//     a     N     Y      N         end of argument
//     a+    Y     Y      N         end of argument
//
pn_file_t* pn_open_path(const char* path, const char* mode);
pn_file_t* pn_open_data(pn_data_t** d, const char* mode);
pn_file_t* pn_open_string(pn_string_t** s, const char* mode);
pn_file_t* pn_open_view(const void* data, size_t size);  // mode is always "r".

// Format strings: "Hello, {0} {1}"
bool pn_format(pn_file_t* file, const char* output_format, const char* input_format, ...);

bool pn_read(pn_file_t* file, const char* format, ...);
bool pn_write(pn_file_t* file, const char* format, ...);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_H_
