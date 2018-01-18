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


class EventType(enum.Enum):
    NULL = 0
    BOOL = 1
    INT = 2
    FLOAT = 3
    DATA = 4
    STRING = 5
    ARRAY_IN = 6
    ARRAY_OUT = 7
    MAP_IN = 8
    MAP_OUT = 9


class Emit(enum.Enum):
    NULL = 0
    TRUE = 1
    FALSE = 2
    INF = 3
    NEG_INF = 4
    NAN = 5
    INT = 6
    FLOAT = 7
    DATA = 8
    ACC_DATA = 9
    STRING = 10
    ACC_STRING = 11
    SHORT_ARRAY_IN = 12
    SHORT_ARRAY_OUT = 13
    LONG_ARRAY_IN = 14
    LONG_ARRAY_OUT = 15
    SHORT_MAP_IN = 16
    SHORT_MAP_OUT = 17
    LONG_MAP_IN = 18
    LONG_MAP_OUT = 19


class Key(enum.Enum):
    NO = 0
    UNQUOTED = 1
    QUOTED = 2


class Acc(enum.Enum):
    DATA = 0
    STRING = 1
    NL = 2
    SP = 3
