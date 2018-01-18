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

#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./utf8.h"

struct strsize {
    const char* data;
    size_t      size;
};

struct datasize {
    const uint8_t* data;
    size_t         size;
};

struct format_arg {
    char type;
    union {
        bool              b;
        int               i;  // bBhHC
        unsigned int      I;
        int32_t           l;
        uint32_t          L;  // C
        int64_t           q;
        uint64_t          Q;
        intptr_t          p;
        uintptr_t         P;
        size_t            z;
        ssize_t           Z;
        double            d;  // f
        const char*       s;
        const pn_array_t* a;
        const pn_map_t*   m;
        const pn_value_t* x;
        struct strsize    S;
        struct datasize   D;  // $
    };
};

const struct format_arg null_arg = {.type = 'n'};

static void set_arg(char format, struct format_arg* dst, va_list vl) {
    switch (format) {
        default: format = 'n'; break;
        case 'n': break;

        case '?': dst->i = va_arg(vl, int); break;

        case 'b':
        case 'B':
        case 'h':
        case 'H': format = 'i';
        case 'i': dst->i = va_arg(vl, int); break;
        case 'I': dst->I = va_arg(vl, unsigned int); break;
        case 'l': dst->l = va_arg(vl, int32_t); break;
        case 'L': dst->L = va_arg(vl, uint32_t); break;
        case 'q': dst->q = va_arg(vl, int64_t); break;
        case 'Q': dst->Q = va_arg(vl, uint64_t); break;
        case 'p': dst->p = va_arg(vl, intptr_t); break;
        case 'P': dst->P = va_arg(vl, uintptr_t); break;
        case 'z': dst->z = va_arg(vl, size_t); break;
        case 'Z': dst->Z = va_arg(vl, ssize_t); break;

        case 'f': format = 'f';
        case 'd': dst->d = va_arg(vl, double); break;

        case 'a': dst->a = va_arg(vl, const pn_array_t*); break;
        case 'm': dst->m = va_arg(vl, const pn_map_t*); break;
        case 'r': dst->x = va_arg(vl, const pn_value_t*); break;
        case 'x': dst->x = va_arg(vl, const pn_value_t*); break;

        case 's': dst->s = va_arg(vl, const char*); break;

        case 'S':
            dst->S.data = va_arg(vl, const char*);
            dst->S.size = va_arg(vl, size_t);
            break;

        case '$':
            dst->D.data = va_arg(vl, const uint8_t*);
            dst->D.size = va_arg(vl, size_t);
            break;

        case 'c': dst->i = va_arg(vl, int); break;
        case 'C': dst->L = va_arg(vl, uint32_t); break;

        case '#': dst->z = va_arg(vl, size_t); break;
    }
    dst->type = format;
}

#define LITERAL(f, s) (fwrite(s, 1, strlen(s), f) == strlen(s))

static bool print_arg(pn_file_t* f, const struct format_arg* arg) {
    switch (arg->type) {
        case 'n': return LITERAL(f, "null");

        case '?': return LITERAL(f, arg->i ? "true" : "false");

        case 'i': return fprintf(f, "%d", arg->i) > 0;
        case 'I': return fprintf(f, "%u", arg->I) > 0;
        case 'l': return fprintf(f, "%" PRId32, arg->l) > 0;
        case 'L': return fprintf(f, "%" PRIu32, arg->L) > 0;
        case 'q': return fprintf(f, "%" PRId64, arg->q) > 0;
        case 'Q': return fprintf(f, "%" PRIu64, arg->Q) > 0;
        case 'p': return fprintf(f, "%zd", arg->p) > 0;
        case 'P': return fprintf(f, "%zd", arg->P) > 0;
        case 'z': return fprintf(f, "%zu", arg->z) > 0;
        case 'Z': return fprintf(f, "%zu", arg->Z) > 0;

        case 'f':
        case 'd': return pn_dump(f, PN_DUMP_SHORT, 'd', arg->d);
        case '$': return pn_dump(f, PN_DUMP_SHORT, '$', arg->D.data, arg->D.size);
        case 'a': return pn_dump(f, PN_DUMP_SHORT, 'a', arg->a);
        case 'm': return pn_dump(f, PN_DUMP_SHORT, 'm', arg->m);

        case 'r': return pn_dump(f, PN_DUMP_SHORT, 'x', arg->x);
        case 'x':
            if (arg->x->type == PN_STRING) {
                size_t len = arg->x->s->count - 1;
                return fwrite(arg->x->s->values, 1, len, f) == len;
            }
            return pn_dump(f, PN_DUMP_SHORT, 'x', arg->x);

        case 's': return LITERAL(f, arg->s);
        case 'S': return fwrite(arg->S.data, 1, arg->S.size, f) == arg->S.size;

        case 'c': {
            char   data[4];
            size_t size;
            pn_chr(arg->i, data, &size);
            return fwrite(data, 1, size, f) == size;
        }

        case 'C': {
            char   data[4];
            size_t size;
            pn_unichr(arg->L, data, &size);
            return fwrite(data, 1, size, f) == size;
        }

        case '#':
            for (size_t i = 0; i < arg->z; ++i) {
                if (putc(0, f) == EOF) {
                    return false;
                }
            }
            return true;
    }

    // Should never happen: set_arg() filters out invalid codes before print_arg().
    return false;
}

static const struct format_arg* get_array_subscript(
        const pn_array_t* a, struct format_arg* arg_storage, const char* subscript,
        size_t subscript_size) {
    int64_t         i;
    pn_error_code_t error;
    if (!pn_strtoll(subscript, subscript_size, &i, &error)) {
        return &null_arg;
    } else if (!((0 <= i) && ((uint64_t)i < a->count))) {
        return &null_arg;
    }
    arg_storage->type = 'x';
    arg_storage->x    = &a->values[i];
    return arg_storage;
}

static const struct format_arg* get_map_subscript(
        const pn_map_t* m, struct format_arg* arg_storage, const char* subscript,
        size_t subscript_size) {
    const pn_value_t* x = pn_mapget_const(m, 'S', subscript, subscript_size);
    if (!x) {
        return &null_arg;
    }
    arg_storage->type = 'x';
    arg_storage->x    = x;
    return arg_storage;
}

static const struct format_arg* get_subscript(
        const struct format_arg* arg, struct format_arg* arg_storage, const char* subscript,
        size_t subscript_size) {
    if (arg->type == 'a') {
        return get_array_subscript(arg->a, arg_storage, subscript, subscript_size);
    } else if (arg->type == 'm') {
        return get_map_subscript(arg->m, arg_storage, subscript, subscript_size);
    } else if (arg->type == 'x') {
        if (arg->x->type == PN_ARRAY) {
            return get_array_subscript(arg->x->a, arg_storage, subscript, subscript_size);
        } else if (arg->x->type == PN_MAP) {
            return get_map_subscript(arg->x->m, arg_storage, subscript, subscript_size);
        }
    }
    return &null_arg;
}

static bool format_segment(
        pn_file_t* f, const char** format, const struct format_arg* args, size_t nargs,
        const struct format_arg** next_arg) {
    ++*format;
    if (**format == '{') {
        ++*format;
        return fputc('{', f) != EOF;
    }

    size_t      span       = strspn(*format, "0123456789");
    const char* format_end = *format + span;
    if (span) {
        int64_t         i;
        pn_error_code_t error;
        if (pn_strtoll(*format, span, &i, &error)) {
            if ((0 <= i) && ((uint64_t)i < nargs)) {
                *next_arg = &args[i];
            } else {
                *next_arg = &null_arg;
            }
        }
    }

    const struct format_arg* arg = *next_arg;
    struct format_arg        arg_storage;
    while (*format_end == '[') {
        ++format_end;
        const char* subscript      = format_end;
        size_t      subscript_size = strcspn(subscript, "[]");
        if (subscript[subscript_size] != ']') {
            goto fail;  // Unclosed array subscript.
        }
        format_end = subscript + subscript_size + 1;
        arg        = get_subscript(arg, &arg_storage, subscript, subscript_size);
    }

    if (*format_end != '}') {
        goto fail;  // Unclosed template parameter.
    }

    if (!print_arg(f, arg)) {
        return false;
    }
    *format = format_end + 1;
    if ((args <= *next_arg) && (*next_arg < (args + nargs - 1))) {
        ++*next_arg;
    } else if (nargs) {
        *next_arg = &args[nargs - 1];
    }
    return true;

fail:
    --*format;
    size_t len = format_end - *format;
    if (fwrite(*format, 1, len, f) < len) {
        return false;
    }
    *format = format_end;
    return true;
}

#define NSTACKARGS 8
bool pn_format(pn_file_t* f, const char* output_format, const char* input_format, ...) {
    struct format_arg  stack_args[NSTACKARGS];
    struct format_arg* heap_args = NULL;
    size_t             nargs     = strlen(input_format);
    struct format_arg* args      = (nargs > NSTACKARGS) ? heap_args : stack_args;

    va_list vl;
    va_start(vl, input_format);
    for (size_t i = 0; i < nargs; ++i) {
        set_arg(input_format[i], &args[i], vl);
    }
    va_end(vl);

    const struct format_arg* next_arg = nargs ? args : &null_arg;
    while (*output_format) {
        size_t span = strcspn(output_format, "{}");
        if (span) {
            if (fwrite(output_format, 1, span, f) < span) {
                return false;
            }
            output_format += span;
        }
        switch (*output_format) {
            case '{':
                if (!format_segment(f, &output_format, args, nargs, &next_arg)) {
                    return false;
                }
                break;

            case '}':
                if (fputc('}', f) == EOF) {
                    return false;
                }
                output_format += (output_format[1] == '}') ? 2 : 1;
                break;
        }
    }
    return true;
}
