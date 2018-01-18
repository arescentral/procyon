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
import os
import re
from . import error
from . import spec
from . import utf
from .token import Token, FLAG_DONE, FLAG_OK, FLAG_VALUE

HT = 9  # "\t"
NL = 10  # "\n"
SP = 32  # " "

with open(os.path.join(os.path.dirname(__file__), "lex.bin"), "rb") as f:
    _transitions = bytearray(f.read())


class Lexer(object):

    Token = collections.namedtuple("Token", "type error error_column lineno line range".split())

    def __init__(self, lines):
        self.token = None

        self._lines = iter(lines)
        self._lineno = 1
        self._depth = 0
        self._started = False

        self._token_type = Token.LINE_IN
        self._token_begin = 0
        self._token_end = 0

        self._line_begin = self._line_end = 0
        self._buffer = bytearray()
        self._prev_width = 0

        self._indent = -1
        self._levels = [-1]

    def next(self):
        self._error = None
        self._error_column = None
        if self._started:
            if len(self._levels) == 1:
                return False
        else:
            self._started = True

        if self._line_end == self._line_begin:
            if self._next_line():
                return self._make_token()
        elif self._update_lexer_level():
            return self._make_token()

        while self._token_end < self._line_end:
            end = self._buffer[self._token_end]
            if (end == HT) or (end == SP):
                self._token_end += 1
            else:
                break
        if self._buffer[self._token_end] == NL:
            if not self._next_line():
                self._fail(error.Error.INTERNAL, self._token_end)
            return self._make_token()

        self._token_begin = self._token_end
        state = 0
        while self._token_end < self._line_end:
            byte = self._buffer[self._token_end]
            byte_class = spec.LEX_BYTE_CLASSES[byte]
            state = spec.LEX_TRANSITIONS[state][byte_class]
            if state & FLAG_DONE:
                break
            self._token_end += 1

        if state & FLAG_OK:
            self._token_type = Token(state & FLAG_VALUE)
            if self._token_type == Token.STAR:
                self._reindent()
                self._token_end = self._token_begin + 1
        else:  # state & FLAG_ERROR
            err = error.Error(state & FLAG_VALUE)
            at = self._token_end
            if err == error.Error.PARTIAL:
                at -= 1
            elif err == error.Error.BADWORD:
                at = self._token_begin
            elif err in (error.Error.BADESC, error.Error.BADUESC):
                while self._buffer[at] != ord("\\"):
                    at -= 1
            self._fail(err, at)

        return self._make_token()

    def _make_token(self):
        self.token = Lexer.Token(self._token_type, self._error, self._error_column, self._lineno,
                                 self._buffer, (self._token_begin, self._token_end))
        return True

    def _next_line(self):
        while True:
            if self._buffer:
                self._lineno += 1
            self._prev_width = len(self._buffer)
            try:
                self._buffer = next(self._lines)
            except StopIteration:
                self._buffer = b""
                self._token_begin = self._token_end = self._line_begin = self._line_end = 0
                self._indent = 0
                if not self._update_lexer_level():
                    self._levels.pop()
                    self._token_type = Token.LINE_OUT
                return True

            try:
                self._buffer = bytearray(self._buffer)
            except TypeError:
                self._buffer = bytearray(self._buffer, "utf-8")

            self._token_begin = self._token_end = self._line_begin = self._line_end = 0
            if not self._buffer.endswith(b"\n"):
                self._buffer += b"\n"
            self._line_end = self._line_begin + len(self._buffer)

            self._indent = 0
            if self._reindent():
                return self._update_lexer_level()

    def _update_lexer_level(self):
        if self._indent > self._levels[-1]:
            self._eq = False
            if self._token_type == Token.LINE_OUT:
                self._indent = self._levels[-1]
                return self._fail(error.Error.OUTDENT, self._token_end)
            self._levels.append(self._indent)
            self._token_type = Token.LINE_IN
            return True

        if self._indent < self._levels[-1]:
            self._levels.pop()
            self._token_type = Token.LINE_OUT
            return True

        if self._eq:
            self._eq = False
            self._token_type = Token.LINE_EQ
            return True

        return False

    def _reindent(self):
        indent = self._indent + self._token_end - self._token_begin
        i = self._token_end
        while i < self._line_end:
            ch = self._buffer[i]
            if ch == SP:
                indent += 1
            elif ch == HT:
                indent = (indent + 2) & ~1
            elif ch == NL:
                return False
            else:
                self._indent = indent
                self._eq = True
                self._token_end = i
                return True
            i += 1

    def _fail(self, code, at):
        self._token_type = Token.ERROR
        self._error = code
        self._error_column = at + 1
        self._token_end = self._line_end - 1
        return True


def lex(lines):
    l = Lexer(lines)
    while l.next():
        yield l.token


def main(args=None):
    import sys
    from .dump import dumps, ShortStyle

    if args is None:
        args = sys.argv

    token_strings = """
        LINE+ LINE= LINE-
        * [ ] { } , > | ! NULL TRUE FALSE INF -INF NAN
        KEY QKEY INT FLOAT DATA STR STR> STR| COMMENT
        ERROR
    """.split()

    if len(args) != 1:
        sys.stderr.write("usage: python -m procyon.lex\n")
        sys.exit(64)
    depth = 0

    for token in lex(sys.stdin):
        column = token.range[0] + 1
        type_str = token_strings[token.type.value]
        value = token.line[token.range[0]:token.range[1]]
        try:
            value = dumps(utf.decode(value), ShortStyle)
        except (UnicodeDecodeError, ValueError):
            value = dumps(value, ShortStyle)
        if token.type.value < Token.STAR.value:
            print("{0}:{1}\t{2}".format(token.lineno, column, type_str))
        elif token.type.value < Token.ERROR.value:
            print("{0}:{1}\t{2}\t{3}".format(token.lineno, column, type_str, value))
        else:
            print("{0}:{1}\tERROR\t{2}:{3}:{4}\t{5}".format(
                token.lineno, column, token.lineno, token.error_column, error.error_message[
                    token.error], value))


__all__ = ["Lexer", "lex"]

if __name__ == "__main__":
    main()
