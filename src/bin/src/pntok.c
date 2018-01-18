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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../c/src/lex.h"

const char token_types[][8] = {
        [PN_TOK_LINE_IN] = "LINE+",    [PN_TOK_LINE_EQ] = "LINE=",    [PN_TOK_LINE_OUT] = "LINE-",

        [PN_TOK_STAR] = "*",           [PN_TOK_ARRAY_IN] = "[",       [PN_TOK_ARRAY_OUT] = "]",
        [PN_TOK_MAP_IN] = "{",         [PN_TOK_MAP_OUT] = "}",        [PN_TOK_COMMA] = ",",
        [PN_TOK_NULL] = "NULL",        [PN_TOK_TRUE] = "TRUE",        [PN_TOK_FALSE] = "FALSE",
        [PN_TOK_INF] = "INF",          [PN_TOK_NEG_INF] = "-INF",     [PN_TOK_NAN] = "NAN",

        [PN_TOK_KEY] = "KEY",          [PN_TOK_QKEY] = "QKEY",        [PN_TOK_INT] = "INT",
        [PN_TOK_FLOAT] = "FLOAT",      [PN_TOK_DATA] = "DATA",        [PN_TOK_STR] = "STR",
        [PN_TOK_STR_WRAP] = "STR>",    [PN_TOK_STR_WRAP_EMPTY] = ">", [PN_TOK_STR_PIPE] = "STR|",
        [PN_TOK_STR_PIPE_EMPTY] = "|", [PN_TOK_STR_BANG] = "!",       [PN_TOK_COMMENT] = "COMMENT",

        [PN_TOK_ERROR] = "ERROR",
};

#define _16x(X) X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X
#define _64x(X) _16x(X), _16x(X), _16x(X), _16x(X)
#define _256x(X) _64x(X), _64x(X), _64x(X), _64x(X)

// clang-format off
static const uint8_t transitions[][256] = {
    [0] = {
        [0000] = _64x(0), _64x(0),
        [0200] = _64x(9), _64x(9),
        [0300] = 9, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2,
                 6, 3, 3, 3, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    },

    [1] = { [0000] = _256x(9), [0200] = _64x(0) },
    [2] = { [0000] = _256x(9), [0200] = _64x(1) },
    [3] = { [0000] = _256x(9), [0200] = _64x(2) },

    [4] = { [0000] = _256x(9), [0240] = _16x(1), _16x(1) },
    [5] = { [0000] = _256x(9), [0200] = _16x(1), _16x(1) },
    [6] = { [0000] = _256x(9), [0220] = _16x(2), _16x(2), _16x(2) },
    [7] = { [0000] = _256x(9), [0200] = _16x(2) },
};
// clang-format on

static bool is_utf8(const char* begin, const char* end) {
    uint8_t state = 0;
    for (; begin != end; ++begin) {
        uint8_t ch = *begin;
        state      = transitions[state][ch];
        if (state == 9) {
            return false;
        }
    }
    return (state == 0);
}

int main(int argc, const char** argv) {
    const char* progname = *(argc--, argv++);
    const char* basename = strrchr(progname, '/');
    if (basename) {
        progname = basename + 1;
    }

    if (argc != 0) {
        pn_format(stderr, "usage: {0}\n", "s", progname);
        exit(64);
    }

    pn_lexer_t lex;
    pn_lexer_init(&lex, stdin);
    pn_error_t error;
    int        indent_level = 0;
    while (true) {
        pn_lexer_next(&lex, &error);
        pn_format(
                stdout, "{0}:{1}\t{2}", "zzs", lex.lineno, lex.token.begin - lex.line.begin + 1,
                token_types[lex.token.type]);
        if (lex.token.type == PN_TOK_ERROR) {
            pn_format(
                    stdout, "\t{0}:{1}:{2}", "zzs", error.lineno, error.column,
                    pn_strerror(error.code));
        }
        if (lex.token.type >= PN_TOK_STAR) {
            pn_format(stdout, "\t", "");
            if (is_utf8(lex.token.begin, lex.token.end)) {
                pn_dump(stdout, PN_DUMP_SHORT, 'S', lex.token.begin,
                        lex.token.end - lex.token.begin);
            } else {
                pn_dump(stdout, PN_DUMP_SHORT, '$', lex.token.begin,
                        lex.token.end - lex.token.begin);
            }
        }
        pn_format(stdout, "\n", "");

        switch (lex.token.type) {
            case PN_TOK_LINE_IN: ++indent_level; break;
            case PN_TOK_LINE_OUT: --indent_level; break;
            default: break;
        }
        if (indent_level == 0) {
            pn_lexer_clear(&lex);
            return 0;
        }
    }
}
