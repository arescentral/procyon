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

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "./io.h"

static bool pn_read_all_data(pn_input_t* in, pn_data_t** data);
static bool pn_read_all_str(pn_input_t* in, pn_string_t** str);

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
    ptrdiff_t Z;

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

static bool read_bytes_strlen(pn_input_t* in, char* data) {
    return pn_raw_read(in, data, strlen(data));
}

static bool read_primitive(pn_input_t* in, size_t count, union pn_primitive* x) {
    if (!pn_raw_read(in, x->data, count)) {
        return false;
    }
    switch (count) {
        case 2: swap16(x); break;
        case 4: swap32(x); break;
        case 8: swap64(x); break;
    }
    return true;
}

static bool skip_bytes(pn_input_t* in, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (pn_getc(in) == EOF) {
            return false;
        }
    }
    return true;
}

#define PN_READ_PRIMITIVE(FIELD, T) \
    (read_primitive(in, sizeof(T), &p) ? (*va_arg(*vl, T*) = p.FIELD, true) : false)
#define PN_READ_BYTE(T) (pn_raw_read(in, &c, 1) ? (*va_arg(*vl, T*) = c, true) : false)

bool pn_read_arg(pn_input_t* in, char format, va_list* vl) {
    uint8_t            c;
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
        case 'Z': return PN_READ_PRIMITIVE(Z, ptrdiff_t);

        case 'f': return PN_READ_PRIMITIVE(f, float);
        case 'd': return PN_READ_PRIMITIVE(d, double);

        case 's': return read_bytes_strlen(in, va_arg(*vl, char*));
        case 'S': {
            char*  data_ptr  = va_arg(*vl, char*);
            size_t data_size = va_arg(*vl, size_t);
            return pn_raw_read(in, data_ptr, data_size);
        }
        case 'u': {
            uint16_t* data_ptr  = va_arg(*vl, uint16_t*);
            size_t    data_size = va_arg(*vl, size_t);
            return pn_raw_read(in, data_ptr, data_size);
        }
        case 'U': {
            uint32_t* data_ptr  = va_arg(*vl, uint32_t*);
            size_t    data_size = va_arg(*vl, size_t);
            return pn_raw_read(in, data_ptr, data_size);
        }
        case '$': {
            uint8_t* data_ptr  = va_arg(*vl, uint8_t*);
            size_t   data_size = va_arg(*vl, size_t);
            return pn_raw_read(in, data_ptr, data_size);
        }
        case 'c': return PN_READ_BYTE(char);
        case 'C': return PN_READ_PRIMITIVE(L, uint32_t);

        case '#': return skip_bytes(in, va_arg(*vl, size_t));

        case '*': return pn_read_all_data(in, va_arg(*vl, pn_data_t**));
        case '+': return pn_read_all_str(in, va_arg(*vl, pn_string_t**));
    }
    return false;
}

bool pn_read(pn_input_t* in, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    for (size_t i = 0; i < strlen(format); ++i) {
        if (!pn_read_arg(in, format[i], &vl)) {
            return false;
        }
    }
    va_end(vl);
    return true;
}

static bool write_byte(pn_output_t* out, uint8_t byte) { return pn_putc(byte, out) != EOF; }

static bool write_bytes_strlen(pn_output_t* out, const char* data) {
    return pn_raw_write(out, data, strlen(data));
}

static bool write_primitive(pn_output_t* out, size_t count, union pn_primitive* x) {
    switch (count) {
        case 2: swap16(x); break;
        case 4: swap32(x); break;
        case 8: swap64(x); break;
    }
    return pn_raw_write(out, x->data, count);
}

static bool write_repeated(pn_output_t* out, int count) {
    for (int i = 0; i < count; ++i) {
        if (pn_putc(0, out) == EOF) {
            return false;
        }
    }
    return true;
}

#define PN_WRITE_PRIMITIVE(FIELD, T, VA_T) \
    (p.FIELD = va_arg(*vl, VA_T), write_primitive(out, sizeof(T), &p))

static bool pn_write_arg(pn_output_t* out, char format, va_list* vl) {
    union pn_primitive p;
    switch (format) {
        case 'n': return true;

        case '?': return write_byte(out, va_arg(*vl, int));

        case 'i': return PN_WRITE_PRIMITIVE(i, int, int);
        case 'I': return PN_WRITE_PRIMITIVE(I, unsigned, unsigned);
        case 'b': return write_byte(out, va_arg(*vl, int));
        case 'B': return write_byte(out, va_arg(*vl, int));
        case 'h': return PN_WRITE_PRIMITIVE(h, int16_t, int);
        case 'H': return PN_WRITE_PRIMITIVE(H, uint16_t, int);
        case 'l': return PN_WRITE_PRIMITIVE(l, int32_t, int32_t);
        case 'L': return PN_WRITE_PRIMITIVE(L, uint32_t, uint32_t);
        case 'q': return PN_WRITE_PRIMITIVE(q, int64_t, int64_t);
        case 'Q': return PN_WRITE_PRIMITIVE(Q, uint64_t, uint64_t);
        case 'p': return PN_WRITE_PRIMITIVE(p, intptr_t, intptr_t);
        case 'P': return PN_WRITE_PRIMITIVE(P, uintptr_t, uintptr_t);
        case 'z': return PN_WRITE_PRIMITIVE(z, size_t, size_t);
        case 'Z': return PN_WRITE_PRIMITIVE(Z, ptrdiff_t, ptrdiff_t);

        case 'f': return PN_WRITE_PRIMITIVE(f, float, double);
        case 'd': return PN_WRITE_PRIMITIVE(d, double, double);

        case 's': return write_bytes_strlen(out, va_arg(*vl, const char*));
        case 'S': {
            const char*  data_ptr  = va_arg(*vl, const char*);
            const size_t data_size = va_arg(*vl, size_t);
            return pn_raw_write(out, data_ptr, data_size);
        }
        case 'u': {
            const uint16_t* data_ptr  = va_arg(*vl, const uint16_t*);
            const size_t    data_size = va_arg(*vl, size_t);
            return pn_raw_write(out, data_ptr, data_size);
        }
        case 'U': {
            const uint32_t* data_ptr  = va_arg(*vl, const uint32_t*);
            const size_t    data_size = va_arg(*vl, size_t);
            return pn_raw_write(out, data_ptr, data_size);
        }
        case '$': {
            const uint8_t* data_ptr  = va_arg(*vl, const uint8_t*);
            const size_t   data_size = va_arg(*vl, size_t);
            return pn_raw_write(out, data_ptr, data_size);
        }
        case 'c': return write_byte(out, va_arg(*vl, int));
        case 'C': return PN_WRITE_PRIMITIVE(L, uint32_t, uint32_t);

        case '#': return write_repeated(out, va_arg(*vl, size_t));
    }
    return false;
}

bool pn_write(pn_output_t* out, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    for (size_t i = 0; i < strlen(format); ++i) {
        if (!pn_write_arg(out, format[i], &vl)) {
            return false;
        }
    }
    va_end(vl);
    return true;
}

int pn_getc(pn_input_t* in) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return EOF;
        case PN_INPUT_TYPE_C_FILE: return getc(in->c_file);
        case PN_INPUT_TYPE_STDIN: return getc(stdin);

        case PN_INPUT_TYPE_VIEW:
            if (in->view->size == 0) {
                in->view->data = NULL;
                return EOF;
            }
            char ch = *(char*)in->view->data;
            --in->view->size;
            in->view->data = (char*)in->view->data + 1;
            return ch;
        default: return EOF;
    }
}

int pn_putc(int ch, pn_output_t* f) {
    switch (f->type) {
        case PN_OUTPUT_TYPE_INVALID: return EOF;
        case PN_OUTPUT_TYPE_C_FILE: return putc(ch, f->c_file);
        case PN_OUTPUT_TYPE_STDOUT: return putc(ch, stdout);
        case PN_OUTPUT_TYPE_STDERR: return putc(ch, stderr);

        case PN_OUTPUT_TYPE_DATA: {
            uint8_t s[] = {ch};
            pn_datacat(f->data, s, 1);
            return true;
        }

        case PN_OUTPUT_TYPE_STRING: {
            char s[] = {ch};
            pn_strncat(f->string, s, 1);
            return true;
        }
        default: return EOF;
    }
}

bool pn_raw_read(pn_input_t* in, void* data, size_t size) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return false;
        case PN_INPUT_TYPE_C_FILE: return fread(data, 1, size, in->c_file) == size;
        case PN_INPUT_TYPE_STDIN: return fread(data, 1, size, stdin) == size;

        case PN_INPUT_TYPE_VIEW:
            if (in->view->size < size) {
                in->view->size = 0;
                in->view->data = NULL;
                return false;
            }
            memmove(data, in->view->data, size);
            in->view->size -= size;
            in->view->data = (char*)in->view->data + size;
            return true;
        default: return false;
    }
}

static bool pn_file_read_data(FILE* in, pn_data_t** data, size_t size) {
    size_t start = (*data)->count;
    pn_dataresize(data, (*data)->count + size);
    size_t count   = fread((*data)->values + start, 1, size, in);
    (*data)->count = start + count;
    return count == size;
}

static bool pn_file_read_all_data(FILE* in, pn_data_t** data) {
    while (true) {
        if (!pn_file_read_data(in, data, 4096)) {
            return feof(in);
        }
    }
}

static bool pn_view_read_all_data(struct pn_input_view* in, pn_data_t** data) {
    pn_datacat(data, in->data, in->size);
    in->data = NULL;
    in->size = 0;
    return true;
}

static bool pn_read_all_data(pn_input_t* in, pn_data_t** data) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return false;
        case PN_INPUT_TYPE_C_FILE: return pn_file_read_all_data(in->c_file, data);
        case PN_INPUT_TYPE_STDIN: return pn_file_read_all_data(stdin, data);
        case PN_INPUT_TYPE_VIEW: return pn_view_read_all_data(in->view, data);
        default: return false;
    }
}

static bool pn_file_read_str(FILE* in, pn_string_t** str, size_t size) {
    size_t start = (*str)->count - 1;
    pn_strresize(str, (*str)->count + size - 1);
    size_t count  = fread((*str)->values + start, 1, size, in);
    (*str)->count = start + count + 1;
    return count == size;
}

static bool pn_file_read_all_str(FILE* in, pn_string_t** str) {
    while (true) {
        if (!pn_file_read_str(in, str, 4096)) {
            (*str)->values[(*str)->count - 1] = '\0';
            return feof(in);
        }
    }
}

static bool pn_view_read_all_str(struct pn_input_view* in, pn_string_t** str) {
    pn_strncat(str, in->data, in->size);
    in->data = NULL;
    in->size = 0;
    return true;
}

static bool pn_read_all_str(pn_input_t* in, pn_string_t** str) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return false;
        case PN_INPUT_TYPE_C_FILE: return pn_file_read_all_str(in->c_file, str);
        case PN_INPUT_TYPE_STDIN: return pn_file_read_all_str(stdin, str);
        case PN_INPUT_TYPE_VIEW: return pn_view_read_all_str(in->view, str);
        default: return false;
    }
}

bool pn_raw_write(pn_output_t* out, const void* data, size_t size) {
    switch (out->type) {
        case PN_OUTPUT_TYPE_INVALID: return false;
        case PN_OUTPUT_TYPE_C_FILE: return fwrite(data, 1, size, out->c_file) == size;
        case PN_OUTPUT_TYPE_STDOUT: return fwrite(data, 1, size, stdout) == size;
        case PN_OUTPUT_TYPE_STDERR: return fwrite(data, 1, size, stderr) == size;
        case PN_OUTPUT_TYPE_DATA: return pn_datacat(out->data, data, size), true;
        case PN_OUTPUT_TYPE_STRING: return pn_strncat(out->string, data, size), true;
        default: return false;
    }
}

ptrdiff_t pn_file_getline(FILE* f, char** data, size_t* size) {
    if (!(data && size)) {
        errno = EINVAL;
        return -1;
    }

    int    ch;
    size_t len = 0;
    do {
        ch = getc(f);
        if (ch == EOF) {
            if (!len) {
                return -1;
            }
            break;
        } else if (len == PTRDIFF_MAX) {
            errno = EOVERFLOW;
            return -1;
        } else if (*size == 0) {
            *size = 16;
            *data = realloc(*data, *size);
        } else if (*size <= len) {
            *size = 2 * *size;
            *data = realloc(*data, *size);
        }
        (*data)[len++] = ch;
    } while (ch != '\n');
    return len;
}

ptrdiff_t pn_getline(pn_input_t* in, char** data, size_t* size) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return -1;
        case PN_INPUT_TYPE_C_FILE: return pn_file_getline(in->c_file, data, size);
        case PN_INPUT_TYPE_STDIN: return pn_file_getline(stdin, data, size);

        case PN_INPUT_TYPE_VIEW: {
            if (!(data && size)) {
                in->view->data = NULL;
                errno          = EINVAL;
                return -1;
            }

            if (in->view->size == 0) {
                in->view->data = NULL;
                return -1;
            }

            void*  nl       = memchr(in->view->data, '\n', in->view->size);
            size_t out_size = nl ? ((char*)nl - (char*)in->view->data + 1) : in->view->size;
            if (*size < (out_size + 1)) {
                *data = realloc(*data, (out_size + 1));
                *size = (out_size + 1);
            }
            memmove(*data, in->view->data, out_size);
            (*data)[out_size] = '\0';
            in->view->data    = (char*)in->view->data + out_size;
            in->view->size -= out_size;
            return out_size;
        }
        default: return -1;
    }
}
