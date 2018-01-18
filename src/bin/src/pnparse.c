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
#include "../../c/src/parse.h"

static const char event_names[][8] = {
        "NULL", "BOOL", "INT", "FLOAT", "DATA", "STRING", "[", "]", "{", "}", "ERROR",
};

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

    pn_lexer_t l;
    pn_lexer_init(&l, stdin);
    pn_parser_t p;
    pn_parser_init(&p, &l, 64);

    pn_error_t error;
    int        indent = 0;
    while (pn_parser_next(&p, &error)) {
        if (p.evt.type == PN_EVT_ERROR) {
            pn_format(
                    stderr, "{0}:{1}: {2}\n", "zzs", error.lineno, error.column,
                    pn_strerror(error.code));
            exit(1);
        }
        if ((p.evt.type == PN_EVT_ARRAY_OUT) || (p.evt.type == PN_EVT_MAP_OUT)) {
            --indent;
        }
        for (int i = 0; i < indent; ++i) {
            pn_format(stdout, "\t", "");
        }
        if (p.evt.k.type) {
            pn_format(stdout, "KEY({0}) ", "r", &p.evt.k);
        }
        pn_format(stdout, "{0}", "s", event_names[p.evt.type]);
        if (p.evt.x.type != PN_NULL) {
            pn_format(stdout, "({0})", "r", &p.evt.x);
        }
        pn_format(stdout, "\n", "");
        if ((p.evt.type == PN_EVT_ARRAY_IN) || (p.evt.type == PN_EVT_MAP_IN)) {
            ++indent;
        }
    }
    pn_parser_clear(&p);
    pn_lexer_clear(&l);

    return 0;
}
