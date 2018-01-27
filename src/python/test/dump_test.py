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
import pytest
from .context import procyon


def test_named():
    assert procyon.dumps(None) == "null\n"
    assert procyon.dumps(True) == "true\n"
    assert procyon.dumps(False) == "false\n"
    assert procyon.dumps(float("inf")) == "inf\n"
    assert procyon.dumps(float("-inf")) == "-inf\n"
    assert procyon.dumps(float("nan")) == "nan\n"


def test_scalar():
    assert procyon.dumps(0) == "0\n"
    assert procyon.dumps(0.0) == "0.0\n"
    assert procyon.dumps(1) == "1\n"
    assert procyon.dumps(-1) == "-1\n"
    assert procyon.dumps(9223372036854775807) == "9223372036854775807\n"
    assert procyon.dumps(-9223372036854775808) == "-9223372036854775808\n"
    assert procyon.dumps(5.0) == "5.0\n"
    assert procyon.dumps(0.5) == "0.5\n"


def test_int_range():
    with pytest.raises(OverflowError):
        procyon.dumps(9223372036854775808)
    with pytest.raises(OverflowError):
        procyon.dumps(-9223372036854775809)


def test_floatrounding():
    tests = [
        (-0.0000000000000020e-308, "-2e-323\n"),  # 4th-least negative denormal
        (-0.0000000000000015e-308, "-1.5e-323\n"),  # 3rd-least negative denormal
        (-0.0000000000000010e-308, "-1e-323\n"),  # 2nd-least negative denormal
        (-0.0000000000000005e-308, "-5e-324\n"),  # least negative denormal
        (+0.0000000000000000e+000, "0.0\n"),  # zero
        (+0.0000000000000005e-308, "5e-324\n"),  # least positive denormal
        (+0.0000000000000010e-308, "1e-323\n"),  # 2nd-least positive denormal
        (+0.0000000000000015e-308, "1.5e-323\n"),  # 3rd-least positive denormal
        (+2.2250738585072004e-308, "2.2250738585072004e-308\n"),  # 2nd-most positive denormal
        (+2.2250738585072009e-308, "2.225073858507201e-308\n"),  # most positive denormal
        (+2.2250738585072014e-308, "2.2250738585072014e-308\n"),  # least positive normal
        (+2.2250738585072019e-308, "2.225073858507202e-308\n"),  # 2nd-least positive normal
        (+1.9999999999999998e-001, "0.19999999999999998\n"),  # previous value before 0.2
        (+2.0000000000000000e-001, "0.2\n"),  # 0.2
        (+2.0000000000000001e-001, "0.2\n"),  # More accurate repr of 0.2
        (+2.0000000000000004e-001, "0.20000000000000004\n"),  # next value after 0.2
        (-5.0000000000000000e-001, "-0.5\n"),
        (+5.0000000000000000e-001, "0.5\n"),
        (+9.9999999999999964e+000, "9.999999999999996\n"),  # two values before 10
        (+9.9999999999999982e+000, "9.999999999999998\n"),  # previous value before 10
        (+1.0000000000000000e+001, "10.0\n"),  # 10
        (+1.0000000000000002e+001, "10.000000000000002\n"),  # next value after 10
        (+1.0000000000000000e-025, "1e-25\n"),  # 10 ^ -25
        (+1.0000000000000000e-024, "1e-24\n"),  # 10 ^ -24
        (+1.0000000000000000e-023, "1e-23\n"),  # 10 ^ -23
        (+1.0000000000000000e-022, "1e-22\n"),  # 10 ^ -22
        (+1.0000000000000000e-021, "1e-21\n"),  # 10 ^ -21
        (+1.0000000000000000e-020, "1e-20\n"),  # 10 ^ -20
        (+1.0000000000000000e-019, "1e-19\n"),  # 10 ^ -19
        (+1.0000000000000000e-018, "1e-18\n"),  # 10 ^ -18
        (+1.0000000000000000e-017, "1e-17\n"),  # 10 ^ -17
        (+1.0000000000000000e-016, "1e-16\n"),  # 10 ^ -16
        (+1.0000000000000000e-015, "1e-15\n"),  # 10 ^ -15
        (+1.0000000000000000e-014, "1e-14\n"),  # 10 ^ -14
        (+1.0000000000000000e-013, "1e-13\n"),  # 10 ^ -13
        (+1.0000000000000000e-012, "1e-12\n"),  # 10 ^ -12
        (+1.0000000000000000e-011, "1e-11\n"),  # 10 ^ -11
        (+1.0000000000000000e-010, "1e-10\n"),  # 10 ^ -10
        (+1.0000000000000000e-009, "1e-09\n"),  # 10 ^ -9
        (+1.0000000000000000e-008, "1e-08\n"),  # 10 ^ -8
        (+1.0000000000000000e-007, "1e-07\n"),  # 10 ^ -7
        (+1.0000000000000000e-006, "1e-06\n"),  # 10 ^ -6
        (+1.0000000000000000e-005, "1e-05\n"),  # 10 ^ -5
        (+1.0000000000000000e-004, "0.0001\n"),  # 10 ^ -4
        (+1.0000000000000000e-003, "0.001\n"),  # 10 ^ -3
        (+1.0000000000000000e-002, "0.01\n"),  # 10 ^ -2
        (+1.0000000000000000e-001, "0.1\n"),  # 10 ^ -1
        (+1.0000000000000000e+000, "1.0\n"),  # 10 ^ 0
        (+1.0000000000000000e+001, "10.0\n"),  # 10 ^ 1
        (+1.0000000000000000e+002, "100.0\n"),  # 10 ^ 2
        (+1.0000000000000000e+003, "1000.0\n"),  # 10 ^ 3
        (+1.0000000000000000e+004, "10000.0\n"),  # 10 ^ 4
        (+1.0000000000000000e+005, "100000.0\n"),  # 10 ^ 5
        (+1.0000000000000000e+006, "1000000.0\n"),  # 10 ^ 6
        (+1.0000000000000000e+007, "10000000.0\n"),  # 10 ^ 7
        (+1.0000000000000000e+008, "100000000.0\n"),  # 10 ^ 8
        (+1.0000000000000000e+009, "1000000000.0\n"),  # 10 ^ 9
        (+1.0000000000000000e+010, "10000000000.0\n"),  # 10 ^ 10
        (+1.0000000000000000e+011, "100000000000.0\n"),  # 10 ^ 11
        (+1.0000000000000000e+012, "1000000000000.0\n"),  # 10 ^ 12
        (+1.0000000000000000e+013, "10000000000000.0\n"),  # 10 ^ 13
        (+1.0000000000000000e+014, "100000000000000.0\n"),  # 10 ^ 14
        (+1.0000000000000000e+015, "1000000000000000.0\n"),  # 10 ^ 15
        (+1.0000000000000000e+016, "1e+16\n"),  # 10 ^ 16
        (+1.0000000000000000e+017, "1e+17\n"),  # 10 ^ 17
        (+1.0000000000000000e+018, "1e+18\n"),  # 10 ^ 18
        (+1.0000000000000000e+019, "1e+19\n"),  # 10 ^ 19
        (+1.0000000000000000e+020, "1e+20\n"),  # 10 ^ 20
        (+1.0000000000000000e+021, "1e+21\n"),  # 10 ^ 21
        (+1.0000000000000000e+022, "1e+22\n"),  # 10 ^ 22
        (+1.0000000000000000e+023, "1e+23\n"),  # 10 ^ 23
        (+1.0000000000000000e+024, "1e+24\n"),  # 10 ^ 24
        (+1.0000000000000000e+025, "1e+25\n"),  # 10 ^ 25
        (+9.9999999999999990e-006, "9.999999999999999e-06\n"),  # almost 10 ^ -5
        (+9.9999999999999990e-005, "9.999999999999999e-05\n"),  # almost 10 ^ -4
        (+9.9999999999999990e-004, "0.0009999999999999998\n"),  # almost 10 ^ -3
        (+9.9999999999999990e-003, "0.009999999999999998\n"),  # almost 10 ^ -2
        (+9.9999999999999990e-002, "0.09999999999999999\n"),  # almost 10 ^ -1
        (+9.9999999999999990e-001, "0.9999999999999999\n"),  # almost 10 ^ 0
        (+9.9999999999999990e+000, "9.999999999999998\n"),  # almost 10 ^ 1
        (+9.9999999999999990e+001, "99.99999999999999\n"),  # almost 10 ^ 2
        (+9.9999999999999990e+002, "999.9999999999999\n"),  # almost 10 ^ 3
        (+9.9999999999999990e+003, "9999.999999999998\n"),  # almost 10 ^ 4
        (+9.9999999999999990e+004, "99999.99999999999\n"),  # almost 10 ^ 5
        (+9.9999999999999990e+005, "999999.9999999999\n"),  # almost 10 ^ 6
        (+9.9999999999999990e+006, "9999999.999999998\n"),  # almost 10 ^ 7
        (+1.7976931348623155e+308, "1.7976931348623155e+308\n"),  # previous value before max
        (+1.7976931348623157e+308, "1.7976931348623157e+308\n"),  # float max
        (+float("inf"), "inf\n"),  # next value after float max
        (1.1, "1.1\n"),
        (1.1 * 3, "3.3000000000000003\n"),
        (1023.9999999999995, "1023.9999999999995\n"),
        (1023.9999999999997, "1023.9999999999997\n"),
        (1023.9999999999998, "1023.9999999999998\n"),
        (1023.9999999999999, "1023.9999999999999\n"),
        (1024.0000000000000, "1024.0\n"),
        (1024.0000000000002, "1024.0000000000002\n"),
        (1024.0000000000005, "1024.0000000000005\n"),
    ]

    for d, s in tests:
        # First check: is the test valid? Does the representation given
        # actually encode the double value?
        assert float(s) == d

        # Second check: do we print the given representation?
        assert procyon.dumps(d) == s


def test_data():
    assert procyon.dumps(b"") == "$\n"
    assert procyon.dumps(b"\x01\x02") == "$0102\n"

    assert procyon.dumps(b"\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff") == (
        "$\t00112233 44556677 8899aabb ccddeeff\n")

    assert procyon.dumps(
        b"\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"
        b"\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"
        b"\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99") == (
            "$\t00112233 44556677 8899aabb ccddeeff 00112233 44556677 8899aabb ccddeeff\n"
            "$\t00112233 44556677 8899\n")


def test_string():
    assert procyon.dumps("") == "\"\"\n"
    assert procyon.dumps("\0") == "\"\\u0000\"\n"
    assert procyon.dumps("\1") == "\"\\u0001\"\n"
    assert procyon.dumps("i") == "\"i\"\n"
    assert procyon.dumps("\177") == "\"\\u007f\"\n"
    assert procyon.dumps("procyon") == "\"procyon\"\n"
    assert procyon.dumps("procyon\n") == ">\tprocyon\n"

    assert procyon.dumps(
        "Four score and seven years ago our fathers brought forth on "
        "this continent a new nation, conceived in liberty, and "
        "dedicated to the proposition that all men are created equal.") == (
            ">\tFour score and seven years ago our fathers brought forth on this\n"
            ">\tcontinent a new nation, conceived in liberty, and dedicated to the\n"
            ">\tproposition that all men are created equal.\n"
            "!\n")

    assert procyon.dumps(
        "Four score and seven years ago our fathers brought forth on this "
        "continent a new nation, conceived in liberty, and dedicated to the "
        "proposition that all men are created equal.\n") == (
            ">\tFour score and seven years ago our fathers brought forth on this\n"
            ">\tcontinent a new nation, conceived in liberty, and dedicated to the\n"
            ">\tproposition that all men are created equal.\n")

    assert procyon.dumps(
        "Space: the final frontier.\n"
        "\n"
        "These are the voyages of the starship Enterprise. Its five-year "
        "mission: to explore strange new worlds, to seek out new life and new "
        "civilizations, to boldly go where no man has gone before.\n") == (
            ">\tSpace: the final frontier.\n"
            ">\n"
            ">\tThese are the voyages of the starship Enterprise. Its five-year mission:\n"
            ">\tto explore strange new worlds, to seek out new life and new\n"
            ">\tcivilizations, to boldly go where no man has gone before.\n")

    assert procyon.dumps(("…" * 72) + " \n") == (
        ">\t……………………………………………………………………………………………………………………………………………………………………………………………… \n")


def test_unicode():
    # Control characters:
    assert procyon.dumps("\0") == "\"\\u0000\"\n"
    assert procyon.dumps("\177") == "\"\\u007f\"\n"
    assert procyon.dumps("\u0080") == "\"\\u0080\"\n"

    # Unicode, 2 bytes:
    assert procyon.dumps("½") == "\"½\"\n"
    assert procyon.dumps("ж") == "\"ж\"\n"
    assert procyon.dumps(":\u0335") == "\":\u0335\"\n"

    # Unicode, 3 bytes:
    assert procyon.dumps("→") == "\"→\"\n"
    assert procyon.dumps("〒") == "\"〒\"\n"

    # Unicode, 4 bytes:
    assert procyon.dumps("🈀") == "\"🈀\"\n"
    assert procyon.dumps("🈐") == "\"🈐\"\n"


def test_list():
    assert procyon.dumps([None]) == "[null]\n"
    assert procyon.dumps([True, False]) == "[true, false]\n"
    assert procyon.dumps([1, 2, 3]) == "[1, 2, 3]\n"
    assert procyon.dumps([0.1, 0.2]) == "[0.1, 0.2]\n"
    assert procyon.dumps([None, True, 1, 1.0]) == "[null, true, 1, 1.0]\n"

    assert procyon.dumps(["hello"]) == "*\t\"hello\"\n"
    assert procyon.dumps(["one", "two", "three"]) == (
        "*\t\"one\"\n"  # force multi-line
        "*\t\"two\"\n"
        "*\t\"three\"\n")
    assert procyon.dumps(["one\ntwo\nthree\n"]) == (
        "*\t>\tone\n"  # force multi-line
        "\t|\ttwo\n"
        "\t|\tthree\n")

    assert procyon.dumps([[None]]) == "*\t[null]\n"
    assert procyon.dumps([["s"]]) == "*\t*\t\"s\"\n"
    assert procyon.dumps([("a", "b"), ("a", "b")]) == ("*\t*\t\"a\"\n"
                                                       "\t*\t\"b\"\n"
                                                       "*\t*\t\"a\"\n"
                                                       "\t*\t\"b\"\n")


def test_map():
    assert procyon.dumps({"null": None}) == "{null: null}\n"
    assert procyon.dumps(
        collections.OrderedDict([("t", True), ("f", False)])) == "{t: true, f: false}\n"
    assert procyon.dumps(collections.OrderedDict([("one", 1), ("two", 2), ("three", 3)
                                                  ])) == "{one: 1, two: 2, three: 3}\n"
    assert procyon.dumps(collections.OrderedDict([("less", 0.1), ("more", 0.2)])) == (
        "{less: 0.1, more: 0.2}\n")
    assert procyon.dumps(
        collections.OrderedDict([
            ("null", None),
            ("bool", True),
            ("int", 1),
            ("float", 1.0),
        ])) == ("{null: null, bool: true, int: 1, float: 1.0}\n")

    assert procyon.dumps({"hello": "world"}) == ("hello:  \"world\"\n")
    assert procyon.dumps(collections.OrderedDict([
        ("1", "one"),
        ("2", "two"),
        ("3", "three"),
    ])) == ("1:  \"one\"\n"
            "2:  \"two\"\n"
            "3:  \"three\"\n")
    assert procyon.dumps({
        "n": "one\ntwo\nthree\n"
    }) == (
        "n:\n"  # force multi-line
        "\t>\tone\n"
        "\t|\ttwo\n"
        "\t|\tthree\n")

    assert procyon.dumps(collections.OrderedDict([
        ("one", "a\nb\n"),
        ("two", "c\nd\n"),
    ])) == ("one:\n"
            "\t>\ta\n"
            "\t|\tb\n"
            "two:\n"
            "\t>\tc\n"
            "\t|\td\n")

    assert procyon.dumps(
        collections.OrderedDict([
            ("one", "one\n"),
            ("two", 2),
            ("three", "three\n"),
            ("four", 4),
        ])) == ("one:\n"
                "\t>\tone\n"
                "two:   2\n"
                "three:\n"
                "\t>\tthree\n"
                "four:  4\n")


def test_composite():
    assert procyon.dumps(
        collections.OrderedDict([
            ("us", collections.OrderedDict([
                ("name", "United States of America"),
                ("ratio", 1.9),
                ("stars", 50),
                ("stripes", 13),
                ("colors", [
                    b"\xb2\x22\x34",
                    b"\xff\xff\xff",
                    b"\x3c\x3b\x6e",
                ]),
                ("nicknames", [
                    "The Stars and Stripes",
                    "Old Glory",
                    "The Star-Spangled Banner",
                ]),
            ])),
            ("cl", collections.OrderedDict([
                ("name", "Republic of Chile"),
                ("ratio", 1.5),
                ("stars", 1),
                ("stripes", 2),
                ("colors", [
                    b"\xda\x29\x1c",
                    b"\xff\xff\xff",
                    b"\x00\x33\xa0",
                ]),
            ])),
            ("cu", collections.OrderedDict([
                ("name", "Republic of Cuba"),
                ("ratio", 2.0),
                ("stars", 1),
                ("stripes", 5),
                ("colors", [
                    b"\xcb\x15\x15",
                    b"\xff\xff\xff",
                    b"\x00\x2a\x8f",
                ]),
            ])),
        ])) == ("us:\n"
                "\tname:     \"United States of America\"\n"
                "\tratio:    1.9\n"
                "\tstars:    50\n"
                "\tstripes:  13\n"
                "\tcolors:\n"
                "\t\t*\t$b22234\n"
                "\t\t*\t$ffffff\n"
                "\t\t*\t$3c3b6e\n"
                "\tnicknames:\n"
                "\t\t*\t\"The Stars and Stripes\"\n"
                "\t\t*\t\"Old Glory\"\n"
                "\t\t*\t\"The Star-Spangled Banner\"\n"
                "cl:\n"
                "\tname:     \"Republic of Chile\"\n"
                "\tratio:    1.5\n"
                "\tstars:    1\n"
                "\tstripes:  2\n"
                "\tcolors:\n"
                "\t\t*\t$da291c\n"
                "\t\t*\t$ffffff\n"
                "\t\t*\t$0033a0\n"
                "cu:\n"
                "\tname:     \"Republic of Cuba\"\n"
                "\tratio:    2.0\n"
                "\tstars:    1\n"
                "\tstripes:  5\n"
                "\tcolors:\n"
                "\t\t*\t$cb1515\n"
                "\t\t*\t$ffffff\n"
                "\t\t*\t$002a8f\n")


def test_circular():
    # Contained within self
    with pytest.raises(ValueError):
        m = {}
        m["m"] = m
        procyon.dumps(m)

    # More deeply nested
    with pytest.raises(ValueError):
        m = {}
        m["m"] = [{"m": [m]}]
        procyon.dumps(m)

    # Not an error for an object to appear multiple times
    m = collections.OrderedDict()
    m["a"] = [None]
    m["b"] = m["a"]
    m["c"] = [m["a"]]
    m["d"] = {"a": m["a"]}
    assert procyon.dumps(m) == (
        "a:  [null]\n"  # force multi-line
        "b:  [null]\n"
        "c:\n"
        "\t*\t[null]\n"
        "d:\n"
        "\ta:  [null]\n")


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
