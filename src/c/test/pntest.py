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

import difflib
import functools
import glob
import os
import pytest
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
TEST_DATA = os.path.join(ROOT, "test")
DIRECTORIES = [os.path.relpath(p, TEST_DATA) for p in glob.glob(os.path.join(TEST_DATA, "*/*"))]

REQUIRED_FILES = frozenset("in.txt tokens.txt canonical.pn".split())
USED_FILES = REQUIRED_FILES | frozenset("parse.txt out.json out.txt out.png err.txt".split())


def run_test(directory, tokenize=None, reformat=None, jsonify=None, flatten=None):
    dirname = os.path.join(TEST_DATA, directory)
    files = frozenset(map(os.path.basename, glob.glob(os.path.join(dirname, "*"))))

    unused_files = (files - USED_FILES)
    missing_files = (REQUIRED_FILES - files)
    assert not unused_files, "unused files in %s" % dirname
    assert not missing_files, "missing files in %s" % dirname
    assert tokenize or reformat or jsonify or flatten, "no tests to perform"

    with open(os.path.join(dirname, "in.txt"), "rb") as f:
        source = f.read()

    if reformat is not None:
        formatted = reformat(source)
        diff(os.path.join(dirname, "canonical.pn"), formatted)
        reformatted = reformat(formatted)
        diff(os.path.join(dirname, "canonical.pn"), reformatted)

    if jsonify is not None:
        json, err = jsonify(source)
        if "out.json" in files:
            assert (err is None) and (json is not None)
            diff(os.path.join(dirname, "out.json"), json)
        else:
            assert (json is None) and (err is not None)
            diff(os.path.join(dirname, "err.txt"), err)

    if flatten is not None:
        if "out.txt" in files:
            txt = flatten(source)
            diff(os.path.join(dirname, "out.txt"), txt)
        if "out.png" in files:
            png = flatten(source)
            diff(os.path.join(dirname, "out.png"), png)


def lex_test(directory, tokenize):
    dirname = os.path.join(TEST_DATA, directory)
    with open(os.path.join(dirname, "in.txt"), "rb") as f:
        source = f.read()
    tokenized = tokenize(source)
    diff(os.path.join(dirname, "tokens.txt"), tokenized)


def parse_test(directory, parse):
    dirname = os.path.join(TEST_DATA, directory)
    with open(os.path.join(dirname, "in.txt"), "rb") as f:
        source = f.read()
    if os.path.isfile(os.path.join(dirname, "parse.txt")):
        parsed, errors = parse(source)
        assert (errors is None) and (parsed is not None)
        diff(os.path.join(dirname, "parse.txt"), parsed)
    elif os.path.isfile(os.path.join(dirname, "err.txt")):
        parsed, errors = parse(source)
        assert (parsed is None) and (errors is not None)
        diff(os.path.join(dirname, "err.txt"), errors)
    else:
        pytest.fail("neither parse.txt nor err.txt present in %s" % directory)


def check_output(args, stdin, **kwds):
    status = kwds.pop("status", 0)
    if not isinstance(stdin, (bytes, bytearray, memoryview)):
        stdin = stdin.encode("utf-8")
    p = subprocess.Popen(args, stdin=subprocess.PIPE, **kwds)
    stdout, stderr = p.communicate(stdin)
    if p.returncode != status:
        raise subprocess.CalledProcessError(p.returncode, args, stdout)
    return stdout, stderr


def split_lines(text):
    assert isinstance(text, bytes)
    lines = text.split(b"\n")
    lines = [line + b"\n" for line in lines[:-1]] + [lines[-1]]
    if lines[-1] == b"":
        lines = lines[:-1]
    return lines


def diff(expected_path, actual):
    with open(expected_path, "rb") as f:
        expected = f.read()
    if expected != actual:
        expected = split_lines(expected)
        actual = split_lines(actual)

        try:
            expected_dec = [l.decode("utf-8") for l in expected]
            actual_dec = [l.decode("utf-8") for l in actual]
        except UnicodeDecodeError:
            expected_dec = [l.decode("latin1") for l in expected]
            actual_dec = [l.decode("latin1") for l in actual]

        for line in difflib.unified_diff(expected_dec, actual_dec, "expected", "actual"):
            sys.stdout.write(line)
        pytest.fail()


CASES = [functools.partial(run_test, directory) for directory in DIRECTORIES]
LEX_CASES = [functools.partial(lex_test, directory) for directory in DIRECTORIES]
PARSE_CASES = [functools.partial(parse_test, directory) for directory in DIRECTORIES]
