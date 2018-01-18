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

#include "./gen_table.h"
#include "./lex.h"
#include "./vector.h"

static void lexer_fail(pn_lexer_t* lex, pn_error_t* error, char* at, pn_error_code_t code) {
    lex->token.type = PN_TOK_ERROR;
    lex->token.end  = lex->line.end - 2;
    error->code     = code;
    error->lineno   = lex->lineno;
    error->column   = at - lex->line.begin + 1;
}

bool pn_lexer_indent(pn_lexer_t* lex) {
    size_t indent = lex->indent + (lex->token.end - lex->token.begin);
    for (char* p = lex->token.end; p < lex->line.end; ++p) {
        switch (*p) {
            case ' ': ++indent; break;
            case '\t': indent = (indent ^ (indent & 0x1)) + 2; break;
            case '\n': return false;
            default:
                lex->indent    = indent;
                lex->eq        = true;
                lex->token.end = p;
                return true;
        }
    }
    return false;
}

static bool update_lexer_level(pn_lexer_t* lex, pn_error_t* error) {
    if (lex->indent > VECTOR_LAST(lex->levels)) {  // Inward indent
        lex->eq = false;
        if (lex->token.type == PN_TOK_LINE_OUT) {
            lex->indent = VECTOR_LAST(lex->levels);
            lexer_fail(lex, error, lex->token.end, PN_ERROR_OUTDENT);
            return true;
        }
        VECTOR_EXTEND(&lex->levels, 1);
        VECTOR_LAST(lex->levels) = lex->indent;
        lex->token.type          = PN_TOK_LINE_IN;
        return true;
    }

    if (lex->indent < VECTOR_LAST(lex->levels)) {  // Outward indent
        --lex->levels->count;
        lex->token.type = PN_TOK_LINE_OUT;
        return true;
    }

    if (lex->eq) {
        lex->eq         = false;
        lex->token.type = PN_TOK_LINE_EQ;
        return true;
    }

    return false;
}

static bool next_line(pn_lexer_t* lex, pn_error_t* error) {
    while (true) {
        if (lex->line.begin != lex->line.end) {
            ++lex->lineno;
        }
        lex->prev_width  = lex->line.end - lex->line.begin;
        ssize_t size     = getline(&lex->buffer.data, &lex->buffer.size, lex->file);
        lex->token.begin = lex->token.end = lex->line.begin = lex->line.end = lex->buffer.data;
        if (size <= 0) {
            if (ferror(lex->file)) {
                lexer_fail(lex, error, NULL, PN_ERROR_SYSTEM);
                return true;
            }

            lex->indent = 0;
            if (update_lexer_level(lex, error)) {
                return true;
            }

            lex->token.type = PN_TOK_LINE_OUT;
            return true;
        }
        lex->line.end = lex->line.begin + size;

        if (lex->line.end[-1] != '\n') {
            *(lex->line.end++) = '\n';  // Overwrite \0 as we don't care about NUL-termination.
        }
        ++lex->line.end;

        lex->indent = 0;
        if (pn_lexer_indent(lex)) {
            return update_lexer_level(lex, error);
        }
    }
}

void pn_lexer_next(pn_lexer_t* lex, pn_error_t* error) {
    // Either initial, when the line is NULL, or final, when it is empty.
    if (lex->line.begin == lex->line.end) {
        if (next_line(lex, error)) {
            return;  // emits LINE*
        }
    } else if (update_lexer_level(lex, error)) {
        return;
    }

    while (lex->token.end < lex->line.end) {
        if ((*lex->token.end == ' ') || (*lex->token.end == '\t')) {
            ++lex->token.end;
        } else {
            break;
        }
    }
    if (*lex->token.end == '\n') {
        if (!next_line(lex, error)) {
            lexer_fail(lex, error, lex->token.end, PN_ERROR_INTERNAL);
        }
        return;
    }

    lex->token.begin = lex->token.end;
    uint8_t state    = 0;
    while (lex->token.end < lex->line.end) {
        uint8_t ch    = *lex->token.end;
        uint8_t class = lex_classes[ch];
        if ((state = lex_table[state][class]) & PN_TOK_FLAG_DONE) {
            break;
        }
        lex->token.end++;
    }

    if (state & PN_TOK_FLAG_OK) {
        lex->token.type = state & PN_TOK_FLAG_VALUE;
        if (lex->token.type == PN_TOK_STAR) {
            pn_lexer_indent(lex);
            lex->token.end = lex->token.begin + 1;
        }
        return;
    }

    char* at = lex->token.end;
    switch (state & PN_TOK_FLAG_VALUE) {
        case PN_ERROR_PARTIAL: at = lex->token.end - 1; break;
        case PN_ERROR_BADWORD: at = lex->token.begin; break;
        case PN_ERROR_BADESC:
        case PN_ERROR_BADUESC:
            while (*at != '\\') {
                --at;
            }
            break;
    }
    return lexer_fail(lex, error, at, state & PN_TOK_FLAG_VALUE);
}

void pn_lexer_init(pn_lexer_t* lex, pn_file_t* file) {
    pn_lexer_t l = {.file = file, .indent = -1, .lineno = 1};
    VECTOR_INIT(&l.levels, 1);
    VECTOR_LAST(l.levels) = -1;
    *lex                  = l;
}

void pn_lexer_clear(pn_lexer_t* lex) {
    free(lex->levels);
    free(lex->buffer.data);
}
