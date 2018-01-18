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

#define _GNU_SOURCE
#include <procyon.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "./file.h"

char pn_file_mode(const char* mode) {
    char ch_mode;
    switch (*mode) {
        case 'r':
        case 'w':
        case 'a': ch_mode = *(mode++); break;
        default: return 0;
    }
    switch (*mode) {
        case '+': ch_mode = toupper(ch_mode); ++mode;
        case '\0': break;
        default: return 0;
    }
    if (*mode) {
        return 0;
    }
    return ch_mode;
}

int pn_close(void* cookie) {
    free(cookie);
    return 0;
}

ssize_t pn_string_read(void* cookie, char* data, size_t size) {
    struct pn_string_cookie* c         = cookie;
    size_t                   len       = (*c->s)->count - 1;
    size_t                   remainder = len - c->at;
    if (remainder < size) {
        size = remainder;
    }
    memcpy(data, (*c->s)->values + c->at, size);
    c->at += size;
    return size;
}

ssize_t pn_string_write(void* cookie, const char* data, size_t size) {
    struct pn_string_cookie* c         = cookie;
    size_t                   save_size = size;
    while (c->at > (int64_t)(*c->s)->count - 1) {
        pn_strncat(c->s, "\0", 1);
    }
    int64_t len = (*c->s)->count - 1;
    if (c->at < len) {
        size_t overwrite_size = len - c->at;
        if (overwrite_size > size) {
            overwrite_size = size;
        }
        memcpy(&(*c->s)->values[c->at], data, overwrite_size);
        data += overwrite_size;
        size -= overwrite_size;
        c->at += overwrite_size;
    }
    pn_strncat(c->s, data, size);
    c->at += size;
    return save_size;
}

int pn_string_seek(void* cookie, int64_t* offset, int whence) {
    struct pn_string_cookie* c = cookie;
    int64_t                  base;
    switch (whence) {
        case SEEK_CUR: base = c->at; break;
        case SEEK_END: base = (*c->s)->count - 1; break;
        case SEEK_SET: base = 0; break;
        default: errno = EINVAL; return -1;
    }
    if ((*offset < 0) && (-*offset > base)) {
        errno = EINVAL;
        return -1;
    }
    *offset = c->at = base + *offset;
    return 0;
}

ssize_t pn_data_read(void* cookie, char* data, size_t size) {
    struct pn_data_cookie* c         = cookie;
    size_t                 len       = (*c->d)->count;
    size_t                 remainder = len - c->at;
    if (remainder < size) {
        size = remainder;
    }
    memcpy(data, (*c->d)->values + c->at, size);
    c->at += size;
    return size;
}

ssize_t pn_data_write(void* cookie, const char* data, size_t size) {
    struct pn_data_cookie* c         = cookie;
    size_t                 save_size = size;
    while (c->at > (int64_t)(*c->d)->count) {
        uint8_t nul = 0;
        pn_datacat(c->d, &nul, 1);
    }
    int64_t len = (*c->d)->count;
    if (c->at < len) {
        size_t overwrite_size = len - c->at;
        if (overwrite_size > size) {
            overwrite_size = size;
        }
        memcpy(&(*c->d)->values[c->at], data, overwrite_size);
        data += overwrite_size;
        size -= overwrite_size;
        c->at += overwrite_size;
    }
    pn_datacat(c->d, (uint8_t*)data, size);
    c->at += size;
    return save_size;
}

int pn_data_seek(void* cookie, int64_t* offset, int whence) {
    struct pn_data_cookie* c = cookie;
    int64_t                base;
    switch (whence) {
        case SEEK_CUR: base = c->at; break;
        case SEEK_END: base = (*c->d)->count; break;
        case SEEK_SET: base = 0; break;
        default: errno = EINVAL; return -1;
    }
    if ((*offset < 0) && (-*offset > base)) {
        errno = EINVAL;
        return -1;
    }
    *offset = c->at = base + *offset;
    return 0;
}

ssize_t pn_view_read(void* cookie, char* data, size_t size) {
    struct pn_view_cookie* c         = cookie;
    size_t                 remainder = c->size - c->at;
    if (remainder < size) {
        size = remainder;
    }
    memcpy(data, c->data + c->at, size);
    c->at += size;
    return size;
}

int pn_view_seek(void* cookie, int64_t* offset, int whence) {
    struct pn_view_cookie* c = cookie;
    size_t                 base;
    switch (whence) {
        case SEEK_CUR: base = c->at; break;
        case SEEK_END: base = c->size; break;
        case SEEK_SET: base = 0; break;
        default: errno = EINVAL; return -1;
    }
    if ((*offset < 0) && ((uint64_t)(-*offset) > base)) {
        errno = EINVAL;
        return -1;
    }
    *offset = c->at = base + *offset;
    return 0;
}
