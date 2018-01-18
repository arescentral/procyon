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

import os
import pygments
from pygments.token import Comment, Error, Keyword, Literal, Name, Number, Punctuation, String, Text
import sys
sys.path.insert(0, os.path.dirname(__file__))
from procyonlexer import ProcyonLexer


def test_null():
    assert list(pygments.lex("null", ProcyonLexer())) == [
        (Keyword.Constant, "null"),
        (Text, "\n"),
    ]


def test_keywords():
    assert list(
        pygments.lex("*\tnull\n"
                     "*\ttrue\n"
                     "*\tfalse\n"
                     "*\tinf\n"
                     "*\t+inf\n"
                     "*\t-inf\n"
                     "*\tnan\n"
                     "*\tfloop\n"
                     "*\tinferno\n", ProcyonLexer())) == [
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "null"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "true"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "false"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "inf"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "+inf"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "-inf"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Keyword.Constant, "nan"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Error, "floop"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Error, "inferno"),
                         (Text, "\n"),
                     ]

    assert list(
        pygments.lex("[null, true, false, inf, +inf, -inf, nan, floop, inferno]\n",
                     ProcyonLexer())) == [
                         (Punctuation, "["),
                         (Keyword.Constant, "null"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "true"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "false"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "inf"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "+inf"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "-inf"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Keyword.Constant, "nan"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Error, "floop"),
                         (Punctuation, ","),
                         (Text, " "),
                         (Error, "inferno"),
                         (Punctuation, "]"),
                         (Text, "\n"),
                     ]


def test_int():
    assert list(pygments.lex("*\t0\n"
                             "*\t+1\n"
                             "*\t-1\n", ProcyonLexer())) == [
                                 (Punctuation, "*"),
                                 (Text, "\t"),
                                 (Number.Integer, "0"),
                                 (Text, "\n"),
                                 (Punctuation, "*"),
                                 (Text, "\t"),
                                 (Number.Integer, "+1"),
                                 (Text, "\n"),
                                 (Punctuation, "*"),
                                 (Text, "\t"),
                                 (Number.Integer, "-1"),
                                 (Text, "\n"),
                             ]

    assert list(pygments.lex("[0, +1, -1]\n", ProcyonLexer())) == [
        (Punctuation, "["),
        (Number.Integer, "0"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Integer, "+1"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Integer, "-1"),
        (Punctuation, "]"),
        (Text, "\n"),
    ]


def test_float():
    assert list(
        pygments.lex("*\t0.0\n"
                     "*\t+1.0\n"
                     "*\t-1.0\n"
                     "*\t1e100\n"
                     "*\t1e-3\n"
                     "*\t1.5e6\n", ProcyonLexer())) == [
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "0.0"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "+1.0"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "-1.0"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "1e100"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "1e-3"),
                         (Text, "\n"),
                         (Punctuation, "*"),
                         (Text, "\t"),
                         (Number.Float, "1.5e6"),
                         (Text, "\n"),
                     ]

    assert list(pygments.lex("[0.0, +1.0, -1.0, 1e100, 1e-3, 1.5e6]\n", ProcyonLexer())) == [
        (Punctuation, "["),
        (Number.Float, "0.0"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "+1.0"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "-1.0"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "1e100"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "1e-3"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "1.5e6"),
        (Punctuation, "]"),
        (Text, "\n"),
    ]


def test_error():
    assert list(pygments.lex("&", ProcyonLexer())) == [
        (Error, "&"),
        (Text, "\n"),
    ]


def test_short_list():
    assert list(pygments.lex("[]", ProcyonLexer())) == [
        (Punctuation, "["),
        (Punctuation, "]"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[,]", ProcyonLexer())) == [
        (Punctuation, "["),
        (Error, ","),
        (Punctuation, "]"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[1,]", ProcyonLexer())) == [
        (Punctuation, "["),
        (Number.Integer, "1"),
        (Punctuation, ","),
        (Error, "]"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[1,2]", ProcyonLexer())) == [
        (Punctuation, "["),
        (Number.Integer, "1"),
        (Punctuation, ","),
        (Number.Integer, "2"),
        (Punctuation, "]"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[null, true, 2, 3.14]", ProcyonLexer())) == [
        (Punctuation, "["),
        (Keyword.Constant, "null"),
        (Punctuation, ","),
        (Text, " "),
        (Keyword.Constant, "true"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Integer, "2"),
        (Punctuation, ","),
        (Text, " "),
        (Number.Float, "3.14"),
        (Punctuation, "]"),
        (Text, "\n"),
    ]


def test_short_map():
    assert list(pygments.lex("{}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Punctuation, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{,}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Error, ","),
        (Punctuation, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{1,}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Error, "1"),
        (Error, ","),
        (Punctuation, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{1:}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Name.Tag, "1"),
        (Punctuation, ":"),
        (Error, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{1:2}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Name.Tag, "1"),
        (Punctuation, ":"),
        (Number.Integer, "2"),
        (Punctuation, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{1:2,}", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Name.Tag, "1"),
        (Punctuation, ":"),
        (Number.Integer, "2"),
        (Punctuation, ","),
        (Error, "}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex('{"":2}', ProcyonLexer())) == [
        (Punctuation, "{"),
        (Name.Tag, '""'),
        (Punctuation, ":"),
        (Number.Integer, "2"),
        (Punctuation, "}"),
        (Text, "\n"),
    ]


def test_long_map():
    assert list(pygments.lex("a:  \"1\"\n"
                             "b:  $02\n"
                             "c:\n"
                             "\t{}\n", ProcyonLexer())) == [
                                 (Name.Tag, "a"),
                                 (Punctuation, ":"),
                                 (Text, "  "),
                                 (String.Double, "\"1\""),
                                 (Text, "\n"),
                                 (Name.Tag, "b"),
                                 (Punctuation, ":"),
                                 (Text, "  "),
                                 (Number.Hex, "$02"),
                                 (Text, "\n"),
                                 (Name.Tag, "c"),
                                 (Punctuation, ":"),
                                 (Text, "\n"),
                                 (Text, "\t"),
                                 (Punctuation, "{"),
                                 (Punctuation, "}"),
                                 (Text, "\n"),
                             ]


def test_long_string():
    assert list(pygments.lex(">  a\n"
                             "> b\n"
                             ">c\n"
                             ">  \n"
                             "> \n"
                             ">\n", ProcyonLexer())) == [
                                 (Punctuation, ">"),
                                 (Text, " "),
                                 (String.Other, " a"),
                                 (Text, "\n"),
                                 (Punctuation, ">"),
                                 (Text, " "),
                                 (String.Other, "b"),
                                 (Text, "\n"),
                                 (Punctuation, ">"),
                                 (String.Other, "c"),
                                 (Text, "\n"),
                                 (Punctuation, ">"),
                                 (Text, " "),
                                 (String.Other, " "),
                                 (Text, "\n"),
                                 (Punctuation, ">"),
                                 (Text, " "),
                                 (Text, "\n"),
                                 (Punctuation, ">"),
                                 (Text, "\n"),
                             ]

    assert list(pygments.lex("|  a\n"
                             "| b\n"
                             "|c\n"
                             "|  \n"
                             "| \n"
                             "|\n", ProcyonLexer())) == [
                                 (Punctuation, "|"),
                                 (Text, " "),
                                 (String.Other, " a"),
                                 (Text, "\n"),
                                 (Punctuation, "|"),
                                 (Text, " "),
                                 (String.Other, "b"),
                                 (Text, "\n"),
                                 (Punctuation, "|"),
                                 (String.Other, "c"),
                                 (Text, "\n"),
                                 (Punctuation, "|"),
                                 (Text, " "),
                                 (String.Other, " "),
                                 (Text, "\n"),
                                 (Punctuation, "|"),
                                 (Text, " "),
                                 (Text, "\n"),
                                 (Punctuation, "|"),
                                 (Text, "\n"),
                             ]

    assert list(pygments.lex("!  a\n"
                             "! b\n"
                             "!c\n"
                             "!  \n"
                             "! \n"
                             "!\n", ProcyonLexer())) == [
                                 (Punctuation, "!"),
                                 (Text, " "),
                                 (Error, " a"),
                                 (Text, "\n"),
                                 (Punctuation, "!"),
                                 (Text, " "),
                                 (Error, "b"),
                                 (Text, "\n"),
                                 (Punctuation, "!"),
                                 (Error, "c"),
                                 (Text, "\n"),
                                 (Punctuation, "!"),
                                 (Text, " "),
                                 (Error, " "),
                                 (Text, "\n"),
                                 (Punctuation, "!"),
                                 (Text, " "),
                                 (Text, "\n"),
                                 (Punctuation, "!"),
                                 (Text, "\n"),
                             ]


def test_unclosed():
    assert list(pygments.lex("[\n"
                             "]\n", ProcyonLexer())) == [
                                 (Punctuation, "["),
                                 (Text, "\n"),
                                 (Error, "]"),
                                 (Text, "\n"),
                             ]

    assert list(pygments.lex("{\n"
                             "}\n", ProcyonLexer())) == [
                                 (Punctuation, "{"),
                                 (Text, "\n"),
                                 (Error, "}"),
                                 (Text, "\n"),
                             ]


def test_comments():
    assert list(pygments.lex("#\n", ProcyonLexer())) == [
        (Comment, "#"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("#\tComment\n", ProcyonLexer())) == [
        (Comment, "#\tComment"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("1#2\n", ProcyonLexer())) == [
        (Number.Integer, "1"),
        (Comment, "#2"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("1:2#3\n", ProcyonLexer())) == [
        (Name.Tag, "1"),
        (Punctuation, ":"),
        (Number.Integer, "2"),
        (Comment, "#3"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[#]\n", ProcyonLexer())) == [
        (Punctuation, "["),
        (Error, "#]"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("{#}\n", ProcyonLexer())) == [
        (Punctuation, "{"),
        (Error, "#}"),
        (Text, "\n"),
    ]

    assert list(pygments.lex("[{1:[#]}]\n", ProcyonLexer())) == [
        (Punctuation, "["),
        (Punctuation, "{"),
        (Name.Tag, "1"),
        (Punctuation, ":"),
        (Punctuation, "["),
        (Error, "#]}]"),
        (Text, "\n"),
    ]


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
