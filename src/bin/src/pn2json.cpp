// Copyright 2017 The Procyon Authors
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

namespace pn2json {
namespace {

typedef enum {
    TRADITIONAL,
    COMMA_FIRST,
    MINIFIED,
    ROOT,
} js_style_t;

pn::string_view progname;
int             style = TRADITIONAL;

struct option opts[] = {
        {"traditional", no_argument, &style, TRADITIONAL},
        {"comma-first", no_argument, &style, COMMA_FIRST},
        {"minify", no_argument, &style, MINIFIED},
        {"root", no_argument, &style, ROOT},
        {},
};

void dump_traditional_json(pn::file_view out, parser* prs, pn_error_t* error);
void dump_comma_first_json(pn::file_view out, parser* prs, pn_error_t* error);
void dump_minified_json(pn::file_view out, parser* prs, pn_error_t* error);
void dump_json_root(pn::file_view out, parser* prs, pn_error_t* error);

void usage(pn::file_view out, int status) {
    pn::format(
            out,
            "usage: {0} [options] [FILE.pn]\n"
            "\n"
            "options:\n"
            "     --traditional            format JSON traditionally (default)\n"
            "     --comma-first            format JSON with comma first\n"
            " -m, --minify                 minify JSON\n"
            " -r, --root                   print root string or data instead of JSON\n"
            " -h, --help                   show this help screen\n",
            progname);
    exit(status);
}

void main(int argc, char* const* argv) {
    const char* basename = strrchr(argv[0], '/');
    if (basename) {
        progname = basename + 1;
    } else {
        progname = argv[0];
    }

    char ch;
    while ((ch = getopt_long(argc, argv, "rmh", opts, NULL)) != -1) {
        switch (ch) {
            case 'r': style = ROOT; break;
            case 'm': style = MINIFIED; break;
            case 'h': usage(stdout, 0); break;
            case 0: break;
            default: usage(stderr, 64); break;
        }
    }

    argc -= optind;
    argv += optind;

    pn::string_view filename;
    pn_error_t      error{};
    try {
        pn::file      open_in;
        pn::file_view in;
        switch (argc) {
            case 0:
                filename = "-";
                in       = stdin;
                break;

            case 1:
                filename = argv[0];
                if (filename == "-") {
                    in = stdin;
                } else {
                    in = open_in = pn::open(argv[0], "r").check();
                }
                break;

            default: usage(stderr, 64); break;
        }

        lexer  lex(in);
        parser prs(&lex, 64);
        switch (style) {
            case TRADITIONAL: dump_traditional_json(stdout, &prs, &error); break;
            case COMMA_FIRST: dump_comma_first_json(stdout, &prs, &error); break;
            case MINIFIED: dump_minified_json(stdout, &prs, &error); break;
            case ROOT: dump_json_root(stdout, &prs, &error); break;
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error(filename.copy().c_str()));
    }
}

void nl_indent(pn::file_view out, int depth) {
    out.write(pn::rune{'\n'}).check();
    for (int i = 0; i < depth; ++i) {
        out.write(pn::rune{'\t'}).check();
    }
}

void dump_float(pn::file_view out, double f) {
    switch (std::fpclassify(f)) {
        case FP_NAN: out.write("null").check(); break;
        case FP_INFINITE: pn::format(stdout, "{0}1e999", (f < 0) ? "-" : ""); break;
        default: pn::dump(stdout, f, pn::dump_short); break;
    }
}

void dump_data(pn::file_view out, pn::data_view d) {
    static const char hex[] = "0123456789abcdef";
    out.write('"').check();
    for (int i = 0; i < d.size(); ++i) {
        format(out, "{0}{1}", hex[(0xf0 & d[i]) >> 4], hex[0x0f & d[i]]);
    }
    out.write('"').check();
}

void dump_string(pn::file_view out, pn::string_view s) {
    out.write('"').check();
    for (int i = 0; i < s.size(); ++i) {
        switch (s.data()[i]) {
            case '\000': out.write("\\u0000").check(); break;
            case '\001': out.write("\\u0001").check(); break;
            case '\002': out.write("\\u0002").check(); break;
            case '\003': out.write("\\u0003").check(); break;
            case '\004': out.write("\\u0004").check(); break;
            case '\005': out.write("\\u0005").check(); break;
            case '\006': out.write("\\u0006").check(); break;
            case '\007': out.write("\\u0007").check(); break;
            case '\b': out.write("\\b").check(); break;
            case '\t': out.write("\\t").check(); break;
            case '\n': out.write("\\n").check(); break;
            case '\013': out.write("\\u000b").check(); break;
            case '\f': out.write("\\f").check(); break;
            case '\r': out.write("\\r").check(); break;
            case '\016': out.write("\\u000e").check(); break;
            case '\017': out.write("\\u000f").check(); break;
            case '\020': out.write("\\u0000").check(); break;
            case '\021': out.write("\\u0001").check(); break;
            case '\022': out.write("\\u0002").check(); break;
            case '\023': out.write("\\u0003").check(); break;
            case '\024': out.write("\\u0004").check(); break;
            case '\025': out.write("\\u0005").check(); break;
            case '\026': out.write("\\u0006").check(); break;
            case '\027': out.write("\\u0007").check(); break;
            case '\030': out.write("\\u0008").check(); break;
            case '\031': out.write("\\u0009").check(); break;
            case '\032': out.write("\\u000a").check(); break;
            case '\033': out.write("\\u000b").check(); break;
            case '\034': out.write("\\u000c").check(); break;
            case '\035': out.write("\\u000d").check(); break;
            case '\036': out.write("\\u000e").check(); break;
            case '\037': out.write("\\u001f").check(); break;
            case '\\': out.write("\\\\").check(); break;
            case '"': out.write("\\\"").check(); break;
            case '\177': out.write("\\u007f").check(); break;
            default: out.write(s.data()[i]).check(); break;
        }
    }
    out.write('"').check();
}

bool is_sequence_in(pn_event_type_t t) {
    switch (t) {
        case PN_EVT_ARRAY_IN:
        case PN_EVT_MAP_IN: return true;
        default: return false;
    }
}

bool is_sequence_out(pn_event_type_t t) {
    switch (t) {
        case PN_EVT_ARRAY_OUT:
        case PN_EVT_MAP_OUT: return true;
        default: return false;
    }
}

bool is_sequence(pn_event_type_t t) { return is_sequence_in(t) || is_sequence_out(t); }

void dump_token(pn::file_view out, pn_event_type_t t, pn::value_cref x) {
    switch (t) {
        case PN_EVT_NULL:
        case PN_EVT_BOOL:
        case PN_EVT_INT: pn::dump(out, x, pn::dump_short); break;
        case PN_EVT_FLOAT: dump_float(out, x.as_float()); break;
        case PN_EVT_DATA: dump_data(out, x.as_data()); break;
        case PN_EVT_STRING: dump_string(out, x.as_string()); break;
        case PN_EVT_ARRAY_IN: out.write(pn::rune{'['}).check(); break;
        case PN_EVT_ARRAY_OUT: out.write(pn::rune{']'}).check(); break;
        case PN_EVT_MAP_IN: out.write(pn::rune{'{'}).check(); break;
        case PN_EVT_MAP_OUT: out.write(pn::rune{'}'}).check(); break;
        case PN_EVT_ERROR: break;
    }
}

// Prints \n on destruction if !*is_first_event.
class line_finisher {
  public:
    line_finisher(pn::file_view out, bool* is_first_event)
            : _out(out), _is_first_event(is_first_event) {}

    ~line_finisher() {
        if (!*_is_first_event) {
            _out.write(pn::rune{'\n'}).check();
        }
    }

  private:
    pn::file_view _out;
    bool*         _is_first_event;
};

void dump_json_root(pn::file_view out, parser* prs, pn_error_t* error) {
    if (!prs->next(error)) {
        throw std::runtime_error("internal error: no events?");
    }
    pn::value_cref x{&prs->event().x};
    switch (x.type()) {
        case PN_DATA: out.write(x.as_data()).check(); break;
        case PN_STRING: out.write(x.as_string()).check(); break;
        default: throw std::runtime_error("root is not data or string");
    }
}

void dump_minified_json(pn::file_view out, parser* prs, pn_error_t* error) {
    bool          is_first_event = true;
    bool          is_first_item  = true;
    line_finisher lf(out, &is_first_event);

    while (prs->next(error)) {
        const pn_event_type_t type = prs->event().type;
        const pn::value_cref  k{&prs->event().k};
        const pn::value_cref  x{&prs->event().x};

        if (!is_sequence_out(type) && !is_first_item) {
            out.write(",").check();
        }
        if (k.is_string()) {
            dump_string(out, k.as_string());
            out.write(":").check();
        }
        dump_token(out, type, x);

        is_first_event = false;
        is_first_item  = is_sequence_in(type);
    }
}

void dump_traditional_json(pn::file_view out, parser* prs, pn_error_t* error) {
    int           long_depth     = 0;
    int           short_depth    = 0;
    bool          is_first_item  = true;
    bool          is_first_event = true;
    line_finisher lf(out, &is_first_event);

    while (prs->next(error)) {
        const pn_event_type_t type = prs->event().type;
        const pn::value_cref  k{&prs->event().k};
        const pn::value_cref  x{&prs->event().x};
        const bool            is_short              = (prs->event().flags & PN_EVT_SHORT);
        const bool            inside_short_sequence = (short_depth > 0);

        if (is_sequence_out(type)) {
            if (!inside_short_sequence) {
                nl_indent(out, long_depth - 1);
            }
        } else if (!is_first_item) {
            if (inside_short_sequence) {
                out.write(", ").check();
            } else {
                out.write(",").check();
                nl_indent(out, long_depth);
            }
        } else if (!inside_short_sequence && !is_first_event) {
            nl_indent(out, long_depth);
        }

        if (k.is_string()) {
            dump_string(out, k.as_string());
            out.write(": ").check();
        }

        dump_token(out, type, x);

        is_first_event = false;
        is_first_item  = is_sequence_in(type);
        if (is_sequence_in(type)) {
            ++(is_short ? short_depth : long_depth);
        } else if (is_sequence_out(type)) {
            --(is_short ? short_depth : long_depth);
        }
    }
}

void dump_comma_first_json(pn::file_view out, parser* prs, pn_error_t* error) {
    int           long_depth     = 0;
    int           short_depth    = 0;
    bool          is_first_item  = true;
    bool          is_first_event = true;
    line_finisher lf(out, &is_first_event);

    while (prs->next(error)) {
        const pn_event_type_t type = prs->event().type;
        const pn::value_cref  k{&prs->event().k};
        const pn::value_cref  x{&prs->event().x};
        const bool            is_short              = (prs->event().flags & PN_EVT_SHORT);
        const bool            inside_short_sequence = (short_depth > 0);

        if (!is_sequence_out(type)) {
            if (!is_first_item) {
                if (inside_short_sequence) {
                    out.write(", ").check();
                } else {
                    nl_indent(out, long_depth - 1);
                    out.write(",\t").check();
                }
            } else if (!is_first_event && !inside_short_sequence) {
                out.write(pn::rune{'\t'});
            }
        }

        if (k.is_string()) {
            dump_string(out, k.as_string());
            if (!is_sequence_in(type) || is_short) {
                out.write(": ").check();
            } else {
                out.write(":").check();
            }
        }

        if (is_sequence(type) && !is_short) {
            if (is_sequence_out(type)) {
                nl_indent(out, long_depth - 1);
            } else if (k.is_string()) {
                nl_indent(out, long_depth);
            }
        }

        dump_token(out, type, x);

        is_first_event = false;
        is_first_item  = is_sequence_in(type);
        if (is_sequence_in(type)) {
            ++(is_short ? short_depth : long_depth);
        } else if (is_sequence_out(type)) {
            --(is_short ? short_depth : long_depth);
        }
    }
}

void print_nested_exception(const std::exception& e) {
    pn::format(stderr, ": {0}", e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& e) {
        print_nested_exception(e);
    }
}

void print_exception(const std::exception& e) {
    pn::format(stderr, "{0}: {1}", progname, e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& e) {
        print_nested_exception(e);
    }
    pn::format(stderr, "\n");
}

}  // namespace
}  // namespace pn2json

int main(int argc, char* const* argv) {
    try {
        pn2json::main(argc, argv);
    } catch (const std::exception& e) {
        pn2json::print_exception(e);
        return 1;
    }
    return 0;
}
