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

#ifndef PROCYON_PARSE_H_
#define PROCYON_PARSE_H_

#include "lex.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef enum {
    PN_EVT_NULL = 0,
    PN_EVT_BOOL,
    PN_EVT_INT,
    PN_EVT_FLOAT,
    PN_EVT_DATA,
    PN_EVT_STRING,
    PN_EVT_ARRAY_IN,
    PN_EVT_ARRAY_OUT,
    PN_EVT_MAP_IN,
    PN_EVT_MAP_OUT,
    PN_EVT_ERROR,
} pn_event_type_t;

typedef enum {
    PN_EVT_SHORT = 1 << 0,
    PN_EVT_LONG  = 1 << 1,
} pn_event_flag_t;

typedef struct {
    pn_event_type_t type;
    uint8_t         flags;
    pn_value_t      k;
    pn_value_t      x;
} pn_event_t;

typedef struct {
    pn_event_t evt;

    pn_lexer_t* lex;
    pn_value_t  data_acc;
    pn_value_t  string_acc;
    pn_value_t  key;

    size_t   stack_count;
    size_t   stack_size;
    uint8_t* stack;
} pn_parser_t;

typedef bool (*pn_parser_fn_t)(pn_parser_t* p, pn_error_t* error);

typedef enum {
    PN_PRS_EMIT_NONE,

    PN_PRS_EMIT_NULL,
    PN_PRS_EMIT_TRUE,
    PN_PRS_EMIT_FALSE,
    PN_PRS_EMIT_INF,
    PN_PRS_EMIT_NEG_INF,
    PN_PRS_EMIT_NAN,
    PN_PRS_EMIT_INT,
    PN_PRS_EMIT_FLOAT,
    PN_PRS_EMIT_DATA,
    PN_PRS_EMIT_ACC_DATA,
    PN_PRS_EMIT_STRING,
    PN_PRS_EMIT_ACC_STRING,
    PN_PRS_EMIT_SHORT_ARRAY_IN,
    PN_PRS_EMIT_SHORT_ARRAY_OUT,
    PN_PRS_EMIT_LONG_ARRAY_IN,
    PN_PRS_EMIT_LONG_ARRAY_OUT,
    PN_PRS_EMIT_SHORT_MAP_IN,
    PN_PRS_EMIT_SHORT_MAP_OUT,
    PN_PRS_EMIT_LONG_MAP_IN,
    PN_PRS_EMIT_LONG_MAP_OUT,
} pn_parser_emit_t;

typedef enum {
    PN_PRS_ACC_NONE,
    PN_PRS_ACC_DATA,
    PN_PRS_ACC_STRING,
    PN_PRS_ACC_SP,
    PN_PRS_ACC_NL,
} pn_parser_acc_t;

typedef enum {
    PN_PRS_KEY_NONE,
    PN_PRS_KEY_UNQUOTED,
    PN_PRS_KEY_QUOTED,
} pn_parser_key_t;

typedef struct {
    uint32_t error : 5;
    uint32_t emit : 5;
    uint32_t extend_count : 2;
    uint32_t extend0 : 6;
    uint32_t extend1 : 6;
    uint32_t acc0 : 3;
    uint32_t acc1 : 3;
    uint32_t key : 2;
} pn_parser_transition_t;

void pn_parser_init(pn_parser_t* p, pn_lexer_t* l, size_t stack_size);
void pn_parser_clear(pn_parser_t* p);
bool pn_parser_next(pn_parser_t* p, pn_error_t* error);

bool pn_parse_int(pn_parser_t* p, pn_error_t* error);
bool pn_parse_float(pn_parser_t* p, pn_error_t* error);
bool pn_parse_data(pn_parser_t* p, pn_error_t* error);
bool pn_flush_data(pn_parser_t* p, pn_error_t* error);
bool pn_parse_short_string(pn_parser_t* p, pn_error_t* error);
bool pn_flush_string(pn_parser_t* p, pn_error_t* error);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_PARSE_H_
