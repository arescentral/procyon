// Copyright 2018 The Procyon Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <pn/file>
#include <pn/value>

#include "../../c/src/lex.h"
#include "../../c/src/parse.h"
#include "../../c/src/utf8.h"
#include "./lex.hpp"
#include "./parse.hpp"

namespace pndump {
namespace {

pn::string_view progname;

void main(int argc, char* const* argv) {
    progname             = *(argc--, argv++);
    const char* basename = strrchr(progname.data(), '/');
    if (basename) {
        progname = basename + 1;
    }

    if (argc != 0) {
        pn_format(pn_stderr, "usage: {0}\n", "s", progname);
        exit(64);
    }

    pn_error_t error{};
    pn::value  x;
    if (!parse(stdin, &x, &error)) {
        throw std::runtime_error(
                pn::format("-:{0}:{1}: {2}", error.lineno, error.column, pn_strerror(error.code))
                        .c_str());
    }
    pn::file_view{stdout}.dump(x).check();
}

void print_nested_exception(const std::exception& e) {
    pn::file_view{stderr}.format(": {0}", e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& e) {
        print_nested_exception(e);
    }
}

void print_exception(const std::exception& e) {
    pn::file_view{stderr}.format("{0}: {1}", progname, e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& e) {
        print_nested_exception(e);
    }
    pn::file_view{stderr}.format("\n");
}

}  // namespace
}  // namespace pndump

int main(int argc, char* const* argv) {
    try {
        pndump::main(argc, argv);
    } catch (const std::exception& e) {
        pndump::print_exception(e);
        return 1;
    }
    return 0;
}
