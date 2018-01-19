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
import enum
import re
from . import spec
from .dump import ShortStyle, dumps
from .error import Error, ProcyonDecodeError
from .lex import Lexer, Token
from .parse_enums import Acc, Emit, EventType, Key
from .types import Type
from .utf import unichr

inf = float("inf")
nan = float("nan")

Event = collections.namedtuple("Event", "type key flags value".split())


def _parse_short_string(s):
    out = ""
    for run, esc, u16, u32 in re.findall(r"([^\\]+)|\\([^Uu])|\\u(....)|\\U(........)", s[1:-1]):
        if run:
            out += run
        elif esc:
            out += {
                "b": "\b",
                "f": "\f",
                "n": "\n",
                "r": "\r",
                "t": "\t",
                '"': '"',
                "\\": "\\",
                "/": "/",
            }[esc]
        elif u16:
            out += unichr(int(u16, 16))
        elif u32:
            out += unichr(int(u32, 16))
    return out


class ProcyonParser(object):
    def __init__(self, l):
        self.l = l
        self.stack = [0]
        self.event = None
        self.acc = []
        self.key = None

    def _value(self):
        begin, end = self.l.token.range
        return self.l.token.line[begin:end].decode("utf-8")

    def _int_value(self):
        i = int(self._value())
        if not (-0x8000000000000000 <= i < 0x8000000000000000):
            raise ProcyonDecodeError(Error.INT_OVERFLOW, self.l.token.lineno,
                                     self.l.token.range[0] + 1)
        return i

    def _float_value(self):
        return float(self._value())

    def _data_value(self):
        return bytearray(int(i, 16) for i in re.findall("[0-9a-zA-Z]{2}", self._value()))

    def _short_string_value(self):
        return _parse_short_string(self._value())

    def _long_string_value(self):
        s = self._value()
        if len(s) <= 1:
            return ""
        elif s[1] in " \t":
            return s[2:]
        else:
            return s[1:]

    def _flush_data(self):
        value = bytearray().join(self.acc)
        self.acc[:] = ()
        return value

    def _flush_long_string(self):
        value = "".join(self.acc)
        self.acc[:] = ()
        return value

    _ACC = {
        Acc.DATA: lambda self: self._data_value(),
        Acc.STRING: lambda self: self._long_string_value(),
        Acc.NL: lambda self: "\n",
        Acc.SP: lambda self: " ",
    }

    _EMIT = {
        Emit.NULL:
        lambda self: Event(EventType.NULL, self.key, "short", None),
        Emit.TRUE:
        lambda self: Event(EventType.BOOL, self.key, "short", True),
        Emit.FALSE:
        lambda self: Event(EventType.BOOL, self.key, "short", False),
        Emit.INF:
        lambda self: Event(EventType.FLOAT, self.key, "short", inf),
        Emit.NEG_INF:
        lambda self: Event(EventType.FLOAT, self.key, "short", -inf),
        Emit.NAN:
        lambda self: Event(EventType.FLOAT, self.key, "short", nan),
        Emit.INT:
        lambda self: Event(EventType.INT, self.key, "short", self._int_value()),
        Emit.FLOAT:
        lambda self: Event(EventType.FLOAT, self.key, "short", self._float_value()),
        Emit.DATA:
        lambda self: Event(EventType.DATA, self.key, "short", self._data_value()),
        Emit.ACC_DATA:
        lambda self: Event(EventType.DATA, self.key, "long", self._flush_data()),
        Emit.STRING:
        lambda self: Event(EventType.STRING, self.key, "short", self._short_string_value()),
        Emit.ACC_STRING:
        lambda self: Event(EventType.STRING, self.key, "long", self._flush_long_string()),
        Emit.SHORT_ARRAY_IN:
        lambda self: Event(EventType.ARRAY_IN, self.key, "short", None),
        Emit.SHORT_ARRAY_OUT:
        lambda self: Event(EventType.ARRAY_OUT, self.key, "short", None),
        Emit.LONG_ARRAY_IN:
        lambda self: Event(EventType.ARRAY_IN, self.key, "long", None),
        Emit.LONG_ARRAY_OUT:
        lambda self: Event(EventType.ARRAY_OUT, self.key, "long", None),
        Emit.SHORT_MAP_IN:
        lambda self: Event(EventType.MAP_IN, self.key, "short", None),
        Emit.SHORT_MAP_OUT:
        lambda self: Event(EventType.MAP_OUT, self.key, "short", None),
        Emit.LONG_MAP_IN:
        lambda self: Event(EventType.MAP_IN, self.key, "long", None),
        Emit.LONG_MAP_OUT:
        lambda self: Event(EventType.MAP_OUT, self.key, "long", None),
    }

    _KEY = {
        Key.QUOTED: lambda self: _parse_short_string(self._value()[:-1]),
        Key.UNQUOTED: lambda self: self._value()[:-1],
    }

    def next(self):
        self.event = None
        while True:
            self._lex_next()
            if not self.stack:
                return False
            state = self.stack.pop()
            error, emit, extend, acc, key = spec.PARSE_TABLE[state][self.l.token.type.value]

            if error is not None:
                if self.l.token.type.value <= Token.LINE_OUT.value:
                    lineno = max(1, self.l.token.lineno - 1)
                    column = max(1, self.l._prev_width)
                else:
                    lineno = self.l.token.lineno
                    column = self.l.token.range[0] + 1
                raise ProcyonDecodeError(error, lineno, column)

            for a in acc:
                self.acc.append(self._ACC[a](self))

            if emit is not None:
                self.event = self._EMIT[emit](self)
                self.key = None

            if key is not None:
                self.key = self._KEY[key](self)

            self.stack.extend(extend)
            if len(self.stack) > 64:
                lineno = self.l.token.lineno
                column = self.l.token.range[0] + 1
                raise ProcyonDecodeError(Error.RECURSION, lineno, column)
            if self.event is not None:
                return True

    def _lex_next(self):
        result = self.l.next()
        if self.l.token.type == Token.ERROR:
            raise ProcyonDecodeError(self.l.token.error, self.l.token.lineno,
                                     self.l.token.error_column)
        return result


def parse(lines):
    l = Lexer(lines)
    p = ProcyonParser(l)
    while p.next():
        yield p.event


def main(args=None):
    import sys
    if args is None:
        args = sys.argv

    event_strings = "NULL BOOL INT FLOAT DATA STRING [ ] { }".split()

    if len(args) != 1:
        sys.stderr.write("usage: python -m procyon.parse\n")
        return 64
    depth = 0

    indent = 0
    try:
        for event in parse(sys.stdin):
            if event.type in [EventType.ARRAY_OUT, EventType.MAP_OUT]:
                indent -= 1
            sys.stdout.write("\t" * indent)
            if event.type in [EventType.ARRAY_IN, EventType.MAP_IN]:
                indent += 1

            if event.key is not None:
                sys.stdout.write("KEY(%s) " % dumps(event.key, ShortStyle))

            if event.value is None:
                sys.stdout.write("%s\n" % (event_strings[event.type.value]))
            else:
                sys.stdout.write("%s(%s)\n" % (event_strings[event.type.value],
                                               dumps(event.value, ShortStyle)))
    except Exception as e:
        sys.stderr.write("%s: %s\n" % (args[0], e))
        return 1


__all__ = ["ProcyonParser", "parse"]

if __name__ == "__main__":
    import sys
    sys.exit(main())
