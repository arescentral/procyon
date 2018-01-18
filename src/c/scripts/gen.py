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

import argparse
import collections
import os
import re
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))
import procyon
from procyon.error import Error
from procyon.token import Token

SPEC_PATH = os.path.join(os.path.dirname(__file__), "..", "src", "spec.pn")

unicode = type("")


def main():
    parser = argparse.ArgumentParser(description="build Procyon lex table")
    parser.add_argument("--c", default=None, type=argparse.FileType("w"))
    parser.add_argument("--python", default=None, type=argparse.FileType("w"))
    args = parser.parse_args()

    if not any([args.c, args.python]):
        sys.stderr.write("%s: no outputs specified\n" % os.path.basename(sys.argv[0]))
        sys.exit(64)

    with open(SPEC_PATH) as f:
        spec = procyon.load(f)

    lex = Lexer(spec)
    parser = Parser(spec)

    if args.c:
        for line in output_c(lex, parser):
            args.c.write((line or "") + "\n")
    if args.python:
        for line in output_python(lex, parser):
            args.python.write((line or "") + "\n")


def output_c(lex, parser):
    yield "#include \"gen_table.h\""

    yield
    yield "const uint8_t lex_classes[256] = {"
    for i in range(0, 256, 16):
        line = "   "
        for j in range(i, i + 16):
            line += " 0x%02x," % lex.partitions[j]
        yield line
    yield "};"

    yield
    yield "const uint8_t lex_transitions[] = {"
    for i, table in enumerate(lex.tables):
        yield "    // 0x%02x" % i
        for kind, value in table:
            if kind == "next":
                yield "    %s," % value
            elif kind == "ok":
                yield "    PN_TOK_FLAG_DONE | PN_TOK_FLAG_OK | PN_TOK_%s," % value
            elif kind == "error":
                yield "    PN_TOK_FLAG_DONE | PN_ERROR_%s," % value
    yield "};"

    yield
    yield "const uint8_t* const lex_table[] = {"
    index = 0
    for table in lex.tables:
        yield "    lex_transitions + %d," % index
        index += len(table)
    yield "};"

    yield
    yield "pn_parser_transition_t parse_defs[] = {"
    for _, parse_def in parser.defs.values():
        fields = []
        if "error" in parse_def:
            fields.append(".error = PN_ERROR_%s" % parse_def["error"])

        if "emit" in parse_def:
            fields.append(".emit = PN_PRS_EMIT_%s" % parse_def["emit"])

        if "extend" in parse_def:
            extend = parse_def["extend"]
            fields.append(".extend_count = %d" % len(extend))
            for i, extend in enumerate(extend):
                fields.append(".extend%d = %d" % (i, extend))

        if "acc" in parse_def:
            for i, acc in enumerate(parse_def["acc"]):
                fields.append(".acc%d = PN_PRS_ACC_%s" % (i, acc))

        if "key" in parse_def:
            fields.append(".key = PN_PRS_KEY_%s" % parse_def["key"])

        yield "    {%s}," % ", ".join(fields)

    yield "};"
    yield

    yield "uint8_t parse_table[][%s] = {" % max(len(t) for t in parser.tables)
    for table in parser.tables:
        yield "    {" + ", ".join(str(x) for x in table) + "},"
    yield "};"


def output_python(lex, parser):
    yield "#!/usr/bin/env python3"
    yield
    yield "from .error import Error"
    yield "from .parse_enums import Acc, Emit, Key"
    yield

    lines = []
    for i in range(0, 256, 16):
        line = ""
        for j in range(i, i + 16):
            line += "\\%03o" % lex.partitions[j]
        lines.append(line)
    yield "LEX_BYTE_CLASSES = bytearray(b\"%s\")" % (
        "\"\n                             b\"".join(lines))

    yield
    yield "LEX_TRANSITIONS = ["
    for table in lex.tables:
        lines = []
        for j in range(0, len(table), 20):
            lines.append("".join(lex_transition_value(*x) for x in table[j:j + 20]))
        yield "    bytearray(b\"%s\")," % "\"\n              b\"".join(lines)
    yield "]"

    yield
    yield "PARSE_DEFS = ["
    for _, parse_def in parser.defs.values():
        error, emit, extend, acc, key = None, None, [], [], None
        if "error" in parse_def:
            error = "Error.%s" % parse_def["error"]
        if "emit" in parse_def:
            emit = "Emit.%s" % parse_def["emit"]
        if "extend" in parse_def:
            extend = parse_def["extend"]
        if "acc" in parse_def:
            acc = "[%s]" % ", ".join("Acc.%s" % a for a in parse_def["acc"])
        if "key" in parse_def:
            key = "Key.%s" % parse_def["key"]
        yield "    (%s, %s, %s, %s, %s)," % (error, emit, extend, acc, key)

    yield "]"
    yield

    yield "PARSE_TABLE = ["
    for table in parser.tables:
        lines = []
        for i in range(0, len(table), 5):
            entries = []
            for entry in table[i:i + 5]:
                entries.append("PARSE_DEFS[%d]" % entry)
            lines.append(", ".join(entries))
        yield "    [%s]," % ",\n     ".join(lines)
    yield "]"


def lex_transition_value(kind, value):
    if kind == "next":
        return "\\%03o" % value
    elif kind == "error":
        return "\\2%02o" % getattr(Error, value).value
    elif kind == "ok":
        return "\\3%02o" % getattr(Token, value).value
    raise AssertionError((kind, value))


class Lexer:
    def __init__(self, spec):
        lex = collections.OrderedDict()
        for s, t in spec["lex"].items():
            lex[s] = t
            if "utf8" in t:
                Lexer.add_utf8_transitions(lex, t["utf8"])

        states = collections.OrderedDict((state, i) for i, state in enumerate(lex))
        self.tables = [Lexer.build_lex_table(lex[state], spec, states) for state in states]
        self.partitions = Lexer.partition_lex(self.tables)

    @staticmethod
    def add_utf8_transitions(data, target):
        target = unicode(target)
        data["ua", target] = {"": "UTF8_TAIL", "\240-\277": ("u1", target)}
        data["ub", target] = {"": "UTF8_TAIL", "\200-\237": ("u1", target)}
        data["uc", target] = {"": "UTF8_TAIL", "\220-\277": ("u2", target)}
        data["ud", target] = {"": "UTF8_TAIL", "\200-\217": ("u2", target)}
        data["u3", target] = {"": "UTF8_TAIL", "\200-\277": ("u2", target)}
        data["u2", target] = {"": "UTF8_TAIL", "\200-\277": ("u1", target)}
        data["u1", target] = {"": "UTF8_TAIL", "\200-\277": target}

    @staticmethod
    def build_lex_table(transitions, spec, states):
        if not isinstance(transitions, dict):
            return [Lexer.lex_table_entry(transitions, spec, states)] * 256

        out = [None] * 256
        for i in range(0o040):
            out[i] = Lexer.lex_table_entry("CTRL", spec, states)
        out[0o177] = Lexer.lex_table_entry("CTRL", spec, states)
        for i in range(0o200, 0o400):
            out[i] = Lexer.lex_table_entry("NONASCII", spec, states)

        for char_set, target in transitions.items():
            if char_set == "utf8":
                for i in range(0o200, 0o400):
                    out[i] = Lexer.lex_table_entry("UTF8_HEAD", spec, states)
                target = unicode(target)
                for i in range(0o302, 0o340):
                    out[i] = ("next", states["u1", target])
                for i in range(0o340, 0o360):
                    out[i] = ("next", states["u2", target])
                for i in range(0o360, 0o365):
                    out[i] = ("next", states["u3", target])
                out[0o340] = ("next", states["ua", target])
                out[0o355] = ("next", states["ub", target])
                out[0o360] = ("next", states["uc", target])
                out[0o364] = ("next", states["ud", target])
            else:
                for x in Lexer.list_chars(char_set):
                    out[ord(x)] = Lexer.lex_table_entry(target, spec, states)

        return out

    @staticmethod
    def partition_lex(table):
        outs = collections.defaultdict(list)

        for i in range(256):
            outs[tuple(row[i] for row in table)].append(i)
        outs = list(enumerate(sorted(outs.values())))

        char_classes = collections.OrderedDict()
        for i, values in outs:
            for v in values:
                char_classes[v] = i

        new_table = []
        for row in table:
            new_table.append([row[values[0]] for _, values in outs])

        table[:] = new_table
        return char_classes

    @staticmethod
    def lex_table_entry(s, spec, states):
        if s in spec["tokens"]:
            return ("ok", s)
        elif s in spec["errors"]:
            return ("error", s)
        elif s in states:
            return ("next", states[s])
        elif unicode(s) in states:
            return ("next", states[unicode(s)])
        raise AssertionError(s)

    @staticmethod
    def list_chars(char_set):
        if not char_set:
            return [chr(i) for i in range(256)]
        if len(char_set) == 1:
            return [char_set]
        r = re.compile("[%s]" % char_set.replace("\\", "\\\\"))
        return [chr(i) for i in range(256) if r.match(chr(i))]


class Parser:
    def __init__(self, spec):
        parse_states = collections.OrderedDict((state, i) for i, state in enumerate(spec["parse"]))
        self.defs = collections.OrderedDict()
        for state, index in parse_states.items():
            for token, token_table in spec["parse"][state].items():
                if "extend" in token_table:
                    token_table["extend"] = list(map(parse_states.get, token_table["extend"]))
                frozen_token_table = freeze(token_table)
                if frozen_token_table not in self.defs:
                    self.defs[frozen_token_table] = (len(self.defs), token_table)

        self.tables = []
        for state in parse_states:
            parse_table = []
            for token in spec["tokens"]:
                token_table = spec["parse"][state].get(token, spec["parse"][state][""])
                index, _ = self.defs[freeze(token_table)]
                parse_table.append(index)
            self.tables.append(parse_table)


def freeze(x):
    if isinstance(x, (type(None), int, float, bytes, unicode)):
        return x
    elif isinstance(x, (list, tuple)):
        return tuple(map(freeze, x))
    elif isinstance(x, (set, frozenset)):
        return frozenset(map(freeze, x))
    elif isinstance(x, dict):
        return frozenset((k, freeze(v)) for k, v in x.items())
    raise TypeError(type(x).__name__)


if __name__ == "__main__":
    main()
