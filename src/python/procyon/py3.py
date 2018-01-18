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
"""Defines names that are no longer available in python3.

name         py2 meaning      py3 meaning
----         -----------      -----------
unicode      unicode          str
long         long             int
xrange       xrange           range
iteritems    dict.iteritems   dict.items
iterkeys     dict.iterkeys    dict.keys
itervalues   dict.itervalues  dict.values
"""

unicode = type(u"")

try:
    long = long
except NameError:
    long = int

try:
    xrange = xrange
except NameError:
    xrange = range

iteritems = lambda d: getattr(d, "iteritems", d.items)()
iterkeys = lambda d: getattr(d, "iterkeys", d.keys)()
itervalues = lambda d: getattr(d, "itervalues", d.values)()
