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

if __name__ == "__main__":
    import json
    import os
    import sys
    from . import dump

    progname = os.path.basename(sys.argv.pop(0))
    infile, outfile = sys.stdin, sys.stdout
    if len(sys.argv) == 1:
        infile = open(sys.argv[0], "rb")
    elif len(sys.argv) == 2:
        infile = open(sys.argv[0], "rb")
        outfile = open(sys.argv[1], "wb")
    elif len(sys.argv) > 2:
        raise SystemExit("usage: %s [INPUT.json [OUTPUT.pn]]" % progname)

    dump(json.load(infile), outfile)
