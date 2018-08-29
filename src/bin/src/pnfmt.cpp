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

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <map>
#include <pn/file>
#include <pn/value>
#include <vector>

#include "../../c/src/utf8.h"
#include "../../c/src/vector.h"
#include "./lex.hpp"

namespace pnfmt {
namespace {

static const char* progname;

const int kDataCompactMaxWidth = 8;  // key: $0f1e2d3c

struct token {
    pn_token_type_t type    = PN_TOK_ERROR;
    pn::string      content = "";
    int             column  = 0;
};

struct line {
    int  indent          = 0;
    int  lineno          = 0;
    bool extra_nl_before = false;

    std::vector<token> tokens;
    std::vector<line>  children;
};

static void      usage(pn::file_view out, int status);
static void      lex_file(pn::string_view path, pn::file_view in, std::vector<line>* roots);
static void      join_tokens(std::vector<line>* lines);
static void      simplify_tokens(std::vector<line>* lines);
static void      wrap_tokens(std::vector<line>* lines);
static void      indent(std::vector<line>* lines, int* lineno, int tab, int column);
static pn::value repr(const std::vector<line>& lines);
static void      output_tokens(
             const std::vector<line>& roots, bool in_place, pn::string_view path,
             pn::value_cref output);
static void format_tokens(
        const std::vector<line>& lines, pn::file_view out, int* lineno, int indent, int column);

#ifndef NDEBUG
static void check_invariants(const std::vector<line>& lines);
#endif

static struct option opts[] = {
        {"in-place", no_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"dump", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {},
};

void main(int argc, char* const* argv) {
    progname             = argv[0];
    const char* basename = strrchr(argv[0], '/');
    if (basename) {
        progname = basename + 1;
    }

    bool      in_place = false;
    pn::value output;
    bool      dump = false;

    char ch;
    while ((ch = getopt_long(argc, argv, "hio:", opts, NULL)) != -1) {
        switch (ch) {
            case 'h': usage(stdout, 0);
            case 'i': in_place = true; break;
            case 'o': output = pn::string{optarg}; break;
            case 'd': dump = true; break;
            case 0: break;
            default: usage(stderr, 64);
        }
    }

    argc -= optind;
    argv += optind;

    if (in_place) {
        if (argc == 0) {
            pn::format(stderr, "{0}: --in-place requires a path\n", progname);
            exit(64);
        } else if (!output.is_null()) {
            pn::format(stderr, "{0}: --in-place conflicts with --output\n", progname);
            exit(64);
        }
    } else if (argc > 1) {
        usage(stderr, 64);
    }

    std::vector<line> roots;
    if (argc == 0) {
        lex_file("-", stdin, &roots);
    } else {
        pn::file f;
        try {
            f = pn::open(argv[0], "r").check();
        } catch (std::runtime_error& e) {
            pn::format(stderr, "{0}: {1}: {2}\n", progname, argv[0], e.what());
            exit(64);
        }
        lex_file(argv[0], f, &roots);
    }

#ifndef NDEBUG
    check_invariants(roots);
#endif
    join_tokens(&roots);
    simplify_tokens(&roots);
    wrap_tokens(&roots);
    int lineno = 0;
    indent(&roots, &lineno, 0, 0);
    if (dump) {
        pn::dump(stdout, repr(roots));
    } else {
        output_tokens(roots, in_place, argv[0], output);
    }
}

static void usage(pn::file_view out, int status) {
    pn::format(
            out,
            "usage: {0} [-i | -o OUT] [IN]\n"
            "\n"
            "options:\n"
            " -i, --in-place               format file in-place\n"
            " -o, --output=FILE            write output to path\n"
            " -h, --help                   show this help screen\n",
            progname);
    exit(status);
}

static pn::value repr(const token& t) {
    pn::map m{
            {"type", t.type}, {"column", t.column}, {"content", t.content.copy()},
    };
    return std::move(m);
}

static pn::value repr(const line& l) {
    pn::map m{{"indent", l.indent}, {"lineno", l.lineno}, {"extra_nl_before", l.extra_nl_before}};
    if (!l.tokens.empty()) {
        pn::array_ref a = m["tokens"].to_array();
        for (const token& token : l.tokens) {
            a.push_back(repr(token));
        }
    }
    if (!l.children.empty()) {
        m["children"] = repr(l.children);
    }
    return std::move(m);
}

static pn::value repr(const std::vector<line>& lines) {
    pn::array a;
    for (const line& l : lines) {
        a.push_back(repr(l));
    }
    return std::move(a);
}

static void output_tokens(
        const std::vector<line>& roots, bool in_place, pn::string_view path,
        pn::value_cref output) {
    int lineno = 0;
    if (in_place) {
        pn::string tmp = path.copy();
        tmp += ".XXXXXX";
        {
            int fd = mkstemp(tmp.data());
            if (fd < 0) {
                pn::format(stderr, "{0}: {1}: {2}\n", progname, tmp, strerror(errno));
                exit(1);
            }
            pn::file out(fdopen(fd, "w"));
            format_tokens(roots, out, &lineno, 0, 0);
            out.write('\n').check();
        }
        if (rename(tmp.c_str(), path.copy().c_str()) < 0) {
            pn::format(stderr, "{0}: {1}: {2}\n", progname, path, strerror(errno));
            unlink(tmp.c_str());
            exit(1);
        }
    } else if (!output.is_null()) {
        pn::file out;
        try {
            out = pn::open(output.as_string(), "w").check();
        } catch (std::runtime_error& e) {
            pn::format(stderr, "{0}: {1}: {2}\n", progname, output.as_string(), e.what());
            exit(1);
        }
        format_tokens(roots, out, &lineno, 0, 0);
        out.write('\n').check();
    } else {
        format_tokens(roots, stdout, &lineno, 0, 0);
        pn::file_view{stdout}.write('\n').check();
    }
}

static void lex_block(
        lexer* lex, std::vector<line>* lines, bool* need_newline, pn::string_view path) {
    pn_error_t error;
    line       line;
    while (true) {
        int prev_lineno = lex->lineno();
        lex->next(&error);
        token token;
        token.type    = lex->token().type;
        token.content = pn::string(lex->token().begin, lex->token().end - lex->token().begin);

        if (lex->lineno() > (prev_lineno + 1)) {
            *need_newline = true;
        }

        switch (lex->token().type) {
            case PN_TOK_LINE_IN:
                *need_newline = false;
                lex_block(lex, &line.children, need_newline, path);
                if (!(line.tokens.empty() && line.children.empty())) {
                    lines->push_back(std::move(line));
                    line = pnfmt::line{};
                }
                continue;

            case PN_TOK_LINE_OUT:
                if (!(line.tokens.empty() && line.children.empty())) {
                    lines->push_back(std::move(line));
                    line = pnfmt::line{};
                }
                return;

            case PN_TOK_LINE_EQ:
                if (!(line.tokens.empty() && line.children.empty())) {
                    lines->push_back(std::move(line));
                    line = pnfmt::line{};
                }
                continue;

            case PN_TOK_ERROR:
                pn::format(
                        stderr, "{0}:{1}:{2}: {3}\n", path.copy().c_str(), lex->lineno(),
                        lex->column(), pn_strerror(error.code));
                break;

            default: break;
        }

        line.tokens.push_back(std::move(token));
        if (*need_newline) {
            line.extra_nl_before = true;
            *need_newline        = false;
        }
    }
}

static void lex_file(pn::string_view path, pn::file_view in, std::vector<line>* roots) {
    bool       need_newline = false;
    lexer      lex(in);
    pn_error_t error;
    lex.next(&error);
    lex_block(&lex, roots, &need_newline, path);
}

#ifndef NDEBUG
static void check_invariants(const std::vector<line>& lines) {
    for (const line& l : lines) {
        // assert(!l.tokens.empty());
        if (!l.tokens.empty()) {
            const token& last_token = l.tokens.back();
            for (const token& t : l.tokens) {
                assert(!t.content.empty());
                switch (t.type) {
                    case PN_TOK_LINE_IN:
                    case PN_TOK_LINE_EQ:
                    case PN_TOK_LINE_OUT:
                        assert(false);  // Should never appear within a line.
                        break;

                    case PN_TOK_STAR:
                    case PN_TOK_STR_WRAP:
                    case PN_TOK_STR_WRAP_EMPTY:
                    case PN_TOK_STR_PIPE:
                    case PN_TOK_STR_PIPE_EMPTY:
                    case PN_TOK_STR_BANG:
                    case PN_TOK_COMMENT:
                    case PN_TOK_ERROR:
                        assert(&t == &last_token);  // Should always be final on line:
                        break;

                    default: break;
                }
            }
        }
        check_invariants(l.children);
    }
}
#endif

static pn::string_view xstring_data(const token& token) {
    if (token.content.size() == 1) {
        return pn::string_view{};
    } else if ((token.content.data()[1] == ' ') || (token.content.data()[1] == '\t')) {
        return token.content.substr(2);
    } else {
        return token.content.substr(1);
    }
}

static void join_tokens(std::vector<line>* lines) {
    std::vector<line> out;
    for (line& next_line : *lines) {
        if (!out.empty() && out.back().children.empty()) {
            line& prev_line = out.back();
            if (!(prev_line.tokens.empty() || next_line.tokens.empty())) {
                token&       prev = prev_line.tokens.back();
                const token& next = next_line.tokens.front();
                switch (prev.type) {
                    case PN_TOK_STR_WRAP:
                    case PN_TOK_STR_PIPE:
                        if (next.type == PN_TOK_STR_WRAP) {
                            prev.content += " ";
                            prev.content += xstring_data(next);
                            continue;
                        }
                        break;

                    case PN_TOK_DATA:
                        if (next.type == PN_TOK_DATA) {
                            prev.content += next.content.substr(1);
                            continue;
                        }
                        break;

                    default: break;
                }
            }
        }
        out.push_back(std::move(next_line));
        join_tokens(&out.back().children);
    }
    lines->swap(out);
}

// Make token values more predictable:
//   !: remove trailing whitespace
//   >, |, #: make empty if empty, use \t if not
//   $: remove internal space, lowercase
//   block key: move RHS into map
static void simplify_tokens(std::vector<line>* lines) {
    for (line& l : *lines) {
        if (l.tokens.size() > 1) {
            switch (l.tokens[0].type) {
                case PN_TOK_KEY:
                case PN_TOK_QKEY: {
                    line               child;
                    std::vector<token> tokens;
                    l.tokens.swap(tokens);
                    for (token& t : tokens) {
                        if (l.tokens.empty()) {
                            l.tokens.emplace_back(std::move(t));
                        } else {
                            child.tokens.emplace_back(std::move(t));
                        }
                    }
                    l.children.swap(child.children);
                    l.children.emplace_back(std::move(child));
                    break;
                }

                default: break;
            }
        }

        for (token& token : l.tokens) {
            switch (token.type) {
                case PN_TOK_DATA: {
                    pn::string s;
                    for (pn::rune r : token.content) {
                        if ((pn::rune{'A'} <= r) && (r <= pn::rune{'F'})) {
                            s += pn::rune{r.value() | 32};
                        } else if (r.value() > ' ') {
                            s += r;
                        }
                    }
                    token.content.swap(s);
                    break;
                }

                case PN_TOK_STR_WRAP:
                case PN_TOK_STR_PIPE: {
                    switch (token.content.data()[1]) {
                        case ' ': token.content.data()[1] = '\t'; break;
                        case '\t': break;
                        default:
                            pn::string s = token.content.substr(0, 1).copy();
                            s += pn::rune{'\t'};
                            s += token.content.substr(1);
                            token.content = std::move(s);
                            break;
                    }
                    break;
                }

                case PN_TOK_STR_WRAP_EMPTY:
                case PN_TOK_STR_PIPE_EMPTY:
                case PN_TOK_STR_BANG:
                    if (token.content.size() > 1) {
                        token.content = token.content.substr(0, 1).copy();
                    }
                    break;

                case PN_TOK_COMMENT: {
                    pn::string::iterator begin = token.content.begin(), end = token.content.end();
                    for (auto it = token.content.end(); it != token.content.begin(); --it) {
                        switch ((*it).value()) {
                            case ' ':
                            case '\t':
                            case 0x3000: end = it; continue;
                        }
                        token.content = token.content.substr(0, end.offset()).copy();
                        break;
                    }

                    ++begin;
                    if (begin == end) {
                        token.content = "#";
                        break;
                    }
                    if ((*begin == pn::rune{'\t'}) || (*begin == pn::rune{' '})) {
                        ++begin;
                    }
                    if (begin == end) {
                        token.content = "#";
                        break;
                    }

                    pn::string new_content = (&token == &l.tokens[0]) ? "#\t" : "# ";
                    new_content +=
                            token.content.substr(begin.offset(), end.offset() - begin.offset());
                    token.content = std::move(new_content);
                    break;
                }

                default: break;
            }
        }
        simplify_tokens(&l.children);
    }
}

static bool is_short_block(const std::vector<line>& lines) {
    if (lines.empty()) {
        return true;
    } else if (lines.size() > 1) {
        return false;
    } else if (!lines[0].children.empty()) {
        return false;
    }
    for (const token& token : lines[0].tokens) {
        switch (token.type) {
            case PN_TOK_STR_WRAP:
            case PN_TOK_STR_WRAP_EMPTY:
            case PN_TOK_STR_PIPE:
            case PN_TOK_STR_PIPE_EMPTY: return false;

            default: continue;
        }
    }
    return true;
}

static void wrap_data(token& t, std::vector<line>* out) {
    if (t.content.size() <= (kDataCompactMaxWidth + 1)) {
        out->emplace_back();
        out->back().tokens.emplace_back(std::move(t));
        return;
    }

    const pn::string_view prefix = "$\t";
    pn::string*           s      = nullptr;
    int                   i      = 1;
    while (true) {
        if (!s) {
            out->emplace_back();
            line* l = &out->back();
            l->tokens.emplace_back();
            token* back = &l->tokens.back();
            back->type  = PN_TOK_DATA;
            s           = &(back->content = prefix.copy());
        }

        for (int col = 0; col < 8; ++col) {
            *s += t.content.substr(i, std::min(4, t.content.size() - i));
            if ((i += 4) >= t.content.size()) {
                return;
            }
            if (col == 7) {
                s = nullptr;
            } else {
                *s += " ";
            }
        }
    }
}

static std::vector<pn::string> wrap_paragraph(pn::string_view in) {
    std::vector<pn::string>    out;
    pn::string_view::size_type split_start_offset   = 0;
    pn::string_view::size_type split_end_offset     = in.npos;
    int                        split_end_rune_count = 0;
    int                        rune_count           = 0;
    bool                       initial_space        = true;
    for (auto it = in.begin(); it != in.end(); ++it) {
        if (*it == pn::rune{' '}) {
            if ((it.offset() != in.size() - 1) && !initial_space) {
                split_end_offset     = it.offset();
                split_end_rune_count = rune_count;
            }
        } else {
            initial_space = false;
        }
        rune_count += pn_rune_width((*it).value());
        if ((rune_count > 72) && (split_end_offset >= 0)) {
            out.push_back(
                    in.substr(split_start_offset, split_end_offset - split_start_offset).copy());
            split_start_offset = split_end_offset + 1;
            split_end_offset   = in.npos;
            rune_count -= (1 + split_end_rune_count);
            initial_space = true;
        }
    }
    out.push_back(in.substr(split_start_offset).copy());
    return out;
}

static pn::rune string_header(pn_token_type_t type) {
    switch (type) {
        case PN_TOK_STR_WRAP:
        case PN_TOK_STR_WRAP_EMPTY: return pn::rune{'>'};
        case PN_TOK_STR_PIPE:
        case PN_TOK_STR_PIPE_EMPTY: return pn::rune{'|'};
        case PN_TOK_STR_BANG: return pn::rune{'!'};
        default: return pn::rune{'?'};
    }
}

static void wrap_string(
        const token& token, bool change_header, pn_token_type_t preferred_header,
        std::vector<line>* out) {
    pn_token_type_t header = token.type;
    if (change_header) {
        header = preferred_header;
    }

    if (xstring_data(token).empty()) {
        out->emplace_back();
        out->back().tokens.emplace_back();
        out->back().tokens.back().type    = token.type;
        out->back().tokens.back().content = string_header(header).copy();
        return;
    }

    auto paragraph = wrap_paragraph(xstring_data(token));
    for (const auto& line : paragraph) {
        out->emplace_back();
        out->back().tokens.emplace_back();
        pnfmt::token* t = &out->back().tokens.back();
        t->type         = header;
        t->content      = pn::format("{0}\t{1}", string_header(header), line);
        header          = PN_TOK_STR_WRAP;
    }
}

static void wrap_tokens(std::vector<line>* lines) {
    std::vector<line> out;
    pn_token_type_t   preferred_str_header = PN_TOK_ERROR;
    bool              is_empty             = false;
    bool              was_empty            = false;
    for (line& l : *lines) {
        if (l.tokens.size() != 1) {
            out.push_back(std::move(l));
            continue;
        }
        token& token = l.tokens[0];
        switch (token.type) {
            case PN_TOK_DATA: {
                wrap_data(token, &out);
                out.back().children = std::move(l.children);
                break;
            }

            case PN_TOK_STR_WRAP:
            case PN_TOK_STR_WRAP_EMPTY:
            case PN_TOK_STR_PIPE:
            case PN_TOK_STR_PIPE_EMPTY:
                if (preferred_str_header == PN_TOK_ERROR) {
                    preferred_str_header = token.type;
                }
                is_empty = xstring_data(token).empty();
                wrap_string(token, was_empty || is_empty, preferred_str_header, &out);
                was_empty           = is_empty;
                out.back().children = std::move(l.children);
                break;

            default: out.push_back(std::move(l)); break;
        }
        wrap_tokens(&out.back().children);
    }
    lines->swap(out);
}

static void indent(std::vector<line>* lines, int* lineno, int tab, int column) {
    int width = 0;
    for (line& l : *lines) {
        if (l.tokens.empty()) {
            continue;
        }
        if (column) {
            l.indent = 0;
        } else {
            l.indent = tab;
        }
        bool needs_space = false;
        for (token& token : l.tokens) {
            token.column = column;
            if (needs_space) {
                ++token.column;
            }
            needs_space = true;
            switch (token.type) {
                case PN_TOK_COMMA:
                case PN_TOK_ARRAY_OUT:
                case PN_TOK_MAP_OUT: token.column = column; break;
                case PN_TOK_ARRAY_IN:
                case PN_TOK_MAP_IN: needs_space = false; break;
                default: break;
            }

            column = token.column + pn_str_width(token.content.data(), token.content.size());
        }

        if (!l.children.empty()) {
            switch (l.tokens.back().type) {
                case PN_TOK_KEY:
                case PN_TOK_QKEY:
                    if (is_short_block(l.children)) {
                        width = std::max(width, column);
                    }
                    break;

                default: break;
            }
        }

        column = 0;
    }

    for (line& l : *lines) {
        if (l.extra_nl_before) {
            ++*lineno;
        }
        l.lineno = *lineno;

        if (l.tokens.empty() || l.children.empty()) {
            ++*lineno;
            continue;
        }
        switch (l.tokens.back().type) {
            case PN_TOK_STAR: indent(&l.children, lineno, tab + 1, 0); break;
            case PN_TOK_KEY:
            case PN_TOK_QKEY:
                if (is_short_block(l.children)) {
                    indent(&l.children, lineno, tab + 1, width + 2);
                } else {
                    ++*lineno;
                    indent(&l.children, lineno, tab + 1, 0);
                }
                break;
            default:
                ++*lineno;
                indent(&l.children, lineno, tab + 1, 0);
                break;
        }
    }
}

static void format_tokens(
        const std::vector<line>& lines, pn::file_view out, int* lineno, int indent, int column) {
    for (const line& l : lines) {
        for (; *lineno < l.lineno; ++*lineno) {
            out.write('\n').check();
            indent = column = 0;
        }

        for (; indent < l.indent; ++indent) {
            out.write('\t').check();
            column = 0;
        }

        for (const token& token : l.tokens) {
            for (; column < token.column; ++column) {
                out.write(' ').check();
            }
            out.write(token.content).check();
            column += pn_str_width(token.content.data(), token.content.size());
        }

        format_tokens(l.children, out, lineno, indent, column);
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
}  // namespace pnfmt

int main(int argc, char* const* argv) {
    try {
        pnfmt::main(argc, argv);
    } catch (const std::exception& e) {
        pnfmt::print_exception(e);
        return 1;
    }
    return 0;
}
