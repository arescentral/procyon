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

#include <pn/procyon.h>

#include <gmock/gmock.h>
#include <limits>
#include <pn/array>
#include <pn/output>
#include <pn/string>

#include "../src/utf8.h"
#include "./matchers.hpp"

using Utf8Test = ::testing::Test;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ne;

namespace pntest {

TEST_F(Utf8Test, GetRune) {
    EXPECT_THAT(pn_rune("\0", 1, 0), Eq('\0'));
    EXPECT_THAT(pn_rune("A....", 5, 0), Eq('A'));
    EXPECT_THAT(pn_rune("\177....", 5, 0), Eq('\177'));
    EXPECT_THAT(pn_rune("\200....", 5, 0), Eq(0xFFFD));
    EXPECT_THAT(pn_rune("\277....", 5, 0), Eq(0xFFFD));
    EXPECT_THAT(pn_rune("\300....", 5, 0), Eq(0xFFFD));
    EXPECT_THAT(pn_rune("\302....", 5, 0), Eq(0xFFFD));
    EXPECT_THAT(pn_rune("\302\200...", 5, 0), Eq(0x80));
    EXPECT_THAT(pn_rune("\337\277...", 5, 0), Eq(0x7FF));
    EXPECT_THAT(pn_rune("\377\377...", 5, 0), Eq(0xFFFD));
}

TEST_F(Utf8Test, NextRune) {
    EXPECT_THAT(pn_rune_next("\0", 1, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("A....", 5, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\177....", 5, 0), Eq(1u));

    EXPECT_THAT(pn_rune_next("\302\200...", 5, 0), Eq(2u));
    EXPECT_THAT(pn_rune_next("\302\200...", 5, 1), Eq(2u));
    EXPECT_THAT(pn_rune_next("\337\277...", 5, 0), Eq(2u));
    EXPECT_THAT(pn_rune_next("\337\277...", 5, 1), Eq(2u));

    EXPECT_THAT(pn_rune_next("\364\217\277\277...", 5, 0), Eq(4u));
    EXPECT_THAT(pn_rune_next("\364\217\277\277...", 5, 1), Eq(2u));
    EXPECT_THAT(pn_rune_next("\364\217\277\277...", 5, 2), Eq(3u));
    EXPECT_THAT(pn_rune_next("\364\217\277\277...", 5, 3), Eq(4u));

    EXPECT_THAT(pn_rune_next("\200....", 5, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\277....", 5, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\300....", 5, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\302....", 5, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\302", 1, 0), Eq(1u));
    EXPECT_THAT(pn_rune_next("\377\377...", 5, 0), Eq(1u));
}

TEST_F(Utf8Test, PrevRune) {
    EXPECT_THAT(pn_rune_prev("\0", 1, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("A....", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\177....", 5, 1), Eq(0u));

    EXPECT_THAT(pn_rune_prev("\302\200...", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\302\200...", 5, 2), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\337\277...", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\337\277...", 5, 2), Eq(0u));

    EXPECT_THAT(pn_rune_prev("\364\217\277\277...", 5, 4), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\364\217\277\277...", 5, 2), Eq(1u));
    EXPECT_THAT(pn_rune_prev("\364\217\277\277...", 5, 3), Eq(2u));
    EXPECT_THAT(pn_rune_prev("\364\217\277\277...", 5, 1), Eq(0u));

    EXPECT_THAT(pn_rune_prev("\200....", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\277....", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\300....", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\302....", 5, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\302", 1, 1), Eq(0u));
    EXPECT_THAT(pn_rune_prev("\377\377...", 5, 1), Eq(0u));
}

static std::vector<uint32_t> runes(const char* s) {
    const size_t          size = strlen(s);
    std::vector<uint32_t> runes;
    for (size_t offset = 0; offset < size; offset = pn_rune_next(s, size, offset)) {
        runes.push_back(pn_rune(s, size, offset));
    }
    return runes;
}

static std::vector<uint32_t> rev_runes(const char* s) {
    const size_t          size = strlen(s);
    std::vector<uint32_t> runes;
    for (size_t offset = size; offset > 0;) {
        offset = pn_rune_prev(s, size, offset);
        runes.push_back(pn_rune(s, size, offset));
    }
    std::reverse(runes.begin(), runes.end());
    return runes;
}

TEST_F(Utf8Test, AllRunes) {
    EXPECT_THAT(runes(""), ElementsAre());
    EXPECT_THAT(rev_runes(""), ElementsAre());

    EXPECT_THAT(runes("1"), ElementsAre('1'));
    EXPECT_THAT(rev_runes("1"), ElementsAre('1'));

    EXPECT_THAT(runes("ASCII"), ElementsAre('A', 'S', 'C', 'I', 'I'));
    EXPECT_THAT(rev_runes("ASCII"), ElementsAre('A', 'S', 'C', 'I', 'I'));

    EXPECT_THAT(runes("\343\201\213\343\201\252"), ElementsAre(0x304b, 0x306a));
    EXPECT_THAT(rev_runes("\343\201\213\343\201\252"), ElementsAre(0x304b, 0x306a));

    // Invalid: '\377' can never occur in UTF-8.
    EXPECT_THAT(runes("\377"), ElementsAre(0xFFFD));
    EXPECT_THAT(rev_runes("\377"), ElementsAre(0xFFFD));

    // Invalid: continuation bytes without headers.
    EXPECT_THAT(
            runes("\200\200\200\200\200"), ElementsAre(0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD));
    EXPECT_THAT(
            rev_runes("\200\200\200\200\200"),
            ElementsAre(0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD));

    // Invalid: overlong encoding of '\0'.
    EXPECT_THAT(runes("\300\200"), ElementsAre(0xFFFD, 0xFFFD));
    EXPECT_THAT(rev_runes("\300\200"), ElementsAre(0xFFFD, 0xFFFD));
}

TEST_F(Utf8Test, RuneWidth) {
    // Single-column characters
    EXPECT_THAT(pn_rune_width(' '), Eq(1u));
    EXPECT_THAT(pn_rune_width('4'), Eq(1u));
    EXPECT_THAT(pn_rune_width('a'), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x0000E9 /* é */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x0003A9 /* Ω */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00044F /* я */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x0005D0 /* א */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x000634 /* ش */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x000CA0 /* ಠ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x002122 /* ™ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00215C /* ⅜ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x002192 /* → */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x002603 /* ☃ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00A729 /* ꜩ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00F8FF /*  */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00FB01 /* ﬁ */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x010020 /* 𐀠 */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x012009 /* 𒀉 */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x013011 /* 𓀑 */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x01D121 /* 𝄡 */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x10FFFF /* ? */), Eq(1u));

    EXPECT_THAT(pn_rune_width(0x00D800 /* Low surrogate code */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00DC00 /* High surrogate code */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00DFFF /* High surrogate code */), Eq(1u));
    EXPECT_THAT(pn_rune_width(0x00FFFD /* � */), Eq(1u));

    // Two-column characters
    EXPECT_THAT(pn_rune_width(0x001112 /* ᄒ */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x003000 /* 　 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x0030D4 /* ピ */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x003122 /* ㄢ */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x003277 /* ㉷ */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x003343 /* ㍃ */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x004E9E /* 亞 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x008FBA /* 辺 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x00908A /* 邊 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x00AC00 /* 가 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x020094 /* 𠂔 */), Eq(2u));

    // Emoji
    EXPECT_THAT(pn_rune_width(0x01F602 /* 😂 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x01F646 /* 🙆 */), Eq(2u));
    EXPECT_THAT(pn_rune_width(0x01F682 /* 🚂 */), Eq(2u));

    // Modifiers
    EXPECT_THAT(pn_rune_width(0x000302 /* COMBINING CIRCUMFLEX ACCENT */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x00032E /* COMBINING BREVE BELOW */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x00035C /* COMBINING DOUBLE BREVE BELOW */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x00036F /* COMBINING LATIN SMALL LETTER X */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x000940 /* DEVANAGARI VOWEL SIGN II */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x001DCF /* COMBINING ZIGZAG BELOW */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x001AB2 /* COMBINING INFINITY */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x002DFD /* COMBINING CYRILLIC LETTER LITTLE YUS */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x003099 /* COMBINING KATAKANA-HIRAGANA VOICED SOUND */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x00FE00 /* VARIATION SELECTOR-1 */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x01DA0F /* SIGNWRITING DREAMY EYEBROWS UP NEUTRAL */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x01D16E /* MUSICAL SYMBOL COMBINING FLAG-1 */), Eq(0u));
    EXPECT_THAT(pn_rune_width(0x0E0102 /* VARIATION SELECTOR-19 */), Eq(0u));

    // Invalid code points
    EXPECT_THAT(pn_rune_width(0x110000), Eq(1u));
    EXPECT_THAT(pn_rune_width(0xFFFFFF), Eq(1u));
    EXPECT_THAT(pn_rune_width(0xFFFFFFFF), Eq(1u));

    // Not really meaningful
    EXPECT_THAT(pn_rune_width('\0'), Eq(1u));
    EXPECT_THAT(pn_rune_width('\t'), Eq(1u));
    EXPECT_THAT(pn_rune_width('\n'), Eq(1u));
}

int str_width(pn::string_view s) { return pn_str_width(s.data(), s.size()); }

TEST_F(Utf8Test, StringWidthGreenTranslations) {
    EXPECT_THAT(str_width("green"), Eq(5));     // English
    EXPECT_THAT(str_width("ɡɹin"), Eq(4));      // English, US IPA
    EXPECT_THAT(str_width("berde"), Eq(5));     // Tagalog
    EXPECT_THAT(str_width("grøn"), Eq(4));      // Danish
    EXPECT_THAT(str_width("πράσινος"), Eq(8));  // Greek
    EXPECT_THAT(str_width("կանաչ"), Eq(5));     // Thai
    EXPECT_THAT(str_width("зелёный"), Eq(7));   // Russian
    EXPECT_THAT(str_width("สีเขียว"), Eq(5));     // Thai
    EXPECT_THAT(str_width("हरा"), Eq(2));       // Hindi
    EXPECT_THAT(str_width("綠色"), Eq(4));      // Chinese
    EXPECT_THAT(str_width("緑色"), Eq(4));      // Japanese, Kanji
    EXPECT_THAT(str_width("みどり"), Eq(6));    // Japanese, Hiragana
    EXPECT_THAT(str_width("グリーン"), Eq(8));  // Japanese, Katakana
    EXPECT_THAT(str_width("초록"), Eq(4));      // Korean

    EXPECT_THAT(str_width("أَخْضَر"), Eq(4));  // Arabic
}

TEST_F(Utf8Test, StringWidth) {
    EXPECT_THAT(str_width(""), Eq(0));
    EXPECT_THAT(str_width("simple"), Eq(6));
    EXPECT_THAT(str_width("1 2 3"), Eq(5));
    EXPECT_THAT(str_width("\303\251go\303\257ste"), Eq(7));    // égoïste (composed)
    EXPECT_THAT(str_width("e\314\201goi\314\210ste"), Eq(7));  // égoïste (decomposed)
    EXPECT_THAT(str_width("r̈öc̈k"), Eq(4));
    EXPECT_THAT(str_width("강남구"), Eq(6));
    EXPECT_THAT(str_width("⌚⌛⏰⏳"), Eq(8));
    EXPECT_THAT(str_width("☿♀♁♂♃♄♅♆"), Eq(8));
    EXPECT_THAT(str_width("♈♉♊♋♌♍♎♏♐♑♒♓"), Eq(24));
    EXPECT_THAT(str_width("hirak゙ana"), Eq(8));
    EXPECT_THAT(str_width("︵横︶"), Eq(6));
    EXPECT_THAT(str_width("Ｗｉｄｅ　Ｌｏａｄ"), Eq(18));
    EXPECT_THAT(str_width("ﾌｼｷﾞﾀﾞﾈ"), Eq(7));
    EXPECT_THAT(str_width("┌┴╫╜╞╝"), Eq(6));
    EXPECT_THAT(str_width("𝔹𝕠𝕝𝕕"), Eq(4));
    EXPECT_THAT(str_width("🏳️"), Eq(1));

    EXPECT_THAT(str_width("ç̌ë̄œ́3̊"), Eq(4));
    EXPECT_THAT(str_width("\314\201\314\201"), Eq(0));  // ¨¨

    // Broken tests:
    EXPECT_THAT(str_width("👩\342\200\215👩\342\200\215👧\342\200\215👦"), Ne(2));  // Family
    EXPECT_THAT(str_width("👩🏻\342\200\215🎤"), Ne(2));                  // Rocker
    EXPECT_THAT(str_width("🏳️\342\200\215🌈"), Ne(2));
}

// Invalid UTF-8 isn't allowed in Procyon files, but let's check it anyway.
TEST_F(Utf8Test, StringWidthInvalidUTF8) {
    EXPECT_THAT(str_width("\300\200"), Eq(2));
    EXPECT_THAT(str_width("\377\377\377\377"), Eq(4));
}

TEST_F(Utf8Test, Category) {
    enum Print {
        nothing   = 0,
        isrune    = (1 << 0),
        iscntrl   = (1 << 10),
        isprint   = (1 << 9),
        isnumeric = (1 << 1),
        isdigit   = (1 << 2),
        isalpha   = (1 << 3),
        isupper   = (1 << 4),
        islower   = (1 << 5),
        istitle   = (1 << 6),
        ispunct   = (1 << 7),
        isspace   = (1 << 8),

        all_space   = isspace,
        all_punct   = ispunct,
        all_title   = istitle,
        all_lower   = islower,
        all_upper   = isupper,
        all_alpha   = isalpha | all_upper | all_lower | all_title,
        all_digit   = isdigit,
        all_numeric = isnumeric | all_digit,
        all_alnum   = all_alpha | all_numeric,
        all_print   = isprint | all_alnum | all_punct | all_space,
        all_cntrl   = iscntrl,
        all_rune    = isrune | all_cntrl | all_print,
    };
    struct {
        const char category[3];
        size_t     width;
        int        flags;
        uint32_t   rune;
    } cases[] = {
            {"Cc", 1, iscntrl, u'\000'},   // (null)
            {"Cc", 1, iscntrl, u'\037'},   // INFORMATION SEPARATOR ONE
            {"Cc", 1, iscntrl, u'\177'},   // DELETE
            {"Cc", 1, iscntrl, u'\n'},     // LINE FEED
            {"Cc", 1, iscntrl, u'\t'},     // CHARACTER TABULATION
            {"Cn", 1, isrune, u'\x0378'},  // (unassigned)
            {"Co", 1, isrune, u'\xf8ff'},  // PRIVATE USE AREA-F8FF (Apple Logo)
            {"Cs", 1, isrune, u'\xd800'},  // (surrogate)

            {"Ll", 1, islower, u'p'},            // LATIN SMALL LETTER P
            {"Ll", 2, islower, u'ｐ'},           // FULLWIDTH LATIN SMALL LETTER P
            {"Lu", 1, isupper, u'P'},            // LATIN CAPITAL LETTER P
            {"Lu", 2, isupper, u'Ｐ'},           // FULLWIDTH LATIN CAPITAL LETTER P
            {"Lm", 1, isalpha, u'ᵖ'},            // MODIFIER LETTER SMALL P
            {"Lo", 1, isalpha, u'ª'},            // FEMININE ORDINAL INDICATOR
            {"Lt", 1, isalpha | istitle, u'ǈ'},  // LATIN CAPITAL LETTER L WITH SMALL LETTER J
            {"Lm", 2, isalpha, u'々'},           // IDEOGRAPHIC ITERATION MARK
            {"Lo", 2, isalpha, u'ᄇ'},           // HANGUL CHOSEONG PIEUP

            {"Mc", 0, isprint, u'\x0903'},  // DEVANAGARI SIGN VISARGA
            {"Mc", 0, isprint, u'\x302f'},  // HANGUL DOUBLE DOT TONE MARK
            {"Me", 0, isprint, u'\x0488'},  // COMBINING CYRILLIC HUNDRED THOUSANDS SIGN
            {"Mn", 0, isprint, u'\x0300'},  // COMBINING GRAVE ACCENT
            {"Mn", 0, isprint, u'\x309a'},  // COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK

            {"Nd", 1, isdigit, u'0'},     // DIGIT ZERO
            {"Nl", 1, isnumeric, u'ⅵ'},   // SMALL ROMAN NUMERAL SIX
            {"No", 1, isnumeric, u'½'},   // VULGAR FRATION ONE HALF
            {"Nd", 2, isdigit, u'０'},    // FULLWIDTH DIGIT ZERO
            {"Nl", 2, isnumeric, u'〇'},  // IDEOGRAPHIC NUMBER ZERO
            {"No", 2, isnumeric, u'㊅'},  // CIRCLED IDEOGRAPH SIX

            {"Pc", 1, ispunct, u'_'},   // LOW LINE (underscore)
            {"Pd", 1, ispunct, u'-'},   // HYPHEN-MINUS
            {"Pe", 1, ispunct, u')'},   // RIGHT PARENTHESIS
            {"Pf", 1, ispunct, u'”'},   // RIGHT DOUBLE QUOTATION MARK
            {"Pi", 1, ispunct, u'“'},   // LEFT DOUBLE QUOTATION MARK
            {"Po", 1, ispunct, u'!'},   // EXCLAMATION MARK
            {"Ps", 1, ispunct, u'('},   // LEFT PARENTHESIS
            {"Pc", 2, ispunct, u'＿'},  // FULLWIDTH
            {"Pd", 2, ispunct, u'〜'},  // WAVE DASH
            {"Pe", 2, ispunct, u'」'},  // RIGHT CORNER BRACKET
            {"Po", 2, ispunct, u'。'},  // IDEOGRAPHIC FULL STOP
            {"Ps", 2, ispunct, u'「'},  // LEFT CORNER BRACKET

            {"Sc", 1, isprint, u'$'},   // DOLLAR SIGN
            {"Sk", 1, isprint, u'^'},   // CIRCUMFLEX ACCENT
            {"Sm", 1, isprint, u'+'},   // PLUS SIGN
            {"So", 1, isprint, u'©'},   // COPYRIGHT SIGN
            {"Sc", 2, isprint, u'￥'},  // FULLWIDTH YEN SIGN
            {"Sk", 2, isprint, u'゜'},  // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
            {"Sm", 2, isprint, u'﹢'},  // SMALL PLUS SIGN
            {"So", 2, isprint, u'〒'},  // POSTAL MARK

            {"Zl", 1, isspace, u'\x2028'},  // LINE SEPARATOR
            {"Zp", 1, isspace, u'\x2029'},  // PARAGRAPH SEPARATOR
            {"Zs", 1, isspace, u' '},       // SPACE
            {"Zs", 1, isspace, u'\xa0'},    // NO-BREAK SPACE
            {"Zs", 2, isspace, u'\x3000'},  // IDEOGRAPHIC SPACE

            {"Xx", 1, 0, 0x110000},    // (not a code point)
            {"Xx", 1, 0, 0xffffffff},  // (not a code point)
    };
    for (const auto c : cases) {
        pn::rune r{c.rune};

        EXPECT_THAT(pn_rune_width(c.rune), Eq(c.width))
                << pn::format("pn_rune_width({}) ({})", pn::dump(r, pn::dump_short), c.category)
                           .c_str();

        EXPECT_THAT(pn_isrune(c.rune), Eq<bool>(c.flags & all_rune))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();

        EXPECT_THAT(pn_iscntrl(c.rune), Eq<bool>(c.flags & all_cntrl))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_isprint(c.rune), Eq<bool>(c.flags & all_print))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();

        EXPECT_THAT(pn_isalnum(c.rune), Eq<bool>(c.flags & all_alnum))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_isdigit(c.rune), Eq<bool>(c.flags & all_digit))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_isnumeric(c.rune), Eq<bool>(c.flags & all_numeric))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();

        EXPECT_THAT(pn_isalpha(c.rune), Eq<bool>(c.flags & all_alpha))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_isupper(c.rune), Eq<bool>(c.flags & all_upper))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_islower(c.rune), Eq<bool>(c.flags & all_lower))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_istitle(c.rune), Eq<bool>(c.flags & all_title))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();

        EXPECT_THAT(pn_ispunct(c.rune), Eq<bool>(c.flags & all_punct))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
        EXPECT_THAT(pn_isspace(c.rune), Eq<bool>(c.flags & all_space))
                << pn::dump(pn::array{r.copy(), c.category}, pn::dump_short).c_str();
    }
}

}  // namespace pntest
