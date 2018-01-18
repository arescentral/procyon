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

#ifndef PROCYON_FILE_H_
#define PROCYON_FILE_H_

#include <stdint.h>

struct pn_string_cookie {
    pn_string_t** s;
    int64_t       at;
};

struct pn_data_cookie {
    pn_data_t** d;
    int64_t     at;
};

struct pn_view_cookie {
    const char* const data;
    const size_t      size;
    int64_t           at;
};

char pn_file_mode(const char* mode);
int  pn_close(void* cookie);

ssize_t pn_string_read(void* cookie, char* data, size_t size);
ssize_t pn_string_write(void* cookie, const char* data, size_t size);
int     pn_string_seek(void* cookie, int64_t* offset, int whence);

ssize_t pn_data_read(void* cookie, char* data, size_t size);
ssize_t pn_data_write(void* cookie, const char* data, size_t size);
int     pn_data_seek(void* cookie, int64_t* offset, int whence);

ssize_t pn_view_read(void* cookie, char* data, size_t size);
int     pn_view_seek(void* cookie, int64_t* offset, int whence);

#endif  // PROCYON_FILE_H_
