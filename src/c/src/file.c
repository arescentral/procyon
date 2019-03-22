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
#include <pn/procyon.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

pn_file_t pn_open_path(const char* path, const char* mode) {
    return pn_wrap_file(fopen(path, mode));
}

pn_file_t pn_wrap_file(FILE* f) {
    pn_file_t file = {.type = f ? PN_FILE_TYPE_C_FILE : PN_FILE_TYPE_INVALID, .c_file = f};
    return file;
}

pn_file_t pn_view_input(const void* data, size_t size) {
    pn_file_t f = {.type = PN_FILE_TYPE_VIEW, .view_data = data, .view_size = size};
    return f;
}

pn_file_t pn_data_input(const pn_data_t* d) { return pn_view_input(d->values, d->count); }
pn_file_t pn_string_input(const pn_string_t* s) { return pn_view_input(s->values, s->count - 1); }

pn_file_t pn_data_output(pn_data_t** d) {
    if (!d || !*d) {
        errno = EINVAL;
        return pn_wrap_file(NULL);
    }
    pn_file_t f = {.type = PN_FILE_TYPE_DATA, .data = d};
    return f;
}

pn_file_t pn_string_output(pn_string_t** s) {
    if (!s || !*s) {
        errno = EINVAL;
        return pn_wrap_file(NULL);
    }
    pn_file_t f = {.type = PN_FILE_TYPE_STRING, .string = s};
    return f;
}

bool pn_close(pn_file_t* file) {
    switch (file->type) {
        case PN_FILE_TYPE_INVALID: return true;
        case PN_FILE_TYPE_STDIN: return !fclose(stdin);
        case PN_FILE_TYPE_STDOUT: return !fclose(stdout);
        case PN_FILE_TYPE_STDERR: return !fclose(stderr);
        case PN_FILE_TYPE_C_FILE: return !fclose(file->c_file);
        case PN_FILE_TYPE_VIEW: return true;
        case PN_FILE_TYPE_DATA: return true;
        case PN_FILE_TYPE_STRING: return true;
    }
}

bool pn_file_eof(const pn_file_t* file) {
    switch (file->type) {
        case PN_FILE_TYPE_INVALID: return true;
        case PN_FILE_TYPE_STDIN: return feof(stdin);
        case PN_FILE_TYPE_STDOUT: return feof(stdout);
        case PN_FILE_TYPE_STDERR: return feof(stderr);
        case PN_FILE_TYPE_C_FILE: return feof(file->c_file);
        case PN_FILE_TYPE_VIEW: return !file->view_data;
        case PN_FILE_TYPE_DATA: return !file->data;
        case PN_FILE_TYPE_STRING: return !file->string;
    }
}

bool pn_file_error(const pn_file_t* file) {
    switch (file->type) {
        case PN_FILE_TYPE_INVALID: return true;
        case PN_FILE_TYPE_STDIN: return ferror(stdin);
        case PN_FILE_TYPE_STDOUT: return ferror(stdout);
        case PN_FILE_TYPE_STDERR: return ferror(stderr);
        case PN_FILE_TYPE_C_FILE: return ferror(file->c_file);
        case PN_FILE_TYPE_VIEW: return false;
        case PN_FILE_TYPE_DATA: return false;
        case PN_FILE_TYPE_STRING: return false;
    }
}

pn_file_t pn_stdin  = {.type = PN_FILE_TYPE_STDIN};
pn_file_t pn_stdout = {.type = PN_FILE_TYPE_STDOUT};
pn_file_t pn_stderr = {.type = PN_FILE_TYPE_STDERR};
