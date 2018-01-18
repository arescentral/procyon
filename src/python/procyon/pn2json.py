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

import json
import math
from . import py3
from .parse import EventType, parse


def _is_scalar(x):
    return (x is None) or isinstance(x, (bool, int, py3.long, float))


def _jsonify_scalar(x):
    if isinstance(x, float):
        if math.isnan(x):
            return "null"
        elif math.isinf(x):
            return "1e999" if (x > 0) else "-1e999"
    return json.dumps(x)


def _jsonify_data(d):
    return '"' + "".join("%02x" % byte for byte in bytearray(d)) + '"'


def _jsonify_string(s):
    return json.dumps(s, ensure_ascii=False)


_SEQ_IN = (EventType.ARRAY_IN, EventType.MAP_IN)
_SEQ_OUT = (EventType.ARRAY_OUT, EventType.MAP_OUT)
_JSONIFY = {
    EventType.NULL: _jsonify_scalar,
    EventType.BOOL: _jsonify_scalar,
    EventType.INT: _jsonify_scalar,
    EventType.FLOAT: _jsonify_scalar,
    EventType.DATA: _jsonify_data,
    EventType.STRING: _jsonify_string,
    EventType.ARRAY_IN: lambda x: "[",
    EventType.ARRAY_OUT: lambda x: "]",
    EventType.MAP_IN: lambda x: "{",
    EventType.MAP_OUT: lambda x: "}",
}


def to_json(f):
    long_depth = 0
    short_depth = 0
    is_first_item = True
    is_first_event = True
    out = []

    for event in parse(f):
        is_short = (event.flags == "short")
        inside_short_sequence = (short_depth > 0)

        if event.type in _SEQ_OUT:
            if not inside_short_sequence:
                out.extend(("\n", "\t" * (long_depth - 1)))
        elif not is_first_item:
            if inside_short_sequence:
                out.append(", ")
            else:
                out.extend((",\n", "\t" * long_depth))
        elif not (inside_short_sequence or is_first_event):
            out.extend(("\n", "\t" * long_depth))

        if event.key is not None:
            out.extend((_jsonify_string(event.key), ": "))

        out.append(_JSONIFY[event.type](event.value))

        is_first_event = False
        is_first_item = (event.type in _SEQ_IN)
        if event.type in _SEQ_IN:
            if is_short:
                short_depth += 1
            else:
                long_depth += 1
        elif event.type in _SEQ_OUT:
            if is_short:
                short_depth -= 1
            else:
                long_depth -= 1
    out.append("\n")
    return "".join(out)


if __name__ == "__main__":
    import os
    import sys
    from . import ProcyonDecodeError, load

    progname = os.path.basename(sys.argv.pop(0))
    infile, outfile = sys.stdin, sys.stdout
    if len(sys.argv) == 1:
        infile = open(sys.argv[0], "rb")
    elif len(sys.argv) == 2:
        infile = open(sys.argv[0], "rb")
        outfile = open(sys.argv[1], "wb")
    elif len(sys.argv) > 2:
        raise SystemExit("usage: %s [INPUT.json [OUTPUT.pn]]" % progname)

    try:
        outfile.write(to_json(infile))
    except Exception as e:
        sys.stderr.write("%s: %s\n" % (progname, e))
        sys.exit(1)
