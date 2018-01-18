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

import logging
import pygments.lexers
from pygments.lexer import RegexLexer, include, bygroups, this, using
from pygments.token import Comment, Error, Keyword, Name, Number, Punctuation, String, Text

logger = logging.getLogger(__name__)


class ProcyonLexer(RegexLexer):
    name = "procyon"
    aliases = ["procyon", "pn"]
    filenames = ["*.pn"]
    mimetypes = ["text/x-procyon"]
    tabsize = 0

    tokens = {
        "root": [
            include("line"),
        ],
        "line": [
            (r"^([ \t]+)?([^\n]+)?(\n)", bygroups(Text, using(this, state="block"), Text)),
        ],
        "block": [
            include("whitespace"),
            (r"\*", Punctuation),
            (r"([>|])([ \t])?(.+)?$", bygroups(Punctuation, Text, String.Other)),
            (r"(!)([ \t])?(.+)?$", bygroups(Punctuation, Text, Error)),
            (r"([A-Za-z0-9_./+-]*)(:)([ \t]+)?(.+)?$", bygroups(Name.Tag, Punctuation, Text,
                                                                using(this, state="value"))),
            (r'("(?:[^"\\]|\\.)*")(:)([ \t]+)?(.+)?$', bygroups(Name.Tag, Punctuation, Text,
                                                                using(this, state="value"))),
            (r"#.*$", Comment),
            include("value"),
            (r".+$", Error),
        ],
        "value": [
            include("whitespace"),
            (r"(true|false|null|[+-]?inf|nan)\b", Keyword.Constant),
            (r"[+-]?(0|[1-9]\d*)(\.\d+)?[Ee][+-]?\d+", Number.Float),
            (r"[+-]?(0|[1-9]\d*)\.\d+", Number.Float),
            (r"[+-]?(0|[1-9]\d*)", Number.Integer),
            (r'"([^"\\]|\\.)*"', String.Double),
            (r"\$[0-9A-Za-z \t]*", Number.Hex),
            (r"\{", Punctuation, ("map-continue", "map-start")),
            (r"\[", Punctuation, ("array-continue", "array-start")),
            (r"#.*$", Comment),
            (r".+$", Error),
        ],
        "element": [
            include("whitespace"),
            (r"(true|false|null|[+-]?inf|nan)\b", Keyword.Constant, "#pop"),
            (r"[+-]?(0|[1-9]\d*)(\.\d+)?[Ee][+-]?\d+", Number.Float, "#pop"),
            (r"[+-]?(0|[1-9]\d*)\.\d+", Number.Float, "#pop"),
            (r"[+-]?(0|[1-9]\d*)", Number.Integer, "#pop"),
            (r'"([^"\\]|\\.)*"', String.Double, "#pop"),
            (r"\$[0-9A-Za-z \t]*", Number.Hex, "#pop"),
            (r"\{", Punctuation, ("#pop", "map-continue", "map-start")),
            (r"\[", Punctuation, ("#pop", "array-continue", "array-start")),
            (r"#.*$|[^]},]+", Error, "#pop"),
        ],
        "map-start": [
            (r"\}", Punctuation, "#pop:2"),
            include("map-key"),
        ],
        "map-key": [
            include("whitespace"),
            (r"([A-Za-z0-9_./+-]*)(:)", bygroups(Name.Tag, Punctuation), ("#pop", "element")),
            (r'("(?:[^"\\]|\\.)*")(:)', bygroups(Name.Tag, Punctuation), ("#pop", "element")),
            (r"#.*|[^]},]+$", Error, "#pop"),
        ],
        "map-continue": [
            (r",", Punctuation, "map-key"),
            (r"\}", Punctuation, "#pop"),
        ],
        "array-start": [
            (r"\]", Punctuation, "#pop:2"),
            include("element"),
        ],
        "array-continue": [
            include("whitespace"),
            (r",", Punctuation, "element"),
            (r"\]", Punctuation, "#pop"),
        ],
        "whitespace": [
            (r"[ \t]+", Text),
        ],
    }


__all__ = ["ProcyonLexer"]
