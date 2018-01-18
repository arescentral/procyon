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

#include <procyon.h>

#include <assert.h>
#include <string.h>

#include "./common.h"
#include "./gen_table.h"
#include "./lex.h"
#include "./parse.h"
#include "./vector.h"

// clang-format off
static const uint32_t hex[] = {
    ['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    ['A'] = 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    ['a'] = 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
};

static const char escape[] = {
    ['\\'] = '\\',
    ['"']  = '"',
    ['/']  = '/',
    ['b']  = '\b',
    ['f']  = '\f',
    ['n']  = '\n',
    ['r']  = '\r',
    ['t']  = '\t',
};
// clang-format on

static bool parser_fail(const pn_parser_t* p, pn_error_t* error, pn_error_code_t code) {
    error->code   = code;
    error->lineno = p->lex->lineno;
    error->column = 1 + (p->lex->token.begin - p->lex->line.begin);
    return false;
}

bool pn_parse_int(pn_parser_t* p, pn_error_t* error) {
    int64_t         i;
    pn_error_code_t code;
    if (!pn_strtoll(p->lex->token.begin, p->lex->token.end - p->lex->token.begin, &i, &code)) {
        return parser_fail(p, error, code);
    }
    pn_set(&p->evt.x, 'q', i);
    return true;
}

bool pn_parse_float(pn_parser_t* p, pn_error_t* error) {
    double          f;
    pn_error_code_t code;
    pn_strtod(p->lex->token.begin, p->lex->token.end - p->lex->token.begin, &f, &code);
    if (code && (code != PN_ERROR_FLOAT_OVERFLOW)) {
        return parser_fail(p, error, code);
    }
    pn_set(&p->evt.x, 'd', f);
    return true;
}

static void parse_data_value(pn_data_t** d, const pn_lexer_t* lex) {
    for (const char *ch = lex->token.begin + 1, *end = lex->token.end; ch != end; ++ch) {
        if ((*ch == ' ') || (*ch == '\t')) {
            continue;
        }
        int     b1   = *(ch++);
        int     b2   = *ch;
        uint8_t byte = (hex[b1] << 4) | hex[b2];
        pn_datacat(d, &byte, 1);
    }
}

bool pn_parse_data(pn_parser_t* p, pn_error_t* error) {
    (void)error;
    pn_set(&p->evt.x, 'x', &pn_dataempty);
    parse_data_value(&p->evt.x.d, p->lex);
    return true;
}

bool pn_flush_data(pn_parser_t* p, pn_error_t* error) {
    (void)error;
    pn_set(&p->evt.x, 'X', &p->data_acc);
    pn_set(&p->data_acc, 'x', &pn_dataempty);
    return true;
}

static void parse_long_string_value(pn_string_t** s, const pn_lexer_t* lex) {
    const char* begin = lex->token.begin + 1;
    if (begin == lex->token.end) {
        return;
    } else if ((*begin == ' ') || (*begin == '\t')) {
        ++begin;
    }
    pn_strncat(s, begin, lex->token.end - begin);  // XXX
}

bool pn_flush_string(pn_parser_t* p, pn_error_t* error) {
    (void)error;
    pn_set(&p->evt.x, 'X', &p->string_acc);
    pn_set(&p->string_acc, 'x', &pn_strempty);
    return true;
}

static void parse_short_string_value(pn_string_t** s, const char* begin, const char* end) {
    for (const char* ch = begin; ch != end; ++ch) {
        if (*ch != '\\') {
            pn_strncat(s, ch, 1);
            continue;
        }
        uint8_t esc   = *(++ch);
        int     count = 0;
        if (esc == 'u') {
            count = 4;
        } else if (esc == 'U') {
            count = 8;
        } else {
            pn_strncat(s, &escape[esc], 1);
            continue;
        }
        uint32_t u = 0;
        for (int i = 0; i < count; ++i) {
            uint8_t b = *(++ch);
            u         = (u << 4) | hex[b];
        }
        if (u < 0x80) {
            char utf[] = {u};
            pn_strncat(s, utf, 1);
        } else if (u < 0x800) {
            char utf[] = {0300 | ((u >> 6) & 0037), 0200 | (u & 0077)};
            pn_strncat(s, utf, 2);
        } else if (u < 0x10000) {
            char utf[] = {0340 | ((u >> 12) & 0017), 0200 | ((u >> 6) & 0077), 0200 | (u & 0077)};
            pn_strncat(s, utf, 3);
        } else {
            char utf[] = {0360 | ((u >> 18) & 0x007), 0200 | ((u >> 12) & 0077),
                          0200 | ((u >> 6) & 0077), 0200 | (u & 0077)};
            pn_strncat(s, utf, 4);
        }
    }
}

bool pn_parse_short_string(pn_parser_t* p, pn_error_t* error) {
    (void)error;
    pn_set(&p->evt.x, 'x', &pn_strempty);
    parse_short_string_value(&p->evt.x.s, p->lex->token.begin + 1, p->lex->token.end - 1);
    return true;
}

static void parse_key(pn_parser_t* p, pn_parser_key_t key) {
    if (key == PN_PRS_KEY_QUOTED) {
        pn_set(&p->key, 'x', &pn_strempty);
        parse_short_string_value(&p->key.s, p->lex->token.begin + 1, p->lex->token.end - 2);
    } else {
        pn_set(&p->key, 'S', p->lex->token.begin, p->lex->token.end - p->lex->token.begin - 1);
    }
}

bool pn_parse(pn_file_t* file, pn_value_t* out, pn_error_t* error) {
    pn_error_t ignore_error;
    error = error ? error : &ignore_error;
    pn_lexer_t lex;
    pn_lexer_init(&lex, file);
    pn_parser_t prs;
    pn_parser_init(&prs, &lex, 64);

    pn_value_t stack[128];
    size_t     stack_count = 0;
    while (pn_parser_next(&prs, error)) {
        if (!((prs.evt.type == PN_EVT_ARRAY_OUT) || (prs.evt.type == PN_EVT_MAP_OUT))) {
            pn_value_t* k = &stack[stack_count++];
            pn_value_t* x = &stack[stack_count++];

            pn_set(k, 'X', &prs.evt.k);
            switch (prs.evt.type) {
                case PN_EVT_NULL:
                case PN_EVT_BOOL:
                case PN_EVT_INT:
                case PN_EVT_FLOAT:
                case PN_EVT_DATA:
                case PN_EVT_STRING: pn_set(x, 'X', &prs.evt.x); break;

                case PN_EVT_ARRAY_IN: pn_setv(x, ""); continue;
                case PN_EVT_MAP_IN: pn_setkv(x, ""); continue;

                default:
                    pn_parser_clear(&prs);
                    pn_lexer_clear(&lex);
                    return false;
            }
        }

        stack_count -= 2;
        pn_value_t* k = &stack[stack_count];
        pn_value_t* x = &stack[stack_count + 1];
        if (stack_count == 0) {
            pn_set(out, 'X', x);
            continue;
        }

        pn_value_t* top = &stack[stack_count - 1];
        if (top->type == PN_ARRAY) {
            pn_arrayext(&top->a, "X", x);
        } else if (top->type == PN_MAP) {
            pn_mapset(&top->m, 'X', 'X', k, x);
        }
    }

    pn_parser_clear(&prs);
    pn_lexer_clear(&lex);
    return true;
}

void pn_parser_init(pn_parser_t* p, pn_lexer_t* l, size_t stack_size) {
    pn_parser_t parser = {.lex = l};
    pn_set(&parser.data_acc, 'x', &pn_dataempty);
    pn_set(&parser.string_acc, 'x', &pn_strempty);
    parser.stack_size  = stack_size;
    parser.stack_count = 1;
    parser.stack       = malloc(parser.stack_size);
    parser.stack[0]    = 0;
    *p                 = parser;
}

void pn_parser_clear(pn_parser_t* p) {
    free(p->stack);
    pn_clear(&p->key);
    pn_clear(&p->string_acc);
    pn_clear(&p->data_acc);
    pn_clear(&p->evt.k);
    pn_clear(&p->evt.x);
}

static void emit(pn_parser_t* p, pn_event_type_t type, pn_event_flag_t flag) {
    p->evt.type  = type;
    p->evt.flags = flag;
}

static void emit_value(
        pn_parser_t* p, pn_event_type_t type, pn_event_flag_t flag, const pn_value_t* value) {
    p->evt.type  = type;
    p->evt.flags = flag;
    p->evt.x     = *value;
}

static void emit_fn(
        pn_parser_t* p, pn_event_type_t type, pn_event_flag_t flag, pn_parser_fn_t fn,
        pn_error_t* error) {
    p->evt.type  = type;
    p->evt.flags = flag;
    if (!fn(p, error)) {
        p->evt.type = PN_EVT_ERROR;
    }
}

bool pn_parser_next(pn_parser_t* p, pn_error_t* error) {
    pn_clear(&p->evt.x);
    while (p->stack_count) {
        uint8_t state = p->stack[--p->stack_count];
        pn_lexer_next(p->lex, error);

        pn_token_type_t token = p->lex->token.type;
        if (token == PN_TOK_ERROR) {
            p->evt.type = PN_EVT_ERROR;
            return true;
        }

        const pn_parser_transition_t* t        = &parse_defs[parse_table[state][token]];
        pn_parser_emit_t              extend[] = {t->extend0, t->extend1};
        pn_parser_acc_t               acc[]    = {t->acc0, t->acc1};

        if (t->error) {
            p->evt.type   = PN_EVT_ERROR;
            error->code   = t->error;
            error->lineno = p->lex->lineno;
            if (token <= PN_TOK_LINE_OUT) {
                if (error->lineno > 1) {
                    --error->lineno;
                }
                if (p->lex->prev_width <= 1) {
                    error->column = 1;
                } else {
                    error->column = p->lex->prev_width - 1;
                }
            } else {
                error->column = 1 + (p->lex->token.begin - p->lex->line.begin);
            }
            return true;
        } else if ((p->stack_count + t->extend_count) > p->stack_size) {
            p->evt.type   = PN_EVT_ERROR;
            error->code   = PN_ERROR_RECURSION;
            error->lineno = p->lex->lineno;
            error->column = 1 + (p->lex->token.begin - p->lex->line.begin);
            return true;
        }

        for (size_t i = 0; i < 2; ++i) {
            switch (acc[i]) {
                case PN_PRS_ACC_NONE: break;
                case PN_PRS_ACC_DATA: parse_data_value(&p->data_acc.d, p->lex); break;
                case PN_PRS_ACC_STRING: parse_long_string_value(&p->string_acc.s, p->lex); break;
                case PN_PRS_ACC_SP: pn_strncat(&p->string_acc.s, " ", 1); break;
                case PN_PRS_ACC_NL: pn_strncat(&p->string_acc.s, "\n", 1); break;
            }
        }

        if (t->emit) {
            pn_clear(&p->evt.k);
            pn_set(&p->evt.k, 'X', &p->key);
        }
        if (t->key) {
            parse_key(p, t->key);
        }

        for (int i = 0; i < t->extend_count; ++i) {
            p->stack[p->stack_count++] = extend[i];
        }

        switch ((pn_parser_emit_t)t->emit) {
            case PN_PRS_EMIT_NONE: break;

            case PN_PRS_EMIT_NULL: emit_value(p, PN_EVT_NULL, PN_EVT_SHORT, &pn_null); return true;
            case PN_PRS_EMIT_TRUE: emit_value(p, PN_EVT_BOOL, PN_EVT_SHORT, &pn_true); return true;
            case PN_PRS_EMIT_FALSE:
                emit_value(p, PN_EVT_BOOL, PN_EVT_SHORT, &pn_false);
                return true;
            case PN_PRS_EMIT_INF: emit_value(p, PN_EVT_FLOAT, PN_EVT_SHORT, &pn_inf); return true;
            case PN_PRS_EMIT_NEG_INF:
                emit_value(p, PN_EVT_FLOAT, PN_EVT_SHORT, &pn_neg_inf);
                return true;
            case PN_PRS_EMIT_NAN: emit_value(p, PN_EVT_FLOAT, PN_EVT_SHORT, &pn_nan); return true;

            case PN_PRS_EMIT_INT:
                emit_fn(p, PN_EVT_INT, PN_EVT_SHORT, pn_parse_int, error);
                return true;
            case PN_PRS_EMIT_FLOAT:
                emit_fn(p, PN_EVT_FLOAT, PN_EVT_SHORT, pn_parse_float, error);
                return true;
            case PN_PRS_EMIT_STRING:
                emit_fn(p, PN_EVT_STRING, PN_EVT_SHORT, pn_parse_short_string, error);
                return true;
            case PN_PRS_EMIT_ACC_STRING:
                emit_fn(p, PN_EVT_STRING, PN_EVT_LONG, pn_flush_string, error);
                return true;
            case PN_PRS_EMIT_DATA:
                emit_fn(p, PN_EVT_DATA, PN_EVT_SHORT, pn_parse_data, error);
                return true;
            case PN_PRS_EMIT_ACC_DATA:
                emit_fn(p, PN_EVT_DATA, PN_EVT_LONG, pn_flush_data, error);
                return true;

            case PN_PRS_EMIT_SHORT_ARRAY_IN: emit(p, PN_EVT_ARRAY_IN, PN_EVT_SHORT); return true;
            case PN_PRS_EMIT_SHORT_ARRAY_OUT: emit(p, PN_EVT_ARRAY_OUT, PN_EVT_SHORT); return true;
            case PN_PRS_EMIT_SHORT_MAP_IN: emit(p, PN_EVT_MAP_IN, PN_EVT_SHORT); return true;
            case PN_PRS_EMIT_SHORT_MAP_OUT: emit(p, PN_EVT_MAP_OUT, PN_EVT_SHORT); return true;
            case PN_PRS_EMIT_LONG_ARRAY_IN: emit(p, PN_EVT_ARRAY_IN, PN_EVT_LONG); return true;
            case PN_PRS_EMIT_LONG_ARRAY_OUT: emit(p, PN_EVT_ARRAY_OUT, PN_EVT_LONG); return true;
            case PN_PRS_EMIT_LONG_MAP_IN: emit(p, PN_EVT_MAP_IN, PN_EVT_LONG); return true;
            case PN_PRS_EMIT_LONG_MAP_OUT: emit(p, PN_EVT_MAP_OUT, PN_EVT_LONG); return true;
        }
    }
    return false;
}
