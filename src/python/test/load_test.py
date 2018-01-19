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
import pytest

try:
    from io import BytesIO, StringIO
except ImportError:
    from cStringIO import StringIO
    BytesIO = StringIO
from .context import procyon, pntest


def test_constants():
    assert procyon.loads("null") is None
    assert procyon.loads("true") is True
    assert procyon.loads("false") is False
    assert procyon.loads("inf") == float("inf")
    assert procyon.loads("+inf") == float("inf")
    assert procyon.loads("-inf") == float("-inf")
    assert math.isnan(procyon.loads("nan"))
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("floop")


def test_integer():
    assert procyon.loads("0") == 0
    assert procyon.loads("1") == 1
    assert procyon.loads("9223372036854775807") == ((2**63) - 1)
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("9223372036854775808")
    assert procyon.loads("-9223372036854775808") == -(2**63)
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("-9223372036854775809")


def test_float():
    assert procyon.loads("0.0") == 0.0
    assert procyon.loads("0.5") == 0.5
    assert procyon.loads("0e0") == 0.0
    assert procyon.loads("0.5e0") == 0.5


def test_data():
    assert procyon.loads("$") == b""
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("$0")
    assert procyon.loads("$00") == b"\x00"
    assert procyon.loads("$ 00") == b"\x00"
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("$ 0 0")
    assert procyon.loads("$00112233") == b"\x00\x11\x22\x33"

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("$ 00 $ 01")
    assert procyon.loads("$ 00\n"  #
                         "$ 01\n") == b"\x00\x01"

    assert procyon.loads("# 00\n"  #
                         "$ 01\n"  #
                         "# 02\n"  #
                         "$ 03\n"  #
                         "# 04\n") == b"\x01\x03"

    assert procyon.loads("[$, $1f, $ffff, $ 0f 1e 2d 3c]") == [
        b"", b"\x1f", b"\xff\xff", b"\x0f\x1e\x2d\x3c"
    ]

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[$abcd\n"  #
                      "$1234]\n")


def test_string():
    assert procyon.loads("\"\"") == ""
    assert procyon.loads("\"yo whaddup\"") == "yo whaddup"
    assert procyon.loads("\"\\/\\\"\\\\\\b\\f\\n\\r\\t\"") == "/\"\\\b\f\n\r\t"
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("\"\\v\"")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("\"\\u000\"")
    assert procyon.loads("\"\\u0001\"") == "\001"
    assert procyon.loads("\"\\u0012\"") == "\022"
    assert procyon.loads("\"\\u0123\"") == "\u0123"
    assert procyon.loads("\"\\u1234\"") == "\u1234"


def test_xstring():
    assert procyon.loads(">") == "\n"
    assert procyon.loads("|") == "\n"
    assert procyon.loads("!") == ""
    assert procyon.loads("> ") == "\n"
    assert procyon.loads("| ") == "\n"
    assert procyon.loads("! ") == ""
    assert procyon.loads(">\t") == "\n"
    assert procyon.loads("|\t") == "\n"
    assert procyon.loads("!\t") == ""
    assert procyon.loads(">>") == ">\n"
    assert procyon.loads("||") == "|\n"
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("!!")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("!\n>\n")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("!\n|\n")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("!\n!\n")

    assert procyon.loads("> one") == "one\n"
    assert procyon.loads("| one") == "one\n"
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("! one")
    assert procyon.loads("| one\n"  #
                         "| two") == "one\ntwo\n"
    assert procyon.loads("| one\n"  #
                         "> two\n"  #
                         "!\n") == "one two"

    assert procyon.loads("| one\n"  #
                         "!\n"  #
                         "# two\n") == "one"

    assert procyon.loads(">\n"
                         "> Line two\n"
                         "> of three.\n"
                         ">\n") == ("\n"
                                    "Line two of three.\n"
                                    "\n")

    assert procyon.loads(">\n"
                         ">\n"
                         "> Line three\n"
                         "> of five.\n"
                         ">\n"
                         ">\n") == ("\n\n"
                                    "Line three of five.\n"
                                    "\n\n")

    assert procyon.loads("> Paragraph\n"
                         "> one.\n"
                         ">\n"
                         "> Paragraph\n"
                         "> two.\n") == ("Paragraph one.\n"
                                         "\n"
                                         "Paragraph two.\n")

    assert procyon.loads("> One.\n"  #
                         ">\n"  #
                         "> Two.\n"  #
                         "!\n") == (
                             "One.\n"  #
                             "\n"  #
                             "Two.")

    assert procyon.loads("| Four score and seven years ago our fathers brought forth on this\n"
                         "> continent a new nation, conceived in liberty, and dedicated to the\n"
                         "> proposition that all men are created equal.\n"
                         "!\n") == ("Four score and seven years ago our fathers brought forth on "
                                    "this continent a new nation, conceived in liberty, and "
                                    "dedicated to the proposition that all men are created equal.")

    assert procyon.loads(
        "| Space: the final frontier.\n"
        ">\n"
        "| These are the voyages of the starship Enterprise. Its five-year mission:\n"
        "> to explore strange new worlds, to seek out new life and new\n"
        "> civilizations, to boldly go where no man has gone before.\n") == (
            "Space: the final frontier.\n"
            "\n"
            "These are the voyages of the starship Enterprise. Its five-year "
            "mission: to explore strange new worlds, to seek out new life and new "
            "civilizations, to boldly go where no man has gone before.\n")


def test_list():
    assert procyon.loads("[]") == []
    assert procyon.loads("[0]") == [0]
    assert procyon.loads("[[[0]]]") == [[[0]]]
    assert procyon.loads("[1, 2, 3]") == [1, 2, 3]

    assert procyon.loads("[1, [2, [3]]]") == [1, [2, [3]]]

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[1")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[1,")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[}")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[1}")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("[1, }")


def test_xlist():
    assert procyon.loads("* 0") == [0]
    assert procyon.loads("* * * 0") == [[[0]]]
    assert procyon.loads("* 1\n"  #
                         "* 2\n"  #
                         "* 3\n") == [1, 2, 3]

    assert procyon.loads("* 1\n"  #
                         "* * 2\n"  #
                         "  * * 3\n") == [1, [2, [3]]]
    assert procyon.loads("*\n"
                         "  1\n"
                         "*\n"
                         "  *\n"
                         "    2\n"
                         "  *\n"
                         "    *\n"
                         "      3\n") == [1, [2, [3]]]

    assert procyon.loads("* 1\n"  #
                         "# :)\n"  #
                         "* 2\n"  #
                         "  # :(\n"  #
                         "* 3\n"  #
                         "# :|\n") == [1, 2, 3]

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("* 1\n"  #
                      "  * 2\n"  #
                      "    * 3\n")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("* * 1\n"  #
                      " * 2\n")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("*")


def test_map():
    assert procyon.loads("{}") == {}
    assert procyon.loads("{0: false}") == {"0": False}
    assert procyon.loads("{0: {1: {2: 3}}}") == {"0": {"1": {"2": 3}}}
    assert procyon.loads("{one: 1, two: 2, three: 3}") == {"one": 1, "two": 2, "three": 3}

    assert procyon.loads("{one: 1, and: {two: 2, and: {three: 3}}}") == {
        "one": 1,
        "and": {
            "two": 2,
            "and": {
                "three": 3
            },
        },
    }

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1,")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1:")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1: 1")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1: 1,")

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{]")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1: ]")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1: 1 ]")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("{1: 1, ]")


def test_xmap():
    assert procyon.loads(": null") == {"": None}
    assert procyon.loads("zero: 0") == {"zero": 0}
    assert procyon.loads("one:\n"  #
                         "  two:\n"  #
                         "    three: 0") == {
                             "one": {
                                 "two": {
                                     "three": 0
                                 }
                             }
                         }
    assert procyon.loads("one: 1\n"  #
                         "two: 2\n"  #
                         "three: 3\n") == {
                             "one": 1,
                             "two": 2,
                             "three": 3
                         }

    assert procyon.loads("one: 1\n"
                         "and:\n"
                         "  two: 2\n"
                         "  and:\n"
                         "    three: 3\n") == {
                             "one": 1,
                             "and": {
                                 "two": 2,
                                 "and": {
                                     "three": 3
                                 },
                             },
                         }
    assert procyon.loads("one:\n"
                         "  1\n"
                         "and:\n"
                         "  two:\n"
                         "    2\n"
                         "  and:\n"
                         "    three:\n"
                         "      3\n") == {
                             "one": 1,
                             "and": {
                                 "two": 2,
                                 "and": {
                                     "three": 3
                                 },
                             },
                         }
    assert procyon.loads("one:\n"
                         "\n"
                         "  1\n"
                         "two:\n"
                         "  \n"
                         "  2\n"
                         "three:\n"
                         "\t\n"
                         "  3\n") == {
                             "one": 1,
                             "two": 2,
                             "three": 3
                         }

    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("one: 1\n"  #
                      "  two: 2\n"  #
                      "    three: 3\n"),
    assert procyon.loads("one: 1\n"
                         "# :)\n"
                         "two: 2\n"
                         "     # :(\n"
                         "three: 3\n") == {
                             "one": 1,
                             "two": 2,
                             "three": 3
                         }
    assert procyon.loads("one: 1\n"
                         "# :)\n"
                         "two: 2\n"
                         "  # :)\n"  # doesn't match indentation of '2'
                         "three: 3\n") == {
                             "one": 1,
                             "two": 2,
                             "three": 3
                         }

    assert procyon.loads("\"\": \"\"\n"  #
                         "\":\": \":\"\n") == {
                             "": "",
                             ":": ":"
                         }

    assert procyon.loads("\"\\u0001\": $01\n"
                         "\"\\n\": $0a\n"
                         "\"\\u007f\": $7f\n"
                         "\"\\u0080\": $c280\n"
                         "\"\\u72ac\\u524d\": $e78aac e5898d\n") == {
                             "\1": b"\1",
                             "\n": b"\n",
                             "\177": b"\177",
                             "\u0080": b"\302\200",
                             "\u72ac\u524d": b"\347\212\254\345\211\215",
                         }


def test_equivalents():
    assert procyon.loads("!") == procyon.loads("\"\"")
    assert procyon.loads("|\n!") == procyon.loads("\"\"")
    assert procyon.loads("|") == procyon.loads("\"\\n\"")
    assert procyon.loads("|\n>\n!") == procyon.loads("\"\\n\"")
    assert procyon.loads("{1: 2}") == procyon.loads("1: 2")
    assert procyon.loads("[1]") == procyon.loads("* 1")


def test_composite():
    assert procyon.loads("us:\n"
                         "  name:     \"United States of America\"\n"
                         "  ratio:    1.9\n"
                         "  stars:    50\n"
                         "  stripes:  13\n"
                         "  colors:   [$b22234, $ffffff, $3c3b6e]\n"
                         "  nicknames:\n"
                         "    * \"The Stars and Stripes\"\n"
                         "    * \"Old Glory\"\n"
                         "    * \"The Star-Spangled Banner\"\n"
                         "cl:\n"
                         "  name:     \"Republic of Chile\"\n"
                         "  ratio:    1.5\n"
                         "  stars:    1\n"
                         "  stripes:  2\n"
                         "  colors:   [$da291c, $ffffff, $0033a0]\n"
                         "cu:\n"
                         "  name:     \"Republic of Cuba\"\n"
                         "  ratio:    2.0\n"
                         "  stars:    1\n"
                         "  stripes:  5\n"
                         "  colors:   [$cb1515, $ffffff, $002a8f]\n") == {
                             "us": {
                                 "name":
                                 "United States of America",
                                 "ratio":
                                 1.9,
                                 "stars":
                                 50,
                                 "stripes":
                                 13,
                                 "colors": [b"\xb2\x22\x34", b"\xff\xff\xff", b"\x3c\x3b\x6e"],
                                 "nicknames": [
                                     "The Stars and Stripes",
                                     "Old Glory",
                                     "The Star-Spangled Banner",
                                 ],
                             },
                             "cl": {
                                 "name": "Republic of Chile",
                                 "ratio": 1.5,
                                 "stars": 1,
                                 "stripes": 2,
                                 "colors": [b"\xda\x29\x1c", b"\xff\xff\xff", b"\x00\x33\xa0"],
                             },
                             "cu": {
                                 "name": "Republic of Cuba",
                                 "ratio": 2.0,
                                 "stars": 1,
                                 "stripes": 5,
                                 "colors": [b"\xcb\x15\x15", b"\xff\xff\xff", b"\x00\x2a\x8f"],
                             },
                         }


def test_comment():
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("# comment")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("* # comment")

    assert procyon.loads("true# comment") == True
    assert procyon.loads("true # comment") == True
    assert procyon.loads("true\n# comment") == True
    assert procyon.loads("1# comment") == 1
    assert procyon.loads("1 # comment") == 1
    assert procyon.loads("1\n# comment") == 1
    assert procyon.loads("\"\"# comment") == ""
    assert procyon.loads("\"\" # comment") == ""
    assert procyon.loads("\"\"\n# comment") == ""
    assert procyon.loads("$00# comment") == b"\0"
    assert procyon.loads("$00 # comment") == b"\0"
    assert procyon.loads("$00\n# comment") == b"\0"
    assert procyon.loads("># comment") == "# comment\n"
    assert procyon.loads("> # comment") == "# comment\n"
    assert procyon.loads(">\n# comment") == "\n"

    assert procyon.loads("* # comment\n"  #
                         "  1\n") == [1]
    assert procyon.loads("* # comment\n"  #
                         "  # etc\n"
                         "  1\n") == [1]

    assert procyon.loads("* 1\n"  #
                         "  # comment\n") == [1]
    assert procyon.loads("* 1\n"  #
                         "  # comment\n"
                         "  # etc\n") == [1]
    assert procyon.loads("* 1\n"  #
                         "# parent\n"
                         "  # child\n") == [1]


def test_same_line():
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("1 1")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("1\n1")


def test_bad():
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("&")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads("]")


def test_stack_smash():
    procyon.loads(("*" * 63) + "null")  # doesn’t throw
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads(("*" * 64) + "null")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads(("*" * 100) + "null")
    with pytest.raises(procyon.ProcyonDecodeError):
        procyon.loads(("*" * 512) + "null")


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
