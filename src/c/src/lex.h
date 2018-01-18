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

#ifndef PROCYON_LEX_H_
#define PROCYON_LEX_H_

#include <procyon.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef enum {
    // Virtual tokens
    PN_TOK_LINE_IN,
    PN_TOK_LINE_EQ,
    PN_TOK_LINE_OUT,

    // Fixed sequences
    PN_TOK_STAR,
    PN_TOK_ARRAY_IN,
    PN_TOK_ARRAY_OUT,
    PN_TOK_MAP_IN,
    PN_TOK_MAP_OUT,
    PN_TOK_COMMA,
    PN_TOK_STR_WRAP_EMPTY,  // >
    PN_TOK_STR_PIPE_EMPTY,  // |
    PN_TOK_STR_BANG,        // !
    PN_TOK_NULL,            // null
    PN_TOK_TRUE,            // true
    PN_TOK_FALSE,           // false
    PN_TOK_INF,             // inf
    PN_TOK_NEG_INF,         // -inf
    PN_TOK_NAN,             // nan

    // Matched sequences
    PN_TOK_KEY,       // key:    hi:     0:      -/.+:
    PN_TOK_QKEY,      // "key":  "hi":   "0":    "\\\"":
    PN_TOK_INT,       // 0       1       -1
    PN_TOK_FLOAT,     // 0.0     1e100   -0.5
    PN_TOK_DATA,      // $       $01     $ 01234567 89abcdef
    PN_TOK_STR,       // ""      "str"   "\n\\\0"
    PN_TOK_STR_WRAP,  // > string line
    PN_TOK_STR_PIPE,  // | string line
    PN_TOK_COMMENT,   // # comment

    PN_TOK_ERROR
} pn_token_type_t;

enum {
    PN_TOK_FLAG_VALUE = 0077,
    PN_TOK_FLAG_OK    = 0100,
    PN_TOK_FLAG_DONE  = 0200,
};

typedef struct {
    pn_file_t* file;

    struct {
        pn_token_type_t type;
        char*           begin;
        char*           end;
    } token;
    size_t  lineno;
    ssize_t indent;
    size_t  prev_width;
    bool    eq;

    struct {
        char* begin;  // data in current line (not NUL-terminated)
        char* end;    // data + size of current line
    } line;

    struct {
        char*  data;
        size_t size;
    } buffer;

    struct {
        size_t  count;
        size_t  size;
        ssize_t values[];
    } * levels;
} pn_lexer_t;

void pn_lexer_init(pn_lexer_t* lex, pn_file_t* file);
void pn_lexer_clear(pn_lexer_t* lex);
void pn_lexer_next(pn_lexer_t* lex, pn_error_t* error);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_LEX_H_
