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
import os
import requests

UNICODE_DATA_FILE = "/tmp/UnicodeData.txt"
UNICODE_DATA_URL = "http://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt"
EAST_ASIAN_WIDTH_FILE = "/tmp/EastAsianWidth.txt"
EAST_ASIAN_WIDTH_URL = "http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt"


def main():
    unidata = cache_unidata()
    eawdata = cache_east_asian_widths()
    widths = [-1 for i in xrange(0x110000)]

    # First, set width to 2 for anything with East Asian Width "W" (wide) or "F" (full-width).
    for line in eawdata.splitlines():
        if "#" in line:
            line, _ = line.split("#", 1)
        line = line.strip()
        if not line:
            continue

        cprange, eaw_class = line.split(";")
        if eaw_class not in "WF":
            continue
        if ".." in cprange:
            cprange = tuple(int(cp, 16) for cp in cprange.split(".."))
        else:
            cp = int(cprange, 16)
            cprange = (cp, cp)

        for cp in xrange(cprange[0], cprange[1] + 1):
            widths[cp] = 2

    # Second, set width to 0 for all modifiers, and clear width for control/space characters.
    # Overrides the above, e.g. in U+3099 COMBINING KATAKANA-HIRAGANA VOICED SOUND MARKER, which is
    # a wide modifier and has width 0 like any other modifier.
    for line in unidata.splitlines():
        cp, _, class_, _ = line.split(";", 3)
        cp = int(cp, 16)

        if class_.startswith("M"):
            widths[cp] = 0
        elif class_.startswith("C") or class_.startswith("Z"):
            widths[cp] = -1
        else:
            if widths[cp] == -1:
                widths[cp] = 1

    widths[ord(" ")] = 1
    widths[ord("　")] = 2

    size = print_level("widths", widths)
    print("// %d bytes" % size)


def print_level(name, values, level=0):
    block_size = 8
    if max(values) < 256:
        ctype = "uint8_t"
        csize = 1
    elif max(values) < 65536:
        ctype = "uint16_t"
        csize = 2
    else:
        ctype = "uint32_t"
        csize = 4

    if len(values) > (block_size * 1.5):
        blocks = set()
        next_values = []
        for i in xrange(0, len(values), block_size):
            block = tuple(values[i:i + block_size])
            while len(block) < block_size:
                block = block + (0, )
            if block not in blocks:
                blocks.add(block)
            next_values.append(block)
        blocks = {block: index for index, block in enumerate(sorted(blocks))}

        print("static const %s %s_%d[][%d] = {" % (ctype, name, level, block_size))
        for block, index in sorted(blocks.iteritems()):
            print("        [%d] = {%s}," % (index, ", ".join(map(str, block))))
        print("};")
        return (csize * block_size * len(blocks)) + print_level(
            name, map(blocks.get, next_values), level + 1)
    else:
        print("static const %s %s_%d[] = {%s};" % (ctype, name, level,
                                                   ", ".join(map(str, values))))
        return csize * len(values)


def cache_unidata():
    if os.path.isfile(UNICODE_DATA_FILE):
        with open(UNICODE_DATA_FILE) as f:
            data = f.read().decode("utf-8")
    else:
        data = requests.get(UNICODE_DATA_URL).text
        with open(UNICODE_DATA_FILE, "w") as f:
            f.write(data.encode("utf-8"))
    return data


def cache_east_asian_widths():
    if os.path.isfile(EAST_ASIAN_WIDTH_FILE):
        with open(EAST_ASIAN_WIDTH_FILE) as f:
            data = f.read().decode("utf-8")
    else:
        data = requests.get(EAST_ASIAN_WIDTH_URL).text
        with open(EAST_ASIAN_WIDTH_FILE, "w") as f:
            f.write(data.encode("utf-8"))
    return data


if __name__ == "__main__":
    main()
