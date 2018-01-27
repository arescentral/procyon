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

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./common.h"
#include "./utf8.h"
#include "./vector.h"

static bool dump_null(pn_file_t* file);
static bool dump_bool(pn_bool_t b, pn_file_t* file);
static bool dump_int(pn_int_t i, pn_file_t* file);
static bool dump_float(pn_float_t f, pn_file_t* file);
static bool should_dump_short_data_view(size_t size);
static bool should_dump_short_data(const pn_data_t* d);
static bool dump_short_data_view(const uint8_t* data, size_t size, pn_file_t* file);
static bool dump_short_data(const pn_data_t* d, pn_file_t* file);
static bool dump_long_data_view(
        const uint8_t* data, size_t size, pn_string_t** indent, pn_file_t* file);
static bool dump_long_data(const pn_data_t* d, pn_string_t** indent, pn_file_t* file);
static bool should_dump_short_string_view(const char* data, size_t size);
static bool should_dump_short_string(const pn_string_t* s);
static bool dump_short_string_view(const char* data, size_t size, pn_file_t* file);
static bool dump_short_string(const pn_string_t* s, pn_file_t* file);
static bool dump_long_string_view(
        const char* data, size_t size, pn_string_t** indent, pn_file_t* file);
static bool dump_long_string(const pn_string_t* s, pn_string_t** indent, pn_file_t* file);
static bool should_dump_short_array(const pn_array_t* a);
static bool dump_short_array(const pn_array_t* a, pn_file_t* file);
static bool dump_long_array(const pn_array_t* a, pn_string_t** indent, pn_file_t* file);
static bool should_dump_short_map(const pn_map_t* m);
static bool dump_short_map(const pn_map_t* m, pn_file_t* file);
static bool dump_long_map(const pn_map_t* m, pn_string_t** indent, pn_file_t* file);

static void pn_indent(pn_string_t** s, int delta, char ch) {
    if (delta < 0) {
        (*s)->count += delta;
    } else if (delta) {
        size_t end = (*s)->count - 1;
        VECTOR_EXTEND(s, delta);
        char* dst = (*s)->values + end;
        memset(dst, ch, delta);
        VECTOR_LAST(*s) = '\0';
    }
}

#define WRITE_CSTRING(S, FILE) (fwrite(S, 1, strlen(S), FILE) == strlen(S))
#define WRITE_SPAN(DATA, SIZE, FILE) (fwrite(DATA, 1, SIZE, FILE) == SIZE)

static bool should_dump_short_value(const pn_value_t* x) {
    switch (x->type) {
        case PN_DATA: return should_dump_short_data(x->d);
        case PN_STRING: return should_dump_short_string(x->s);
        case PN_ARRAY: return should_dump_short_array(x->a);
        case PN_MAP: return should_dump_short_map(x->m);
        default: return true;
    }
}

static bool dump_short_value(const pn_value_t* x, pn_file_t* file) {
    switch (x->type) {
        case PN_NULL: return dump_null(file);
        case PN_BOOL: return dump_bool(x->b, file);
        case PN_INT: return dump_int(x->i, file);
        case PN_FLOAT: return dump_float(x->f, file);
        case PN_DATA: return dump_short_data(x->d, file);
        case PN_STRING: return dump_short_string(x->s, file);
        case PN_ARRAY: return dump_short_array(x->a, file);
        case PN_MAP: return dump_short_map(x->m, file);
    }
}

static bool dump_long_value(const pn_value_t* x, pn_string_t** indent, pn_file_t* file) {
    switch (x->type) {
        case PN_NULL: return dump_null(file);
        case PN_BOOL: return dump_bool(x->b, file);
        case PN_INT: return dump_int(x->i, file);
        case PN_FLOAT: return dump_float(x->f, file);
        case PN_DATA: return dump_long_data(x->d, indent, file);
        case PN_STRING: return dump_long_string(x->s, indent, file);
        case PN_ARRAY: return dump_long_array(x->a, indent, file);
        case PN_MAP: return dump_long_map(x->m, indent, file);
    }
}

static bool dump_null(pn_file_t* file) { return fwrite("null", 1, 4, file) == 4; }

static bool dump_bool(pn_bool_t b, pn_file_t* file) {
    if (b) {
        return fwrite("true", 1, 4, file) == 4;
    } else {
        return fwrite("false", 1, 5, file) == 5;
    }
}

static bool dump_int(pn_int_t i, pn_file_t* file) { return fprintf(file, "%" PRId64, i) > 0; }

static bool dump_float(pn_float_t f, pn_file_t* file) {
    char repr[32];
    pn_dtoa(repr, f);
    return WRITE_CSTRING(repr, file);
}

static bool start_line(pn_string_t* indent, pn_file_t* file) {
    return (putc('\n', file) != EOF) && WRITE_SPAN(indent->values, indent->count - 1, file);
}

static bool should_dump_short_data_view(size_t size) { return size <= 4; }
static bool should_dump_short_data(const pn_data_t* d) {
    return should_dump_short_data_view(d->count);
}

static bool dump_repeated_data(size_t size, pn_file_t* file) {
    if (putc('$', file) == EOF) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if (!WRITE_CSTRING("00", file)) {
            return false;
        }
    }
    return true;
}

static bool dump_short_data_view(const uint8_t* data, size_t size, pn_file_t* file) {
    if (putc('$', file) == EOF) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if (fprintf(file, "%02x", data[i]) <= 0) {
            return false;
        }
    }
    return true;
}

static bool dump_short_data(const pn_data_t* d, pn_file_t* file) {
    return dump_short_data_view(d->values, d->count, file);
}

static bool dump_long_data_view(
        const uint8_t* data, size_t size, pn_string_t** indent, pn_file_t* file) {
    for (size_t i = 0; i < size; ++i) {
        if (i == 0) {
            fprintf(file, "$\t");
        } else if ((i % 32) == 0) {
            if (!(start_line(*indent, file) && WRITE_CSTRING("$\t", file))) {
                return false;
            }
        } else if ((i % 4) == 0) {
            if (putc(' ', file) == EOF) {
                return false;
            }
        }
        if (fprintf(file, "%02x", data[i]) <= 0) {
            return false;
        }
    }
    return true;
}

static bool dump_long_data(const pn_data_t* d, pn_string_t** indent, pn_file_t* file) {
    return dump_long_data_view(d->values, d->count, indent, file);
}

static bool should_dump_short_string_view(const char* data, size_t size) {
    bool has_nl = false;
    for (size_t i = 0; i < size; i = pn_rune_next(data, size, i)) {
        uint32_t r = pn_rune(data, size, i);
        if (r == '\n') {
            has_nl = true;
        } else if (!pn_isprint(r)) {
            return true;  // non-printable characters require short form
        }
    }
    if (has_nl) {
        return false;
    }
    return size <= 72;
}

static bool should_dump_short_string(const pn_string_t* s) {
    return should_dump_short_string_view(s->values, s->count - 1);
}

static bool dump_short_string_view(const char* data, size_t size, pn_file_t* file) {
    if (putc('"', file) == EOF) {
        return false;
    }
    for (size_t i = 0, next; i < size; i = next) {
        next          = pn_rune_next(data, size, i);
        uint32_t    r = pn_rune(data, size, i);
        const char* literal;
        switch (r) {
            case '\b': literal = "\\b"; break;
            case '\t': literal = "\\t"; break;
            case '\n': literal = "\\n"; break;
            case '\f': literal = "\\f"; break;
            case '\r': literal = "\\r"; break;
            case '"': literal = "\\\""; break;
            case '\\': literal = "\\\\"; break;
            default:
                if (pn_isprint(r)) {
                    if (!WRITE_SPAN(data + i, next - i, file)) {
                        return false;
                    }
                } else if (r < 0x10000) {
                    if (fprintf(file, "\\u%04x", r) <= 0) {
                        return false;
                    }
                } else {
                    if (fprintf(file, "\\U%08x", r) <= 0) {
                        return false;
                    }
                }
                continue;
        }
        if (!WRITE_CSTRING(literal, file)) {
            return false;
        }
    }
    if (putc('"', file) == EOF) {
        return false;
    }
    return true;
}

static bool dump_short_string(const pn_string_t* s, pn_file_t* file) {
    return dump_short_string_view(s->values, s->count - 1, file);
}

static size_t short_string_width(const pn_string_t* s) {
    size_t      width = 2;  // ""
    const char* data  = s->values;
    size_t      size  = s->count - 1;
    for (size_t i = 0, next; i < size; i = next) {
        next       = pn_rune_next(data, size, i);
        uint32_t r = pn_rune(data, size, i);
        switch (r) {
            case '\b':
            case '\t':
            case '\n':
            case '\f':
            case '\r':
            case '"':
            case '\\': width += 2; break;
            default:
                if (pn_isprint(r)) {
                    width += next - i;
                } else if (r < 0x10000) {
                    width += 6;
                } else {
                    width += 10;
                }
                break;
        }
    }
    return width;
}

// If (data, size) should not or cannot be broken, returns false.
// If there is a space before the 72nd column, returns the last one.
// If there is no space before the 72nd column, returns the first one.
static bool split_line(const char* data, size_t size, size_t* part) {
    if (size <= 72) {
        return false;
    }
    char* space = strchr(data, ' ');
    if ((!space) || (space == (data + size - 1))) {
        return false;
    }
    *part = space - data;
    while ((space = strchr(data + *part + 1, ' '))) {
        if ((space - data) <= 72) {
            *part = space - data;
        } else {
            break;
        }
    }
    return true;
}

static bool dump_long_string_view(
        const char* data, size_t size, pn_string_t** indent, pn_file_t* file) {
    const char* const begin      = data;
    bool              can_use_gt = true;
    ++size;
    while (true) {
        size_t line_size = strcspn(data, "\n");
        if (data != begin) {
            if (!start_line(*indent, file)) {
                return false;
            }
        }
        if (can_use_gt || (line_size == 0)) {
            if (putc('>', file) == EOF) {
                return false;
            }
        } else {
            if (putc('|', file) == EOF) {
                return false;
            }
        }

        if (line_size > 0) {
            if (putc('\t', file) == EOF) {
                return false;
            }
            size_t split;
            while (split_line(data, line_size, &split)) {
                if (!WRITE_SPAN(data, split, file)) {
                    return false;
                }
                ++split;  // cover space
                data += split;
                size -= split;
                line_size -= split;
                if (!(start_line(*indent, file) && WRITE_CSTRING(">\t", file))) {
                    return false;
                }
            }
            if (!WRITE_SPAN(data, line_size, file)) {
                return false;
            }
            can_use_gt = false;
        } else {
            can_use_gt = true;
        }

        data += line_size + 1;
        size -= line_size + 1;
        if (size <= 1) {
            if (size == 0) {
                if (!(start_line(*indent, file) && (putc('!', file) != EOF))) {
                    return false;
                }
            }
            return true;
        }
    }
}

static bool dump_long_string(const pn_string_t* s, pn_string_t** indent, pn_file_t* file) {
    return dump_long_string_view(s->values, s->count - 1, indent, file);
}

static bool should_dump_short_array(const pn_array_t* a) {
    for (size_t i = 0; i < a->count; ++i) {
        if (a->values[i].type >= PN_DATA) {
            return false;
        }
    }
    return true;
}

static bool dump_short_array(const pn_array_t* a, pn_file_t* file) {
    if (putc('[', file) == EOF) {
        return false;
    }
    for (const pn_value_t *x = a->values, *end = a->values + a->count; x != end; ++x) {
        if (x != a->values) {
            if (!WRITE_CSTRING(", ", file)) {
                return false;
            }
        }
        if (!dump_short_value(x, file)) {
            return false;
        }
    }
    if (putc(']', file) == EOF) {
        return false;
    }
    return true;
}

static bool dump_long_array(const pn_array_t* a, pn_string_t** indent, pn_file_t* file) {
    for (const pn_value_t *x = a->values, *end = a->values + a->count; x != end; ++x) {
        if (x != a->values) {
            if (!start_line(*indent, file)) {
                return false;
            }
        }
        pn_indent(indent, +1, '\t');
        if (!WRITE_CSTRING("*\t", file)) {
            return false;
        }
        if (should_dump_short_value(x)) {
            if (!dump_short_value(x, file)) {
                return false;
            }
        } else {
            if (!dump_long_value(x, indent, file)) {
                return false;
            }
        }
        pn_indent(indent, -1, 0);
    }
    return true;
}

static bool should_dump_short_map(const pn_map_t* m) {
    for (size_t i = 0; i < m->count; ++i) {
        if (m->values[i].value.type >= PN_DATA) {
            return false;
        }
    }
    return true;
}

static bool needs_quotes(const pn_string_t* key) {
    // clang-format off
    bool ok[256] = {
        ['0'] = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        ['+'] = 1, ['-'] = 1, ['_'] = 1, ['.'] = 1, ['/'] = 1,
        ['A'] = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        ['a'] = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    // clang-format on
    for (size_t i = 0; i < key->count - 1; ++i) {
        if (!ok[(uint8_t)key->values[i]]) {
            return true;
        }
    }
    return false;
}

static bool dump_key(const pn_string_t* key, int padding, pn_file_t* file) {
    if (needs_quotes(key)) {
        if (!dump_short_string(key, file)) {
            return false;
        }
    } else {
        if (!WRITE_SPAN(key->values, key->count - 1, file)) {
            return false;
        }
    }
    return fprintf(file, ":%*s", padding, "") > 0;
}

static size_t key_width(const pn_string_t* key) {
    if (needs_quotes(key)) {
        return short_string_width(key);
    } else {
        return key->count - 1;
    }
}

static bool dump_short_map(const pn_map_t* m, pn_file_t* file) {
    if (putc('{', file) == EOF) {
        return false;
    }
    for (const pn_kv_pair_t *x = m->values, *end = m->values + m->count; x != end; ++x) {
        if (x != m->values) {
            if (!WRITE_CSTRING(", ", file)) {
                return false;
            }
        }
        if (!(dump_key(x->key, 1, file) && dump_short_value(&x->value, file))) {
            return false;
        }
    }
    if (putc('}', file) == EOF) {
        return false;
    }
    return true;
}

static bool dump_long_map(const pn_map_t* m, pn_string_t** indent, pn_file_t* file) {
    size_t padding = 0;
    for (const pn_kv_pair_t *x = m->values, *end = m->values + m->count; x != end; ++x) {
        if (should_dump_short_value(&x->value)) {
            size_t width = key_width(x->key);
            if (key_width(x->key) > padding) {
                padding = width;
            }
        }
    }
    padding += 3;

    for (const pn_kv_pair_t *x = m->values, *end = m->values + m->count; x != end; ++x) {
        if (x != m->values) {
            if (!start_line(*indent, file)) {
                return false;
            }
        }
        size_t width = key_width(x->key);
        if (should_dump_short_value(&x->value)) {
            if (!(dump_key(x->key, padding - 1 - width, file) &&
                  dump_short_value(&x->value, file))) {
                return false;
            }
        } else {
            pn_indent(indent, +1, '\t');
            if (!(dump_key(x->key, 0, file) && start_line(*indent, file) &&
                  dump_long_value(&x->value, indent, file))) {
                return false;
            }
            pn_indent(indent, -1, 0);
        }
    }
    return true;
}

bool pn_dump(pn_file_t* file, int flags, int format, ...) {
    pn_value_t     x;
    char           ch[4];
    const char*    s = NULL;
    const uint8_t* d = NULL;
    bool           z = false;
    size_t         size;
    bool           owned = false;

    va_list vl;
    va_start(vl, format);
    switch (format) {
        default:
        case 'n':
        case 'N': x.type = PN_NULL; break;

        case '?': x.type = PN_BOOL, x.b = va_arg(vl, int); break;

        case 'i': x.type = PN_INT, x.i = va_arg(vl, int); break;
        case 'I': x.type = PN_INT, x.i = va_arg(vl, unsigned int); break;
        case 'b': x.type = PN_INT, x.i = va_arg(vl, int); break;
        case 'B': x.type = PN_INT, x.i = va_arg(vl, int); break;
        case 'h': x.type = PN_INT, x.i = va_arg(vl, int); break;
        case 'H': x.type = PN_INT, x.i = va_arg(vl, int); break;
        case 'l': x.type = PN_INT, x.i = va_arg(vl, int32_t); break;
        case 'L': x.type = PN_INT, x.i = va_arg(vl, uint32_t); break;
        case 'q': x.type = PN_INT, x.i = va_arg(vl, int64_t); break;
        case 'Q': x.type = PN_INT, x.i = va_arg(vl, uint64_t); break;
        case 'p': x.type = PN_INT, x.i = va_arg(vl, intptr_t); break;
        case 'P': x.type = PN_INT, x.i = va_arg(vl, uintptr_t); break;
        case 'z': x.type = PN_INT, x.i = va_arg(vl, size_t); break;
        case 'Z': x.type = PN_INT, x.i = va_arg(vl, ssize_t); break;

        case 'f': x.type = PN_FLOAT, x.f = va_arg(vl, double); break;
        case 'd': x.type = PN_FLOAT, x.f = va_arg(vl, double); break;

        case 'a': x.type = PN_ARRAY, x.a = va_arg(vl, pn_array_t*); break;
        case 'A': x.type = PN_ARRAY, x.a = va_arg(vl, pn_array_t*), owned = true; break;
        case 'm': x.type = PN_MAP, x.m = va_arg(vl, pn_map_t*); break;
        case 'M': x.type = PN_MAP, x.m = va_arg(vl, pn_map_t*), owned = true; break;

        case 'x': x = *va_arg(vl, pn_value_t*); break;
        case 'X': x = *va_arg(vl, pn_value_t*), owned = true; break;

        case 's': s = va_arg(vl, const char*), size = strlen(s); break;
        case 'S': s = va_arg(vl, const char*), size = va_arg(vl, size_t); break;
        case '$': d = va_arg(vl, const uint8_t*), size = va_arg(vl, size_t); break;
        case 'c': pn_chr(va_arg(vl, int), ch, &size), s = ch; break;
        case 'C': pn_unichr(va_arg(vl, uint32_t), ch, &size), s = ch; break;
        case '#': size = va_arg(vl, size_t); break;
    }
    va_end(vl);

    bool result = false;
    if (s) {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_string_view(s, size)) {
            result = dump_short_string_view(s, size, file);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_string_view(s, size, &indent.s, file);
            pn_clear(&indent);
        }
    } else if (d) {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_data_view(size)) {
            result = dump_short_data_view(d, size, file);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_data_view(d, size, &indent.s, file);
            pn_clear(&indent);
        }
    } else if (z) {
        result = dump_repeated_data(size, file);
    } else {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_value(&x)) {
            result = dump_short_value(&x, file);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_value(&x, &indent.s, file);
            pn_clear(&indent);
        }
    }

    if (result && !(flags & PN_DUMP_SHORT)) {
        result = (putc('\n', file) != EOF);
    }
    return result;
}
