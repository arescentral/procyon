#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2017 The Procyon Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import, division, print_function, unicode_literals

import enum


class Error(enum.Enum):
    OK = 0
    INTERNAL = 1
    SYSTEM = 2

    OUTDENT = 3

    CHILD = 4
    SIBLING = 5
    SUFFIX = 6
    LONG = 7
    SHORT = 8
    ARRAY_END = 9
    MAP_KEY = 10
    MAP_END = 11
    CTRL = 12
    NONASCII = 13
    UTF8_HEAD = 14
    UTF8_TAIL = 15
    BADCHAR = 16
    DATACHAR = 17
    PARTIAL = 18
    BADWORD = 19
    BADESC = 20
    BADUESC = 21
    STREOL = 22
    BANG_SUFFIX = 23
    BANG_LAST = 24
    INT_OVERFLOW = 25
    INVALID_INT = 26
    FLOAT_OVERFLOW = 27
    INVALID_FLOAT = 28
    RECURSION = 29


error_message = {}
error_message[Error.OK] = "ok"
error_message[Error.INTERNAL] = "internal error"
error_message[Error.SYSTEM] = "system error"
error_message[Error.OUTDENT] = "unindent does not match any outer indentation level"
error_message[Error.CHILD] = "unexpected child"
error_message[Error.SIBLING] = "unexpected sibling"
error_message[Error.SUFFIX] = "expected end-of-line"
error_message[Error.LONG] = "expected value"
error_message[Error.SHORT] = "expected value"
error_message[Error.ARRAY_END] = "expected ',' or ']'"
error_message[Error.MAP_KEY] = "expected key",
error_message[Error.MAP_END] = "expected ',' or '}'"
error_message[Error.BADCHAR] = "invalid character"
error_message[Error.DATACHAR] = "word char in data"
error_message[Error.PARTIAL] = "partial byte"
error_message[Error.CTRL] = "invalid control character"
error_message[Error.NONASCII] = "invalid non-ASCII character"
error_message[Error.UTF8_HEAD] = "invalid UTF-8 start byte"
error_message[Error.UTF8_TAIL] = "invalid UTF-8 continuation byte"
error_message[Error.BADWORD] = "unknown word"
error_message[Error.BADESC] = "invalid escape"
error_message[Error.BADUESC] = "invalid \\uXXXX escape"
error_message[Error.STREOL] = "eol while scanning string"
error_message[Error.BANG_SUFFIX] = "expected eol after '!'"
error_message[Error.BANG_LAST] = "expected eos after '!'"
error_message[Error.INT_OVERFLOW] = "integer overflow"
error_message[Error.INVALID_INT] = "invalid integer"
error_message[Error.FLOAT_OVERFLOW] = "float overflow"
error_message[Error.INVALID_FLOAT] = "invalid float"
error_message[Error.RECURSION] = "recursion limit exceeded"


class ProcyonDecodeError(ValueError):
    def __init__(self, code, lineno, column):
        ValueError.__init__(self, "{0}:{1}: {2}".format(lineno, column, error_message[code]))
        self.code = code
        self.lineno = lineno
        self.column = column
