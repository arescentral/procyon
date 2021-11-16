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

import enum


class Type(enum.Enum):
    NULL = 0
    BOOL = 1
    INT = 2
    FLOAT = 3
    DATA = 4
    STRING = 5
    ARRAY = 6
    MAP = 7


def typeof(x):
    if x is None:
        return Type.NULL
    elif (x is True) or (x is False):
        return Type.BOOL
    elif isinstance(x, int):
        return Type.INT
    elif isinstance(x, float):
        return Type.FLOAT
    elif isinstance(x, (bytes, bytearray, memoryview)):
        return Type.DATA
    elif isinstance(x, str):
        return Type.STRING
    elif isinstance(x, (tuple, list)):
        return Type.ARRAY
    elif isinstance(x, dict):
        return Type.MAP
    else:
        raise TypeError("%r is not Procyon-serializable" % x)
