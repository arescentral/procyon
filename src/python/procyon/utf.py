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

import sys
from . import py3

if sys.maxunicode == 0x10FFFF:

    try:
        unichr = unichr
    except NameError:
        unichr = chr

    def code_points(s):
        return map(ord, s)

else:
    assert sys.maxunicode == 0xFFFF

    import struct

    def unichr(ch):
        return struct.pack(">L", ch).decode("utf-32be")

    def code_points(s):
        assert isinstance(s, py3.unicode)
        utf32 = s.encode("utf-32be")
        for i in py3.xrange(0, len(utf32), 4):
            yield struct.unpack(">L", utf32[i:i + 4])[0]


if len(b"\355\240\200".decode("utf-8", errors="replace")) == 1:
    import re
    surrogate_re = re.compile(br"\355[\240-\277]")

    def decode(b):
        b = bytes(b)
        m = surrogate_re.search(b)
        if m:
            start = m.pos + 1
            end = m.pos + 2
            raise UnicodeDecodeError(str("utf-8"), str("invalid continuation byte"), start, end, b)
        return b.decode("utf-8")

else:

    def decode(b):
        return b.decode("utf-8")


def has_surrogates(s):
    assert isinstance(s, py3.unicode)
    return any(0xD800 <= cp < 0xE000 for cp in code_points(s))
