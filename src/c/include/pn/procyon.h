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
#include <stddef.h>
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
typedef struct pn_input   pn_input_t;
typedef struct pn_output  pn_output_t;

typedef uint32_t pn_rune_t;

typedef bool    pn_bool_t;
typedef int64_t pn_int_t;
typedef double  pn_float_t;

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
// p: intptr_t, P: uintptr_t (or a pointer), z: size_t, Z: ptrdiff_t
// f: float, d: double
// s: const char* (NUL-terminated), S: const char* and size_t
// u: const uint16_t* (UTF-16) and size_t, U: const uint32_t* (UTF-32) and size_t
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

size_t pn_rune_width(pn_rune_t rune);
size_t pn_str_width(const char* data, size_t size);

bool pn_isrune(pn_rune_t r);

bool pn_isalnum(pn_rune_t r);    // Abc123あいう英美四㊀㊁㊂
bool pn_isalpha(pn_rune_t r);    // Abcあいう英美四
bool pn_iscntrl(pn_rune_t r);    // \0\n\t\x9f
bool pn_isdigit(pn_rune_t r);    // 123۱۲۳𝟙𝟚𝟛
bool pn_islower(pn_rune_t r);    // abcáḅçａｂｃ
bool pn_isnumeric(pn_rune_t r);  // 123۱۲۳½⅔¾㊀㊁㊂
bool pn_isprint(pn_rune_t r);    // A$ :)
bool pn_ispunct(pn_rune_t r);    // 「(+±-〜:)」
bool pn_isspace(pn_rune_t r);    // \x20\u3000
bool pn_isupper(pn_rune_t r);    // ABCÁḄÇＡＢＣ
bool pn_istitle(pn_rune_t r);    // ǅᾼ

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

typedef enum {
    PN_INPUT_TYPE_INVALID = 0,
    PN_INPUT_TYPE_C_FILE  = 1,
    PN_INPUT_TYPE_STDIN   = 2,
    PN_INPUT_TYPE_VIEW    = 5,
} pn_input_type_t;

struct pn_input {
    pn_input_type_t type;
    union {
        FILE*                 c_file;
        struct pn_input_view* view;
    };
};

typedef enum {
    PN_OUTPUT_TYPE_INVALID = 0,
    PN_OUTPUT_TYPE_C_FILE  = 1,
    PN_OUTPUT_TYPE_STDOUT  = 3,
    PN_OUTPUT_TYPE_STDERR  = 4,
    PN_OUTPUT_TYPE_DATA    = 6,
    PN_OUTPUT_TYPE_STRING  = 7,
} pn_output_type_t;

struct pn_output {
    pn_output_type_t type;
    union {
        FILE*         c_file;
        pn_data_t**   data;
        pn_string_t** string;
    };
};

bool pn_parse(pn_input_t* input, pn_value_t* out, pn_error_t* error);

enum {
    PN_DUMP_DEFAULT = 0,
    PN_DUMP_SHORT   = 1,
};
bool pn_dump(pn_output_t* output, int flags, int format, ...);

typedef enum {
    PN_TEXT          = 0,
    PN_BINARY        = 1,
    PN_APPEND_TEXT   = 2,
    PN_APPEND_BINARY = 3,
} pn_path_flags_t;

pn_input_t pn_path_input(const char* path, pn_path_flags_t flags);
pn_input_t pn_file_input(FILE* f);
pn_input_t pn_data_input(const pn_data_t* d);
pn_input_t pn_string_input(const pn_string_t* s);
pn_input_t pn_view_input(const void* data, size_t size);

pn_output_t pn_path_output(const char* path, pn_path_flags_t flags);
pn_output_t pn_file_output(FILE* f);
pn_output_t pn_data_output(pn_data_t** d);
pn_output_t pn_string_output(pn_string_t** s);

bool pn_input_close(pn_input_t* in);
bool pn_input_eof(const pn_input_t* in);
bool pn_input_error(const pn_input_t* in);
bool pn_output_close(pn_output_t* out);
bool pn_output_eof(const pn_output_t* out);
bool pn_output_error(const pn_output_t* out);

extern pn_input_t  pn_stdin;
extern pn_output_t pn_stdout;
extern pn_output_t pn_stderr;

// Format strings: "Hello, {0} {1}"
bool pn_format(pn_output_t* out, const char* output_format, const char* input_format, ...);

bool pn_read(pn_input_t* in, const char* format, ...);
bool pn_write(pn_output_t* out, const char* format, ...);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_H_
