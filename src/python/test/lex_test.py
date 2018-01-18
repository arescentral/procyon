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

try:
    from io import BytesIO, StringIO
except ImportError:
    from cStringIO import StringIO
    BytesIO = StringIO
import sys
from .context import procyon, pntest
"""
def test_xlist():
    assert lexs("*") == [line_in, star("*"), line_out]
    assert lexs("**") == [line_in, star("*"), line_in, star("*"), line_out, line_out]

    assert lexs("***") == [
        line_in,
        star("*"), line_in,
        star("*"), line_in,
        star("*"), line_out, line_out, line_out
    ]
    assert lexs("***0") == [
        line_in,
        star("*"), line_in,
        star("*"), line_in,
        star("*"), line_in,
        i("0"), line_out, line_out, line_out, line_out
    ]

    assert lexs("* *") == [line_in, star("*"), line_in, star("*"), line_out, line_out]
    assert lexs("* * *") == [
        line_in,
        star("*"), line_in,
        star("*"), line_in,
        star("*"), line_out, line_out, line_out
    ]

    assert lexs("*\n"
                "  *\n"
                "    *\n") == [
                    line_in,
                    star("*"), line_in,
                    star("*"), line_in,
                    star("*"), line_out, line_out, line_out
                ]
    assert lexs("*    \n"
                "  *  \n"
                "    *\n") == [
                    line_in,
                    star("*"), line_in,
                    star("*"), line_in,
                    star("*"), line_out, line_out, line_out
                ]
    assert lexs("***\n"
                " **\n"
                "  *\n") == [
                    line_in,
                    star("*"), line_in,
                    star("*"), line_in,
                    star("*"), line_out, line_eq,
                    star("*"), line_in,
                    star("*"), line_eq,
                    star("*"), line_out, line_out, line_out
                ]
    assert lexs("* \t  *\t*\n"
                "        *\n") == [
                    line_in,
                    star("*"), line_in,
                    star("*"), line_in,
                    star("*"), line_eq,
                    star("*"), line_out, line_out, line_out
                ]


def test_map():
    assert lexs(":") == [line_in, xkey(":"), line_out]
    assert lexs("0:") == [line_in, xkey("0:"), line_out]
    assert lexs("a:") == [line_in, xkey("a:"), line_out]
    assert lexs("+:") == [line_in, xkey("+:"), line_out]

    assert lexs("1:1") == [line_in, xkey("1:"), line_in, i("1"), line_out, line_out]
    assert lexs("1:  1") == [line_in, xkey("1:"), line_in, i("1"), line_out, line_out]
    assert lexs("{1:1}") == [line_in, map_in, key("1:"), i("1"), map_out, line_out]
    assert lexs("{1:  1}") == [line_in, map_in, key("1:"), i("1"), map_out, line_out]

    assert lexs("1:2\n3:4") == [
        line_in,
        xkey("1:"), line_in,
        i("2"), line_out, line_eq,
        xkey("3:"), line_in,
        i("4"), line_out, line_out
    ]
    assert lexs("{1:2,3:4}") == [
        line_in, map_in,
        key("1:"), i("2"), comma,
        key("3:"), i("4"), map_out, line_out
    ]
    assert lexs("1:  2\n3:  4") == [
        line_in,
        xkey("1:"), line_in,
        i("2"), line_out, line_eq,
        xkey("3:"), line_in,
        i("4"), line_out, line_out
    ]
    assert lexs("{1: 2, 3: 4}") == [
        line_in, map_in,
        key("1:"), i("2"), comma,
        key("3:"), i("4"), map_out, line_out
    ]


def test_comment():
    # Missing values
    assert lexs("# comment") == [line_in, comment("# comment"), line_out]
    assert lexs("* # comment") == [
        line_in, star("*"), line_in,
        comment("# comment"), line_out, line_out
    ]

    # These won't parse, but should lex.
    assert lexs("true# comment") == [line_in, true, comment("# comment"), line_out]
    assert lexs("true # comment") == [line_in, true, comment("# comment"), line_out]
    assert lexs("1# comment") == [line_in, i("1"), comment("# comment"), line_out]
    assert lexs("1 # comment") == [line_in, i("1"), comment("# comment"), line_out]
    assert lexs("\"\"# comment") == [line_in, s("\"\""), comment("# comment"), line_out]
    assert lexs("\"\" # comment") == [line_in, s("\"\""), comment("# comment"), line_out]
    assert lexs("$00# comment") == [line_in, d("$00"), comment("# comment"), line_out]
    assert lexs("$00 # comment") == [line_in, d("$00 "), comment("# comment"), line_out]
    assert lexs("># comment") == [line_in, wrap("># comment"), line_out]
    assert lexs("> # comment") == [line_in, wrap("> # comment"), line_out]


def test_bytes():
    assert lexs(b"\342\200\246") == [line_in, err(E.NONASCII, "\u2026"), line_out]
    assert lexs(b"> \342\200\246") == [line_in, wrap("> \u2026"), line_out]


def test_unicode():
    assert lexs(u"\u2026") == [line_in, err(E.NONASCII, "\u2026"), line_out]
    assert lexs(u"> \u2026") == [line_in, wrap("> \u2026"), line_out]
"""


def tokenize(source):
    sys.stdin = BytesIO(source)
    sys.stdout = StringIO()
    procyon.lex.main(["procyon.lex"])
    output = sys.stdout.getvalue()
    sys.stdin = sys.__stdin__
    sys.stdout = sys.__stdout__
    return output.encode("utf-8")


def test_func(run):
    run(tokenize)


def pytest_generate_tests(metafunc):
    metafunc.parametrize("run", pntest.LEX_CASES, ids=pntest.DIRECTORIES)


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
