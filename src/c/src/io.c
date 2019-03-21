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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "./io.h"

union pn_primitive {
    int       i;
    unsigned  I;
    int16_t   h;
    uint16_t  H;
    int32_t   l;
    uint32_t  L;
    int64_t   q;
    uint64_t  Q;
    intptr_t  p;
    uintptr_t P;
    size_t    z;
    ssize_t   Z;

    float  f;
    double d;

    char data[8];
};

static void swap16(union pn_primitive* x) { x->H = htons(x->H); }
static void swap32(union pn_primitive* x) { x->L = htonl(x->L); }
static void swap64(union pn_primitive* x) {
    if (htons(0x0001) != 0x0001) {
        uint64_t hi = ntohl(x->Q);
        uint64_t lo = ntohl(x->Q >> 32);
        x->Q        = (hi << 32) | lo;
    }
}

static bool read_bytes_strlen(pn_file_t file, char* data) {
    return pn_raw_read(file, data, strlen(data));
}

static bool read_primitive(pn_file_t file, size_t count, union pn_primitive* out) {
    if (!pn_raw_read(file, out->data, count)) {
        return false;
    }
    switch (count) {
        case 2: swap16(out); break;
        case 4: swap32(out); break;
        case 8: swap64(out); break;
    }
    return true;
}

static bool skip_bytes(pn_file_t file, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (pn_getc(file) == EOF) {
            return false;
        }
    }
    return true;
}

#define PN_READ_PRIMITIVE(FIELD, T) \
    (read_primitive(file, sizeof(T), &p) ? (*va_arg(*vl, T*) = p.FIELD, true) : false)
#define PN_READ_BYTE(T) (((c = pn_getc(file)) != EOF) ? (*va_arg(*vl, T*) = c, true) : false)

bool pn_read_arg(pn_file_t file, char format, va_list* vl) {
    int                c;
    union pn_primitive p;
    switch (format) {
        default: return false;

        case 'n': return true;

        case '?': return PN_READ_BYTE(bool);

        case 'i': return PN_READ_PRIMITIVE(i, int);
        case 'I': return PN_READ_PRIMITIVE(I, unsigned);
        case 'b': return PN_READ_BYTE(int8_t);
        case 'B': return PN_READ_BYTE(uint8_t);
        case 'h': return PN_READ_PRIMITIVE(h, int16_t);
        case 'H': return PN_READ_PRIMITIVE(H, uint16_t);
        case 'l': return PN_READ_PRIMITIVE(l, int32_t);
        case 'L': return PN_READ_PRIMITIVE(L, uint32_t);
        case 'q': return PN_READ_PRIMITIVE(q, int64_t);
        case 'Q': return PN_READ_PRIMITIVE(Q, uint64_t);
        case 'p': return PN_READ_PRIMITIVE(p, intptr_t);
        case 'P': return PN_READ_PRIMITIVE(P, uintptr_t);
        case 'z': return PN_READ_PRIMITIVE(z, size_t);
        case 'Z': return PN_READ_PRIMITIVE(Z, ssize_t);

        case 'f': return PN_READ_PRIMITIVE(f, float);
        case 'd': return PN_READ_PRIMITIVE(d, double);

        case 's': return read_bytes_strlen(file, va_arg(*vl, char*));
        case 'S': return pn_raw_read(file, va_arg(*vl, char*), va_arg(*vl, size_t));
        case '$': return pn_raw_read(file, va_arg(*vl, uint8_t*), va_arg(*vl, size_t));
        case 'c': return PN_READ_BYTE(char);
        case 'C': return PN_READ_PRIMITIVE(L, uint32_t);

        case '#': return skip_bytes(file, va_arg(*vl, size_t));
    }
    return false;
}

bool pn_read(pn_file_t file, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    for (size_t i = 0; i < strlen(format); ++i) {
        if (!pn_read_arg(file, format[i], &vl)) {
            return false;
        }
    }
    va_end(vl);
    return true;
}

static bool write_byte(pn_file_t file, uint8_t byte) { return pn_putc(byte, file) != EOF; }

static bool write_bytes_strlen(pn_file_t file, const char* data) {
    return pn_raw_write(file, data, strlen(data));
}

static bool write_primitive(pn_file_t file, size_t count, union pn_primitive* out) {
    switch (count) {
        case 2: swap16(out); break;
        case 4: swap32(out); break;
        case 8: swap64(out); break;
    }
    return pn_raw_write(file, out->data, count);
}

static bool write_repeated(pn_file_t file, int count) {
    for (int i = 0; i < count; ++i) {
        if (pn_putc(0, file) == EOF) {
            return false;
        }
    }
    return true;
}

#define PN_WRITE_PRIMITIVE(FIELD, T, VA_T) \
    (p.FIELD = va_arg(*vl, VA_T), write_primitive(file, sizeof(T), &p))

static bool pn_write_arg(pn_file_t file, char format, va_list* vl) {
    union pn_primitive p;
    switch (format) {
        case 'n': return true;

        case '?': return write_byte(file, va_arg(*vl, int));

        case 'i': return PN_WRITE_PRIMITIVE(i, int, int);
        case 'I': return PN_WRITE_PRIMITIVE(I, unsigned, unsigned);
        case 'b': return write_byte(file, va_arg(*vl, int));
        case 'B': return write_byte(file, va_arg(*vl, int));
        case 'h': return PN_WRITE_PRIMITIVE(h, int16_t, int);
        case 'H': return PN_WRITE_PRIMITIVE(H, uint16_t, int);
        case 'l': return PN_WRITE_PRIMITIVE(l, int32_t, int32_t);
        case 'L': return PN_WRITE_PRIMITIVE(L, uint32_t, uint32_t);
        case 'q': return PN_WRITE_PRIMITIVE(q, int64_t, int64_t);
        case 'Q': return PN_WRITE_PRIMITIVE(Q, uint64_t, uint64_t);
        case 'p': return PN_WRITE_PRIMITIVE(p, intptr_t, intptr_t);
        case 'P': return PN_WRITE_PRIMITIVE(P, uintptr_t, uintptr_t);
        case 'z': return PN_WRITE_PRIMITIVE(z, size_t, size_t);
        case 'Z': return PN_WRITE_PRIMITIVE(Z, ssize_t, ssize_t);

        case 'f': return PN_WRITE_PRIMITIVE(f, float, double);
        case 'd': return PN_WRITE_PRIMITIVE(d, double, double);

        case 's': return write_bytes_strlen(file, va_arg(*vl, const char*));
        case 'S': return pn_raw_write(file, va_arg(*vl, const char*), va_arg(*vl, size_t));
        case '$': return pn_raw_write(file, va_arg(*vl, const uint8_t*), va_arg(*vl, size_t));
        case 'c': return write_byte(file, va_arg(*vl, int));
        case 'C': return PN_WRITE_PRIMITIVE(L, uint32_t, uint32_t);

        case '#': return write_repeated(file, va_arg(*vl, size_t));
    }
    return false;
}

bool pn_write(pn_file_t file, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    for (size_t i = 0; i < strlen(format); ++i) {
        if (!pn_write_arg(file, format[i], &vl)) {
            return false;
        }
    }
    va_end(vl);
    return true;
}

int pn_getc(pn_file_t f) {
    switch (f.type) {
        case PN_FILE_TYPE_INVALID: return EOF;
        case PN_FILE_TYPE_STDIN: return getc(stdin);
        case PN_FILE_TYPE_STDOUT: return getc(stdout);
        case PN_FILE_TYPE_STDERR: return getc(stderr);
        case PN_FILE_TYPE_C_FILE: return getc(f.c_file);
    }
}

int pn_putc(int ch, pn_file_t f) {
    switch (f.type) {
        case PN_FILE_TYPE_INVALID: return EOF;
        case PN_FILE_TYPE_STDIN: return putc(ch, stdin);
        case PN_FILE_TYPE_STDOUT: return putc(ch, stdout);
        case PN_FILE_TYPE_STDERR: return putc(ch, stderr);
        case PN_FILE_TYPE_C_FILE: return putc(ch, f.c_file);
    }
}

bool pn_raw_read(pn_file_t f, void* data, size_t size) {
    switch (f.type) {
        case PN_FILE_TYPE_INVALID: return false;
        case PN_FILE_TYPE_STDIN: return fread(data, 1, size, stdin) == size;
        case PN_FILE_TYPE_STDOUT: return fread(data, 1, size, stdout) == size;
        case PN_FILE_TYPE_STDERR: return fread(data, 1, size, stderr) == size;
        case PN_FILE_TYPE_C_FILE: return fread(data, 1, size, f.c_file) == size;
    }
}

bool pn_raw_write(pn_file_t f, const void* data, size_t size) {
    switch (f.type) {
        case PN_FILE_TYPE_INVALID: return false;
        case PN_FILE_TYPE_STDIN: return fwrite(data, 1, size, stdin) == size;
        case PN_FILE_TYPE_STDOUT: return fwrite(data, 1, size, stdout) == size;
        case PN_FILE_TYPE_STDERR: return fwrite(data, 1, size, stderr) == size;
        case PN_FILE_TYPE_C_FILE: return fwrite(data, 1, size, f.c_file) == size;
    }
}

ssize_t pn_getline(pn_file_t f, char** data, size_t* size) {
    switch (f.type) {
        case PN_FILE_TYPE_INVALID: return -1;
        case PN_FILE_TYPE_STDIN: return getline(data, size, stdin);
        case PN_FILE_TYPE_STDOUT: return getline(data, size, stdout);
        case PN_FILE_TYPE_STDERR: return getline(data, size, stderr);
        case PN_FILE_TYPE_C_FILE: return getline(data, size, f.c_file);
    }
}
