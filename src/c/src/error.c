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

static const char* error_messages[] = {
        [PN_OK]                 = "ok",
        [PN_ERROR_INTERNAL]     = "internal error",
        [PN_ERROR_SYSTEM]       = "system error",
        [PN_ERROR_OUTDENT]      = "unindent does not match any outer indentation level",
        [PN_ERROR_CHILD]        = "unexpected child",
        [PN_ERROR_SIBLING]      = "unexpected sibling",
        [PN_ERROR_SUFFIX]       = "expected end-of-line",
        [PN_ERROR_LONG]         = "expected value",
        [PN_ERROR_SHORT]        = "expected value",
        [PN_ERROR_ARRAY_END]    = "expected ',' or ']'",
        [PN_ERROR_MAP_KEY]      = "expected key",
        [PN_ERROR_MAP_END]      = "expected ',' or '}'",
        [PN_ERROR_BADCHAR]      = "invalid character",
        [PN_ERROR_DATACHAR]     = "word char in data",
        [PN_ERROR_PARTIAL]      = "partial byte",
        [PN_ERROR_CTRL]         = "invalid control character",
        [PN_ERROR_NONASCII]     = "invalid non-ASCII character",
        [PN_ERROR_UTF8_HEAD]    = "invalid UTF-8 start byte",
        [PN_ERROR_UTF8_TAIL]    = "invalid UTF-8 continuation byte",
        [PN_ERROR_BADWORD]      = "unknown word",
        [PN_ERROR_BADESC]       = "invalid escape",
        [PN_ERROR_BADUESC]      = "invalid \\uXXXX escape",
        [PN_ERROR_STREOL]       = "eol while scanning string",
        [PN_ERROR_BANG_SUFFIX]  = "expected eol after '!'",
        [PN_ERROR_BANG_LAST]    = "expected eos after !",
        [PN_ERROR_INT_OVERFLOW] = "integer overflow",
        [PN_ERROR_INVALID_INT]  = "invalid integer",
        [PN_ERROR_RECURSION]    = "recursion limit exceeded",
};
const char* pn_strerror(pn_error_code_t code) { return error_messages[code]; }
