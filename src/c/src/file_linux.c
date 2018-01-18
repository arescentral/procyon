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
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./file.h"
#include "./vector.h"

pn_file_t* pn_open_path(const char* path, const char* mode) { return fopen(path, mode); }

static pn_file_t* open_cb(
        void* cookie, size_t cookie_size, const char* mode,          //
        ssize_t read(void* cookie, char* data, size_t size),         //
        ssize_t write(void* cookie, const char* data, size_t size),  //
        int     seek(void* cookie, off64_t* offset, int whence),     //
        int     close(void* cookie)) {
    void* c = malloc(cookie_size);
    memcpy(c, cookie, cookie_size);
    cookie_io_functions_t io_funcs = {read, write, seek, close};
    pn_file_t*            f        = fopencookie(c, mode, io_funcs);
    if (f) {
        if (setvbuf(f, NULL, _IONBF, 0) == 0) {
            return f;
        }
        fclose(f);
    } else {
        free(c);
    }
    return NULL;
}

ssize_t pn_linux_string_read(void* cookie, char* data, size_t size) {
    return pn_string_read(cookie, data, size);
}

ssize_t pn_linux_string_write(void* cookie, const char* data, size_t size) {
    return pn_string_write(cookie, data, size);
}

int pn_linux_string_seek(void* cookie, int64_t* offset, int whence) {
    off64_t off = *offset;
    if (pn_string_seek(cookie, &off, whence) == 0) {
        *offset = off;
        return 0;
    }
    return -1;
}

pn_file_t* pn_open_string(pn_string_t** s, const char* mode) {
    struct pn_string_cookie cookie = {.s = s};
    bool                    read   = false;
    bool                    write  = false;

    if (!s || !mode) {
        errno = EINVAL;
        return NULL;
    }

    switch (pn_file_mode(mode)) {
        case 'R': write = true;
        case 'r':
            read      = true;
            cookie.at = 0;
            break;

        case 'W': read = true;
        case 'w':
            write       = true;
            cookie.at   = 0;
            (*s)->count = 1;
            break;

        case 'A': read = true;
        case 'a':
            write     = true;
            cookie.at = (*s)->count - 1;
            break;

        default: errno = EINVAL; return NULL;
    }

    return open_cb(
            &cookie, sizeof(cookie), mode, read ? pn_linux_string_read : NULL,
            write ? pn_linux_string_write : NULL, pn_linux_string_seek, pn_close);
}

ssize_t pn_linux_data_read(void* cookie, char* data, size_t size) {
    return pn_data_read(cookie, data, size);
}

ssize_t pn_linux_data_write(void* cookie, const char* data, size_t size) {
    return pn_data_write(cookie, data, size);
}

int pn_linux_data_seek(void* cookie, int64_t* offset, int whence) {
    off64_t off = *offset;
    if (pn_data_seek(cookie, &off, whence) == 0) {
        *offset = off;
        return 0;
    }
    return -1;
}

pn_file_t* pn_open_data(pn_data_t** d, const char* mode) {
    struct pn_data_cookie cookie = {.d = d};
    bool                  read   = false;
    bool                  write  = false;

    if (!d || !mode) {
        errno = EINVAL;
        return NULL;
    }

    switch (pn_file_mode(mode)) {
        case 'R': write = true;
        case 'r':
            read      = true;
            cookie.at = 0;
            break;

        case 'W': read = true;
        case 'w':
            write       = true;
            cookie.at   = 0;
            (*d)->count = 0;
            break;

        case 'A': read = true;
        case 'a':
            write     = true;
            cookie.at = (*d)->count;
            break;

        default: errno = EINVAL; return NULL;
    }

    return open_cb(
            &cookie, sizeof(cookie), mode, read ? pn_linux_data_read : NULL,
            write ? pn_linux_data_write : NULL, pn_linux_data_seek, pn_close);
}

ssize_t pn_linux_view_read(void* cookie, char* data, size_t size) {
    return pn_view_read(cookie, data, size);
}

int pn_linux_view_seek(void* cookie, int64_t* offset, int whence) {
    off64_t off = *offset;
    if (pn_view_seek(cookie, &off, whence) == 0) {
        *offset = off;
        return 0;
    }
    return -1;
}

pn_file_t* pn_open_view(const void* data, size_t size) {
    struct pn_view_cookie cookie = {.data = data, .size = size, .at = 0};
    return open_cb(
            &cookie, sizeof(cookie), "r", pn_linux_view_read, NULL, pn_linux_view_seek, pn_close);
}
