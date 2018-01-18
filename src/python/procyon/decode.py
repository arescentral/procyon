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

import collections
import json
from .error import Error, ProcyonDecodeError
from .lex import Lexer
from .parse import EventType, ProcyonParser
from .token import Token


class ProcyonDecoder(object):
    def parse(self, lines):
        return self._decode(ProcyonParser(Lexer(lines)))

    def _decode(self, p):
        stack = []
        while p.next():
            if p.event.type in [EventType.ARRAY_OUT, EventType.MAP_OUT]:
                pass
            elif p.event.type == EventType.ARRAY_IN:
                stack.append((p.event.key, []))
                continue
            elif p.event.type == EventType.MAP_IN:
                stack.append((p.event.key, collections.OrderedDict()))
                continue
            else:
                stack.append((p.event.key, p.event.value))

            k, x = stack.pop()
            if not stack:
                result = x
                continue

            _, top = stack[-1]
            if isinstance(top, list):
                top.append(x)
            elif isinstance(top, dict):
                top[k] = x

        return result


def load(f):
    return ProcyonDecoder().parse(f)


def loads(s):
    return ProcyonDecoder().parse(s.splitlines())


__all__ = ["ProcyonDecoder", "load", "loads"]
