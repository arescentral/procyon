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

import math
import re
import unicodedata
from . import error
from . import py3
from . import utf
from .types import Type, typeof

_NO_QUOTE_RE = re.compile("^[A-Za-z0-9._/+-]*$")
_UNPRINTABLE_RE = re.compile("[\\000-\\011\\013-\\037\\177]")


class DefaultStyle(object):
    always_short = False
    separators = (": ", ", ")


class ShortStyle(object):
    always_short = True
    separators = (": ", ", ")


class MinifiedStyle(object):
    always_short = True
    separators = (":", ",")


class ProcyonEncoder(object):
    def __init__(self, style, converter):
        self.always_short = style.always_short
        self.colon, self.comma = style.separators
        self.converter = _make_converter(converter)

    def iterencode(self, obj):
        markers = set()
        if self.converter is not None:
            obj = self.converter(obj)
        if self._should_dump_short_value(obj):
            result = self._dump_short_value(obj, markers)
        else:
            result = self._dump_long_value(obj, markers, "")
        for chunk in result:
            yield chunk
        if not self.always_short:
            yield "\n"

    def encode(self, obj):
        return "".join(self.iterencode(obj))

    def _should_dump_short_value(self, x):
        if self.always_short:
            return True
        return ProcyonEncoder._SHOULD_DUMP_SHORT_VALUE[typeof(x).value](x)

    def _dump_short_value(self, x, markers):
        return self._DUMP_SHORT_VALUE[typeof(x).value](self, x, markers)

    def _dump_long_value(self, x, markers, indent):
        return self._DUMP_LONG_VALUE[typeof(x).value](self, x, markers, indent)

    def _dump_int(self, i):
        if not (-0x8000000000000000 <= i < 0x8000000000000000):
            raise OverflowError("Python int too large to convert to Procyon int")
        return py3.unicode(i)

    def _dump_float(self, f):
        if math.isnan(f):
            return "nan"
        elif math.isinf(f):
            if f > 0:
                return "inf"
            else:
                return "-inf"
        return repr(f)

    @staticmethod
    def _should_dump_short_data(d):
        return len(d) <= 4

    def _dump_short_data(self, d):
        yield "$"
        for byte in bytearray(d):
            yield "%02x" % byte

    def _dump_long_data(self, d, indent):
        for i, byte in enumerate(bytearray(d)):
            if i == 0:
                yield "$\t"
            elif (i % 32) == 0:
                yield "\n%s$\t" % indent
            elif (i % 4) == 0:
                yield " "
            yield "%02x" % byte

    @staticmethod
    def _should_dump_short_string(s):
        has_nl = False
        if _UNPRINTABLE_RE.search(s):
            return True
        elif "\n" in s:
            return False
        return len(s) < 72

    def _dump_short_string(self, s):
        yield '"'
        for cp in utf.code_points(s):
            c = utf.unichr(cp)
            if c in "\b\f\n\r\t\"\\":
                yield {
                    "\b": "\\b",
                    "\f": "\\f",
                    "\n": "\\n",
                    "\r": "\\r",
                    "\t": "\\t",
                    "\"": "\\\"",
                    "\\": "\\\\",
                }[c]
                continue
            category = unicodedata.category(c)
            if category == "Cs":
                raise ValueError("string %r contains surrogate code points" % s)
            elif category in ("Cc", "Cf", "Co", "Cn"):
                if cp < 0x10000:
                    yield "\\u%04x" % cp
                else:
                    yield "\\U%08x" % cp
            else:
                yield c
        yield '"'

    @staticmethod
    def _wrap_lines(s):
        while True:
            if len(s) <= 72:
                yield s
                return
            head, tail = s[:73], s[73:]
            if " " in head:
                line, line_tail = head.rsplit(" ", 1)
                if line_tail or tail:
                    yield line
                    s = line_tail + tail
                else:
                    yield s
                    return
            elif " " in tail:
                line_tail, tail = tail.split(" ", 1)
                yield head + line_tail
                s = tail
            else:
                yield s
                return

    def _dump_long_string(self, s, indent):
        paragraphs = s.split("\n")
        trailing_newline = not paragraphs[-1]
        if trailing_newline:
            paragraphs.pop(-1)

        prefix = ""
        can_use_gt = True
        for paragraph in paragraphs:
            if prefix:
                yield prefix
            else:
                prefix = "\n" + indent

            if can_use_gt or not paragraph:
                yield ">"
            else:
                yield "|"
            if not paragraph:
                can_use_gt = True
                continue
            can_use_gt = False

            yield "\t"
            line_prefix = ""
            for line in ProcyonEncoder._wrap_lines(paragraph):
                if line_prefix:
                    yield line_prefix
                else:
                    line_prefix = "\n%s>\t" % indent
                yield line

        if not trailing_newline:
            yield prefix + "!"

    @staticmethod
    def _should_dump_short_array(a):
        for x in a:
            if typeof(x).value <= Type.FLOAT.value:
                continue
            return False
        return True

    def _dump_short_array(self, a, markers):
        if markers is not None:
            id_a = id(a)
            if id_a in markers:
                raise ValueError("Circular reference detected")
            markers.add(id_a)
        yield "["
        separator = ""
        for x in a:
            if separator:
                yield separator
            else:
                separator = self.comma
            for chunk in self._dump_short_value(x, markers):
                yield chunk
        yield "]"
        if markers is not None:
            markers.remove(id_a)

    def _dump_long_array(self, a, markers, indent):
        if markers is not None:
            id_a = id(a)
            if id_a in markers:
                raise ValueError("Circular reference detected")
            markers.add(id_a)
        prefix = "*\t"
        tail_prefix = "\n%s*\t" % indent
        indent += "\t"
        for i, x in enumerate(a):
            yield prefix
            prefix = tail_prefix
            if self.converter is not None:
                x = self.converter(x)
            if self._should_dump_short_value(x):
                for chunk in self._dump_short_value(x, markers):
                    yield chunk
            else:
                for chunk in self._dump_long_value(x, markers, indent):
                    yield chunk
        if markers is not None:
            markers.remove(id_a)

    def _dump_key(self, k):
        if _NO_QUOTE_RE.match(k):
            return k
        return self._dump_short_string(k)

    @staticmethod
    def _should_dump_short_map(m):
        for x in py3.itervalues(m):
            if typeof(x).value <= Type.FLOAT.value:
                continue
            return False
        return True

    def _dump_short_map(self, m, markers):
        if markers is not None:
            id_m = id(m)
            if id_m in markers:
                raise ValueError("Circular reference detected")
            markers.add(id_m)
        yield "{"
        separator = ""
        for k, v in py3.iteritems(m):
            if not isinstance(k, py3.unicode):
                raise TypeError("key %r is not a string" % k)

            if separator:
                yield separator
            else:
                separator = self.comma
            yield self._dump_key(k)
            yield self.colon
            for chunk in self._dump_short_value(v, markers):
                yield chunk
        yield "}"
        if markers is not None:
            markers.remove(id_m)

    def _dump_long_map(self, m, markers, indent):
        if markers is not None:
            id_m = id(m)
            if id_m in markers:
                raise ValueError("Circular reference detected")
            markers.add(id_m)
        prefix = ""
        tail_prefix = "\n%s" % indent
        indent += "\t"

        adjusted = []
        max_short_key_width = 0
        for k, v in py3.iteritems(m):
            if not isinstance(k, py3.unicode):
                raise TypeError("key %r is not a string" % k)
            k = self._dump_key(k)
            if self.converter is not None:
                v = self.converter(v)
            short = self._should_dump_short_value(v)
            if short and (len(k) > max_short_key_width):
                max_short_key_width = len(k)
            adjusted.append((k, v, short))

        for k, v, short in adjusted:
            yield prefix
            prefix = tail_prefix
            if short:
                yield (k + ":").ljust(max_short_key_width + 3)
                for chunk in self._dump_short_value(v, markers):
                    yield chunk
            else:
                yield k
                if k:
                    yield ":\n" + indent
                else:
                    yield ":\t"
                for chunk in self._dump_long_value(v, markers, indent):
                    yield chunk
        if markers is not None:
            markers.remove(id_m)

    _SHOULD_DUMP_SHORT_VALUE = [
        lambda x: True,
        lambda x: True,
        lambda x: True,
        lambda x: True,
        lambda x: ProcyonEncoder._should_dump_short_data(x),
        lambda x: ProcyonEncoder._should_dump_short_string(x),
        lambda x: ProcyonEncoder._should_dump_short_array(x),
        lambda x: ProcyonEncoder._should_dump_short_map(x),
    ]

    _DUMP_SHORT_VALUE = [
        lambda self, x, markers: ["null"],
        lambda self, x, markers: ["false", "true"][x],
        lambda self, x, markers: [self._dump_int(x)],
        lambda self, x, markers: [self._dump_float(x)],
        lambda self, x, markers: self._dump_short_data(x),
        lambda self, x, markers: self._dump_short_string(x),
        lambda self, x, markers: self._dump_short_array(x, markers),
        lambda self, x, markers: self._dump_short_map(x, markers),
    ]

    _DUMP_LONG_VALUE = [
        lambda self, x, markers, indent: ["null"],
        lambda self, x, markers, indent: ["false", "true"][x],
        lambda self, x, markers, indent: [self._dump_int(x)],
        lambda self, x, markers, indent: [self._dump_float(x)],
        lambda self, x, markers, indent: self._dump_long_data(x, indent),
        lambda self, x, markers, indent: self._dump_long_string(x, indent),
        lambda self, x, markers, indent: self._dump_long_array(x, markers, indent),
        lambda self, x, markers, indent: self._dump_long_map(x, markers, indent),
    ]


def _make_converter(converter):
    if converter is None:
        return None
    elif isinstance(converter, tuple):
        if not all(callable(x) for x in converter):
            raise TypeError("converter tuple elements must be callable")

        def convert(x):
            for convert_step in converter:
                x = convert_step(x)
            return x

        return convert
    elif callable(converter):
        return converter
    elif not isinstance(converter, dict):
        raise TypeError("converter must be callable, tuple, or dict")

    convert_none = converter.pop(None, None)
    convert_bool = converter.pop(bool, None)
    convert_int = converter.pop(int, None)
    convert_float = converter.pop(float, None)
    convert_data = converter.pop(bytes, None)
    convert_string = converter.pop(py3.unicode, None)
    convert_array = converter.pop(list, None)
    convert_map = converter.pop(dict, None)

    if not all(isinstance(k, (type, type(None))) for k in converter):
        raise TypeError("converter dict keys must be type or None")
    elif not all(isinstance(v, tuple) or callable(v) for v in py3.itervalues(converter)):
        raise TypeError("converter dict values must be tuple or callable")
    converter = {k: _make_converter(v) for k, v in py3.iteritems(converter)}

    def convert(x):
        if x is None:
            return convert_none(x) if convert_none is not None else x
        elif (x is True) or (x is False):
            return convert_bool(x) if convert_bool is not None else x
        elif isinstance(x, (int, py3.long)):
            return convert_int(x) if convert_int is not None else x
        elif isinstance(x, float):
            return convert_float(x) if convert_float is not None else x
        elif isinstance(x, (bytes, bytearray, memoryview)):
            return convert_data(x) if convert_data is not None else x
        elif isinstance(x, py3.unicode):
            return convert_string(x) if convert_string is not None else x
        elif isinstance(x, (list, tuple)):
            return convert_array(x) if convert_array is not None else x
        elif isinstance(x, dict):
            return convert_map(x) if convert_map is not None else x
        for t, convert_t in py3.iteritems(converter):
            if isinstance(x, t):
                return convert_t(x)
        raise TypeError("%r is not Procyon-serializable" % x)

    return convert


_DEFAULT_ARGS = (DefaultStyle, None)
_default_encoder = ProcyonEncoder(*_DEFAULT_ARGS)


def _get_encoder(style, converter):
    if (style, converter) == _DEFAULT_ARGS:
        return _default_encoder
    return ProcyonEncoder(style, converter)


def dump(obj, fp, style=DefaultStyle, converter=None):
    encoder = _get_encoder(style, converter)
    for chunk in encoder.iterencode(obj):
        fp.write(chunk)


def dumps(obj, style=DefaultStyle, converter=None):
    encoder = _get_encoder(style, converter)
    return encoder.encode(obj)


__all__ = ["dump", "dumps"]
