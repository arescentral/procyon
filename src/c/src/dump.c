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

#include <pn/procyon.h>

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./common.h"
#include "./io.h"
#include "./utf8.h"
#include "./vector.h"

static bool dump_null(pn_output_t* out);
static bool dump_bool(pn_bool_t b, pn_output_t* out);
static bool dump_int(pn_int_t i, pn_output_t* out);
static bool dump_float(pn_float_t f, pn_output_t* out);
static bool should_dump_short_data_view(size_t size);
static bool should_dump_short_data(const pn_data_t* d);
static bool dump_short_data_view(const uint8_t* data, size_t size, pn_output_t* out);
static bool dump_short_data(const pn_data_t* d, pn_output_t* out);
static bool dump_long_data_view(
        const uint8_t* data, size_t size, pn_string_t** indent, pn_output_t* out);
static bool dump_long_data(const pn_data_t* d, pn_string_t** indent, pn_output_t* out);
static bool should_dump_short_string_view(const char* data, size_t size);
static bool should_dump_short_string(const pn_string_t* s);
static bool dump_short_string_view(const char* data, size_t size, pn_output_t* out);
static bool dump_short_string(const pn_string_t* s, pn_output_t* out);
static bool dump_long_string_view(
        const char* data, size_t size, pn_string_t** indent, pn_output_t* out);
static bool dump_long_string(const pn_string_t* s, pn_string_t** indent, pn_output_t* out);
static bool should_dump_short_array(const pn_array_t* a);
static bool dump_short_array(const pn_array_t* a, pn_output_t* out);
static bool dump_long_array(const pn_array_t* a, pn_string_t** indent, pn_output_t* out);
static bool should_dump_short_map(const pn_map_t* m);
static bool dump_short_map(const pn_map_t* m, pn_output_t* out);
static bool dump_long_map(const pn_map_t* m, pn_string_t** indent, pn_output_t* out);

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

static bool should_dump_short_value(const pn_value_t* x) {
    switch (x->type) {
        case PN_DATA: return should_dump_short_data(x->d);
        case PN_STRING: return should_dump_short_string(x->s);
        case PN_ARRAY: return should_dump_short_array(x->a);
        case PN_MAP: return should_dump_short_map(x->m);
        default: return true;
    }
}

static bool dump_short_value(const pn_value_t* x, pn_output_t* out) {
    switch (x->type) {
        case PN_NULL: return dump_null(out);
        case PN_BOOL: return dump_bool(x->b, out);
        case PN_INT: return dump_int(x->i, out);
        case PN_FLOAT: return dump_float(x->f, out);
        case PN_DATA: return dump_short_data(x->d, out);
        case PN_STRING: return dump_short_string(x->s, out);
        case PN_ARRAY: return dump_short_array(x->a, out);
        case PN_MAP: return dump_short_map(x->m, out);
    }
}

static bool dump_long_value(const pn_value_t* x, pn_string_t** indent, pn_output_t* out) {
    switch (x->type) {
        case PN_NULL: return dump_null(out);
        case PN_BOOL: return dump_bool(x->b, out);
        case PN_INT: return dump_int(x->i, out);
        case PN_FLOAT: return dump_float(x->f, out);
        case PN_DATA: return dump_long_data(x->d, indent, out);
        case PN_STRING: return dump_long_string(x->s, indent, out);
        case PN_ARRAY: return dump_long_array(x->a, indent, out);
        case PN_MAP: return dump_long_map(x->m, indent, out);
    }
}

static bool dump_null(pn_output_t* out) { return pn_raw_write(out, "null", 4); }

static bool dump_bool(pn_bool_t b, pn_output_t* out) {
    return pn_raw_write(out, b ? "true" : "false", b ? 4 : 5);
}

static bool dump_int(pn_int_t i, pn_output_t* out) {
    char    buf[32];
    ssize_t len;
    return ((len = sprintf(buf, "%" PRId64, i)) > 0) && pn_raw_write(out, buf, len);
}

static bool dump_float(pn_float_t f, pn_output_t* out) {
    char repr[32];
    pn_dtoa(repr, f);
    return pn_raw_write(out, repr, strlen(repr));
}

static bool start_line(pn_string_t* indent, pn_output_t* out) {
    return (pn_putc('\n', out) != EOF) && pn_raw_write(out, indent->values, indent->count - 1);
}

static bool should_dump_short_data_view(size_t size) { return size <= 4; }
static bool should_dump_short_data(const pn_data_t* d) {
    return should_dump_short_data_view(d->count);
}

static bool dump_repeated_data(size_t size, pn_output_t* out) {
    if (pn_putc('$', out) == EOF) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if (!pn_raw_write(out, "00", 2)) {
            return false;
        }
    }
    return true;
}

bool dump_hex(pn_output_t* out, uint64_t x, size_t len) {
    static const char hex_digits[] = "0123456789abcdef";
    char              buf[16];
    char*             ptr = buf + 16;
    for (size_t i = 0; i < len; ++i) {
        *(--ptr) = hex_digits[0x0f & x];
        x >>= 4;
    }
    return pn_raw_write(out, ptr, len);
}

static bool dump_short_data_view(const uint8_t* data, size_t size, pn_output_t* out) {
    if (pn_putc('$', out) == EOF) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if (!dump_hex(out, data[i], 2)) {
            return false;
        }
    }
    return true;
}

static bool dump_short_data(const pn_data_t* d, pn_output_t* out) {
    return dump_short_data_view(d->values, d->count, out);
}

static bool dump_long_data_view(
        const uint8_t* data, size_t size, pn_string_t** indent, pn_output_t* out) {
    for (size_t i = 0; i < size; ++i) {
        if (i == 0) {
            pn_write(out, "S", "$\t", (size_t)2);
        } else if ((i % 16) == 0) {
            if (!(start_line(*indent, out) && pn_raw_write(out, "$\t", 2))) {
                return false;
            }
        } else if ((i % 2) == 0) {
            if (pn_putc(' ', out) == EOF) {
                return false;
            }
        }
        if (!dump_hex(out, data[i], 2)) {
            return false;
        }
    }
    return true;
}

static bool dump_long_data(const pn_data_t* d, pn_string_t** indent, pn_output_t* out) {
    return dump_long_data_view(d->values, d->count, indent, out);
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

static bool dump_short_string_view(const char* data, size_t size, pn_output_t* out) {
    if (pn_putc('"', out) == EOF) {
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
                    if (!pn_raw_write(out, data + i, next - i)) {
                        return false;
                    }
                } else if (r < 0x10000) {
                    if (!(pn_write(out, "S", "\\u", (size_t)2) && dump_hex(out, r, 4))) {
                        return false;
                    }
                } else {
                    if (!(pn_write(out, "S", "\\U", (size_t)2) && dump_hex(out, r, 8))) {
                        return false;
                    }
                }
                continue;
        }
        if (!pn_raw_write(out, literal, strlen(literal))) {
            return false;
        }
    }
    if (pn_putc('"', out) == EOF) {
        return false;
    }
    return true;
}

static bool dump_short_string(const pn_string_t* s, pn_output_t* out) {
    return dump_short_string_view(s->values, s->count - 1, out);
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
                    width += pn_rune_width(r);
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
    const char* space = NULL;
    size_t      width = 0;
    for (size_t i = 0; i < size; i = pn_rune_next(data, size, i)) {
        uint32_t rune = pn_rune(data, size, i);
        width += pn_rune_width(rune);
        if ((width > 72) && (i == size - 1)) {
            if (space) {
                *part = space - data;
                return true;
            }
            return false;
        }
        if (data[i] == ' ') {
            space = &data[i];
        }
        if (space && (width > 72)) {
            size_t part_size = space - data;
            if (part_size < (size - 1)) {
                *part = part_size;
                return true;
            }
            return false;
        }
    }
    return false;
}

static bool dump_long_string_view(
        const char* data, size_t size, pn_string_t** indent, pn_output_t* out) {
    const char* const begin      = data;
    bool              can_use_gt = true;
    ++size;
    while (true) {
        size_t line_size = strcspn(data, "\n");
        if (data != begin) {
            if (!start_line(*indent, out)) {
                return false;
            }
        }
        if (can_use_gt || (line_size == 0)) {
            if (pn_putc('>', out) == EOF) {
                return false;
            }
        } else {
            if (pn_putc('|', out) == EOF) {
                return false;
            }
        }

        if (line_size > 0) {
            if (pn_putc('\t', out) == EOF) {
                return false;
            }
            size_t split;
            while (split_line(data, line_size, &split)) {
                if (!pn_raw_write(out, data, split)) {
                    return false;
                }
                ++split;  // cover space
                data += split;
                size -= split;
                line_size -= split;
                if (!(start_line(*indent, out) && pn_raw_write(out, ">\t", 2))) {
                    return false;
                }
            }
            if (!pn_raw_write(out, data, line_size)) {
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
                if (!(start_line(*indent, out) && (pn_putc('!', out) != EOF))) {
                    return false;
                }
            }
            return true;
        }
    }
}

static bool dump_long_string(const pn_string_t* s, pn_string_t** indent, pn_output_t* out) {
    return dump_long_string_view(s->values, s->count - 1, indent, out);
}

static bool should_dump_short_array(const pn_array_t* a) {
    for (size_t i = 0; i < a->count; ++i) {
        if (a->values[i].type >= PN_DATA) {
            return false;
        }
    }
    return true;
}

static bool dump_short_array(const pn_array_t* a, pn_output_t* out) {
    if (pn_putc('[', out) == EOF) {
        return false;
    }
    for (const pn_value_t *x = a->values, *end = a->values + a->count; x != end; ++x) {
        if (x != a->values) {
            if (!pn_raw_write(out, ", ", 2)) {
                return false;
            }
        }
        if (!dump_short_value(x, out)) {
            return false;
        }
    }
    if (pn_putc(']', out) == EOF) {
        return false;
    }
    return true;
}

static bool dump_long_array(const pn_array_t* a, pn_string_t** indent, pn_output_t* out) {
    for (const pn_value_t *x = a->values, *end = a->values + a->count; x != end; ++x) {
        if (x != a->values) {
            if (!start_line(*indent, out)) {
                return false;
            }
        }
        pn_indent(indent, +1, '\t');
        if (!pn_raw_write(out, "*\t", 2)) {
            return false;
        }
        if (should_dump_short_value(x)) {
            if (!dump_short_value(x, out)) {
                return false;
            }
        } else {
            if (!dump_long_value(x, indent, out)) {
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
    static const bool ok[256] = {
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

static bool write_padding(pn_output_t* out, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (!pn_raw_write(out, " ", 1)) {
            return false;
        }
    }
    return true;
}

static bool dump_key(const pn_string_t* key, int padding, pn_output_t* out) {
    if (needs_quotes(key)) {
        return dump_short_string(key, out) && pn_raw_write(out, ":", 1) &&
               write_padding(out, padding);
    } else {
        return pn_raw_write(out, key->values, key->count - 1) && pn_raw_write(out, ":", 1) &&
               write_padding(out, padding);
    }
}

static size_t key_width(const pn_string_t* key) {
    if (needs_quotes(key)) {
        return short_string_width(key);
    } else {
        return key->count - 1;
    }
}

static bool dump_short_map(const pn_map_t* m, pn_output_t* out) {
    if (pn_putc('{', out) == EOF) {
        return false;
    }
    for (const pn_kv_pair_t *x = m->values, *end = m->values + m->count; x != end; ++x) {
        if (x != m->values) {
            if (!pn_raw_write(out, ", ", 2)) {
                return false;
            }
        }
        if (!(dump_key(x->key, 1, out) && dump_short_value(&x->value, out))) {
            return false;
        }
    }
    if (pn_putc('}', out) == EOF) {
        return false;
    }
    return true;
}

static bool dump_long_map(const pn_map_t* m, pn_string_t** indent, pn_output_t* out) {
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
            if (!start_line(*indent, out)) {
                return false;
            }
        }
        size_t width = key_width(x->key);
        if (should_dump_short_value(&x->value)) {
            if (!(dump_key(x->key, padding - 1 - width, out) &&
                  dump_short_value(&x->value, out))) {
                return false;
            }
        } else {
            pn_indent(indent, +1, '\t');
            if (!(dump_key(x->key, 0, out) && start_line(*indent, out) &&
                  dump_long_value(&x->value, indent, out))) {
                return false;
            }
            pn_indent(indent, -1, 0);
        }
    }
    return true;
}

bool pn_dump(pn_output_t* out, int flags, int format, ...) {
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
        case 'c': pn_ascchr(va_arg(vl, int), ch, &size), s = ch; break;
        case 'C': pn_unichr(va_arg(vl, uint32_t), ch, &size), s = ch; break;
        case '#': size = va_arg(vl, size_t); break;
    }
    va_end(vl);

    bool result = false;
    if (s) {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_string_view(s, size)) {
            result = dump_short_string_view(s, size, out);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_string_view(s, size, &indent.s, out);
            pn_clear(&indent);
        }
    } else if (d) {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_data_view(size)) {
            result = dump_short_data_view(d, size, out);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_data_view(d, size, &indent.s, out);
            pn_clear(&indent);
        }
    } else if (z) {
        result = dump_repeated_data(size, out);
    } else {
        if ((flags & PN_DUMP_SHORT) || should_dump_short_value(&x)) {
            result = dump_short_value(&x, out);
        } else {
            pn_value_t indent;
            pn_set(&indent, 's', "");
            result = dump_long_value(&x, &indent.s, out);
            pn_clear(&indent);
        }
    }

    if (result && !(flags & PN_DUMP_SHORT)) {
        result = (pn_putc('\n', out) != EOF);
    }
    return result;
}
