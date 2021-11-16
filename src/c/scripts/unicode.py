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

import collections
import os
import requests

UNICODE_DATA_FILE = "/tmp/UnicodeData.txt"
UNICODE_DATA_URL = "http://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt"
EAST_ASIAN_WIDTH_FILE = "/tmp/EastAsianWidth.txt"
EAST_ASIAN_WIDTH_URL = "http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt"

CATEGORY = {
    "Cc": 0o00,
    "Cf": 0o01,
    "Cn": 0o02,
    "Co": 0o03,
    "Cs": 0o04,
    "Ll": 0o10,
    "Lm": 0o11,
    "Lo": 0o12,
    "Lt": 0o13,
    "Lu": 0o14,
    "Mc": 0o20,
    "Me": 0o21,
    "Mn": 0o22,
    "Nd": 0o30,
    "Nl": 0o31,
    "No": 0o32,
    "Pc": 0o40,
    "Pd": 0o41,
    "Pe": 0o42,
    "Pf": 0o43,
    "Pi": 0o44,
    "Po": 0o45,
    "Ps": 0o46,
    "Sc": 0o50,
    "Sk": 0o51,
    "Sm": 0o52,
    "So": 0o53,
    "Zl": 0o60,
    "Zp": 0o61,
    "Zs": 0o62,
}

EAST_ASIAN_WIDTH = {
    "A": 0o000,
    "F": 0o100,
    "H": 0o000,
    "N": 0o000,
    "Na": 0o000,
    "W": 0o100,
}


def main():
    unidata = cache_unidata()
    eawdata = cache_east_asian_widths()
    default = CATEGORY["Cn"] | EAST_ASIAN_WIDTH["F"]
    data = [default for i in range(0x110000)]

    # First, clear double-width bit for anything East Asian Width A/H/N/Na.
    for line in eawdata.splitlines():
        if "#" in line:
            line, _ = line.split("#", 1)
        line = line.strip()
        if not line:
            continue

        cprange, eaw = line.split(";")
        if ".." in cprange:
            cprange = tuple(int(cp, 16) for cp in cprange.split(".."))
        else:
            cp = int(cprange, 16)
            cprange = (cp, cp)

        for cp in range(cprange[0], cprange[1] + 1):
            data[cp] = (data[cp] & 0o077) | EAST_ASIAN_WIDTH[eaw]

    # Second, set category bits.
    firsts = {}
    lasts = {}
    for line in unidata.splitlines():
        cp, name, category, _ = line.split(";", 3)
        cp = int(cp, 16)
        if name.endswith(", First>"):
            firsts[name[:-8]] = (cp, category)
        elif name.endswith(", Last>"):
            lasts[name[:-7]] = cp
        else:
            data[cp] = (data[cp] & 0o100) | CATEGORY[category]

    assert frozenset(firsts) == frozenset(lasts)
    for name, (first, category) in firsts.items():
        last = lasts[name]
        for cp in range(first, last + 1):
            data[cp] = (data[cp] & 0o100) | CATEGORY[category]

    size = print_level("data", data)
    print("// %d bytes" % size)


def print_level(name, values, level=0):
    block_size = 8
    if level == 0:
        ctype = "int8_t"
        csize = 1
    elif max(values) < 256:
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
        for i in range(0, len(values), block_size):
            block = tuple(values[i:i + block_size])
            while len(block) < block_size:
                block = block + (0, )
            if block not in blocks:
                blocks.add(block)
            next_values.append(block)
        blocks = {block: index for index, block in enumerate(sorted(blocks))}

        print("static const %s %s_%d[][%d] = {" % (ctype, name, level, block_size))
        for block, index in sorted(blocks.items()):
            print("        [%d] = {%s}," % (index, ", ".join(map(str, block))))
        print("};")
        return (csize * block_size * len(blocks)) + print_level(
            name, [blocks.get(x) for x in next_values], level + 1)
    else:
        print("static const %s %s_%d[] = {%s};" %
              (ctype, name, level, ", ".join(map(str, values))))
        return csize * len(values)


def cache_unidata():
    if os.path.isfile(UNICODE_DATA_FILE):
        with open(UNICODE_DATA_FILE) as f:
            data = f.read()
    else:
        data = requests.get(UNICODE_DATA_URL).text
        with open(UNICODE_DATA_FILE, "w") as f:
            f.write(data)
    return data


def cache_east_asian_widths():
    if os.path.isfile(EAST_ASIAN_WIDTH_FILE):
        with open(EAST_ASIAN_WIDTH_FILE) as f:
            data = f.read()
    else:
        data = requests.get(EAST_ASIAN_WIDTH_URL).text
        with open(EAST_ASIAN_WIDTH_FILE, "w") as f:
            f.write(data)
    return data


if __name__ == "__main__":
    main()
