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
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./common.h"
#include "./utf8.h"
#include "./vector.h"

const pn_value_t pn_null    = {};
const pn_value_t pn_true    = {.type = PN_BOOL, .b = true};
const pn_value_t pn_false   = {.type = PN_BOOL, .b = false};
const pn_value_t pn_inf     = {.type = PN_FLOAT, .f = INFINITY};
const pn_value_t pn_neg_inf = {.type = PN_FLOAT, .f = -INFINITY};
const pn_value_t pn_nan     = {.type = PN_FLOAT, .f = NAN};
const pn_value_t pn_zero    = {.type = PN_INT, .i = 0};
const pn_value_t pn_zerof   = {.type = PN_FLOAT, .f = 0.0};

static const pn_data_t data_empty = {0, sizeof(data_empty)};
static const union {
    struct {
        size_t count;
        size_t size;
        char   values[1];
    } initializer;
    pn_string_t string;
} string_empty                      = {{1, sizeof(string_empty), ""}};
static const pn_array_t array_empty = {0, sizeof(array_empty)};
static const pn_map_t   map_empty   = {0, sizeof(map_empty)};

const pn_value_t pn_dataempty  = {.type = PN_DATA, .d = (pn_data_t*)&data_empty};
const pn_value_t pn_strempty   = {.type = PN_STRING, .s = (pn_string_t*)&string_empty.string};
const pn_value_t pn_arrayempty = {.type = PN_ARRAY, .a = (pn_array_t*)&array_empty};
const pn_value_t pn_mapempty   = {.type = PN_MAP, .m = (pn_map_t*)&map_empty};

static void pn_copy(pn_value_t* dst, const pn_value_t* src) {
    switch (src->type) {
        default: *dst = *src; break;

        case PN_DATA:
            dst->type = PN_DATA;
            dst->d    = pn_datadup(src->d);
            break;

        case PN_STRING:
            dst->type = PN_STRING;
            dst->s    = pn_strdup(src->s);
            break;

        case PN_ARRAY:
            dst->type = PN_ARRAY;
            dst->a    = pn_arraydup(src->a);
            break;

        case PN_MAP:
            dst->type = PN_MAP;
            dst->m    = pn_mapdup(src->m);
            break;
    }
}

static void pn_move(pn_value_t* dst, pn_value_t* src) {
    *dst      = *src;
    src->type = PN_NULL;
}

bool pn_vset(pn_value_t* dst, char format, va_list vl) {
    switch (format) {
        // clang-format off
        default: dst->type = PN_NULL; return false;
        case 'n': dst->type = PN_NULL; return true;
        case 'N': dst->type = PN_NULL; *va_arg(vl, pn_value_t**) = dst; return true;

        case '?': dst->type = PN_BOOL; dst->b = va_arg(vl, int); return true;

        case 'i': dst->type = PN_INT; dst->i = va_arg(vl, int); return true;
        case 'I': dst->type = PN_INT; dst->i = va_arg(vl, unsigned int); return true;
        case 'b': dst->type = PN_INT; dst->i = va_arg(vl, int); return true;
        case 'B': dst->type = PN_INT; dst->i = va_arg(vl, int); return true;
        case 'h': dst->type = PN_INT; dst->i = va_arg(vl, int); return true;
        case 'H': dst->type = PN_INT; dst->i = va_arg(vl, int); return true;
        case 'l': dst->type = PN_INT; dst->i = va_arg(vl, int32_t); return true;
        case 'L': dst->type = PN_INT; dst->i = va_arg(vl, uint32_t); return true;
        case 'q': dst->type = PN_INT; dst->i = va_arg(vl, int64_t); return true;
        case 'Q': dst->type = PN_INT; dst->i = va_arg(vl, uint64_t); return true;
        case 'p': dst->type = PN_INT; dst->i = va_arg(vl, intptr_t); return true;
        case 'P': dst->type = PN_INT; dst->i = va_arg(vl, uintptr_t); return true;
        case 'z': dst->type = PN_INT; dst->i = va_arg(vl, size_t); return true;
        case 'Z': dst->type = PN_INT; dst->i = va_arg(vl, ssize_t); return true;

        case 'f': dst->type = PN_FLOAT; dst->f = va_arg(vl, double); return true;
        case 'd': dst->type = PN_FLOAT; dst->f = va_arg(vl, double); return true;

        case 'a': dst->type = PN_ARRAY, dst->a = pn_arraydup(va_arg(vl, const pn_array_t*)); return true;
        case 'A': dst->type = PN_ARRAY, dst->a = va_arg(vl, pn_array_t*); return true;
        case 'm': dst->type = PN_MAP, dst->m = pn_mapdup(va_arg(vl, const pn_map_t*)); return true;
        case 'M': dst->type = PN_MAP, dst->m = va_arg(vl, pn_map_t*); return true;

        case 'x': pn_copy(dst, va_arg(vl, const pn_value_t*)); return true;
        case 'X': pn_move(dst, va_arg(vl, pn_value_t*)); return true;
        // clang-format on

        case 's': {
            const char* arg = va_arg(vl, const char*);
            dst->type       = PN_STRING;
            dst->s          = pn_string_new(arg, strlen(arg));
            return true;
        }

        case 'S': {
            const char*  data = va_arg(vl, const char*);
            const size_t size = va_arg(vl, size_t);
            dst->type         = PN_STRING;
            dst->s            = pn_string_new(data, size);
            return true;
        }

        case '$': {
            const uint8_t* data = va_arg(vl, const uint8_t*);
            const size_t   size = va_arg(vl, size_t);
            dst->type           = PN_DATA;
            dst->d              = pn_data_new(data, size);
            return true;
        }

        case 'c': {
            char   data[4];
            size_t size;
            pn_chr(va_arg(vl, int), data, &size);
            dst->type = PN_STRING;
            dst->s    = pn_string_new(data, size);
            return true;
        }

        case 'C': {
            char   data[4];
            size_t size;
            pn_unichr(va_arg(vl, uint32_t), data, &size);
            dst->type = PN_STRING;
            dst->s    = pn_string_new(data, size);
            return true;
        }

        case '#': {
            int size  = va_arg(vl, size_t);
            dst->type = PN_DATA;
            VECTOR_INIT(&dst->d, size);
            memset(dst->d->values, 0, size);
            return true;
        }
    }
}

void pn_set(pn_value_t* dst, int format, ...) {
    va_list vl;
    va_start(vl, format);
    pn_vset(dst, format, vl);
    va_end(vl);
}

void pn_setv(pn_value_t* dst, const char* format, ...) {
    dst->type = PN_ARRAY;
    VECTOR_INIT(&dst->a, strlen(format));

    va_list vl;
    va_start(vl, format);
    pn_value_t* x = dst->a->values;
    while (*format) {
        if (!pn_vset(x, *(format++), vl)) {
            --dst->a->count;
            continue;
        }
        ++x;
    }
    va_end(vl);
}

void pn_setkv(pn_value_t* dst, const char* format, ...) {
    dst->type = PN_MAP;
    VECTOR_INIT(&dst->m, strlen(format) / 2);

    va_list vl;
    va_start(vl, format);
    pn_kv_pair_t* kv = dst->m->values;
    while (format[0] && format[1]) {
        char       key_format   = *(format++);
        char       value_format = *(format++);
        pn_value_t key;
        if (!pn_vset(&key, key_format, vl)) {
            --dst->m->count;
            continue;
        } else if (key.type != PN_STRING) {
            pn_clear(&key);
            --dst->m->count;
            continue;
        }
        kv->key = key.s;
        if (!pn_vset(&kv->value, value_format, vl)) {
            pn_clear(&key);
            --dst->m->count;
            continue;
        }
        ++kv;
    }
    va_end(vl);
}

void pn_swap(pn_value_t* x, pn_value_t* y) {
    pn_value_t z = *x;
    *x           = *y;
    *y           = z;
}

void pn_clear(pn_value_t* x) {
    switch (x->type) {
        case PN_DATA: free(x->d); break;
        case PN_STRING: free(x->s); break;
        case PN_ARRAY: pn_arrayfree(x->a); break;
        case PN_MAP: pn_mapfree(x->m); break;
        default: break;
    }
    x->type = PN_NULL;
}

int pn_cmp(const pn_value_t* x, const pn_value_t* y) {
    if (x->type != y->type) {
        return (x->type < y->type) ? -1 : 1;
    }
    switch (x->type) {
        case PN_NULL: return 0;
        case PN_BOOL: return PN_CMP(x->b, y->b);
        case PN_INT: return PN_CMP(x->i, y->i);
        case PN_FLOAT: return PN_CMP(x->f, y->f);

        case PN_DATA: return pn_datacmp(x->d, y->d);
        case PN_STRING: return pn_strcmp(x->s, y->s);
        case PN_ARRAY: return pn_arraycmp(x->a, y->a);
        case PN_MAP: return pn_mapcmp(x->m, y->m);
    }
}

pn_data_t* pn_datadup(const pn_data_t* d) {
    pn_data_t* new = malloc(d->size);
    memcpy(new, d, d->size);
    return new;
}

int pn_datacmp(const pn_data_t* d1, const pn_data_t* d2) {
    return pn_memncmp(d1->values, d1->count, d2->values, d2->count);
}

void pn_datacat(pn_data_t** d, const uint8_t* data, size_t size) {
    if (!size) {
        return;
    }
    // TODO(sfiera): what to do when `data` is in `d->values`?
    // Reallocating `d` could cause the underlying data to move.
    size_t end = (*d)->count;
    VECTOR_EXTEND(d, size);
    uint8_t* dst = (*d)->values + end;
    memcpy(dst, data, size);
}

void pn_dataresize(pn_data_t** d, size_t size) {
    if (size > (*d)->count) {
        VECTOR_EXTEND(d, size - (*d)->count);
    } else {
        (*d)->count = size;
    }
}

pn_string_t* pn_strdup(const pn_string_t* s) {
    pn_string_t* new = malloc(s->size);
    memcpy(new, s, s->size);
    return new;
}

int pn_strcmp(const pn_string_t* s1, const pn_string_t* s2) {
    return pn_memncmp(s1->values, s1->count - 1, s2->values, s2->count - 1);
}

void pn_strcat(pn_string_t** s, const char* src) {
    // TODO(sfiera): what to do when `src` is in `s->values`?
    // Reallocating `s` could cause the underlying data to move.
    size_t size = strlen(src);
    size_t end  = (*s)->count - 1;
    VECTOR_EXTEND(s, size);
    char* dst = (*s)->values + end;
    memcpy(dst, src, size + 1);
}

void pn_strncat(pn_string_t** s, const char* src, size_t len) {
    if (!len) {
        return;
    }
    // TODO(sfiera): what to do when `src` is in `s->values`?
    // Reallocating `s` could cause the underlying data to move.
    size_t end = (*s)->count - 1;
    VECTOR_EXTEND(s, len);
    char* dst = (*s)->values + end;
    memcpy(dst, src, len);
    VECTOR_LAST(*s) = '\0';
}

void pn_strresize(pn_string_t** s, size_t size) {
    if ((size + 1) > (*s)->count) {
        VECTOR_EXTEND(s, size + 1 - (*s)->count);
    } else {
        (*s)->count = size + 1;
    }
}

void pn_strreplace(
        pn_string_t** s, size_t at, size_t remove_size, const char* replace_data,
        size_t replace_size) {
    if (replace_size != remove_size) {
        size_t original_size = (*s)->count;
        if (replace_size > remove_size) {
            VECTOR_EXTEND(s, replace_size - remove_size);
        } else {
            (*s)->count -= remove_size - replace_size;
        }
        char* start       = (*s)->values + at;
        char* replace_end = start + replace_size;
        char* remove_end  = start + remove_size;
        char* string_end  = (*s)->values + original_size;
        memmove(replace_end, remove_end, string_end - remove_end);
    }
    if (replace_size) {
        memcpy((*s)->values + at, replace_data, replace_size);
    }
}

pn_array_t* pn_arraydup(const pn_array_t* a) {
    pn_array_t* new = malloc(a->size);
    new->count      = a->count;
    new->size       = a->size;
    for (size_t i = 0; i < a->count; ++i) {
        pn_copy(&new->values[i], &a->values[i]);
    }
    return new;
}

void pn_arrayfree(pn_array_t* a) {
    if (!a) {
        return;
    }
    for (size_t i = 0; i < a->count; ++i) {
        pn_clear(&a->values[i]);
    }
    free(a);
}

int pn_arraycmp(const pn_array_t* l1, const pn_array_t* l2) {
    size_t count = (l1->count < l2->count) ? l1->count : l2->count;
    for (size_t i = 0; i < count; ++i) {
        int c = pn_cmp(&l1->values[i], &l2->values[i]);
        if (c) {
            return c;
        }
    }
    return PN_CMP(l1->count, l2->count);
}

void pn_arrayext(pn_array_t** a, const char* format, ...) {
    size_t start = (*a)->count;
    VECTOR_EXTEND(a, strlen(format));
    size_t count = (*a)->count - start;

    va_list vl;
    va_start(vl, format);
    for (size_t i = 0; i < count; ++i) {
        pn_vset(&(*a)->values[start + i], format[i], vl);
    }
    va_end(vl);
}

void pn_arrayins(pn_array_t** a, size_t index, int format, ...) {
    VECTOR_EXTEND(a, 1);
    pn_value_t* src = &(*a)->values[index];
    void*       dst = src + 1;
    void*       end = &(*a)->values[(*a)->count];
    memmove(dst, src, end - dst);

    va_list vl;
    va_start(vl, format);
    pn_vset(&(*a)->values[index], format, vl);
    va_end(vl);
}

void pn_arraydel(pn_array_t** a, size_t index) {
    pn_value_t* dst = &(*a)->values[index];
    void*       src = dst + 1;
    void*       end = &(*a)->values[(*a)->count--];
    pn_clear(dst);
    memmove(dst, src, end - src);
}

void pn_arrayresize(pn_array_t** a, size_t size) {
    size_t old_size = (*a)->count;
    if (size > old_size) {
        VECTOR_EXTEND(a, size - old_size);
        for (size_t i = old_size; i < size; ++i) {
            pn_set(&(*a)->values[i], 'n');
        }
    } else if (size < old_size) {
        (*a)->count = size;
        for (size_t i = size; i < old_size; ++i) {
            pn_clear(&(*a)->values[i]);
        }
    }
}

pn_map_t* pn_mapdup(const pn_map_t* m) {
    pn_map_t* new = malloc(m->size);
    new->count    = m->count;
    new->size     = m->size;
    for (size_t i = 0; i < m->count; ++i) {
        new->values[i].key = pn_strdup(m->values[i].key);
        pn_copy(&new->values[i].value, &m->values[i].value);
    }
    return new;
}

void pn_mapfree(pn_map_t* m) {
    if (!m) {
        return;
    }
    for (size_t i = 0; i < m->count; ++i) {
        free(m->values[i].key);
        pn_clear(&m->values[i].value);
    }
    free(m);
}

int pn_mapcmp(const pn_map_t* m1, const pn_map_t* m2) {
    size_t count = (m1->count < m2->count) ? m1->count : m2->count;
    for (size_t i = 0; i < count; ++i) {
        int c = pn_strcmp(m1->values[i].key, m2->values[i].key);
        if (c) {
            return c;
        }
        c = pn_cmp(&m1->values[i].value, &m2->values[i].value);
        if (c) {
            return c;
        }
    }
    return PN_CMP(m1->count, m2->count);
}

static bool map_find(pn_map_t* m, const char* key_data, size_t key_size, size_t* index) {
    for (size_t i = 0; i < m->count; ++i) {
        pn_kv_pair_t* item = &m->values[i];
        if (pn_memncmp(item->key->values, item->key->count - 1, key_data, key_size) == 0) {
            *index = i;
            return true;
        }
    }
    return false;
}

// Returns true if the query is valid and an item was found.
// Sets *index to index of an item if it was found. Must be non-NULL.
// Sets *key if no item was found, but the query is valid. Caller assumes ownership. Can be NULL.
// Invalid queries are when key_format is not one of [csSxX], or when [xX] uses a non-string.
static bool map_vfind(
        pn_map_t** m, size_t* index, pn_string_t** key, char key_format, va_list vl) {
    switch (key_format) {
        case 'c': {
            char   arg_data[4];
            size_t arg_size;
            pn_chr(va_arg(vl, int), arg_data, &arg_size);
            if (map_find(*m, arg_data, arg_size, index)) {
                return true;
            } else if (key) {
                *key = pn_string_new(arg_data, arg_size);
            }
        } break;

        case 'C': {
            char   arg_data[4];
            size_t arg_size;
            pn_unichr(va_arg(vl, uint32_t), arg_data, &arg_size);
            if (map_find(*m, arg_data, arg_size, index)) {
                return true;
            } else if (key) {
                *key = pn_string_new(arg_data, arg_size);
            }
        } break;

        case 's': {
            const char* arg_data = va_arg(vl, const char*);
            size_t      arg_size = strlen(arg_data);
            if (map_find(*m, arg_data, arg_size, index)) {
                return true;
            } else if (key) {
                *key = pn_string_new(arg_data, arg_size);
            }
        } break;

        case 'S': {
            const char* arg_data = va_arg(vl, const char*);
            size_t      arg_size = va_arg(vl, size_t);
            if (map_find(*m, arg_data, arg_size, index)) {
                return true;
            } else if (key) {
                *key = pn_string_new(arg_data, arg_size);
            }
        } break;

        case 'x': {
            const pn_value_t* arg = va_arg(vl, const pn_value_t*);
            if (arg->type != PN_STRING) {
                return false;
            } else if (map_find(*m, arg->s->values, arg->s->count - 1, index)) {
                return true;
            } else if (key) {
                *key = pn_strdup(arg->s);
            }
        } break;

        case 'X': {
            pn_value_t* arg = va_arg(vl, pn_value_t*);
            if (arg->type != PN_STRING) {
                pn_clear(arg);
                return false;
            } else if (map_find(*m, arg->s->values, arg->s->count - 1, index)) {
                pn_clear(arg);
                return true;
            } else if (key) {
                *key = arg->s;
            } else {
                pn_clear(arg);
            }
        } break;
    }
    return false;
}

pn_value_t* pn_mapget(pn_map_t* m, int key_format, ...) {
    va_list vl;
    va_start(vl, key_format);
    size_t      index;
    pn_value_t* value = NULL;
    if (map_vfind(&m, &index, NULL, key_format, vl)) {
        value = &m->values[index].value;
    }
    va_end(vl);
    return value;
}

const pn_value_t* pn_mapget_const(const pn_map_t* m, int key_format, ...) {
    va_list vl;
    va_start(vl, key_format);
    size_t            index;
    const pn_value_t* value = NULL;
    if (map_vfind((pn_map_t**)&m, &index, NULL, key_format, vl)) {
        value = &m->values[index].value;
    }
    va_end(vl);
    return value;
}

bool pn_mapset(pn_map_t** m, int key_format, int value_format, ...) {
    bool    is_new = false;
    va_list vl;
    va_start(vl, value_format);
    size_t       index;
    pn_string_t* key = NULL;
    if (map_vfind(m, &index, &key, key_format, vl)) {
        is_new = false;
        pn_clear(&(*m)->values[index].value);
        pn_vset(&(*m)->values[index].value, value_format, vl);
    } else if (key) {
        is_new = true;
        VECTOR_EXTEND(m, 1);
        VECTOR_LAST(*m).key = key;
        pn_vset(&VECTOR_LAST(*m).value, value_format, vl);
    }
    va_end(vl);
    return is_new;
}

bool pn_mapdel(pn_map_t** m, int key_format, ...) {
    va_list vl;
    va_start(vl, key_format);
    size_t      index;
    pn_value_t* value = NULL;
    if (map_vfind(m, &index, NULL, key_format, vl)) {
        --(*m)->count;
        free((*m)->values[index].key);
        value = &(*m)->values[index].value;
        pn_clear(value);
        for (; index < (*m)->count; ++index) {
            (*m)->values[index] = (*m)->values[index + 1];
        }
    }
    va_end(vl);
    return value;
}

bool pn_mappop(pn_map_t** m, pn_value_t* x, int key_format, ...) {
    va_list vl;
    va_start(vl, key_format);
    size_t      index;
    pn_value_t* value = NULL;
    if (map_vfind(m, &index, NULL, key_format, vl)) {
        --(*m)->count;
        free((*m)->values[index].key);
        value = &(*m)->values[index].value;
        *x    = *value;
        for (; index < (*m)->count; ++index) {
            (*m)->values[index] = (*m)->values[index + 1];
        }
    }
    va_end(vl);
    return value;
}
