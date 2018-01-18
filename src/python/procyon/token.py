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


class Token(enum.Enum):
    # Virtual tokens
    LINE_IN = 0
    LINE_EQ = 1
    LINE_OUT = 2

    # Fixed sequences
    STAR = 3
    ARRAY_IN = 4
    ARRAY_OUT = 5
    MAP_IN = 6
    MAP_OUT = 7
    COMMA = 8
    STR_WRAP_EMPTY = 9  # >
    STR_PIPE_EMPTY = 10  # |
    STR_BANG = 11  # !
    NULL = 12  # null
    TRUE = 13  # true
    FALSE = 14  # false
    INF = 15  # inf
    NEG_INF = 16  # -inf
    NAN = 17  # nan

    # Matched sequences
    KEY = 18  # key:    hi:     0:      -/.+:
    QKEY = 19  # "key":  "hi":   "0":    "\\\"":
    INT = 20  # 0       1       -1
    FLOAT = 21  # 0.0     1e100   -0.5
    DATA = 22  # $       $01     $ 01234567 89abcdef
    STR = 23  # ""      "str"   "\n\\\0"
    STR_WRAP = 24  # > string line
    STR_PIPE = 25  # | string line
    COMMENT = 26  # # comment

    ERROR = 27


FLAG_VALUE = 0o077
FLAG_OK = 0o100
FLAG_DONE = 0o200
