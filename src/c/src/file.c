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

#include "io.h"

pn_input_t pn_path_input(const char* path, pn_path_flags_t flags) {
    switch (flags) {
        case PN_TEXT:
        case PN_APPEND_TEXT: return pn_file_input(fopen(path, "r"));
        case PN_BINARY:
        case PN_APPEND_BINARY: return pn_file_input(fopen(path, "rb"));
    }
}

pn_input_t pn_file_input(FILE* f) {
    pn_input_t in = {.type = f ? PN_INPUT_TYPE_C_FILE : PN_INPUT_TYPE_INVALID, .c_file = f};
    return in;
}

pn_input_t pn_data_input(const pn_data_t* d) {
    if (!d) {
        errno = EINVAL;
        return pn_file_input(NULL);
    }
    return pn_view_input(d->values, d->count);
}

pn_input_t pn_string_input(const pn_string_t* s) {
    if (!s) {
        errno = EINVAL;
        return pn_file_input(NULL);
    }
    return pn_view_input(s->values, s->count);
}

pn_input_t pn_view_input(const void* data, size_t size) {
    struct pn_input_view* view = malloc(sizeof(struct pn_input_view));
    view->data                 = data;
    view->size                 = size;
    pn_input_t in              = {.type = PN_INPUT_TYPE_VIEW, .view = view};
    return in;
}

pn_output_t pn_path_output(const char* path, pn_path_flags_t flags) {
    switch (flags) {
        case PN_TEXT: return pn_file_output(fopen(path, "w"));
        case PN_APPEND_TEXT: return pn_file_output(fopen(path, "a"));
        case PN_BINARY: return pn_file_output(fopen(path, "wb"));
        case PN_APPEND_BINARY: return pn_file_output(fopen(path, "ab"));
    }
}

pn_output_t pn_file_output(FILE* f) {
    pn_output_t out = {.type = f ? PN_OUTPUT_TYPE_C_FILE : PN_OUTPUT_TYPE_INVALID, .c_file = f};
    return out;
}

pn_output_t pn_data_output(pn_data_t** d) {
    if (!d || !*d) {
        errno = EINVAL;
        return pn_file_output(NULL);
    }
    pn_output_t out = {.type = PN_OUTPUT_TYPE_DATA, .data = d};
    return out;
}

pn_output_t pn_string_output(pn_string_t** s) {
    if (!s || !*s) {
        errno = EINVAL;
        return pn_file_output(NULL);
    }
    pn_output_t out = {.type = PN_OUTPUT_TYPE_STRING, .string = s};
    return out;
}

bool pn_input_close(pn_input_t* in) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return true;
        case PN_INPUT_TYPE_C_FILE: return !fclose(in->c_file);
        case PN_INPUT_TYPE_STDIN: return !fclose(stdin);
        case PN_INPUT_TYPE_VIEW: return free(in->view), true;
    }
}

bool pn_input_eof(const pn_input_t* in) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return true;
        case PN_INPUT_TYPE_C_FILE: return feof(in->c_file);
        case PN_INPUT_TYPE_STDIN: return feof(stdin);
        case PN_INPUT_TYPE_VIEW: return !in->view->data;
    }
}

bool pn_input_error(const pn_input_t* in) {
    switch (in->type) {
        case PN_INPUT_TYPE_INVALID: return true;
        case PN_INPUT_TYPE_C_FILE: return ferror(in->c_file);
        case PN_INPUT_TYPE_STDIN: return ferror(stdin);
        case PN_INPUT_TYPE_VIEW: return false;
    }
}

bool pn_output_close(pn_output_t* out) {
    switch (out->type) {
        case PN_OUTPUT_TYPE_INVALID: return true;
        case PN_OUTPUT_TYPE_C_FILE: return !fclose(out->c_file);
        case PN_OUTPUT_TYPE_STDOUT: return !fclose(stdout);
        case PN_OUTPUT_TYPE_STDERR: return !fclose(stderr);
        case PN_OUTPUT_TYPE_DATA: return true;
        case PN_OUTPUT_TYPE_STRING: return true;
    }
}

bool pn_output_eof(const pn_output_t* out) {
    switch (out->type) {
        case PN_OUTPUT_TYPE_INVALID: return true;
        case PN_OUTPUT_TYPE_C_FILE: return feof(out->c_file);
        case PN_OUTPUT_TYPE_STDOUT: return feof(stdout);
        case PN_OUTPUT_TYPE_STDERR: return feof(stderr);
        case PN_OUTPUT_TYPE_DATA: return !out->data;
        case PN_OUTPUT_TYPE_STRING: return !out->string;
    }
}

bool pn_output_error(const pn_output_t* out) {
    switch (out->type) {
        case PN_OUTPUT_TYPE_INVALID: return true;
        case PN_OUTPUT_TYPE_C_FILE: return ferror(out->c_file);
        case PN_OUTPUT_TYPE_STDOUT: return ferror(stdout);
        case PN_OUTPUT_TYPE_STDERR: return ferror(stderr);
        case PN_OUTPUT_TYPE_DATA: return false;
        case PN_OUTPUT_TYPE_STRING: return false;
    }
}

pn_input_t  pn_stdin  = {.type = PN_INPUT_TYPE_STDIN};
pn_output_t pn_stdout = {.type = PN_OUTPUT_TYPE_STDOUT};
pn_output_t pn_stderr = {.type = PN_OUTPUT_TYPE_STDERR};
