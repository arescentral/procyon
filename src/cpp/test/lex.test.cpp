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

#include <procyon.h>

#include <gmock/gmock.h>

#include "../src/lex.h"
#include "./matchers.hpp"

using testing::Eq;
using testing::PrintToString;

namespace pntest {
namespace {

using LexTest = testing::Test;

std::ostream& operator<<(std::ostream& ostr, pn_token_type_t t) {
    switch (t) {
        case PN_TOK_ERROR: return ostr << "ERROR";
        case PN_TOK_LINE_IN: return ostr << "LINE+";
        case PN_TOK_LINE_EQ: return ostr << "LINE=";
        case PN_TOK_LINE_OUT: return ostr << "LINE-";

        case PN_TOK_STAR: return ostr << "'*'";
        case PN_TOK_COMMA: return ostr << "','";
        case PN_TOK_ARRAY_IN: return ostr << "'['";
        case PN_TOK_ARRAY_OUT: return ostr << "']'";
        case PN_TOK_MAP_IN: return ostr << "'{'";
        case PN_TOK_MAP_OUT: return ostr << "'}'";

        case PN_TOK_NULL: return ostr << "null";
        case PN_TOK_TRUE: return ostr << "true";
        case PN_TOK_FALSE: return ostr << "false";
        case PN_TOK_INF: return ostr << "inf";
        case PN_TOK_NEG_INF: return ostr << "-inf";
        case PN_TOK_NAN: return ostr << "nan";
        case PN_TOK_STR_WRAP_EMPTY: return ostr << ">";
        case PN_TOK_STR_PIPE_EMPTY: return ostr << "|";
        case PN_TOK_STR_BANG: return ostr << "!";

        case PN_TOK_KEY: return ostr << "KEY";
        case PN_TOK_QKEY: return ostr << "QKEY";
        case PN_TOK_INT: return ostr << "INT";
        case PN_TOK_FLOAT: return ostr << "FLOAT";
        case PN_TOK_DATA: return ostr << "DATA";
        case PN_TOK_STR: return ostr << "STR";
        case PN_TOK_STR_WRAP: return ostr << "STR>";
        case PN_TOK_STR_PIPE: return ostr << "STR|";
        case PN_TOK_COMMENT: return ostr << "COMMENT";
    }
}

struct Token {
    pn_token_type_t type;
    std::string     value;
    pn_error_t      error;
};
bool operator==(const Token& x, const Token& y) {
    return (x.type == y.type) && (x.value == y.value);
}

std::ostream& operator<<(std::ostream& ostr, Token tok) {
    ostr << tok.type;
    if (tok.type >= PN_TOK_KEY) {
        ostr << "(" << PrintToString(tok.value) << ")";
    }
    return ostr;
}

using Tokens = std::vector<Token>;
using Errors = std::vector<pn_error_t>;

Tokens lex(pn::string_view data) {
    pn::file   f = data.open();
    pn_lexer_t lex;
    pn_lexer_init(&lex, f.c_obj());

    Tokens tokens;
    size_t level = 0;
    while (true) {
        pn_error_t error;
        pn_lexer_next(&lex, &error);

        std::string content(lex.token.begin, lex.token.end - lex.token.begin);
        Token       token_with_content{lex.token.type, content, {}};
        Token       token_without_content{lex.token.type, std::string(), {}};

        switch (lex.token.type) {
            case PN_TOK_ERROR: tokens.push_back(Token{lex.token.type, content, error}); break;

            case PN_TOK_LINE_IN:
                tokens.push_back(token_without_content);
                ++level;
                break;

            case PN_TOK_LINE_OUT:
                tokens.push_back(token_without_content);
                if (--level == 0) {
                    pn_lexer_clear(&lex);
                    return tokens;
                }
                break;

            case PN_TOK_LINE_EQ:
            case PN_TOK_ARRAY_IN:
            case PN_TOK_MAP_IN:
            case PN_TOK_ARRAY_OUT:
            case PN_TOK_MAP_OUT:
            case PN_TOK_STAR:
            case PN_TOK_COMMA:
            case PN_TOK_STR_WRAP_EMPTY:
            case PN_TOK_STR_PIPE_EMPTY:
            case PN_TOK_STR_BANG: tokens.push_back(token_without_content); break;

            case PN_TOK_NULL:
            case PN_TOK_TRUE:
            case PN_TOK_FALSE:
            case PN_TOK_INF:
            case PN_TOK_NEG_INF:
            case PN_TOK_NAN:
            case PN_TOK_KEY:
            case PN_TOK_QKEY:
            case PN_TOK_INT:
            case PN_TOK_FLOAT:
            case PN_TOK_DATA:
            case PN_TOK_STR:
            case PN_TOK_STR_WRAP:
            case PN_TOK_STR_PIPE:
            case PN_TOK_COMMENT: tokens.push_back(token_with_content); break;
        }
    }
}

::testing::Matcher<const Tokens&> LexesTo(const Tokens& tokens) { return Eq(tokens); }

Token line_in   = {PN_TOK_LINE_IN, "", {}};
Token line_eq   = {PN_TOK_LINE_EQ, "", {}};
Token line_out  = {PN_TOK_LINE_OUT, "", {}};
Token star      = {PN_TOK_STAR, "", {}};
Token comma     = {PN_TOK_COMMA, "", {}};
Token array_in  = {PN_TOK_ARRAY_IN, "", {}};
Token array_out = {PN_TOK_ARRAY_OUT, "", {}};
Token map_in    = {PN_TOK_MAP_IN, "", {}};
Token map_out   = {PN_TOK_MAP_OUT, "", {}};
Token wrape     = {PN_TOK_STR_WRAP_EMPTY, "", {}};
Token pipee     = {PN_TOK_STR_PIPE_EMPTY, "", {}};
Token bang      = {PN_TOK_STR_BANG, "", {}};
Token null      = {PN_TOK_NULL, "null", {}};
Token true_     = {PN_TOK_TRUE, "true", {}};
Token false_    = {PN_TOK_FALSE, "false", {}};
Token inf       = {PN_TOK_INF, "inf", {}};
Token pos_inf   = {PN_TOK_INF, "+inf", {}};
Token neg_inf   = {PN_TOK_NEG_INF, "-inf", {}};
Token nan       = {PN_TOK_NAN, "nan", {}};

Token i(const std::string& i) { return Token{PN_TOK_INT, i, {}}; }
Token f(const std::string& f) { return Token{PN_TOK_FLOAT, f, {}}; }
Token key(const std::string& k) { return Token{PN_TOK_KEY, k, {}}; }
Token qkey(const std::string& k) { return Token{PN_TOK_QKEY, k, {}}; }
Token data(const std::string& data) { return Token{PN_TOK_DATA, data, {}}; }
Token str(const std::string& str) { return Token{PN_TOK_STR, str, {}}; }
Token wrap(const std::string& str) { return Token{PN_TOK_STR_WRAP, str, {}}; }
Token pipe(const std::string& str) { return Token{PN_TOK_STR_PIPE, str, {}}; }
Token comment(const std::string& comment) { return Token{PN_TOK_COMMENT, comment, {}}; }
Token error(const std::string& content, pn_error_t error) {
    return Token{PN_TOK_ERROR, content, error};
}

TEST_F(LexTest, CreateClear) {
    pn_lexer_t lex = {};
    pn_lexer_clear(&lex);  // OK to clear zeroed-out lexer.
    pn_lexer_init(&lex, NULL);
    pn_lexer_clear(&lex);
}

TEST_F(LexTest, Bad) {
    EXPECT_THAT(lex("&"), LexesTo({line_in, error("&", {PN_ERROR_BADCHAR, 1, 1}), line_out}));
    EXPECT_THAT(
            lex(std::string("\000", 1)),
            LexesTo({line_in, error(std::string(1, '\000'), {PN_ERROR_CTRL, 1, 1}), line_out}));

    EXPECT_THAT(
            lex("\001\n"
                "\037\n"
                "\177\n"
                "\310\n"),
            LexesTo({line_in, error("\001", {PN_ERROR_CTRL, 1, 1}), line_eq,
                     error("\037", {PN_ERROR_CTRL, 2, 1}), line_eq,
                     error("\177", {PN_ERROR_CTRL, 3, 1}), line_eq,
                     error("\310", {PN_ERROR_NONASCII, 4, 1}), line_out}));

    // Technically, these are control characters, but are handled separately.
    EXPECT_THAT(lex("\t"), LexesTo({line_in, line_out}));
    EXPECT_THAT(lex("\n"), LexesTo({line_in, line_out}));
}

TEST_F(LexTest, Indent) {
    EXPECT_THAT(lex("1"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1\n"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1\n\n"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("\n1"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("\n\n1"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("\n1\n"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("\n\n1\n\n"), LexesTo({line_in, i("1"), line_out}));

    EXPECT_THAT(lex("1\n2"), LexesTo({line_in, i("1"), line_eq, i("2"), line_out}));
    EXPECT_THAT(lex("1\n2\n"), LexesTo({line_in, i("1"), line_eq, i("2"), line_out}));
    EXPECT_THAT(lex("1\n\n2"), LexesTo({line_in, i("1"), line_eq, i("2"), line_out}));

    EXPECT_THAT(lex("  1\n"), LexesTo({line_in, i("1"), line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_out}));
    EXPECT_THAT(
            lex("1\n"
                "              2\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_out}));
    EXPECT_THAT(
            lex("1\n"
                "\t2\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "\t3\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_eq, i("3"), line_out, line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "    3\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_in, i("3"), line_out, line_out,
                     line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "    3\n"
                " \t4\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_in, i("3"), line_out, line_eq, i("4"),
                     line_out, line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "3\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_eq, i("3"), line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "3\n"
                "  4\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_eq, i("3"), line_in, i("4"),
                     line_out, line_out}));
    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "3\n"
                "        4\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out, line_eq, i("3"), line_in, i("4"),
                     line_out, line_out}));

    EXPECT_THAT(
            lex("1\n"
                "  2\n"
                "    3\n"
                "4\n"
                "5\n"
                "  6\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_in, i("3"), line_out, line_out,
                     line_eq, i("4"), line_eq, i("5"), line_in, i("6"), line_out, line_out}));

    EXPECT_THAT(
            lex("1\n"
                "    \n"
                "  2\n"
                "    3\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_in, i("3"), line_out, line_out,
                     line_out}));

    EXPECT_THAT(
            lex("1\n"
                "    2\n"
                "  3\n"),
            LexesTo({line_in, i("1"), line_in, i("2"), line_out,
                     error("  3", {PN_ERROR_OUTDENT, 3, 3}), line_out}));
}

TEST_F(LexTest, Constants) {
    EXPECT_THAT(lex("null"), LexesTo({line_in, null, line_out}));
    EXPECT_THAT(lex("true"), LexesTo({line_in, true_, line_out}));
    EXPECT_THAT(lex("false"), LexesTo({line_in, false_, line_out}));
    EXPECT_THAT(lex("inf"), LexesTo({line_in, inf, line_out}));
    EXPECT_THAT(lex("+inf"), LexesTo({line_in, pos_inf, line_out}));
    EXPECT_THAT(lex("-inf"), LexesTo({line_in, neg_inf, line_out}));
    EXPECT_THAT(lex("nan"), LexesTo({line_in, nan, line_out}));
}

TEST_F(LexTest, Words) {
    EXPECT_THAT(lex("1"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1 "), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1\t"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1\n"), LexesTo({line_in, i("1"), line_out}));

    EXPECT_THAT(lex("10"), LexesTo({line_in, i("10"), line_out}));
    EXPECT_THAT(lex("1_"), LexesTo({line_in, error("1_", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1/"), LexesTo({line_in, error("1/", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1-"), LexesTo({line_in, error("1-", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1+"), LexesTo({line_in, error("1+", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("-1"), LexesTo({line_in, i("-1"), line_out}));
    EXPECT_THAT(lex("+1"), LexesTo({line_in, i("+1"), line_out}));
    EXPECT_THAT(lex("1."), LexesTo({line_in, error("1.", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1e"), LexesTo({line_in, error("1e", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1f"), LexesTo({line_in, error("1f", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1x"), LexesTo({line_in, error("1x", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(
            lex("1\1"), LexesTo({line_in, i("1"), error("\1", {PN_ERROR_CTRL, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("1\177"),
            LexesTo({line_in, i("1"), error("\177", {PN_ERROR_CTRL, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("1\377"),
            LexesTo({line_in, i("1"), error("\377", {PN_ERROR_NONASCII, 1, 2}), line_out}));

    EXPECT_THAT(lex("1:"), LexesTo({line_in, key("1:"), line_out}));
    EXPECT_THAT(lex("1,"), LexesTo({line_in, i("1"), comma, line_out}));
    EXPECT_THAT(lex("1["), LexesTo({line_in, i("1"), array_in, line_out}));
    EXPECT_THAT(lex("1#"), LexesTo({line_in, i("1"), comment("#"), line_out}));
    EXPECT_THAT(lex("1$"), LexesTo({line_in, i("1"), data("$"), line_out}));

    EXPECT_THAT(lex("1.0"), LexesTo({line_in, f("1.0"), line_out}));
    EXPECT_THAT(lex("1e0"), LexesTo({line_in, f("1e0"), line_out}));
    EXPECT_THAT(lex("1e-"), LexesTo({line_in, error("1e-", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1e+"), LexesTo({line_in, error("1e+", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1e-0"), LexesTo({line_in, f("1e-0"), line_out}));
    EXPECT_THAT(lex("1e+0"), LexesTo({line_in, f("1e+0"), line_out}));

    EXPECT_THAT(lex("0"), LexesTo({line_in, i("0"), line_out}));
    EXPECT_THAT(lex("0."), LexesTo({line_in, error("0.", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("0.0"), LexesTo({line_in, f("0.0"), line_out}));
    EXPECT_THAT(lex("0e"), LexesTo({line_in, error("0e", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("0e0"), LexesTo({line_in, f("0e0"), line_out}));
    EXPECT_THAT(
            lex("01.0"), LexesTo({line_in, error("01.0", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(
            lex("01e0"), LexesTo({line_in, error("01e0", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("0x0"), LexesTo({line_in, error("0x0", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("0:"), LexesTo({line_in, key("0:"), line_out}));
    EXPECT_THAT(lex("-0"), LexesTo({line_in, i("-0"), line_out}));
    EXPECT_THAT(lex("+0"), LexesTo({line_in, i("+0"), line_out}));

    EXPECT_THAT(lex("1 1"), LexesTo({line_in, i("1"), i("1"), line_out}));
    EXPECT_THAT(lex("0 1 "), LexesTo({line_in, i("0"), i("1"), line_out}));

    EXPECT_THAT(lex("1"), LexesTo({line_in, i("1"), line_out}));
    EXPECT_THAT(lex("1."), LexesTo({line_in, error("1.", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1.1"), LexesTo({line_in, f("1.1"), line_out}));
    EXPECT_THAT(
            lex("1.1e"), LexesTo({line_in, error("1.1e", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1.1e1"), LexesTo({line_in, f("1.1e1"), line_out}));
    EXPECT_THAT(
            lex("1.e1"), LexesTo({line_in, error("1.e1", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("1e1"), LexesTo({line_in, f("1e1"), line_out}));
    EXPECT_THAT(lex("1e"), LexesTo({line_in, error("1e", {PN_ERROR_BADWORD, 1, 1}), line_out}));

    EXPECT_THAT(lex("1:"), LexesTo({line_in, key("1:"), line_out}));
    EXPECT_THAT(lex("1.:"), LexesTo({line_in, key("1.:"), line_out}));
    EXPECT_THAT(lex("1.1:"), LexesTo({line_in, key("1.1:"), line_out}));
    EXPECT_THAT(lex("1.1e:"), LexesTo({line_in, key("1.1e:"), line_out}));
    EXPECT_THAT(lex("1.1e1:"), LexesTo({line_in, key("1.1e1:"), line_out}));
    EXPECT_THAT(lex("1.e1:"), LexesTo({line_in, key("1.e1:"), line_out}));
    EXPECT_THAT(lex("1e1:"), LexesTo({line_in, key("1e1:"), line_out}));
    EXPECT_THAT(lex("1e:"), LexesTo({line_in, key("1e:"), line_out}));

    EXPECT_THAT(lex("+1"), LexesTo({line_in, i("+1"), line_out}));
    EXPECT_THAT(lex("+1."), LexesTo({line_in, error("+1.", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("+1.1"), LexesTo({line_in, f("+1.1"), line_out}));
    EXPECT_THAT(
            lex("+1.1e"), LexesTo({line_in, error("+1.1e", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("+1.1e1"), LexesTo({line_in, f("+1.1e1"), line_out}));
    EXPECT_THAT(
            lex("+1.e1"), LexesTo({line_in, error("+1.e1", {PN_ERROR_BADWORD, 1, 1}), line_out}));
    EXPECT_THAT(lex("+1e1"), LexesTo({line_in, f("+1e1"), line_out}));
    EXPECT_THAT(lex("+1e"), LexesTo({line_in, error("+1e", {PN_ERROR_BADWORD, 1, 1}), line_out}));

    EXPECT_THAT(lex("+1:"), LexesTo({line_in, key("+1:"), line_out}));
    EXPECT_THAT(lex("+1.:"), LexesTo({line_in, key("+1.:"), line_out}));
    EXPECT_THAT(lex("+1.1:"), LexesTo({line_in, key("+1.1:"), line_out}));
    EXPECT_THAT(lex("+1.1e:"), LexesTo({line_in, key("+1.1e:"), line_out}));
    EXPECT_THAT(lex("+1.1e1:"), LexesTo({line_in, key("+1.1e1:"), line_out}));
    EXPECT_THAT(lex("+1.e1:"), LexesTo({line_in, key("+1.e1:"), line_out}));
    EXPECT_THAT(lex("+1e1:"), LexesTo({line_in, key("+1e1:"), line_out}));
    EXPECT_THAT(lex("+1e:"), LexesTo({line_in, key("+1e:"), line_out}));
}

TEST_F(LexTest, Data) {
    EXPECT_THAT(lex("$"), LexesTo({line_in, data("$"), line_out}));
    EXPECT_THAT(lex("$abcd"), LexesTo({line_in, data("$abcd"), line_out}));
    EXPECT_THAT(lex("$ ab cd"), LexesTo({line_in, data("$ ab cd"), line_out}));
    EXPECT_THAT(
            lex("$ 01234567 89abcdef"), LexesTo({line_in, data("$ 01234567 89abcdef"), line_out}));
    EXPECT_THAT(
            lex("$ abcd\n"
                "$ 1234\n"),
            LexesTo({line_in, data("$ abcd"), line_eq, data("$ 1234"), line_out}));

    EXPECT_THAT(
            lex("[$, $1f, $ffff, $ 0f 1e 2d 3c]"),
            LexesTo({line_in, array_in, data("$"), comma, data("$1f"), comma, data("$ffff"), comma,
                     data("$ 0f 1e 2d 3c"), array_out, line_out}));

    EXPECT_THAT(
            lex("[$abcd\n"
                "$1234]\n"),
            LexesTo({line_in, array_in, data("$abcd"), line_eq, data("$1234"), array_out,
                     line_out}));

    EXPECT_THAT(lex("$a"), LexesTo({line_in, error("$a", {PN_ERROR_PARTIAL, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("$ a b c d"),
            LexesTo({line_in, error("$ a b c d", {PN_ERROR_PARTIAL, 1, 3}), line_out}));
    EXPECT_THAT(
            lex("$abcdefgh"),
            LexesTo({line_in, error("$abcdefgh", {PN_ERROR_DATACHAR, 1, 8}), line_out}));
}

TEST_F(LexTest, String) {
    EXPECT_THAT(lex("\"\""), LexesTo({line_in, str("\"\""), line_out}));
    EXPECT_THAT(lex("\"yo whaddup\""), LexesTo({line_in, str("\"yo whaddup\""), line_out}));
    EXPECT_THAT(
            lex("\"\\/\\\"\\\\\\b\\f\\n\\r\\t\""),
            LexesTo({line_in, str("\"\\/\\\"\\\\\\b\\f\\n\\r\\t\""), line_out}));
    EXPECT_THAT(
            lex("\"\\v\""),
            LexesTo({line_in, error("\"\\v\"", {PN_ERROR_BADESC, 1, 2}), line_out}));

    EXPECT_THAT(lex("\"\":"), LexesTo({line_in, qkey("\"\":"), line_out}));
    EXPECT_THAT(lex("\"yo whaddup\":"), LexesTo({line_in, qkey("\"yo whaddup\":"), line_out}));
    EXPECT_THAT(lex("\"\":\"\""), LexesTo({line_in, qkey("\"\":"), str("\"\""), line_out}));

    EXPECT_THAT(lex("\""), LexesTo({line_in, error("\"", {PN_ERROR_STREOL, 1, 2}), line_out}));
    EXPECT_THAT(lex("\"\\"), LexesTo({line_in, error("\"\\", {PN_ERROR_STREOL, 1, 3}), line_out}));
    EXPECT_THAT(
            lex("\"\\\""), LexesTo({line_in, error("\"\\\"", {PN_ERROR_STREOL, 1, 4}), line_out}));
    EXPECT_THAT(
            lex("\"\\u\""),
            LexesTo({line_in, error("\"\\u\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\\u0\""),
            LexesTo({line_in, error("\"\\u0\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\\u00\""),
            LexesTo({line_in, error("\"\\u00\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\\u000\""),
            LexesTo({line_in, error("\"\\u000\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(lex("\"\\u0000\""), LexesTo({line_in, str("\"\\u0000\""), line_out}));
    EXPECT_THAT(lex("\"\\u00000\""), LexesTo({line_in, str("\"\\u00000\""), line_out}));
    EXPECT_THAT(
            lex("\"\\u00000"),
            LexesTo({line_in, error("\"\\u00000", {PN_ERROR_STREOL, 1, 9}), line_out}));
    EXPECT_THAT(
            lex("\"\\u0000"),
            LexesTo({line_in, error("\"\\u0000", {PN_ERROR_STREOL, 1, 8}), line_out}));
    EXPECT_THAT(
            lex("\"\\u000"),
            LexesTo({line_in, error("\"\\u000", {PN_ERROR_STREOL, 1, 7}), line_out}));
    EXPECT_THAT(
            lex("\"\\u00"),
            LexesTo({line_in, error("\"\\u00", {PN_ERROR_STREOL, 1, 6}), line_out}));
    EXPECT_THAT(
            lex("\"\\u0"), LexesTo({line_in, error("\"\\u0", {PN_ERROR_STREOL, 1, 5}), line_out}));
    EXPECT_THAT(
            lex("\"\\u"), LexesTo({line_in, error("\"\\u", {PN_ERROR_STREOL, 1, 4}), line_out}));
    EXPECT_THAT(lex("\"\\"), LexesTo({line_in, error("\"\\", {PN_ERROR_STREOL, 1, 3}), line_out}));

    EXPECT_THAT(
            lex("\"\177\""),
            LexesTo({line_in, error("\"\177\"", {PN_ERROR_CTRL, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\200\""),
            LexesTo({line_in, error("\"\200\"", {PN_ERROR_UTF8_HEAD, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\377\""),
            LexesTo({line_in, error("\"\377\"", {PN_ERROR_UTF8_HEAD, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\302A\""),
            LexesTo({line_in, error("\"\302A\"", {PN_ERROR_UTF8_TAIL, 1, 3}), line_out}));

    EXPECT_THAT(
            lex("[\"a\", \"b\", \"c\"]"),
            LexesTo({line_in, array_in, str("\"a\""), comma, str("\"b\""), comma, str("\"c\""),
                     array_out, line_out}));
}

TEST_F(LexTest, StringSurrogates) {
    // Reject unescaped surrogates:
    EXPECT_THAT(lex("\"\355\237\277\""), LexesTo({line_in, str("\"\355\237\277\""), line_out}));
    EXPECT_THAT(
            lex("\"\355\240\200\""),
            LexesTo({line_in, error("\"\355\240\200\"", {PN_ERROR_UTF8_TAIL, 1, 3}), line_out}));
    EXPECT_THAT(
            lex("\"\355\277\277\""),
            LexesTo({line_in, error("\"\355\277\277\"", {PN_ERROR_UTF8_TAIL, 1, 3}), line_out}));
    EXPECT_THAT(lex("\"\356\200\200\""), LexesTo({line_in, str("\"\356\200\200\""), line_out}));

    // Reject escaped surrogates:
    EXPECT_THAT(lex("\"\\uD7FF\""), LexesTo({line_in, str("\"\\uD7FF\""), line_out}));
    EXPECT_THAT(
            lex("\"\\uD800\""),
            LexesTo({line_in, error("\"\\uD800\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(
            lex("\"\\uDFFF\""),
            LexesTo({line_in, error("\"\\uDFFF\"", {PN_ERROR_BADUESC, 1, 2}), line_out}));
    EXPECT_THAT(lex("\"\\uE000\""), LexesTo({line_in, str("\"\\uE000\""), line_out}));
}

TEST_F(LexTest, XString) {
    EXPECT_THAT(lex(">"), LexesTo({line_in, wrape, line_out}));
    EXPECT_THAT(lex("|"), LexesTo({line_in, pipee, line_out}));
    EXPECT_THAT(lex("!"), LexesTo({line_in, bang, line_out}));
    EXPECT_THAT(lex("> "), LexesTo({line_in, wrape, line_out}));
    EXPECT_THAT(lex("| "), LexesTo({line_in, pipee, line_out}));
    EXPECT_THAT(lex("! "), LexesTo({line_in, bang, line_out}));
    EXPECT_THAT(lex(">\t"), LexesTo({line_in, wrape, line_out}));
    EXPECT_THAT(lex("|\t"), LexesTo({line_in, pipee, line_out}));
    EXPECT_THAT(lex("!\t"), LexesTo({line_in, bang, line_out}));
    EXPECT_THAT(lex(">>\t"), LexesTo({line_in, wrap(">>\t"), line_out}));
    EXPECT_THAT(lex("||\t"), LexesTo({line_in, pipe("||\t"), line_out}));
    EXPECT_THAT(
            lex("!!\t"),
            LexesTo({line_in, error("!!\t", {PN_ERROR_BANG_SUFFIX, 1, 2}), line_out}));

    EXPECT_THAT(lex("!\n!\n"), LexesTo({line_in, bang, line_eq, bang, line_out}));

    EXPECT_THAT(lex("> one\n"), LexesTo({line_in, wrap("> one"), line_out}));
    EXPECT_THAT(lex("| one\n"), LexesTo({line_in, pipe("| one"), line_out}));
    EXPECT_THAT(
            lex("! one\n"),
            LexesTo({line_in, error("! one", {PN_ERROR_BANG_SUFFIX, 1, 3}), line_out}));
    EXPECT_THAT(
            lex("> one\n"
                "| two\n"
                "| three\n"
                "!\n"),
            LexesTo({line_in, wrap("> one"), line_eq, pipe("| two"), line_eq, pipe("| three"),
                     line_eq, bang, line_out}));
}

TEST_F(LexTest, XList) {
    EXPECT_THAT(lex("*"), LexesTo({line_in, star, line_out}));
    EXPECT_THAT(lex("**"), LexesTo({line_in, star, line_in, star, line_out, line_out}));
    EXPECT_THAT(
            lex("***"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_out, line_out, line_out}));
    EXPECT_THAT(
            lex("***0"), LexesTo({line_in, star, line_in, star, line_in, star, line_in, i("0"),
                                  line_out, line_out, line_out, line_out}));

    EXPECT_THAT(lex("* *"), LexesTo({line_in, star, line_in, star, line_out, line_out}));
    EXPECT_THAT(
            lex("* * *"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_out, line_out, line_out}));

    EXPECT_THAT(
            lex("*\n"
                "  *\n"
                "    *\n"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_out, line_out, line_out}));
    EXPECT_THAT(
            lex("*    \n"
                "  *  \n"
                "    *\n"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_out, line_out, line_out}));
    EXPECT_THAT(
            lex("***\n"
                " **\n"
                "  *\n"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_out, line_eq, star, line_in,
                     star, line_eq, star, line_out, line_out, line_out}));
    EXPECT_THAT(
            lex("* \t  *\t*\n"
                "        *\n"),
            LexesTo({line_in, star, line_in, star, line_in, star, line_eq, star, line_out,
                     line_out, line_out}));
}

TEST_F(LexTest, Map) {
    EXPECT_THAT(lex(":"), LexesTo({line_in, key(":"), line_out}));
    EXPECT_THAT(lex("0:"), LexesTo({line_in, key("0:"), line_out}));
    EXPECT_THAT(lex("a:"), LexesTo({line_in, key("a:"), line_out}));
    EXPECT_THAT(lex("+:"), LexesTo({line_in, key("+:"), line_out}));

    EXPECT_THAT(lex("1:1"), LexesTo({line_in, key("1:"), i("1"), line_out}));
    EXPECT_THAT(lex("1:  1"), LexesTo({line_in, key("1:"), i("1"), line_out}));
    EXPECT_THAT(lex("{1:1}"), LexesTo({line_in, map_in, key("1:"), i("1"), map_out, line_out}));
    EXPECT_THAT(lex("{1:  1}"), LexesTo({line_in, map_in, key("1:"), i("1"), map_out, line_out}));

    EXPECT_THAT(
            lex("1:2\n3:4"),
            LexesTo({line_in, key("1:"), i("2"), line_eq, key("3:"), i("4"), line_out}));
    EXPECT_THAT(
            lex("{1:2,3:4}"), LexesTo({line_in, map_in, key("1:"), i("2"), comma, key("3:"),
                                       i("4"), map_out, line_out}));
    EXPECT_THAT(
            lex("1:  2\n3:  4"),
            LexesTo({line_in, key("1:"), i("2"), line_eq, key("3:"), i("4"), line_out}));
    EXPECT_THAT(
            lex("{1: 2, 3: 4}"), LexesTo({line_in, map_in, key("1:"), i("2"), comma, key("3:"),
                                          i("4"), map_out, line_out}));
}

TEST_F(LexTest, Comment) {
    // Missing values
    EXPECT_THAT(lex("# comment"), LexesTo({line_in, comment("# comment"), line_out}));
    EXPECT_THAT(
            lex("* # comment"),
            LexesTo({line_in, star, line_in, comment("# comment"), line_out, line_out}));

    // These won't parse, but should lex.
    EXPECT_THAT(lex("true# comment"), LexesTo({line_in, true_, comment("# comment"), line_out}));
    EXPECT_THAT(lex("true # comment"), LexesTo({line_in, true_, comment("# comment"), line_out}));
    EXPECT_THAT(lex("1# comment"), LexesTo({line_in, i("1"), comment("# comment"), line_out}));
    EXPECT_THAT(lex("1 # comment"), LexesTo({line_in, i("1"), comment("# comment"), line_out}));
    EXPECT_THAT(
            lex("\"\"# comment"), LexesTo({line_in, str("\"\""), comment("# comment"), line_out}));
    EXPECT_THAT(
            lex("\"\" # comment"),
            LexesTo({line_in, str("\"\""), comment("# comment"), line_out}));
    EXPECT_THAT(
            lex("$00# comment"), LexesTo({line_in, data("$00"), comment("# comment"), line_out}));
    EXPECT_THAT(
            lex("$00 # comment"),
            LexesTo({line_in, data("$00 "), comment("# comment"), line_out}));
    EXPECT_THAT(lex("># comment"), LexesTo({line_in, wrap("># comment"), line_out}));
    EXPECT_THAT(lex("> # comment"), LexesTo({line_in, wrap("> # comment"), line_out}));
}

}  // namespace
}  // namespace pntest
