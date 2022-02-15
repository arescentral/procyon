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

// This file was adapted from dtoa.c and g_fmt.c by David M. Gay.

// The author of this software is David M. Gay.
//
// Copyright (c) 1991, 1996, 2000, 2001 by Lucent Technologies.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose without fee is hereby granted, provided that this entire notice
// is included in all copies of any software which is or includes a copy
// or modification of this software and in all copies of the supporting
// documentation for such software.
//
// THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
// WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
// REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
// OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

// Please send bug reports to David M. Gay (dmg at acm dot org,
// with " at " changed at "@" and " dot " changed to ".").

// On a machine with IEEE extended-precision registers, it is
// necessary to specify double-precision (53-bit) rounding precision
// before invoking strtod or dtoa.  If the machine uses (the equivalent
// of) Intel 80x87 arithmetic, the call
//      _control87(PC_53, MCW_PC);
// does this with many compilers.  Whether this or another call is
// appropriate depends on the compiler; for this to work, it may be
// necessary to #include "float.h" or another system-dependent header
// file.

// strtod for IEEE-arithmetic machines.
// (Note that IEEE arithmetic is disabled by gcc's -ffast-math flag.)
//
// This strtod returns a nearest machine number to the input decimal
// string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
// broken by the IEEE round-even rule.  Otherwise ties are broken by
// biased rounding (add half and chop).
//
// Inspired loosely by William D. Clinger's paper "How to Read Floating
// Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
//
// Modifications:
//
//      1. We only require IEEE double-precision
//         arithmetic (not IEEE double-extended).
//      2. We get by with floating-point arithmetic in a case that
//         Clinger missed -- when we're computing d * 10^n
//         for a small integer d and the integer n is not too
//         much larger than 22 (the maximum integer k for which
//         we can represent 10^k exactly), we may be able to
//         compute (d*10^k) * 10^(e-k) with just one roundoff.
//      3. than a bit-at-a-time adjustment of the binary
//         result in the hard case, we use floating-point
//         arithmetic to determine the adjustment to within
//         one bit; only in really hard cases do we need to
//         compute a second residual.
//      4. Because of 3., we don't need a large table of powers of 10
//         for ten-to-e (just some small tables, e.g. of 10^k
//         for 0 <= k <= 22).

//
// #define IEEE_8087 for IEEE-arithmetic machines where the least
//      significant byte has the lowest address.
// #define IEEE_MC68k for IEEE-arithmetic machines where the most
//      significant byte has the lowest address.

#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <pn/procyon.h>
#ifndef _MSC_VER
#include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>

#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define DTOA_IS_SSE2_ARCH 1
#include <immintrin.h>
#endif

#define IEEE_8087
#define Omit_Private_Memory

#ifdef DEBUG
#include <assert.h>
#include <stdio.h>
#define Bug(x)                      \
    {                               \
        fprintf(stderr, "%s\n", x); \
        exit(1);                    \
    }
#define Debug(x) x
int dtoa_stats[7];  // strtod_{64,96,bigcomp},dtoa_{exact,64,96,bigcomp}
#else
#define assert(x)  // nothing
#define Debug(x)   // nothing
#endif

#if defined(IEEE_8087) + defined(IEEE_MC68k) != 1
#error Exactly one of IEEE_8087 or IEEE_MC68k should be defined.
#endif

int dtoa_divmax = 2;  // Permit experimenting: on some systems, 64-bit integer
                      // division is slow enough that we may sometimes want to
                      // avoid using it.   We assume (but do not check) that
                      // dtoa_divmax <= 27.

typedef struct BF96 {     // Normalized 96-bit software floating point numbers
    uint32_t b0, b1, b2;  // b0 = most significant, binary point just to its left
    int      e;           // number represented = b * 2^e, with .5 <= b < 1
} BF96;

static BF96 pten[667] = {
        {0xeef453d6, 0x923bd65a, 0x113faa29, -1136}, {0x9558b466, 0x1b6565f8, 0x4ac7ca59, -1132},
        {0xbaaee17f, 0xa23ebf76, 0x5d79bcf0, -1129}, {0xe95a99df, 0x8ace6f53, 0xf4d82c2c, -1126},
        {0x91d8a02b, 0xb6c10594, 0x79071b9b, -1122}, {0xb64ec836, 0xa47146f9, 0x9748e282, -1119},
        {0xe3e27a44, 0x4d8d98b7, 0xfd1b1b23, -1116}, {0x8e6d8c6a, 0xb0787f72, 0xfe30f0f5, -1112},
        {0xb208ef85, 0x5c969f4f, 0xbdbd2d33, -1109}, {0xde8b2b66, 0xb3bc4723, 0xad2c7880, -1106},
        {0x8b16fb20, 0x3055ac76, 0x4c3bcb50, -1102}, {0xaddcb9e8, 0x3c6b1793, 0xdf4abe24, -1099},
        {0xd953e862, 0x4b85dd78, 0xd71d6dad, -1096}, {0x87d4713d, 0x6f33aa6b, 0x8672648c, -1092},
        {0xa9c98d8c, 0xcb009506, 0x680efdaf, -1089}, {0xd43bf0ef, 0xfdc0ba48, 0x0212bd1b, -1086},
        {0x84a57695, 0xfe98746d, 0x014bb630, -1082}, {0xa5ced43b, 0x7e3e9188, 0x419ea3bd, -1079},
        {0xcf42894a, 0x5dce35ea, 0x52064cac, -1076}, {0x818995ce, 0x7aa0e1b2, 0x7343efeb, -1072},
        {0xa1ebfb42, 0x19491a1f, 0x1014ebe6, -1069}, {0xca66fa12, 0x9f9b60a6, 0xd41a26e0, -1066},
        {0xfd00b897, 0x478238d0, 0x8920b098, -1063}, {0x9e20735e, 0x8cb16382, 0x55b46e5f, -1059},
        {0xc5a89036, 0x2fddbc62, 0xeb2189f7, -1056}, {0xf712b443, 0xbbd52b7b, 0xa5e9ec75, -1053},
        {0x9a6bb0aa, 0x55653b2d, 0x47b233c9, -1049}, {0xc1069cd4, 0xeabe89f8, 0x999ec0bb, -1046},
        {0xf148440a, 0x256e2c76, 0xc00670ea, -1043}, {0x96cd2a86, 0x5764dbca, 0x38040692, -1039},
        {0xbc807527, 0xed3e12bc, 0xc6050837, -1036}, {0xeba09271, 0xe88d976b, 0xf7864a44, -1033},
        {0x93445b87, 0x31587ea3, 0x7ab3ee6a, -1029}, {0xb8157268, 0xfdae9e4c, 0x5960ea05, -1026},
        {0xe61acf03, 0x3d1a45df, 0x6fb92487, -1023}, {0x8fd0c162, 0x06306bab, 0xa5d3b6d4, -1019},
        {0xb3c4f1ba, 0x87bc8696, 0x8f48a489, -1016}, {0xe0b62e29, 0x29aba83c, 0x331acdab, -1013},
        {0x8c71dcd9, 0xba0b4925, 0x9ff0c08b, -1009}, {0xaf8e5410, 0x288e1b6f, 0x07ecf0ae, -1006},
        {0xdb71e914, 0x32b1a24a, 0xc9e82cd9, -1003}, {0x892731ac, 0x9faf056e, 0xbe311c08, -999},
        {0xab70fe17, 0xc79ac6ca, 0x6dbd630a, -996},  {0xd64d3d9d, 0xb981787d, 0x092cbbcc, -993},
        {0x85f04682, 0x93f0eb4e, 0x25bbf560, -989},  {0xa76c5823, 0x38ed2621, 0xaf2af2b8, -986},
        {0xd1476e2c, 0x07286faa, 0x1af5af66, -983},  {0x82cca4db, 0x847945ca, 0x50d98d9f, -979},
        {0xa37fce12, 0x6597973c, 0xe50ff107, -976},  {0xcc5fc196, 0xfefd7d0c, 0x1e53ed49, -973},
        {0xff77b1fc, 0xbebcdc4f, 0x25e8e89c, -970},  {0x9faacf3d, 0xf73609b1, 0x77b19161, -966},
        {0xc795830d, 0x75038c1d, 0xd59df5b9, -963},  {0xf97ae3d0, 0xd2446f25, 0x4b057328, -960},
        {0x9becce62, 0x836ac577, 0x4ee367f9, -956},  {0xc2e801fb, 0x244576d5, 0x229c41f7, -953},
        {0xf3a20279, 0xed56d48a, 0x6b435275, -950},  {0x9845418c, 0x345644d6, 0x830a1389, -946},
        {0xbe5691ef, 0x416bd60c, 0x23cc986b, -943},  {0xedec366b, 0x11c6cb8f, 0x2cbfbe86, -940},
        {0x94b3a202, 0xeb1c3f39, 0x7bf7d714, -936},  {0xb9e08a83, 0xa5e34f07, 0xdaf5ccd9, -933},
        {0xe858ad24, 0x8f5c22c9, 0xd1b3400f, -930},  {0x91376c36, 0xd99995be, 0x23100809, -926},
        {0xb5854744, 0x8ffffb2d, 0xabd40a0c, -923},  {0xe2e69915, 0xb3fff9f9, 0x16c90c8f, -920},
        {0x8dd01fad, 0x907ffc3b, 0xae3da7d9, -916},  {0xb1442798, 0xf49ffb4a, 0x99cd11cf, -913},
        {0xdd95317f, 0x31c7fa1d, 0x40405643, -910},  {0x8a7d3eef, 0x7f1cfc52, 0x482835ea, -906},
        {0xad1c8eab, 0x5ee43b66, 0xda324365, -903},  {0xd863b256, 0x369d4a40, 0x90bed43e, -900},
        {0x873e4f75, 0xe2224e68, 0x5a7744a6, -896},  {0xa90de353, 0x5aaae202, 0x711515d0, -893},
        {0xd3515c28, 0x31559a83, 0x0d5a5b44, -890},  {0x8412d999, 0x1ed58091, 0xe858790a, -886},
        {0xa5178fff, 0x668ae0b6, 0x626e974d, -883},  {0xce5d73ff, 0x402d98e3, 0xfb0a3d21, -880},
        {0x80fa687f, 0x881c7f8e, 0x7ce66634, -876},  {0xa139029f, 0x6a239f72, 0x1c1fffc1, -873},
        {0xc9874347, 0x44ac874e, 0xa327ffb2, -870},  {0xfbe91419, 0x15d7a922, 0x4bf1ff9f, -867},
        {0x9d71ac8f, 0xada6c9b5, 0x6f773fc3, -863},  {0xc4ce17b3, 0x99107c22, 0xcb550fb4, -860},
        {0xf6019da0, 0x7f549b2b, 0x7e2a53a1, -857},  {0x99c10284, 0x4f94e0fb, 0x2eda7444, -853},
        {0xc0314325, 0x637a1939, 0xfa911155, -850},  {0xf03d93ee, 0xbc589f88, 0x793555ab, -847},
        {0x96267c75, 0x35b763b5, 0x4bc1558b, -843},  {0xbbb01b92, 0x83253ca2, 0x9eb1aaed, -840},
        {0xea9c2277, 0x23ee8bcb, 0x465e15a9, -837},  {0x92a1958a, 0x7675175f, 0x0bfacd89, -833},
        {0xb749faed, 0x14125d36, 0xcef980ec, -830},  {0xe51c79a8, 0x5916f484, 0x82b7e127, -827},
        {0x8f31cc09, 0x37ae58d2, 0xd1b2ecb8, -823},  {0xb2fe3f0b, 0x8599ef07, 0x861fa7e6, -820},
        {0xdfbdcece, 0x67006ac9, 0x67a791e0, -817},  {0x8bd6a141, 0x006042bd, 0xe0c8bb2c, -813},
        {0xaecc4991, 0x4078536d, 0x58fae9f7, -810},  {0xda7f5bf5, 0x90966848, 0xaf39a475, -807},
        {0x888f9979, 0x7a5e012d, 0x6d8406c9, -803},  {0xaab37fd7, 0xd8f58178, 0xc8e5087b, -800},
        {0xd5605fcd, 0xcf32e1d6, 0xfb1e4a9a, -797},  {0x855c3be0, 0xa17fcd26, 0x5cf2eea0, -793},
        {0xa6b34ad8, 0xc9dfc06f, 0xf42faa48, -790},  {0xd0601d8e, 0xfc57b08b, 0xf13b94da, -787},
        {0x823c1279, 0x5db6ce57, 0x76c53d08, -783},  {0xa2cb1717, 0xb52481ed, 0x54768c4b, -780},
        {0xcb7ddcdd, 0xa26da268, 0xa9942f5d, -777},  {0xfe5d5415, 0x0b090b02, 0xd3f93b35, -774},
        {0x9efa548d, 0x26e5a6e1, 0xc47bc501, -770},  {0xc6b8e9b0, 0x709f109a, 0x359ab641, -767},
        {0xf867241c, 0x8cc6d4c0, 0xc30163d2, -764},  {0x9b407691, 0xd7fc44f8, 0x79e0de63, -760},
        {0xc2109436, 0x4dfb5636, 0x985915fc, -757},  {0xf294b943, 0xe17a2bc4, 0x3e6f5b7b, -754},
        {0x979cf3ca, 0x6cec5b5a, 0xa705992c, -750},  {0xbd8430bd, 0x08277231, 0x50c6ff78, -747},
        {0xece53cec, 0x4a314ebd, 0xa4f8bf56, -744},  {0x940f4613, 0xae5ed136, 0x871b7795, -740},
        {0xb9131798, 0x99f68584, 0x28e2557b, -737},  {0xe757dd7e, 0xc07426e5, 0x331aeada, -734},
        {0x9096ea6f, 0x3848984f, 0x3ff0d2c8, -730},  {0xb4bca50b, 0x065abe63, 0x0fed077a, -727},
        {0xe1ebce4d, 0xc7f16dfb, 0xd3e84959, -724},  {0x8d3360f0, 0x9cf6e4bd, 0x64712dd7, -720},
        {0xb080392c, 0xc4349dec, 0xbd8d794d, -717},  {0xdca04777, 0xf541c567, 0xecf0d7a0, -714},
        {0x89e42caa, 0xf9491b60, 0xf41686c4, -710},  {0xac5d37d5, 0xb79b6239, 0x311c2875, -707},
        {0xd77485cb, 0x25823ac7, 0x7d633293, -704},  {0x86a8d39e, 0xf77164bc, 0xae5dff9c, -700},
        {0xa8530886, 0xb54dbdeb, 0xd9f57f83, -697},  {0xd267caa8, 0x62a12d66, 0xd072df63, -694},
        {0x8380dea9, 0x3da4bc60, 0x4247cb9e, -690},  {0xa4611653, 0x8d0deb78, 0x52d9be85, -687},
        {0xcd795be8, 0x70516656, 0x67902e27, -684},  {0x806bd971, 0x4632dff6, 0x00ba1cd8, -680},
        {0xa086cfcd, 0x97bf97f3, 0x80e8a40e, -677},  {0xc8a883c0, 0xfdaf7df0, 0x6122cd12, -674},
        {0xfad2a4b1, 0x3d1b5d6c, 0x796b8057, -671},  {0x9cc3a6ee, 0xc6311a63, 0xcbe33036, -667},
        {0xc3f490aa, 0x77bd60fc, 0xbedbfc44, -664},  {0xf4f1b4d5, 0x15acb93b, 0xee92fb55, -661},
        {0x99171105, 0x2d8bf3c5, 0x751bdd15, -657},  {0xbf5cd546, 0x78eef0b6, 0xd262d45a, -654},
        {0xef340a98, 0x172aace4, 0x86fb8971, -651},  {0x9580869f, 0x0e7aac0e, 0xd45d35e6, -647},
        {0xbae0a846, 0xd2195712, 0x89748360, -644},  {0xe998d258, 0x869facd7, 0x2bd1a438, -641},
        {0x91ff8377, 0x5423cc06, 0x7b6306a3, -637},  {0xb67f6455, 0x292cbf08, 0x1a3bc84c, -634},
        {0xe41f3d6a, 0x7377eeca, 0x20caba5f, -631},  {0x8e938662, 0x882af53e, 0x547eb47b, -627},
        {0xb23867fb, 0x2a35b28d, 0xe99e619a, -624},  {0xdec681f9, 0xf4c31f31, 0x6405fa00, -621},
        {0x8b3c113c, 0x38f9f37e, 0xde83bc40, -617},  {0xae0b158b, 0x4738705e, 0x9624ab50, -614},
        {0xd98ddaee, 0x19068c76, 0x3badd624, -611},  {0x87f8a8d4, 0xcfa417c9, 0xe54ca5d7, -607},
        {0xa9f6d30a, 0x038d1dbc, 0x5e9fcf4c, -604},  {0xd47487cc, 0x8470652b, 0x7647c320, -601},
        {0x84c8d4df, 0xd2c63f3b, 0x29ecd9f4, -597},  {0xa5fb0a17, 0xc777cf09, 0xf4681071, -594},
        {0xcf79cc9d, 0xb955c2cc, 0x7182148d, -591},  {0x81ac1fe2, 0x93d599bf, 0xc6f14cd8, -587},
        {0xa21727db, 0x38cb002f, 0xb8ada00e, -584},  {0xca9cf1d2, 0x06fdc03b, 0xa6d90811, -581},
        {0xfd442e46, 0x88bd304a, 0x908f4a16, -578},  {0x9e4a9cec, 0x15763e2e, 0x9a598e4e, -574},
        {0xc5dd4427, 0x1ad3cdba, 0x40eff1e1, -571},  {0xf7549530, 0xe188c128, 0xd12bee59, -568},
        {0x9a94dd3e, 0x8cf578b9, 0x82bb74f8, -564},  {0xc13a148e, 0x3032d6e7, 0xe36a5236, -561},
        {0xf18899b1, 0xbc3f8ca1, 0xdc44e6c3, -558},  {0x96f5600f, 0x15a7b7e5, 0x29ab103a, -554},
        {0xbcb2b812, 0xdb11a5de, 0x7415d448, -551},  {0xebdf6617, 0x91d60f56, 0x111b495b, -548},
        {0x936b9fce, 0xbb25c995, 0xcab10dd9, -544},  {0xb84687c2, 0x69ef3bfb, 0x3d5d514f, -541},
        {0xe65829b3, 0x046b0afa, 0x0cb4a5a3, -538},  {0x8ff71a0f, 0xe2c2e6dc, 0x47f0e785, -534},
        {0xb3f4e093, 0xdb73a093, 0x59ed2167, -531},  {0xe0f218b8, 0xd25088b8, 0x306869c1, -528},
        {0x8c974f73, 0x83725573, 0x1e414218, -524},  {0xafbd2350, 0x644eeacf, 0xe5d1929e, -521},
        {0xdbac6c24, 0x7d62a583, 0xdf45f746, -518},  {0x894bc396, 0xce5da772, 0x6b8bba8c, -514},
        {0xab9eb47c, 0x81f5114f, 0x066ea92f, -511},  {0xd686619b, 0xa27255a2, 0xc80a537b, -508},
        {0x8613fd01, 0x45877585, 0xbd06742c, -504},  {0xa798fc41, 0x96e952e7, 0x2c481138, -501},
        {0xd17f3b51, 0xfca3a7a0, 0xf75a1586, -498},  {0x82ef8513, 0x3de648c4, 0x9a984d73, -494},
        {0xa3ab6658, 0x0d5fdaf5, 0xc13e60d0, -491},  {0xcc963fee, 0x10b7d1b3, 0x318df905, -488},
        {0xffbbcfe9, 0x94e5c61f, 0xfdf17746, -485},  {0x9fd561f1, 0xfd0f9bd3, 0xfeb6ea8b, -481},
        {0xc7caba6e, 0x7c5382c8, 0xfe64a52e, -478},  {0xf9bd690a, 0x1b68637b, 0x3dfdce7a, -475},
        {0x9c1661a6, 0x51213e2d, 0x06bea10c, -471},  {0xc31bfa0f, 0xe5698db8, 0x486e494f, -468},
        {0xf3e2f893, 0xdec3f126, 0x5a89dba3, -465},  {0x986ddb5c, 0x6b3a76b7, 0xf8962946, -461},
        {0xbe895233, 0x86091465, 0xf6bbb397, -458},  {0xee2ba6c0, 0x678b597f, 0x746aa07d, -455},
        {0x94db4838, 0x40b717ef, 0xa8c2a44e, -451},  {0xba121a46, 0x50e4ddeb, 0x92f34d62, -448},
        {0xe896a0d7, 0xe51e1566, 0x77b020ba, -445},  {0x915e2486, 0xef32cd60, 0x0ace1474, -441},
        {0xb5b5ada8, 0xaaff80b8, 0x0d819992, -438},  {0xe3231912, 0xd5bf60e6, 0x10e1fff6, -435},
        {0x8df5efab, 0xc5979c8f, 0xca8d3ffa, -431},  {0xb1736b96, 0xb6fd83b3, 0xbd308ff8, -428},
        {0xddd0467c, 0x64bce4a0, 0xac7cb3f6, -425},  {0x8aa22c0d, 0xbef60ee4, 0x6bcdf07a, -421},
        {0xad4ab711, 0x2eb3929d, 0x86c16c98, -418},  {0xd89d64d5, 0x7a607744, 0xe871c7bf, -415},
        {0x87625f05, 0x6c7c4a8b, 0x11471cd7, -411},  {0xa93af6c6, 0xc79b5d2d, 0xd598e40d, -408},
        {0xd389b478, 0x79823479, 0x4aff1d10, -405},  {0x843610cb, 0x4bf160cb, 0xcedf722a, -401},
        {0xa54394fe, 0x1eedb8fe, 0xc2974eb4, -398},  {0xce947a3d, 0xa6a9273e, 0x733d2262, -395},
        {0x811ccc66, 0x8829b887, 0x0806357d, -391},  {0xa163ff80, 0x2a3426a8, 0xca07c2dc, -388},
        {0xc9bcff60, 0x34c13052, 0xfc89b393, -385},  {0xfc2c3f38, 0x41f17c67, 0xbbac2078, -382},
        {0x9d9ba783, 0x2936edc0, 0xd54b944b, -378},  {0xc5029163, 0xf384a931, 0x0a9e795e, -375},
        {0xf64335bc, 0xf065d37d, 0x4d4617b5, -372},  {0x99ea0196, 0x163fa42e, 0x504bced1, -368},
        {0xc06481fb, 0x9bcf8d39, 0xe45ec286, -365},  {0xf07da27a, 0x82c37088, 0x5d767327, -362},
        {0x964e858c, 0x91ba2655, 0x3a6a07f8, -358},  {0xbbe226ef, 0xb628afea, 0x890489f7, -355},
        {0xeadab0ab, 0xa3b2dbe5, 0x2b45ac74, -352},  {0x92c8ae6b, 0x464fc96f, 0x3b0b8bc9, -348},
        {0xb77ada06, 0x17e3bbcb, 0x09ce6ebb, -345},  {0xe5599087, 0x9ddcaabd, 0xcc420a6a, -342},
        {0x8f57fa54, 0xc2a9eab6, 0x9fa94682, -338},  {0xb32df8e9, 0xf3546564, 0x47939822, -335},
        {0xdff97724, 0x70297ebd, 0x59787e2b, -332},  {0x8bfbea76, 0xc619ef36, 0x57eb4edb, -328},
        {0xaefae514, 0x77a06b03, 0xede62292, -325},  {0xdab99e59, 0x958885c4, 0xe95fab36, -322},
        {0x88b402f7, 0xfd75539b, 0x11dbcb02, -318},  {0xaae103b5, 0xfcd2a881, 0xd652bdc2, -315},
        {0xd59944a3, 0x7c0752a2, 0x4be76d33, -312},  {0x857fcae6, 0x2d8493a5, 0x6f70a440, -308},
        {0xa6dfbd9f, 0xb8e5b88e, 0xcb4ccd50, -305},  {0xd097ad07, 0xa71f26b2, 0x7e2000a4, -302},
        {0x825ecc24, 0xc873782f, 0x8ed40066, -298},  {0xa2f67f2d, 0xfa90563b, 0x72890080, -295},
        {0xcbb41ef9, 0x79346bca, 0x4f2b40a0, -292},  {0xfea126b7, 0xd78186bc, 0xe2f610c8, -289},
        {0x9f24b832, 0xe6b0f436, 0x0dd9ca7d, -285},  {0xc6ede63f, 0xa05d3143, 0x91503d1c, -282},
        {0xf8a95fcf, 0x88747d94, 0x75a44c63, -279},  {0x9b69dbe1, 0xb548ce7c, 0xc986afbe, -275},
        {0xc24452da, 0x229b021b, 0xfbe85bad, -272},  {0xf2d56790, 0xab41c2a2, 0xfae27299, -269},
        {0x97c560ba, 0x6b0919a5, 0xdccd879f, -265},  {0xbdb6b8e9, 0x05cb600f, 0x5400e987, -262},
        {0xed246723, 0x473e3813, 0x290123e9, -259},  {0x9436c076, 0x0c86e30b, 0xf9a0b672, -255},
        {0xb9447093, 0x8fa89bce, 0xf808e40e, -252},  {0xe7958cb8, 0x7392c2c2, 0xb60b1d12, -249},
        {0x90bd77f3, 0x483bb9b9, 0xb1c6f22b, -245},  {0xb4ecd5f0, 0x1a4aa828, 0x1e38aeb6, -242},
        {0xe2280b6c, 0x20dd5232, 0x25c6da63, -239},  {0x8d590723, 0x948a535f, 0x579c487e, -235},
        {0xb0af48ec, 0x79ace837, 0x2d835a9d, -232},  {0xdcdb1b27, 0x98182244, 0xf8e43145, -229},
        {0x8a08f0f8, 0xbf0f156b, 0x1b8e9ecb, -225},  {0xac8b2d36, 0xeed2dac5, 0xe272467e, -222},
        {0xd7adf884, 0xaa879177, 0x5b0ed81d, -219},  {0x86ccbb52, 0xea94baea, 0x98e94712, -215},
        {0xa87fea27, 0xa539e9a5, 0x3f2398d7, -212},  {0xd29fe4b1, 0x8e88640e, 0x8eec7f0d, -209},
        {0x83a3eeee, 0xf9153e89, 0x1953cf68, -205},  {0xa48ceaaa, 0xb75a8e2b, 0x5fa8c342, -202},
        {0xcdb02555, 0x653131b6, 0x3792f412, -199},  {0x808e1755, 0x5f3ebf11, 0xe2bbd88b, -195},
        {0xa0b19d2a, 0xb70e6ed6, 0x5b6aceae, -192},  {0xc8de0475, 0x64d20a8b, 0xf245825a, -189},
        {0xfb158592, 0xbe068d2e, 0xeed6e2f0, -186},  {0x9ced737b, 0xb6c4183d, 0x55464dd6, -182},
        {0xc428d05a, 0xa4751e4c, 0xaa97e14c, -179},  {0xf5330471, 0x4d9265df, 0xd53dd99f, -176},
        {0x993fe2c6, 0xd07b7fab, 0xe546a803, -172},  {0xbf8fdb78, 0x849a5f96, 0xde985204, -169},
        {0xef73d256, 0xa5c0f77c, 0x963e6685, -166},  {0x95a86376, 0x27989aad, 0xdde70013, -162},
        {0xbb127c53, 0xb17ec159, 0x5560c018, -159},  {0xe9d71b68, 0x9dde71af, 0xaab8f01e, -156},
        {0x92267121, 0x62ab070d, 0xcab39613, -152},  {0xb6b00d69, 0xbb55c8d1, 0x3d607b97, -149},
        {0xe45c10c4, 0x2a2b3b05, 0x8cb89a7d, -146},  {0x8eb98a7a, 0x9a5b04e3, 0x77f3608e, -142},
        {0xb267ed19, 0x40f1c61c, 0x55f038b2, -139},  {0xdf01e85f, 0x912e37a3, 0x6b6c46de, -136},
        {0x8b61313b, 0xbabce2c6, 0x2323ac4b, -132},  {0xae397d8a, 0xa96c1b77, 0xabec975e, -129},
        {0xd9c7dced, 0x53c72255, 0x96e7bd35, -126},  {0x881cea14, 0x545c7575, 0x7e50d641, -122},
        {0xaa242499, 0x697392d2, 0xdde50bd1, -119},  {0xd4ad2dbf, 0xc3d07787, 0x955e4ec6, -116},
        {0x84ec3c97, 0xda624ab4, 0xbd5af13b, -112},  {0xa6274bbd, 0xd0fadd61, 0xecb1ad8a, -109},
        {0xcfb11ead, 0x453994ba, 0x67de18ed, -106},  {0x81ceb32c, 0x4b43fcf4, 0x80eacf94, -102},
        {0xa2425ff7, 0x5e14fc31, 0xa1258379, -99},   {0xcad2f7f5, 0x359a3b3e, 0x096ee458, -96},
        {0xfd87b5f2, 0x8300ca0d, 0x8bca9d6e, -93},   {0x9e74d1b7, 0x91e07e48, 0x775ea264, -89},
        {0xc6120625, 0x76589dda, 0x95364afe, -86},   {0xf79687ae, 0xd3eec551, 0x3a83ddbd, -83},
        {0x9abe14cd, 0x44753b52, 0xc4926a96, -79},   {0xc16d9a00, 0x95928a27, 0x75b7053c, -76},
        {0xf1c90080, 0xbaf72cb1, 0x5324c68b, -73},   {0x971da050, 0x74da7bee, 0xd3f6fc16, -69},
        {0xbce50864, 0x92111aea, 0x88f4bb1c, -66},   {0xec1e4a7d, 0xb69561a5, 0x2b31e9e3, -63},
        {0x9392ee8e, 0x921d5d07, 0x3aff322e, -59},   {0xb877aa32, 0x36a4b449, 0x09befeb9, -56},
        {0xe69594be, 0xc44de15b, 0x4c2ebe68, -53},   {0x901d7cf7, 0x3ab0acd9, 0x0f9d3701, -49},
        {0xb424dc35, 0x095cd80f, 0x538484c1, -46},   {0xe12e1342, 0x4bb40e13, 0x2865a5f2, -43},
        {0x8cbccc09, 0x6f5088cb, 0xf93f87b7, -39},   {0xafebff0b, 0xcb24aafe, 0xf78f69a5, -36},
        {0xdbe6fece, 0xbdedd5be, 0xb573440e, -33},   {0x89705f41, 0x36b4a597, 0x31680a88, -29},
        {0xabcc7711, 0x8461cefc, 0xfdc20d2b, -26},   {0xd6bf94d5, 0xe57a42bc, 0x3d329076, -23},
        {0x8637bd05, 0xaf6c69b5, 0xa63f9a49, -19},   {0xa7c5ac47, 0x1b478423, 0x0fcf80dc, -16},
        {0xd1b71758, 0xe219652b, 0xd3c36113, -13},   {0x83126e97, 0x8d4fdf3b, 0x645a1cac, -9},
        {0xa3d70a3d, 0x70a3d70a, 0x3d70a3d7, -6},    {0xcccccccc, 0xcccccccc, 0xcccccccc, -3},
        {0x80000000, 0x00000000, 0x00000000, 1},     {0xa0000000, 0x00000000, 0x00000000, 4},
        {0xc8000000, 0x00000000, 0x00000000, 7},     {0xfa000000, 0x00000000, 0x00000000, 10},
        {0x9c400000, 0x00000000, 0x00000000, 14},    {0xc3500000, 0x00000000, 0x00000000, 17},
        {0xf4240000, 0x00000000, 0x00000000, 20},    {0x98968000, 0x00000000, 0x00000000, 24},
        {0xbebc2000, 0x00000000, 0x00000000, 27},    {0xee6b2800, 0x00000000, 0x00000000, 30},
        {0x9502f900, 0x00000000, 0x00000000, 34},    {0xba43b740, 0x00000000, 0x00000000, 37},
        {0xe8d4a510, 0x00000000, 0x00000000, 40},    {0x9184e72a, 0x00000000, 0x00000000, 44},
        {0xb5e620f4, 0x80000000, 0x00000000, 47},    {0xe35fa931, 0xa0000000, 0x00000000, 50},
        {0x8e1bc9bf, 0x04000000, 0x00000000, 54},    {0xb1a2bc2e, 0xc5000000, 0x00000000, 57},
        {0xde0b6b3a, 0x76400000, 0x00000000, 60},    {0x8ac72304, 0x89e80000, 0x00000000, 64},
        {0xad78ebc5, 0xac620000, 0x00000000, 67},    {0xd8d726b7, 0x177a8000, 0x00000000, 70},
        {0x87867832, 0x6eac9000, 0x00000000, 74},    {0xa968163f, 0x0a57b400, 0x00000000, 77},
        {0xd3c21bce, 0xcceda100, 0x00000000, 80},    {0x84595161, 0x401484a0, 0x00000000, 84},
        {0xa56fa5b9, 0x9019a5c8, 0x00000000, 87},    {0xcecb8f27, 0xf4200f3a, 0x00000000, 90},
        {0x813f3978, 0xf8940984, 0x40000000, 94},    {0xa18f07d7, 0x36b90be5, 0x50000000, 97},
        {0xc9f2c9cd, 0x04674ede, 0xa4000000, 100},   {0xfc6f7c40, 0x45812296, 0x4d000000, 103},
        {0x9dc5ada8, 0x2b70b59d, 0xf0200000, 107},   {0xc5371912, 0x364ce305, 0x6c280000, 110},
        {0xf684df56, 0xc3e01bc6, 0xc7320000, 113},   {0x9a130b96, 0x3a6c115c, 0x3c7f4000, 117},
        {0xc097ce7b, 0xc90715b3, 0x4b9f1000, 120},   {0xf0bdc21a, 0xbb48db20, 0x1e86d400, 123},
        {0x96769950, 0xb50d88f4, 0x13144480, 127},   {0xbc143fa4, 0xe250eb31, 0x17d955a0, 130},
        {0xeb194f8e, 0x1ae525fd, 0x5dcfab08, 133},   {0x92efd1b8, 0xd0cf37be, 0x5aa1cae5, 137},
        {0xb7abc627, 0x050305ad, 0xf14a3d9e, 140},   {0xe596b7b0, 0xc643c719, 0x6d9ccd05, 143},
        {0x8f7e32ce, 0x7bea5c6f, 0xe4820023, 147},   {0xb35dbf82, 0x1ae4f38b, 0xdda2802c, 150},
        {0xe0352f62, 0xa19e306e, 0xd50b2037, 153},   {0x8c213d9d, 0xa502de45, 0x4526f422, 157},
        {0xaf298d05, 0x0e4395d6, 0x9670b12b, 160},   {0xdaf3f046, 0x51d47b4c, 0x3c0cdd76, 163},
        {0x88d8762b, 0xf324cd0f, 0xa5880a69, 167},   {0xab0e93b6, 0xefee0053, 0x8eea0d04, 170},
        {0xd5d238a4, 0xabe98068, 0x72a49045, 173},   {0x85a36366, 0xeb71f041, 0x47a6da2b, 177},
        {0xa70c3c40, 0xa64e6c51, 0x999090b6, 180},   {0xd0cf4b50, 0xcfe20765, 0xfff4b4e3, 183},
        {0x82818f12, 0x81ed449f, 0xbff8f10e, 187},   {0xa321f2d7, 0x226895c7, 0xaff72d52, 190},
        {0xcbea6f8c, 0xeb02bb39, 0x9bf4f8a6, 193},   {0xfee50b70, 0x25c36a08, 0x02f236d0, 196},
        {0x9f4f2726, 0x179a2245, 0x01d76242, 200},   {0xc722f0ef, 0x9d80aad6, 0x424d3ad2, 203},
        {0xf8ebad2b, 0x84e0d58b, 0xd2e08987, 206},   {0x9b934c3b, 0x330c8577, 0x63cc55f4, 210},
        {0xc2781f49, 0xffcfa6d5, 0x3cbf6b71, 213},   {0xf316271c, 0x7fc3908a, 0x8bef464e, 216},
        {0x97edd871, 0xcfda3a56, 0x97758bf0, 220},   {0xbde94e8e, 0x43d0c8ec, 0x3d52eeed, 223},
        {0xed63a231, 0xd4c4fb27, 0x4ca7aaa8, 226},   {0x945e455f, 0x24fb1cf8, 0x8fe8caa9, 230},
        {0xb975d6b6, 0xee39e436, 0xb3e2fd53, 233},   {0xe7d34c64, 0xa9c85d44, 0x60dbbca8, 236},
        {0x90e40fbe, 0xea1d3a4a, 0xbc8955e9, 240},   {0xb51d13ae, 0xa4a488dd, 0x6babab63, 243},
        {0xe264589a, 0x4dcdab14, 0xc696963c, 246},   {0x8d7eb760, 0x70a08aec, 0xfc1e1de5, 250},
        {0xb0de6538, 0x8cc8ada8, 0x3b25a55f, 253},   {0xdd15fe86, 0xaffad912, 0x49ef0eb7, 256},
        {0x8a2dbf14, 0x2dfcc7ab, 0x6e356932, 260},   {0xacb92ed9, 0x397bf996, 0x49c2c37f, 263},
        {0xd7e77a8f, 0x87daf7fb, 0xdc33745e, 266},   {0x86f0ac99, 0xb4e8dafd, 0x69a028bb, 270},
        {0xa8acd7c0, 0x222311bc, 0xc40832ea, 273},   {0xd2d80db0, 0x2aabd62b, 0xf50a3fa4, 276},
        {0x83c7088e, 0x1aab65db, 0x792667c6, 280},   {0xa4b8cab1, 0xa1563f52, 0x577001b8, 283},
        {0xcde6fd5e, 0x09abcf26, 0xed4c0226, 286},   {0x80b05e5a, 0xc60b6178, 0x544f8158, 290},
        {0xa0dc75f1, 0x778e39d6, 0x696361ae, 293},   {0xc913936d, 0xd571c84c, 0x03bc3a19, 296},
        {0xfb587849, 0x4ace3a5f, 0x04ab48a0, 299},   {0x9d174b2d, 0xcec0e47b, 0x62eb0d64, 303},
        {0xc45d1df9, 0x42711d9a, 0x3ba5d0bd, 306},   {0xf5746577, 0x930d6500, 0xca8f44ec, 309},
        {0x9968bf6a, 0xbbe85f20, 0x7e998b13, 313},   {0xbfc2ef45, 0x6ae276e8, 0x9e3fedd8, 316},
        {0xefb3ab16, 0xc59b14a2, 0xc5cfe94e, 319},   {0x95d04aee, 0x3b80ece5, 0xbba1f1d1, 323},
        {0xbb445da9, 0xca61281f, 0x2a8a6e45, 326},   {0xea157514, 0x3cf97226, 0xf52d09d7, 329},
        {0x924d692c, 0xa61be758, 0x593c2626, 333},   {0xb6e0c377, 0xcfa2e12e, 0x6f8b2fb0, 336},
        {0xe498f455, 0xc38b997a, 0x0b6dfb9c, 339},   {0x8edf98b5, 0x9a373fec, 0x4724bd41, 343},
        {0xb2977ee3, 0x00c50fe7, 0x58edec91, 346},   {0xdf3d5e9b, 0xc0f653e1, 0x2f2967b6, 349},
        {0x8b865b21, 0x5899f46c, 0xbd79e0d2, 353},   {0xae67f1e9, 0xaec07187, 0xecd85906, 356},
        {0xda01ee64, 0x1a708de9, 0xe80e6f48, 359},   {0x884134fe, 0x908658b2, 0x3109058d, 363},
        {0xaa51823e, 0x34a7eede, 0xbd4b46f0, 366},   {0xd4e5e2cd, 0xc1d1ea96, 0x6c9e18ac, 369},
        {0x850fadc0, 0x9923329e, 0x03e2cf6b, 373},   {0xa6539930, 0xbf6bff45, 0x84db8346, 376},
        {0xcfe87f7c, 0xef46ff16, 0xe6126418, 379},   {0x81f14fae, 0x158c5f6e, 0x4fcb7e8f, 383},
        {0xa26da399, 0x9aef7749, 0xe3be5e33, 386},   {0xcb090c80, 0x01ab551c, 0x5cadf5bf, 389},
        {0xfdcb4fa0, 0x02162a63, 0x73d9732f, 392},   {0x9e9f11c4, 0x014dda7e, 0x2867e7fd, 396},
        {0xc646d635, 0x01a1511d, 0xb281e1fd, 399},   {0xf7d88bc2, 0x4209a565, 0x1f225a7c, 402},
        {0x9ae75759, 0x6946075f, 0x3375788d, 406},   {0xc1a12d2f, 0xc3978937, 0x0052d6b1, 409},
        {0xf209787b, 0xb47d6b84, 0xc0678c5d, 412},   {0x9745eb4d, 0x50ce6332, 0xf840b7ba, 416},
        {0xbd176620, 0xa501fbff, 0xb650e5a9, 419},   {0xec5d3fa8, 0xce427aff, 0xa3e51f13, 422},
        {0x93ba47c9, 0x80e98cdf, 0xc66f336c, 426},   {0xb8a8d9bb, 0xe123f017, 0xb80b0047, 429},
        {0xe6d3102a, 0xd96cec1d, 0xa60dc059, 432},   {0x9043ea1a, 0xc7e41392, 0x87c89837, 436},
        {0xb454e4a1, 0x79dd1877, 0x29babe45, 439},   {0xe16a1dc9, 0xd8545e94, 0xf4296dd6, 442},
        {0x8ce2529e, 0x2734bb1d, 0x1899e4a6, 446},   {0xb01ae745, 0xb101e9e4, 0x5ec05dcf, 449},
        {0xdc21a117, 0x1d42645d, 0x76707543, 452},   {0x899504ae, 0x72497eba, 0x6a06494a, 456},
        {0xabfa45da, 0x0edbde69, 0x0487db9d, 459},   {0xd6f8d750, 0x9292d603, 0x45a9d284, 462},
        {0x865b8692, 0x5b9bc5c2, 0x0b8a2392, 466},   {0xa7f26836, 0xf282b732, 0x8e6cac77, 469},
        {0xd1ef0244, 0xaf2364ff, 0x3207d795, 472},   {0x8335616a, 0xed761f1f, 0x7f44e6bd, 476},
        {0xa402b9c5, 0xa8d3a6e7, 0x5f16206c, 479},   {0xcd036837, 0x130890a1, 0x36dba887, 482},
        {0x80222122, 0x6be55a64, 0xc2494954, 486},   {0xa02aa96b, 0x06deb0fd, 0xf2db9baa, 489},
        {0xc83553c5, 0xc8965d3d, 0x6f928294, 492},   {0xfa42a8b7, 0x3abbf48c, 0xcb772339, 495},
        {0x9c69a972, 0x84b578d7, 0xff2a7604, 499},   {0xc38413cf, 0x25e2d70d, 0xfef51385, 502},
        {0xf46518c2, 0xef5b8cd1, 0x7eb25866, 505},   {0x98bf2f79, 0xd5993802, 0xef2f773f, 509},
        {0xbeeefb58, 0x4aff8603, 0xaafb550f, 512},   {0xeeaaba2e, 0x5dbf6784, 0x95ba2a53, 515},
        {0x952ab45c, 0xfa97a0b2, 0xdd945a74, 519},   {0xba756174, 0x393d88df, 0x94f97111, 522},
        {0xe912b9d1, 0x478ceb17, 0x7a37cd56, 525},   {0x91abb422, 0xccb812ee, 0xac62e055, 529},
        {0xb616a12b, 0x7fe617aa, 0x577b986b, 532},   {0xe39c4976, 0x5fdf9d94, 0xed5a7e85, 535},
        {0x8e41ade9, 0xfbebc27d, 0x14588f13, 539},   {0xb1d21964, 0x7ae6b31c, 0x596eb2d8, 542},
        {0xde469fbd, 0x99a05fe3, 0x6fca5f8e, 545},   {0x8aec23d6, 0x80043bee, 0x25de7bb9, 549},
        {0xada72ccc, 0x20054ae9, 0xaf561aa7, 552},   {0xd910f7ff, 0x28069da4, 0x1b2ba151, 555},
        {0x87aa9aff, 0x79042286, 0x90fb44d2, 559},   {0xa99541bf, 0x57452b28, 0x353a1607, 562},
        {0xd3fa922f, 0x2d1675f2, 0x42889b89, 565},   {0x847c9b5d, 0x7c2e09b7, 0x69956135, 569},
        {0xa59bc234, 0xdb398c25, 0x43fab983, 572},   {0xcf02b2c2, 0x1207ef2e, 0x94f967e4, 575},
        {0x8161afb9, 0x4b44f57d, 0x1d1be0ee, 579},   {0xa1ba1ba7, 0x9e1632dc, 0x6462d92a, 582},
        {0xca28a291, 0x859bbf93, 0x7d7b8f75, 585},   {0xfcb2cb35, 0xe702af78, 0x5cda7352, 588},
        {0x9defbf01, 0xb061adab, 0x3a088813, 592},   {0xc56baec2, 0x1c7a1916, 0x088aaa18, 595},
        {0xf6c69a72, 0xa3989f5b, 0x8aad549e, 598},   {0x9a3c2087, 0xa63f6399, 0x36ac54e2, 602},
        {0xc0cb28a9, 0x8fcf3c7f, 0x84576a1b, 605},   {0xf0fdf2d3, 0xf3c30b9f, 0x656d44a2, 608},
        {0x969eb7c4, 0x7859e743, 0x9f644ae5, 612},   {0xbc4665b5, 0x96706114, 0x873d5d9f, 615},
        {0xeb57ff22, 0xfc0c7959, 0xa90cb506, 618},   {0x9316ff75, 0xdd87cbd8, 0x09a7f124, 622},
        {0xb7dcbf53, 0x54e9bece, 0x0c11ed6d, 625},   {0xe5d3ef28, 0x2a242e81, 0x8f1668c8, 628},
        {0x8fa47579, 0x1a569d10, 0xf96e017d, 632},   {0xb38d92d7, 0x60ec4455, 0x37c981dc, 635},
        {0xe070f78d, 0x3927556a, 0x85bbe253, 638},   {0x8c469ab8, 0x43b89562, 0x93956d74, 642},
        {0xaf584166, 0x54a6babb, 0x387ac8d1, 645},   {0xdb2e51bf, 0xe9d0696a, 0x06997b05, 648},
        {0x88fcf317, 0xf22241e2, 0x441fece3, 652},   {0xab3c2fdd, 0xeeaad25a, 0xd527e81c, 655},
        {0xd60b3bd5, 0x6a5586f1, 0x8a71e223, 658},   {0x85c70565, 0x62757456, 0xf6872d56, 662},
        {0xa738c6be, 0xbb12d16c, 0xb428f8ac, 665},   {0xd106f86e, 0x69d785c7, 0xe13336d7, 668},
        {0x82a45b45, 0x0226b39c, 0xecc00246, 672},   {0xa34d7216, 0x42b06084, 0x27f002d7, 675},
        {0xcc20ce9b, 0xd35c78a5, 0x31ec038d, 678},   {0xff290242, 0xc83396ce, 0x7e670471, 681},
        {0x9f79a169, 0xbd203e41, 0x0f0062c6, 685},   {0xc75809c4, 0x2c684dd1, 0x52c07b78, 688},
        {0xf92e0c35, 0x37826145, 0xa7709a56, 691},   {0x9bbcc7a1, 0x42b17ccb, 0x88a66076, 695},
        {0xc2abf989, 0x935ddbfe, 0x6acff893, 698},   {0xf356f7eb, 0xf83552fe, 0x0583f6b8, 701},
        {0x98165af3, 0x7b2153de, 0xc3727a33, 705},   {0xbe1bf1b0, 0x59e9a8d6, 0x744f18c0, 708},
        {0xeda2ee1c, 0x7064130c, 0x1162def0, 711},   {0x9485d4d1, 0xc63e8be7, 0x8addcb56, 715},
        {0xb9a74a06, 0x37ce2ee1, 0x6d953e2b, 718},   {0xe8111c87, 0xc5c1ba99, 0xc8fa8db6, 721},
        {0x910ab1d4, 0xdb9914a0, 0x1d9c9892, 725},   {0xb54d5e4a, 0x127f59c8, 0x2503beb6, 728},
        {0xe2a0b5dc, 0x971f303a, 0x2e44ae64, 731},   {0x8da471a9, 0xde737e24, 0x5ceaecfe, 735},
        {0xb10d8e14, 0x56105dad, 0x7425a83e, 738},   {0xdd50f199, 0x6b947518, 0xd12f124e, 741},
        {0x8a5296ff, 0xe33cc92f, 0x82bd6b70, 745},   {0xace73cbf, 0xdc0bfb7b, 0x636cc64d, 748},
        {0xd8210bef, 0xd30efa5a, 0x3c47f7e0, 751},   {0x8714a775, 0xe3e95c78, 0x65acfaec, 755},
        {0xa8d9d153, 0x5ce3b396, 0x7f1839a7, 758},   {0xd31045a8, 0x341ca07c, 0x1ede4811, 761},
        {0x83ea2b89, 0x2091e44d, 0x934aed0a, 765},   {0xa4e4b66b, 0x68b65d60, 0xf81da84d, 768},
        {0xce1de406, 0x42e3f4b9, 0x36251260, 771},   {0x80d2ae83, 0xe9ce78f3, 0xc1d72b7c, 775},
        {0xa1075a24, 0xe4421730, 0xb24cf65b, 778},   {0xc94930ae, 0x1d529cfc, 0xdee033f2, 781},
        {0xfb9b7cd9, 0xa4a7443c, 0x169840ef, 784},   {0x9d412e08, 0x06e88aa5, 0x8e1f2895, 788},
        {0xc491798a, 0x08a2ad4e, 0xf1a6f2ba, 791},   {0xf5b5d7ec, 0x8acb58a2, 0xae10af69, 794},
        {0x9991a6f3, 0xd6bf1765, 0xacca6da1, 798},   {0xbff610b0, 0xcc6edd3f, 0x17fd090a, 801},
        {0xeff394dc, 0xff8a948e, 0xddfc4b4c, 804},   {0x95f83d0a, 0x1fb69cd9, 0x4abdaf10, 808},
        {0xbb764c4c, 0xa7a4440f, 0x9d6d1ad4, 811},   {0xea53df5f, 0xd18d5513, 0x84c86189, 814},
        {0x92746b9b, 0xe2f8552c, 0x32fd3cf5, 818},   {0xb7118682, 0xdbb66a77, 0x3fbc8c33, 821},
        {0xe4d5e823, 0x92a40515, 0x0fabaf3f, 824},   {0x8f05b116, 0x3ba6832d, 0x29cb4d87, 828},
        {0xb2c71d5b, 0xca9023f8, 0x743e20e9, 831},   {0xdf78e4b2, 0xbd342cf6, 0x914da924, 834},
        {0x8bab8eef, 0xb6409c1a, 0x1ad089b6, 838},   {0xae9672ab, 0xa3d0c320, 0xa184ac24, 841},
        {0xda3c0f56, 0x8cc4f3e8, 0xc9e5d72d, 844},   {0x88658996, 0x17fb1871, 0x7e2fa67c, 848},
        {0xaa7eebfb, 0x9df9de8d, 0xddbb901b, 851},   {0xd51ea6fa, 0x85785631, 0x552a7422, 854},
        {0x8533285c, 0x936b35de, 0xd53a8895, 858},   {0xa67ff273, 0xb8460356, 0x8a892aba, 861},
        {0xd01fef10, 0xa657842c, 0x2d2b7569, 864},   {0x8213f56a, 0x67f6b29b, 0x9c3b2962, 868},
        {0xa298f2c5, 0x01f45f42, 0x8349f3ba, 871},   {0xcb3f2f76, 0x42717713, 0x241c70a9, 874},
        {0xfe0efb53, 0xd30dd4d7, 0xed238cd3, 877},   {0x9ec95d14, 0x63e8a506, 0xf4363804, 881},
        {0xc67bb459, 0x7ce2ce48, 0xb143c605, 884},   {0xf81aa16f, 0xdc1b81da, 0xdd94b786, 887},
        {0x9b10a4e5, 0xe9913128, 0xca7cf2b4, 891},   {0xc1d4ce1f, 0x63f57d72, 0xfd1c2f61, 894},
        {0xf24a01a7, 0x3cf2dccf, 0xbc633b39, 897},   {0x976e4108, 0x8617ca01, 0xd5be0503, 901},
        {0xbd49d14a, 0xa79dbc82, 0x4b2d8644, 904},   {0xec9c459d, 0x51852ba2, 0xddf8e7d6, 907},
        {0x93e1ab82, 0x52f33b45, 0xcabb90e5, 911},   {0xb8da1662, 0xe7b00a17, 0x3d6a751f, 914},
        {0xe7109bfb, 0xa19c0c9d, 0x0cc51267, 917},   {0x906a617d, 0x450187e2, 0x27fb2b80, 921},
        {0xb484f9dc, 0x9641e9da, 0xb1f9f660, 924},   {0xe1a63853, 0xbbd26451, 0x5e7873f8, 927},
        {0x8d07e334, 0x55637eb2, 0xdb0b487b, 931},   {0xb049dc01, 0x6abc5e5f, 0x91ce1a9a, 934},
        {0xdc5c5301, 0xc56b75f7, 0x7641a140, 937},   {0x89b9b3e1, 0x1b6329ba, 0xa9e904c8, 941},
        {0xac2820d9, 0x623bf429, 0x546345fa, 944},   {0xd732290f, 0xbacaf133, 0xa97c1779, 947},
        {0x867f59a9, 0xd4bed6c0, 0x49ed8eab, 951},   {0xa81f3014, 0x49ee8c70, 0x5c68f256, 954},
        {0xd226fc19, 0x5c6a2f8c, 0x73832eec, 957},   {0x83585d8f, 0xd9c25db7, 0xc831fd53, 961},
        {0xa42e74f3, 0xd032f525, 0xba3e7ca8, 964},   {0xcd3a1230, 0xc43fb26f, 0x28ce1bd2, 967},
        {0x80444b5e, 0x7aa7cf85, 0x7980d163, 971},   {0xa0555e36, 0x1951c366, 0xd7e105bc, 974},
        {0xc86ab5c3, 0x9fa63440, 0x8dd9472b, 977},   {0xfa856334, 0x878fc150, 0xb14f98f6, 980},
        {0x9c935e00, 0xd4b9d8d2, 0x6ed1bf9a, 984},   {0xc3b83581, 0x09e84f07, 0x0a862f80, 987},
        {0xf4a642e1, 0x4c6262c8, 0xcd27bb61, 990},   {0x98e7e9cc, 0xcfbd7dbd, 0x8038d51c, 994},
        {0xbf21e440, 0x03acdd2c, 0xe0470a63, 997},   {0xeeea5d50, 0x04981478, 0x1858ccfc, 1000},
        {0x95527a52, 0x02df0ccb, 0x0f37801e, 1004},  {0xbaa718e6, 0x8396cffd, 0xd3056025, 1007},
        {0xe950df20, 0x247c83fd, 0x47c6b82e, 1010},  {0x91d28b74, 0x16cdd27e, 0x4cdc331d, 1014},
        {0xb6472e51, 0x1c81471d, 0xe0133fe4, 1017},  {0xe3d8f9e5, 0x63a198e5, 0x58180fdd, 1020},
        {0x8e679c2f, 0x5e44ff8f, 0x570f09ea, 1024},  {0xb201833b, 0x35d63f73, 0x2cd2cc65, 1027},
        {0xde81e40a, 0x034bcf4f, 0xf8077f7e, 1030},  {0x8b112e86, 0x420f6191, 0xfb04afaf, 1034},
        {0xadd57a27, 0xd29339f6, 0x79c5db9a, 1037},  {0xd94ad8b1, 0xc7380874, 0x18375281, 1040},
        {0x87cec76f, 0x1c830548, 0x8f229391, 1044},  {0xa9c2794a, 0xe3a3c69a, 0xb2eb3875, 1047},
        {0xd433179d, 0x9c8cb841, 0x5fa60692, 1050},  {0x849feec2, 0x81d7f328, 0xdbc7c41b, 1054},
        {0xa5c7ea73, 0x224deff3, 0x12b9b522, 1057},  {0xcf39e50f, 0xeae16bef, 0xd768226b, 1060},
        {0x81842f29, 0xf2cce375, 0xe6a11583, 1064},  {0xa1e53af4, 0x6f801c53, 0x60495ae3, 1067},
        {0xca5e89b1, 0x8b602368, 0x385bb19c, 1070},  {0xfcf62c1d, 0xee382c42, 0x46729e03, 1073},
        {0x9e19db92, 0xb4e31ba9, 0x6c07a2c2, 1077}};

static int16_t Lhint[2098] = {
        // 18,
        19,  19,  19,  19,  20,  20,  20,  21,  21,  21,  22,  22,  22,  23,  23,  23,  23,  24,
        24,  24,  25,  25,  25,  26,  26,  26,  26,  27,  27,  27,  28,  28,  28,  29,  29,  29,
        29,  30,  30,  30,  31,  31,  31,  32,  32,  32,  32,  33,  33,  33,  34,  34,  34,  35,
        35,  35,  35,  36,  36,  36,  37,  37,  37,  38,  38,  38,  38,  39,  39,  39,  40,  40,
        40,  41,  41,  41,  41,  42,  42,  42,  43,  43,  43,  44,  44,  44,  44,  45,  45,  45,
        46,  46,  46,  47,  47,  47,  47,  48,  48,  48,  49,  49,  49,  50,  50,  50,  51,  51,
        51,  51,  52,  52,  52,  53,  53,  53,  54,  54,  54,  54,  55,  55,  55,  56,  56,  56,
        57,  57,  57,  57,  58,  58,  58,  59,  59,  59,  60,  60,  60,  60,  61,  61,  61,  62,
        62,  62,  63,  63,  63,  63,  64,  64,  64,  65,  65,  65,  66,  66,  66,  66,  67,  67,
        67,  68,  68,  68,  69,  69,  69,  69,  70,  70,  70,  71,  71,  71,  72,  72,  72,  72,
        73,  73,  73,  74,  74,  74,  75,  75,  75,  75,  76,  76,  76,  77,  77,  77,  78,  78,
        78,  78,  79,  79,  79,  80,  80,  80,  81,  81,  81,  82,  82,  82,  82,  83,  83,  83,
        84,  84,  84,  85,  85,  85,  85,  86,  86,  86,  87,  87,  87,  88,  88,  88,  88,  89,
        89,  89,  90,  90,  90,  91,  91,  91,  91,  92,  92,  92,  93,  93,  93,  94,  94,  94,
        94,  95,  95,  95,  96,  96,  96,  97,  97,  97,  97,  98,  98,  98,  99,  99,  99,  100,
        100, 100, 100, 101, 101, 101, 102, 102, 102, 103, 103, 103, 103, 104, 104, 104, 105, 105,
        105, 106, 106, 106, 106, 107, 107, 107, 108, 108, 108, 109, 109, 109, 110, 110, 110, 110,
        111, 111, 111, 112, 112, 112, 113, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116,
        116, 116, 117, 117, 117, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 121, 121, 121,
        122, 122, 122, 122, 123, 123, 123, 124, 124, 124, 125, 125, 125, 125, 126, 126, 126, 127,
        127, 127, 128, 128, 128, 128, 129, 129, 129, 130, 130, 130, 131, 131, 131, 131, 132, 132,
        132, 133, 133, 133, 134, 134, 134, 134, 135, 135, 135, 136, 136, 136, 137, 137, 137, 137,
        138, 138, 138, 139, 139, 139, 140, 140, 140, 141, 141, 141, 141, 142, 142, 142, 143, 143,
        143, 144, 144, 144, 144, 145, 145, 145, 146, 146, 146, 147, 147, 147, 147, 148, 148, 148,
        149, 149, 149, 150, 150, 150, 150, 151, 151, 151, 152, 152, 152, 153, 153, 153, 153, 154,
        154, 154, 155, 155, 155, 156, 156, 156, 156, 157, 157, 157, 158, 158, 158, 159, 159, 159,
        159, 160, 160, 160, 161, 161, 161, 162, 162, 162, 162, 163, 163, 163, 164, 164, 164, 165,
        165, 165, 165, 166, 166, 166, 167, 167, 167, 168, 168, 168, 169, 169, 169, 169, 170, 170,
        170, 171, 171, 171, 172, 172, 172, 172, 173, 173, 173, 174, 174, 174, 175, 175, 175, 175,
        176, 176, 176, 177, 177, 177, 178, 178, 178, 178, 179, 179, 179, 180, 180, 180, 181, 181,
        181, 181, 182, 182, 182, 183, 183, 183, 184, 184, 184, 184, 185, 185, 185, 186, 186, 186,
        187, 187, 187, 187, 188, 188, 188, 189, 189, 189, 190, 190, 190, 190, 191, 191, 191, 192,
        192, 192, 193, 193, 193, 193, 194, 194, 194, 195, 195, 195, 196, 196, 196, 197, 197, 197,
        197, 198, 198, 198, 199, 199, 199, 200, 200, 200, 200, 201, 201, 201, 202, 202, 202, 203,
        203, 203, 203, 204, 204, 204, 205, 205, 205, 206, 206, 206, 206, 207, 207, 207, 208, 208,
        208, 209, 209, 209, 209, 210, 210, 210, 211, 211, 211, 212, 212, 212, 212, 213, 213, 213,
        214, 214, 214, 215, 215, 215, 215, 216, 216, 216, 217, 217, 217, 218, 218, 218, 218, 219,
        219, 219, 220, 220, 220, 221, 221, 221, 221, 222, 222, 222, 223, 223, 223, 224, 224, 224,
        224, 225, 225, 225, 226, 226, 226, 227, 227, 227, 228, 228, 228, 228, 229, 229, 229, 230,
        230, 230, 231, 231, 231, 231, 232, 232, 232, 233, 233, 233, 234, 234, 234, 234, 235, 235,
        235, 236, 236, 236, 237, 237, 237, 237, 238, 238, 238, 239, 239, 239, 240, 240, 240, 240,
        241, 241, 241, 242, 242, 242, 243, 243, 243, 243, 244, 244, 244, 245, 245, 245, 246, 246,
        246, 246, 247, 247, 247, 248, 248, 248, 249, 249, 249, 249, 250, 250, 250, 251, 251, 251,
        252, 252, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 256, 256, 256, 256, 257,
        257, 257, 258, 258, 258, 259, 259, 259, 259, 260, 260, 260, 261, 261, 261, 262, 262, 262,
        262, 263, 263, 263, 264, 264, 264, 265, 265, 265, 265, 266, 266, 266, 267, 267, 267, 268,
        268, 268, 268, 269, 269, 269, 270, 270, 270, 271, 271, 271, 271, 272, 272, 272, 273, 273,
        273, 274, 274, 274, 274, 275, 275, 275, 276, 276, 276, 277, 277, 277, 277, 278, 278, 278,
        279, 279, 279, 280, 280, 280, 280, 281, 281, 281, 282, 282, 282, 283, 283, 283, 283, 284,
        284, 284, 285, 285, 285, 286, 286, 286, 287, 287, 287, 287, 288, 288, 288, 289, 289, 289,
        290, 290, 290, 290, 291, 291, 291, 292, 292, 292, 293, 293, 293, 293, 294, 294, 294, 295,
        295, 295, 296, 296, 296, 296, 297, 297, 297, 298, 298, 298, 299, 299, 299, 299, 300, 300,
        300, 301, 301, 301, 302, 302, 302, 302, 303, 303, 303, 304, 304, 304, 305, 305, 305, 305,
        306, 306, 306, 307, 307, 307, 308, 308, 308, 308, 309, 309, 309, 310, 310, 310, 311, 311,
        311, 311, 312, 312, 312, 313, 313, 313, 314, 314, 314, 315, 315, 315, 315, 316, 316, 316,
        317, 317, 317, 318, 318, 318, 318, 319, 319, 319, 320, 320, 320, 321, 321, 321, 321, 322,
        322, 322, 323, 323, 323, 324, 324, 324, 324, 325, 325, 325, 326, 326, 326, 327, 327, 327,
        327, 328, 328, 328, 329, 329, 329, 330, 330, 330, 330, 331, 331, 331, 332, 332, 332, 333,
        333, 333, 333, 334, 334, 334, 335, 335, 335, 336, 336, 336, 336, 337, 337, 337, 338, 338,
        338, 339, 339, 339, 339, 340, 340, 340, 341, 341, 341, 342, 342, 342, 342, 343, 343, 343,
        344, 344, 344, 345, 345, 345, 346, 346, 346, 346, 347, 347, 347, 348, 348, 348, 349, 349,
        349, 349, 350, 350, 350, 351, 351, 351, 352, 352, 352, 352, 353, 353, 353, 354, 354, 354,
        355, 355, 355, 355, 356, 356, 356, 357, 357, 357, 358, 358, 358, 358, 359, 359, 359, 360,
        360, 360, 361, 361, 361, 361, 362, 362, 362, 363, 363, 363, 364, 364, 364, 364, 365, 365,
        365, 366, 366, 366, 367, 367, 367, 367, 368, 368, 368, 369, 369, 369, 370, 370, 370, 370,
        371, 371, 371, 372, 372, 372, 373, 373, 373, 374, 374, 374, 374, 375, 375, 375, 376, 376,
        376, 377, 377, 377, 377, 378, 378, 378, 379, 379, 379, 380, 380, 380, 380, 381, 381, 381,
        382, 382, 382, 383, 383, 383, 383, 384, 384, 384, 385, 385, 385, 386, 386, 386, 386, 387,
        387, 387, 388, 388, 388, 389, 389, 389, 389, 390, 390, 390, 391, 391, 391, 392, 392, 392,
        392, 393, 393, 393, 394, 394, 394, 395, 395, 395, 395, 396, 396, 396, 397, 397, 397, 398,
        398, 398, 398, 399, 399, 399, 400, 400, 400, 401, 401, 401, 402, 402, 402, 402, 403, 403,
        403, 404, 404, 404, 405, 405, 405, 405, 406, 406, 406, 407, 407, 407, 408, 408, 408, 408,
        409, 409, 409, 410, 410, 410, 411, 411, 411, 411, 412, 412, 412, 413, 413, 413, 414, 414,
        414, 414, 415, 415, 415, 416, 416, 416, 417, 417, 417, 417, 418, 418, 418, 419, 419, 419,
        420, 420, 420, 420, 421, 421, 421, 422, 422, 422, 423, 423, 423, 423, 424, 424, 424, 425,
        425, 425, 426, 426, 426, 426, 427, 427, 427, 428, 428, 428, 429, 429, 429, 429, 430, 430,
        430, 431, 431, 431, 432, 432, 432, 433, 433, 433, 433, 434, 434, 434, 435, 435, 435, 436,
        436, 436, 436, 437, 437, 437, 438, 438, 438, 439, 439, 439, 439, 440, 440, 440, 441, 441,
        441, 442, 442, 442, 442, 443, 443, 443, 444, 444, 444, 445, 445, 445, 445, 446, 446, 446,
        447, 447, 447, 448, 448, 448, 448, 449, 449, 449, 450, 450, 450, 451, 451, 451, 451, 452,
        452, 452, 453, 453, 453, 454, 454, 454, 454, 455, 455, 455, 456, 456, 456, 457, 457, 457,
        457, 458, 458, 458, 459, 459, 459, 460, 460, 460, 461, 461, 461, 461, 462, 462, 462, 463,
        463, 463, 464, 464, 464, 464, 465, 465, 465, 466, 466, 466, 467, 467, 467, 467, 468, 468,
        468, 469, 469, 469, 470, 470, 470, 470, 471, 471, 471, 472, 472, 472, 473, 473, 473, 473,
        474, 474, 474, 475, 475, 475, 476, 476, 476, 476, 477, 477, 477, 478, 478, 478, 479, 479,
        479, 479, 480, 480, 480, 481, 481, 481, 482, 482, 482, 482, 483, 483, 483, 484, 484, 484,
        485, 485, 485, 485, 486, 486, 486, 487, 487, 487, 488, 488, 488, 488, 489, 489, 489, 490,
        490, 490, 491, 491, 491, 492, 492, 492, 492, 493, 493, 493, 494, 494, 494, 495, 495, 495,
        495, 496, 496, 496, 497, 497, 497, 498, 498, 498, 498, 499, 499, 499, 500, 500, 500, 501,
        501, 501, 501, 502, 502, 502, 503, 503, 503, 504, 504, 504, 504, 505, 505, 505, 506, 506,
        506, 507, 507, 507, 507, 508, 508, 508, 509, 509, 509, 510, 510, 510, 510, 511, 511, 511,
        512, 512, 512, 513, 513, 513, 513, 514, 514, 514, 515, 515, 515, 516, 516, 516, 516, 517,
        517, 517, 518, 518, 518, 519, 519, 519, 520, 520, 520, 520, 521, 521, 521, 522, 522, 522,
        523, 523, 523, 523, 524, 524, 524, 525, 525, 525, 526, 526, 526, 526, 527, 527, 527, 528,
        528, 528, 529, 529, 529, 529, 530, 530, 530, 531, 531, 531, 532, 532, 532, 532, 533, 533,
        533, 534, 534, 534, 535, 535, 535, 535, 536, 536, 536, 537, 537, 537, 538, 538, 538, 538,
        539, 539, 539, 540, 540, 540, 541, 541, 541, 541, 542, 542, 542, 543, 543, 543, 544, 544,
        544, 544, 545, 545, 545, 546, 546, 546, 547, 547, 547, 548, 548, 548, 548, 549, 549, 549,
        550, 550, 550, 551, 551, 551, 551, 552, 552, 552, 553, 553, 553, 554, 554, 554, 554, 555,
        555, 555, 556, 556, 556, 557, 557, 557, 557, 558, 558, 558, 559, 559, 559, 560, 560, 560,
        560, 561, 561, 561, 562, 562, 562, 563, 563, 563, 563, 564, 564, 564, 565, 565, 565, 566,
        566, 566, 566, 567, 567, 567, 568, 568, 568, 569, 569, 569, 569, 570, 570, 570, 571, 571,
        571, 572, 572, 572, 572, 573, 573, 573, 574, 574, 574, 575, 575, 575, 575, 576, 576, 576,
        577, 577, 577, 578, 578, 578, 579, 579, 579, 579, 580, 580, 580, 581, 581, 581, 582, 582,
        582, 582, 583, 583, 583, 584, 584, 584, 585, 585, 585, 585, 586, 586, 586, 587, 587, 587,
        588, 588, 588, 588, 589, 589, 589, 590, 590, 590, 591, 591, 591, 591, 592, 592, 592, 593,
        593, 593, 594, 594, 594, 594, 595, 595, 595, 596, 596, 596, 597, 597, 597, 597, 598, 598,
        598, 599, 599, 599, 600, 600, 600, 600, 601, 601, 601, 602, 602, 602, 603, 603, 603, 603,
        604, 604, 604, 605, 605, 605, 606, 606, 606, 607, 607, 607, 607, 608, 608, 608, 609, 609,
        609, 610, 610, 610, 610, 611, 611, 611, 612, 612, 612, 613, 613, 613, 613, 614, 614, 614,
        615, 615, 615, 616, 616, 616, 616, 617, 617, 617, 618, 618, 618, 619, 619, 619, 619, 620,
        620, 620, 621, 621, 621, 622, 622, 622, 622, 623, 623, 623, 624, 624, 624, 625, 625, 625,
        625, 626, 626, 626, 627, 627, 627, 628, 628, 628, 628, 629, 629, 629, 630, 630, 630, 631,
        631, 631, 631, 632, 632, 632, 633, 633, 633, 634, 634, 634, 634, 635, 635, 635, 636, 636,
        636, 637, 637, 637, 638, 638, 638, 638, 639, 639, 639, 640, 640, 640, 641, 641, 641, 641,
        642, 642, 642, 643, 643, 643, 644, 644, 644, 644, 645, 645, 645, 646, 646, 646, 647, 647,
        647, 647, 648, 648, 648, 649, 649, 649, 650, 650};

static uint64_t pfive[27] = {5ll,
                             25ll,
                             125ll,
                             625ll,
                             3125ll,
                             15625ll,
                             78125ll,
                             390625ll,
                             1953125ll,
                             9765625ll,
                             48828125ll,
                             244140625ll,
                             1220703125ll,
                             6103515625ll,
                             30517578125ll,
                             152587890625ll,
                             762939453125ll,
                             3814697265625ll,
                             19073486328125ll,
                             95367431640625ll,
                             476837158203125ll,
                             2384185791015625ll,
                             11920928955078125ll,
                             59604644775390625ll,
                             298023223876953125ll,
                             1490116119384765625ll,
                             7450580596923828125ll};

static int pfivebits[25] = {3,  5,  7,  10, 12, 14, 17, 19, 21, 24, 26, 28, 31,
                            33, 35, 38, 40, 42, 45, 47, 49, 52, 54, 56, 59};

typedef union {
    double   d;
    uint32_t L[2];
    uint64_t LL;
} U;

#ifdef IEEE_8087
#define word0(x) (x)->L[1]
#define word1(x) (x)->L[0]
#else
#define word0(x) (x)->L[0]
#define word1(x) (x)->L[1]
#endif

#define strtod_diglim 40

#define Exp_shift 20
#define Exp_shift1 20
#define Exp_msk1 0x100000
#define Exp_msk11 0x100000
#define Exp_mask 0x7ff00000
#define P 53
#define Nbits 53
#define Bias 1023
#define Emax 1023
#define Emin (-1022)
#define Exp_1 0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask 0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask 0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14

#ifdef ROUND_BIASED_without_Round_Up
#undef ROUND_BIASED
#define ROUND_BIASED
#endif

typedef struct BCinfo BCinfo;
struct BCinfo {
    int dp0, dp1, dplen, dsign, e0, inexact, nd, nd0, rounding, scale, uflchk;
};

#define Kmax 7

struct Bigint {
    struct Bigint* next;
    int            k, maxwds, sign, wds;
    uint32_t       x[1];
};

typedef struct Bigint Bigint;
typedef struct ThInfo {
    Bigint* Freelist[Kmax + 1];
    Bigint* P5s;
} ThInfo;

#ifdef _MSC_VER
static __declspec(thread) ThInfo ti_instance;
static ThInfo* get_TI(void) {
    return &ti_instance;
}
#else
static pthread_key_t  ti_key;
static pthread_once_t ti_once;
static void           ti_init() { pthread_key_create(&ti_key, free); }

static ThInfo* get_TI(void) {
    pthread_once(&ti_once, ti_init);
    ThInfo* ti = pthread_getspecific(ti_key);
    if (!ti) {
        ti = malloc(sizeof(ThInfo));
        memset(ti, 0, sizeof(ThInfo));
        pthread_setspecific(ti_key, ti);
    }
    return ti;
}
#endif

static Bigint* Balloc(int k, ThInfo** PTI) {
    int     x;
    Bigint* rv;
    ThInfo* TI;

    if (!(TI = *PTI)) {
        *PTI = TI = get_TI();
    }
    // The k > Kmax case does not need ACQUIRE_DTOA_LOCK(0),
    // but this case seems very unlikely.
    if (k <= Kmax && (rv = TI->Freelist[k])) {
        TI->Freelist[k] = rv->next;
    } else {
        x          = 1 << k;
        rv         = (Bigint*)malloc(sizeof(Bigint) + (x - 1) * sizeof(uint32_t));
        rv->k      = k;
        rv->maxwds = x;
    }
    rv->sign = rv->wds = 0;
    return rv;
}

static void Bfree(Bigint* v, ThInfo** PTI) {
    ThInfo* TI;
    if (v) {
        if (v->k > Kmax) {
            free((void*)v);
        } else {
            if (!(TI = *PTI)) {
                *PTI = TI = get_TI();
            }
            v->next            = TI->Freelist[v->k];
            TI->Freelist[v->k] = v;
        }
    }
}

#define Bcopy(x, y) \
    memcpy((char*)&x->sign, (char*)&y->sign, y->wds * sizeof(int32_t) + 2 * sizeof(int))

static Bigint* multadd(Bigint* b, int m, int a, ThInfo** PTI)  // multiply by m and add a
{
    int       i, wds;
    uint32_t* x;
    uint64_t  carry, y;
    Bigint*   b1;

    wds   = b->wds;
    x     = b->x;
    i     = 0;
    carry = a;
    do {
        y     = *x * (uint64_t)m + carry;
        carry = y >> 32;
        *x++  = y & UINT32_C(0xffffffff);
    } while (++i < wds);
    if (carry) {
        if (wds >= b->maxwds) {
            b1 = Balloc(b->k + 1, PTI);
            Bcopy(b1, b);
            Bfree(b, PTI);
            b = b1;
        }
        b->x[wds++] = carry;
        b->wds      = wds;
    }
    return b;
}

static Bigint* s2b(const char* s, int nd0, int nd, uint32_t y9, int dplen, ThInfo** PTI) {
    Bigint* b;
    int     i, k;
    int32_t x, y;

    x = (nd + 8) / 9;
    for (k = 0, y = 1; x > y; y <<= 1, k++) {
    }
    b       = Balloc(k, PTI);
    b->x[0] = y9;
    b->wds  = 1;

    i = 9;
    if (9 < nd0) {
        s += 9;
        do {
            b = multadd(b, 10, *s++ - '0', PTI);
        } while (++i < nd0);
        s += dplen;
    } else {
        s += dplen + 9;
    }
    for (; i < nd; i++) {
        b = multadd(b, 10, *s++ - '0', PTI);
    }
    return b;
}

static int hi0bits(uint32_t x) {
    int k = 0;

    if (!(x & 0xffff0000)) {
        k = 16;
        x <<= 16;
    }
    if (!(x & 0xff000000)) {
        k += 8;
        x <<= 8;
    }
    if (!(x & 0xf0000000)) {
        k += 4;
        x <<= 4;
    }
    if (!(x & 0xc0000000)) {
        k += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000)) {
        k++;
        if (!(x & 0x40000000)) {
            return 32;
        }
    }
    return k;
}

static int lo0bits(uint32_t* y) {
    int      k;
    uint32_t x = *y;

    if (x & 7) {
        if (x & 1) {
            return 0;
        }
        if (x & 2) {
            *y = x >> 1;
            return 1;
        }
        *y = x >> 2;
        return 2;
    }
    k = 0;
    if (!(x & 0xffff)) {
        k = 16;
        x >>= 16;
    }
    if (!(x & 0xff)) {
        k += 8;
        x >>= 8;
    }
    if (!(x & 0xf)) {
        k += 4;
        x >>= 4;
    }
    if (!(x & 0x3)) {
        k += 2;
        x >>= 2;
    }
    if (!(x & 1)) {
        k++;
        x >>= 1;
        if (!x) {
            return 32;
        }
    }
    *y = x;
    return k;
}

static Bigint* i2b(int i, ThInfo** PTI) {
    Bigint* b;

    b       = Balloc(1, PTI);
    b->x[0] = i;
    b->wds  = 1;
    return b;
}

static Bigint* mult(Bigint* a, Bigint* b, ThInfo** PTI) {
    Bigint*   c;
    int       k, wa, wb, wc;
    uint32_t *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    uint32_t  y;
    uint64_t  carry, z;

    if (a->wds < b->wds) {
        c = a;
        a = b;
        b = c;
    }
    k  = a->k;
    wa = a->wds;
    wb = b->wds;
    wc = wa + wb;
    if (wc > a->maxwds) {
        k++;
    }
    c = Balloc(k, PTI);
    for (x = c->x, xa = x + wc; x < xa; x++) {
        *x = 0;
    }
    xa  = a->x;
    xae = xa + wa;
    xb  = b->x;
    xbe = xb + wb;
    xc0 = c->x;
    for (; xb < xbe; xc0++) {
        if ((y = *xb++)) {
            x     = xa;
            xc    = xc0;
            carry = 0;
            do {
                z     = *x++ * (uint64_t)y + *xc + carry;
                carry = z >> 32;
                *xc++ = z & UINT32_C(0xffffffff);
            } while (x < xae);
            *xc = carry;
        }
    }
    for (xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) {
    }
    c->wds = wc;
    return c;
}

static Bigint* pow5mult(Bigint* b, int k, ThInfo** PTI) {
    Bigint *   b1, *p5, *p51;
    ThInfo*    TI;
    int        i;
    static int p05[3] = {5, 25, 125};

    if ((i = k & 3)) {
        b = multadd(b, p05[i - 1], 0, PTI);
    }

    if (!(k >>= 2)) {
        return b;
    }
    if (!(TI = *PTI)) {
        *PTI = TI = get_TI();
    }
    if (!(p5 = TI->P5s)) {
        // first time
        if (!(TI = *PTI)) {
            *PTI = TI = get_TI();
        }
        if (!(p5 = TI->P5s)) {
            p5 = TI->P5s = i2b(625, PTI);
            p5->next     = 0;
        }
    }
    for (;;) {
        if (k & 1) {
            b1 = mult(b, p5, PTI);
            Bfree(b, PTI);
            b = b1;
        }
        if (!(k >>= 1)) {
            break;
        }
        if (!(p51 = p5->next)) {
            if (!TI && !(TI = *PTI)) {
                *PTI = TI = get_TI();
            }
            if (!(p51 = p5->next)) {
                p51 = p5->next = mult(p5, p5, PTI);
                p51->next      = 0;
            }
        }
        p5 = p51;
    }
    return b;
}

static Bigint* lshift(Bigint* b, int k, ThInfo** PTI) {
    int       i, k1, n, n1;
    Bigint*   b1;
    uint32_t *x, *x1, *xe, z;

    n  = k >> 5;
    k1 = b->k;
    n1 = n + b->wds + 1;
    for (i = b->maxwds; n1 > i; i <<= 1) {
        k1++;
    }
    b1 = Balloc(k1, PTI);
    x1 = b1->x;
    for (i = 0; i < n; i++) {
        *x1++ = 0;
    }
    x  = b->x;
    xe = x + b->wds;
    if (k &= 0x1f) {
        k1 = 32 - k;
        z  = 0;
        do {
            *x1++ = *x << k | z;
            z     = *x++ >> k1;
        } while (x < xe);
        if ((*x1 = z)) {
            ++n1;
        }
    } else
        do {
            *x1++ = *x++;
        } while (x < xe);
    b1->wds = n1 - 1;
    Bfree(b, PTI);
    return b1;
}

static int cmp(Bigint* a, Bigint* b) {
    uint32_t *xa, *xa0, *xb, *xb0;
    int       i, j;

    i = a->wds;
    j = b->wds;
#ifdef DEBUG
    if (i > 1 && !a->x[i - 1]) {
        Bug("cmp called with a->x[a->wds-1] == 0");
    }
    if (j > 1 && !b->x[j - 1]) {
        Bug("cmp called with b->x[b->wds-1] == 0");
    }
#endif
    if (i -= j) {
        return i;
    }
    xa0 = a->x;
    xa  = xa0 + j;
    xb0 = b->x;
    xb  = xb0 + j;
    for (;;) {
        if (*--xa != *--xb) {
            return *xa < *xb ? -1 : 1;
        }
        if (xa <= xa0) {
            break;
        }
    }
    return 0;
}

static Bigint* diff(Bigint* a, Bigint* b, ThInfo** PTI) {
    Bigint*   c;
    int       i, wa, wb;
    uint32_t *xa, *xae, *xb, *xbe, *xc;
    uint64_t  borrow, y;

    i = cmp(a, b);
    if (!i) {
        c       = Balloc(0, PTI);
        c->wds  = 1;
        c->x[0] = 0;
        return c;
    }
    if (i < 0) {
        c = a;
        a = b;
        b = c;
        i = 1;
    } else
        i = 0;
    c       = Balloc(a->k, PTI);
    c->sign = i;
    wa      = a->wds;
    xa      = a->x;
    xae     = xa + wa;
    wb      = b->wds;
    xb      = b->x;
    xbe     = xb + wb;
    xc      = c->x;
    borrow  = 0;
    do {
        y      = (uint64_t)*xa++ - *xb++ - borrow;
        borrow = y >> 32 & (uint32_t)1;
        *xc++  = y & UINT32_C(0xffffffff);
    } while (xb < xbe);
    while (xa < xae) {
        y      = *xa++ - borrow;
        borrow = y >> 32 & (uint32_t)1;
        *xc++  = y & UINT32_C(0xffffffff);
    }
    while (!*--xc) {
        wa--;
    }
    c->wds = wa;
    return c;
}

static double ulp(U* x) {
    int32_t L;
    U       u;

    L         = (word0(x) & Exp_mask) - (P - 1) * Exp_msk1;
    word0(&u) = L;
    word1(&u) = 0;
    return u.d;
}

static double b2d(Bigint* a, int* e) {
    uint32_t *xa, *xa0, w, y, z;
    int       k;
    U         d;

    xa0 = a->x;
    xa  = xa0 + a->wds;
    y   = *--xa;
#ifdef DEBUG
    if (!y) {
        Bug("zero y in b2d");
    }
#endif
    k  = hi0bits(y);
    *e = 32 - k;
    if (k < Ebits) {
        word0(&d) = Exp_1 | y >> (Ebits - k);
        w         = xa > xa0 ? *--xa : 0;
        word1(&d) = y << ((32 - Ebits) + k) | w >> (Ebits - k);
        goto ret_d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
        word0(&d) = Exp_1 | y << k | z >> (32 - k);
        y         = xa > xa0 ? *--xa : 0;
        word1(&d) = z << k | y >> (32 - k);
    } else {
        word0(&d) = Exp_1 | y;
        word1(&d) = z;
    }
ret_d:
    return d.d;
}

static Bigint* d2b(U* d, int* e, int* bits, ThInfo** PTI) {
    Bigint*   b;
    int       de, k;
    uint32_t *x, y, z;
    int       i;

    b = Balloc(1, PTI);
    x = b->x;

    z = word0(d) & Frac_mask;
    word0(d) &= 0x7fffffff;  // clear sign bit, which we ignore
    if ((de = (int)(word0(d) >> Exp_shift))) {
        z |= Exp_msk1;
    }
    if ((y = word1(d))) {
        if ((k = lo0bits(&y))) {
            x[0] = y | z << (32 - k);
            z >>= k;
        } else {
            x[0] = y;
        }
        i = b->wds = (x[1] = z) ? 2 : 1;
    } else {
        k    = lo0bits(&z);
        x[0] = z;
        i = b->wds = 1;
        k += 32;
    }
    if (de) {
        *e    = de - Bias - (P - 1) + k;
        *bits = P - k;
    } else {
        *e    = de - Bias - (P - 1) + 1 + k;
        *bits = 32 * i - hi0bits(x[i - 1]);
    }
    return b;
}

static double ratio(Bigint* a, Bigint* b) {
    U   da, db;
    int k, ka, kb;

    da.d = b2d(a, &ka);
    db.d = b2d(b, &kb);
    k    = ka - kb + 32 * (a->wds - b->wds);
    if (k > 0) {
        word0(&da) += k * Exp_msk1;
    } else {
        k = -k;
        word0(&db) += k * Exp_msk1;
    }
    return da.d / db.d;
}

static const double tens[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                              1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                              1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

static const double bigtens[]  = {1e16, 1e32, 1e64, 1e128, 1e256};
static const double tinytens[] = {
        1e-16, 1e-32, 1e-64, 1e-128, 9007199254740992. * 9007199254740992.e-256
        // = 2^106 * 1e-256
};
// The factor of 2^53 in tinytens[4] helps us avoid setting the underflow
// flag unnecessarily.  It leads to a song and dance at the end of strtod.
#define Scale_Bit 0x10
#define n_bigtens 5

#undef Need_Hexdig

#ifdef Need_Hexdig  //{
#if 0
#else
static uint8_t hexdig[256] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  26, 27, 28, 29, 30, 31, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0};
#endif
#endif  // } Need_Hexdig

#define ULbits 32
#define kshift 5
#define kmask 31

static int dshift(Bigint* b, int p2) {
    int rv = hi0bits(b->x[b->wds - 1]) - 4;
    if (p2 > 0) {
        rv -= p2;
    }
    return rv & kmask;
}

static int quorem(Bigint* b, Bigint* S) {
    int       n;
    uint32_t *bx, *bxe, q, *sx, *sxe;
    uint64_t  borrow, carry, y, ys;

    n = S->wds;
#ifdef DEBUG
    if (b->wds > n) {
        Bug("oversize b in quorem");
    }
#endif
    if (b->wds < n) {
        return 0;
    }
    sx  = S->x;
    sxe = sx + --n;
    bx  = b->x;
    bxe = bx + n;
    q   = *bxe / (*sxe + 1);  // ensure q <= true quotient
#ifdef DEBUG
    // An oversized q is possible when quorem is called from bigcomp and
    // the input is near, e.g., twice the smallest denormalized number.
    if (q > 15) {
        Bug("oversized quotient in quorem");
    }
#endif
    if (q) {
        borrow = 0;
        carry  = 0;
        do {
            ys     = *sx++ * (uint64_t)q + carry;
            carry  = ys >> 32;
            y      = *bx - (ys & UINT32_C(0xffffffff)) - borrow;
            borrow = y >> 32 & (uint32_t)1;
            *bx++  = y & UINT32_C(0xffffffff);
        } while (sx <= sxe);
        if (!*bxe) {
            bx = b->x;
            while (--bxe > bx && !*bxe) {
                --n;
            }
            b->wds = n;
        }
    }
    if (cmp(b, S) >= 0) {
        q++;
        borrow = 0;
        carry  = 0;
        bx     = b->x;
        sx     = S->x;
        do {
            ys     = *sx++ + carry;
            carry  = ys >> 32;
            y      = *bx - (ys & UINT32_C(0xffffffff)) - borrow;
            borrow = y >> 32 & (uint32_t)1;
            *bx++  = y & UINT32_C(0xffffffff);
        } while (sx <= sxe);
        bx  = b->x;
        bxe = bx + n;
        if (!*bxe) {
            while (--bxe > bx && !*bxe) {
                --n;
            }
            b->wds = n;
        }
    }
    return q;
}

static double sulp(U* x, BCinfo* bc) {
    U      u;
    double rv;
    int    i;

    rv = ulp(x);
    if (!bc->scale || (i = 2 * P + 1 - ((word0(x) & Exp_mask) >> Exp_shift)) <= 0) {
        return rv;  // Is there an example where i <= 0 ?
    }
    word0(&u) = Exp_1 + (i << Exp_shift);
    word1(&u) = 0;
    return rv * u.d;
}

static void bigcomp(U* rv, const char* s0, BCinfo* bc, ThInfo** PTI) {
    Bigint *b, *d;
    int     b2, bbits, d2, dd, dig, dsign, i, j, nd, nd0, p2, p5, speccase;

    dsign    = bc->dsign;
    nd       = bc->nd;
    nd0      = bc->nd0;
    p5       = nd + bc->e0 - 1;
    speccase = 0;
    if (rv->d == 0.) {  // special case: value near underflow-to-zero
                        // threshold was rounded to zero
        b         = i2b(1, PTI);
        p2        = Emin - P + 1;
        bbits     = 1;
        word0(rv) = (P + 2) << Exp_shift;
        i         = 0;
        speccase  = 1;
        --p2;
        dsign = 0;
        goto have_i;
    } else
        b = d2b(rv, &p2, &bbits, PTI);
    p2 -= bc->scale;
    // floor(log2(rv)) == bbits - 1 + p2
    // Check for denormal case.
    i = P - bbits;
    if (i > (j = P - Emin - 1 + p2)) {
        Bfree(b, PTI);
        b         = i2b(1, PTI);
        p2        = Emin;
        i         = P - 1;
        word0(rv) = (1 + bc->scale) << Exp_shift;
        word1(rv) = 0;
    }
    {
        b = lshift(b, ++i, PTI);
        b->x[0] |= 1;
    }
have_i:
    p2 -= p5 + i;
    d = i2b(1, PTI);
    // Arrange for convenient computation of quotients:
    // shift left if necessary so divisor has 4 leading 0 bits.
    if (p5 > 0) {
        d = pow5mult(d, p5, PTI);
    } else if (p5 < 0) {
        b = pow5mult(b, -p5, PTI);
    }
    if (p2 > 0) {
        b2 = p2;
        d2 = 0;
    } else {
        b2 = 0;
        d2 = -p2;
    }
    i = dshift(d, d2);
    if ((b2 += i) > 0) {
        b = lshift(b, b2, PTI);
    }
    if ((d2 += i) > 0) {
        d = lshift(d, d2, PTI);
    }

    // Now b/d = exactly half-way between the two floating-point values
    // on either side of the input string.  Compute first digit of b/d.

    if (!(dig = quorem(b, d))) {
        b   = multadd(b, 10, 0, PTI);  // very unlikely
        dig = quorem(b, d);
    }

    // Compare b/d with s0

    for (i = 0; i < nd0;) {
        if ((dd = s0[i++] - '0' - dig)) {
            goto ret;
        }
        if (!b->x[0] && b->wds == 1) {
            if (i < nd) {
                dd = 1;
            }
            goto ret;
        }
        b   = multadd(b, 10, 0, PTI);
        dig = quorem(b, d);
    }
    for (j = bc->dp1; i++ < nd;) {
        if ((dd = s0[j++] - '0' - dig)) {
            goto ret;
        }
        if (!b->x[0] && b->wds == 1) {
            if (i < nd) {
                dd = 1;
            }
            goto ret;
        }
        b   = multadd(b, 10, 0, PTI);
        dig = quorem(b, d);
    }
    if (dig > 0 || b->x[0] || b->wds > 1) {
        dd = -1;
    }
ret:
    Bfree(b, PTI);
    Bfree(d, PTI);
    if (speccase) {
        if (dd <= 0) {
            rv->d = 0.;
        }
    } else if (dd < 0) {
        if (!dsign) {  // does not happen for round-near
        retlow1:
            rv->d -= sulp(rv, bc);
        }
    } else if (dd > 0) {
        if (dsign) {
        rethi1:
            rv->d += sulp(rv, bc);
        }
    } else {
        // Exact half-way case:  apply round-even rule.
        if ((j = ((word0(rv) & Exp_mask) >> Exp_shift) - bc->scale) <= 0) {
            i = 1 - j;
            if (i <= 31) {
                if (word1(rv) & (0x1 << i)) {
                    goto odd;
                }
            } else if (word0(rv) & (0x1 << (i - 32))) {
                goto odd;
            }
        } else if (word1(rv) & 1) {
        odd:
            if (dsign) {
                goto rethi1;
            }
            goto retlow1;
        }
    }
}

typedef struct {
    const char* data;
    size_t      size;
} string_view;

static char string_view_deref(string_view s) { return s.size ? *s.data : '\0'; }
static void string_view_next(string_view* s) { ++s->data, --s->size; }
static void string_view_prev(string_view* s) { --s->data, ++s->size; }

static double pn_strtod3(string_view s00, pn_error_code_t* error) {
    int     j;
    Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;
    ThInfo* TI = 0;

    *error         = PN_OK;
    U           rv = {.d = 0.0};
    string_view s  = s00;
    if (!s.size) {
        // ERROR: empty input
        *error = PN_ERROR_INVALID_FLOAT;
        return 0.0;
    }

    // Consume initial zeros. If that's everything, return 0.0.
    bool leading_zero = false;
    if (string_view_deref(s) == '0') {
        leading_zero = true;
        while ((string_view_next(&s), string_view_deref(s)) == '0') {
        }
        if (!s.size) {
            return 0.0;
        }
    }

    // Consume digits before decimal point. First one will be non-zero. Record their value into
    // `acc`, up to the first 19 digits (any more will overflow a uint64_t).
    string_view s0 = s;
    int         c, ndigits = 0;
    uint64_t    acc = 0;
    for (; (c = string_view_deref(s)) >= '0' && c <= '9'; ndigits++, string_view_next(&s)) {
        if (ndigits < 19) {
            acc = 10 * acc + c - '0';
        }
    }

    // Walk backwards to determine the number of trailing zeroes; record to `nz1`.
    BCinfo bc  = {.dplen = 0, .uflchk = 0, .dp0 = s.data - s0.data, .dp1 = s.data - s0.data};
    int    nd0 = ndigits, nz1 = 0;
    for (string_view s1 = s;
         s1.data > s0.data && (string_view_prev(&s1), string_view_deref(s1) == '0');) {
        ++nz1;
    }

    // If there's a decimal, process digits after it.
    int nz = 0, nf = 0;
    if (c == '.') {
        c        = (string_view_next(&s), string_view_deref(s));
        bc.dp1   = s.data - s0.data;
        bc.dplen = bc.dp1 - bc.dp0;

        // If it was all zeroes so far, keep counting them.
        if (!ndigits) {
            for (; c == '0'; c = (string_view_next(&s), string_view_deref(s))) {
                nz++;
            }
            if (c > '0' && c <= '9') {
                bc.dp0 = s0.data - s.data;
                bc.dp1 = bc.dp0 + bc.dplen;
                s0     = s;
                nf += nz;
                nz = 0;
            } else {
                goto dig_done;
            }
        }

        // If there are non-zero digits, record them to `acc`.
        for (; c >= '0' && c <= '9'; c = (string_view_next(&s), string_view_deref(s))) {
            nz++;
            if (c -= '0') {
                nf += nz;
                for (int i = 1; i < nz; ++i) {
                    if (++ndigits <= 19) {
                        acc *= 10;
                    }
                }
                if (++ndigits <= 19) {
                    acc = 10 * acc + c;
                }
                nz = nz1 = 0;
            }
        }
    }

dig_done:;
    int e = 0;
    if (c == 'e' || c == 'E') {
        if (!ndigits && !nz && !leading_zero) {
            // ERROR: nothing before exponent
            *error = PN_ERROR_INVALID_FLOAT;
            return 0.0;
        }
        s00       = s;
        int esign = 0;
        switch (c = (string_view_next(&s), string_view_deref(s))) {
            case '-': esign = 1;
            case '+': c = (string_view_next(&s), string_view_deref(s));
        }
        if (c >= '0' && c <= '9') {
            while (c == '0') {
                c = (string_view_next(&s), string_view_deref(s));
            }
            if (c > '0' && c <= '9') {
                int32_t     L  = c - '0';
                string_view s1 = s;
                while ((c = (string_view_next(&s), string_view_deref(s))) >= '0' && c <= '9') {
                    if (L < 20000) {
                        L = 10 * L + c - '0';
                    }
                }
                if (s.data - s1.data > 8 || L > 19999) {
                    // Avoid confusion from exponents
                    // so large that e might overflow.
                    e = 19999;  // safe for 16 bit ints
                } else {
                    e = (int)L;
                }
                if (esign) {
                    e = -e;
                }
            }
        } else {
            s = s00;
        }
    }

    // We should have processed all of `s` by now. If not, fail.
    if (s.size) {
        *error = PN_ERROR_INVALID_FLOAT;
        return 0.0;
    }

    // If we didn't see any non-zero digits, return immediately; this is a failure if we didn't see
    // any zeroes either.
    if (!ndigits) {
        *error = (nz || leading_zero) ? PN_OK : PN_ERROR_INVALID_FLOAT;
        return 0.0;
    }
    int e1 = bc.e0 = e -= nf;

    // Now we have nd0 digits, starting at s0, followed by a
    // decimal point, followed by ndigits-nd0 digits.  The number we're
    // after is the integer represented by those digits times
    // 10**e

    if (!nd0) {
        nd0 = ndigits;
    }
    bd0 = 0;
    if (ndigits <= DBL_DIG) {
        rv.d = acc;
        if (!e) {
            return rv.d;
        }
        if (e > 0) {
            if (e <= Ten_pmax) {
                return rv.d * tens[e];
            }
            int i = DBL_DIG - ndigits;
            if (e <= Ten_pmax + i) {
                // A fancier test would sometimes let us do this for larger i values.
                e -= i;
                return rv.d * tens[i] * tens[e];
            }
        } else if (e >= -Ten_pmax) {
            return rv.d / tens[-e];
        }
    }
    int k = ndigits < 19 ? ndigits : 19;
    e1 += ndigits - k;  // scale factor = 10^e1

    Debug(++dtoa_stats[0]);
    int i = e1 + 342;
    if (i < 0) {
        goto underflow;
    } else if (i > 650) {
        goto overflow;
    }

    const BF96* p10 = &pten[i];
    uint64_t    terv, brv = acc;
    // shift brv left, with i =  number of bits shifted
    i = 0;
    if (!(brv & 0xffffffff00000000ull)) {
        i = 32;
        brv <<= 32;
    }
    if (!(brv & 0xffff000000000000ull)) {
        i += 16;
        brv <<= 16;
    }
    if (!(brv & 0xff00000000000000ull)) {
        i += 8;
        brv <<= 8;
    }
    if (!(brv & 0xf000000000000000ull)) {
        i += 4;
        brv <<= 4;
    }
    if (!(brv & 0xc000000000000000ull)) {
        i += 2;
        brv <<= 2;
    }
    if (!(brv & 0x8000000000000000ull)) {
        i += 1;
        brv <<= 1;
    }
    int erv = (64 + 0x3fe) + p10->e - i;
    if (erv <= 0 && ndigits > 19) {
        goto many_digits;  // denormal: may need to look at all digits
    }
    uint64_t bhi = brv >> 32;
    uint64_t blo = brv & 0xffffffffull;
    // Unsigned 32-bit ints lie in [0,2^32-1] and
    // unsigned 64-bit ints lie in [0, 2^64-1].  The product of two unsigned
    // 32-bit ints is <= 2^64 - 2*2^32-1 + 1 = 2^64 - 1 - 2*(2^32 - 1), so
    // we can add two unsigned 32-bit ints to the product of two such ints,
    // and 64 bits suffice to contain the result.
    uint64_t t01 = bhi * p10->b1;
    uint64_t t10 = blo * p10->b0 + (t01 & 0xffffffffull);
    uint64_t t00 = bhi * p10->b0 + (t01 >> 32) + (t10 >> 32);
    if (t00 & 0x8000000000000000ull) {
        if ((t00 & 0x3ff) && (~t00 & 0x3fe)) {  // unambiguous result?
            if (ndigits > 19 && ((t00 + (1 << i) + 2) & 0x400) ^ (t00 & 0x400)) {
                goto many_digits;
            } else if (erv <= 0) {
                goto denormal;
            } else if (t00 & 0x400 && t00 & 0xbff) {
                goto roundup;
            }
            goto noround;
        }
    } else {
        if ((t00 & 0x1ff) && (~t00 & 0x1fe)) {  // unambiguous result?
            if (ndigits > 19 && ((t00 + (1 << i) + 2) & 0x200) ^ (t00 & 0x200)) {
                goto many_digits;
            } else if (erv <= 1) {
                goto denormal1;
            } else if (t00 & 0x200) {
                goto roundup1;
            }
            goto noround1;
        }
    }
    // 3 multiplies did not suffice; try a 96-bit approximation
    Debug(++dtoa_stats[1]);
    uint64_t t02    = bhi * p10->b2;
    uint64_t t11    = blo * p10->b1 + (t02 & 0xffffffffull);
    bool     bexact = true;
    if (e1 < 0 || e1 > 41 || (t10 | t11) & 0xffffffffull || ndigits > 19) {
        bexact = false;
    }
    uint64_t tlo = (t10 & 0xffffffffull) + (t02 >> 32) + (t11 >> 32);
    if (!bexact && (tlo + 0x10) >> 32 > tlo >> 32) {
        goto many_digits;
    }
    t00 += tlo >> 32;
    if (t00 & 0x8000000000000000ull) {
        if (erv <= 0) {  // denormal result
            if (ndigits >= 20 || !((tlo & 0xfffffff0) | (t00 & 0x3ff))) {
                goto many_digits;
            }
        denormal:
            if (erv <= -52) {
                if (erv < -52 || !(t00 & 0x7fffffffffffffffull)) {
                    goto underflow;
                }
                goto tiniest;
            }
            uint64_t tg = 1ull << (11 - erv);
            t00 &= ~(tg - 1);  // clear low bits
            if (t00 & tg) {
                t00 += tg << 1;
                if (!(t00 & 0x8000000000000000ull)) {
                    if (++erv > 0) {
                        goto smallest_normal;
                    }
                    t00 = 0x8000000000000000ull;
                }
            }
            rv.LL  = t00 >> (12 - erv);
            *error = PN_ERROR_FLOAT_OVERFLOW;
            return rv.d;
        }
        if (bexact) {
            if (t00 & 0x400 && (tlo & 0xffffffff) | (t00 & 0xbff)) {
                goto roundup;
            }
            goto noround;
        }
        if ((tlo & 0xfffffff0) | (t00 & 0x3ff) &&
            (ndigits <= 19 ||
             ((t00 + (1ull << i)) & 0xfffffffffffffc00ull) == (t00 & 0xfffffffffffffc00ull))) {
            // Unambiguous result.
            // If ndigits > 19, then incrementing the 19th digit
            // does not affect rv.
            if (t00 & 0x400) {  // round up
            roundup:
                t00 += 0x800;
                if (!(t00 & 0x8000000000000000ull)) {
                    // rounded up to a power of 2
                    if (erv >= 0x7fe) {
                        goto overflow;
                    }
                    terv  = erv + 1;
                    rv.LL = terv << 52;
                    return rv.d;
                }
            }
        noround:
            if (erv >= 0x7ff) {
                goto overflow;
            }
            terv  = erv;
            rv.LL = (terv << 52) | ((t00 & 0x7ffffffffffff800ull) >> 11);
            return rv.d;
        }
    } else {
        if (erv <= 1) {  // denormal result
            if (ndigits >= 20 || !((tlo & 0xfffffff0) | (t00 & 0x1ff))) {
                goto many_digits;
            }
        denormal1:
            if (erv <= -51) {
                if (erv < -51 || !(t00 & 0x3fffffffffffffffull)) {
                    goto underflow;
                }
            tiniest:
                rv.LL  = 1;
                *error = PN_ERROR_FLOAT_OVERFLOW;
                return rv.d;
            }
            uint64_t tg = 1ull << (11 - erv);
            if (t00 & tg) {
                if (0x8000000000000000ull & (t00 += (tg << 1)) && erv == 1) {
                smallest_normal:
                    rv.LL = 0x0010000000000000ull;
                    return rv.d;
                }
            }
            if (erv <= -52) {
                goto underflow;
            }
            rv.LL  = t00 >> (12 - erv);
            *error = PN_ERROR_FLOAT_OVERFLOW;
            return rv.d;
        }
        if (bexact) {
            if (t00 & 0x200 && (t00 & 0x5ff || tlo)) {
                goto roundup1;
            }
            goto noround1;
        }
        if ((tlo & 0xfffffff0) | (t00 & 0x1ff) &&
            (ndigits <= 19 ||
             ((t00 + (1ull << i)) & 0x7ffffffffffffe00ull) == (t00 & 0x7ffffffffffffe00ull))) {
            // Unambiguous result.
            if (t00 & 0x200) {  // round up
            roundup1:
                t00 += 0x400;
                if (!(t00 & 0x4000000000000000ull)) {
                    // rounded up to a power of 2
                    if (erv >= 0x7ff) {
                        goto overflow;
                    }
                    terv  = erv;
                    rv.LL = terv << 52;
                    return rv.d;
                }
            }
        noround1:
            if (erv >= 0x800) {
                goto overflow;
            }
            terv  = erv - 1;
            rv.LL = (terv << 52) | ((t00 & 0x3ffffffffffffc00ull) >> 10);
            return rv.d;
        }
    }
many_digits:
    Debug(++dtoa_stats[2]);
    uint32_t y, z;
    if (ndigits > 17) {
        if (ndigits > 18) {
            acc /= 100;
            e1 += 2;
        } else {
            acc /= 10;
            e1 += 1;
        }
        y = acc / 100000000;
    } else if (ndigits > 9) {
        i = ndigits - 9;
        y = (acc >> i) / pfive[i - 1];
    } else {
        y = acc;
    }
    rv.d = acc;

    bc.scale = 0;

    // Get starting approximation = rv * 10**e1

    if (e1 > 0) {
        if ((i = e1 & 15)) {
            rv.d *= tens[i];
        }
        if (e1 &= ~15) {
            if (e1 > DBL_MAX_10_EXP) {
                goto overflow;
            }
            e1 >>= 4;
            for (j = 0; e1 > 1; j++, e1 >>= 1) {
                if (e1 & 1) {
                    rv.d *= bigtens[j];
                }
            }
            // The last multiplication could overflow.
            word0(&rv) -= P * Exp_msk1;
            rv.d *= bigtens[j];
            if ((z = word0(&rv) & Exp_mask) > Exp_msk1 * (DBL_MAX_EXP + Bias - P)) {
                goto overflow;
            }
            if (z > Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P)) {
                // set to largest number
                // (Can't trust DBL_MAX)
                rv.LL = UINT64_C(0x7fefffffffffffff);
            } else {
                word0(&rv) += P * Exp_msk1;
            }
        }
    } else if (e1 < 0) {
        e1 = -e1;
        if ((i = e1 & 15)) {
            rv.d /= tens[i];
        }
        if (e1 >>= 4) {
            if (e1 >= 1 << n_bigtens) {
                goto underflow;
            }
            if (e1 & Scale_Bit) {
                bc.scale = 2 * P;
            }
            for (j = 0; e1 > 0; j++, e1 >>= 1) {
                if (e1 & 1) {
                    rv.d *= tinytens[j];
                }
            }
            if (bc.scale && (j = 2 * P + 1 - ((word0(&rv) & Exp_mask) >> Exp_shift)) > 0) {
                // scaled rv is denormal; clear j low bits
                if (j >= 32) {
                    if (j > 54) {
                        goto underflow;
                    }
                    word1(&rv) = 0;
                    if (j >= 53) {
                        word0(&rv) = (P + 2) * Exp_msk1;
                    } else {
                        word0(&rv) &= 0xffffffff << (j - 32);
                    }
                } else {
                    word1(&rv) &= 0xffffffff << j;
                }
            }
            if (!rv.d) {
                goto underflow;
            }
        }
    }

    // Now the hard part -- adjusting rv to the correct value.

    // Put digits into bd: true value = bd * 10^e

    bc.nd  = ndigits - nz1;
    bc.nd0 = nd0;  // Only needed if nd > strtod_diglim, but done here
                   // to silence an erroneous warning about bc.nd0
                   // possibly not being initialized.
    if (ndigits > strtod_diglim) {
        // ASSERT(strtod_diglim >= 18); 18 == one more than the
        // minimum number of decimal digits to distinguish double values
        // in IEEE arithmetic.
        i = j = 18;
        if (i > nd0) {
            j += bc.dplen;
        }
        for (;;) {
            if (--j < bc.dp1 && j >= bc.dp0) {
                j = bc.dp0 - 1;
            }
            if (s0.data[j] != '0') {
                break;
            }
            --i;
        }
        e += ndigits - i;
        ndigits = i;
        if (nd0 > ndigits) {
            nd0 = ndigits;
        }
        if (ndigits < 9) {  // must recompute y
            y = 0;
            for (i = 0; i < nd0; ++i) {
                y = 10 * y + s0.data[i] - '0';
            }
            for (j = bc.dp1; i < ndigits; ++i) {
                y = 10 * y + s0.data[j++] - '0';
            }
        }
    }
    bd0 = s2b(s0.data, nd0, ndigits, y, bc.dplen, &TI);

    bool req_bigcomp = false;
    for (;;) {
        int      bb2, bb5, bbe, bd2, bd5, bbbits, bs2;
        uint32_t Lsb, Lsb1;
        U        aadj2, adj;

        bd = Balloc(bd0->k, &TI);
        Bcopy(bd, bd0);
        bb = d2b(&rv, &bbe, &bbbits, &TI);  // rv = bb * 2^bbe
        bs = i2b(1, &TI);

        if (e >= 0) {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        } else {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0) {
            bb2 += bbe;
        } else {
            bd2 -= bbe;
        }
        bs2  = bb2;
        Lsb  = LSB;
        Lsb1 = 0;
        j    = bbe - bc.scale;
        i    = j + bbbits - 1;  // logb(rv)
        j    = P + 1 - bbbits;
        if (i < Emin) {  // denormal
            i = Emin - i;
            j -= i;
            if (i < 32) {
                Lsb <<= i;
            } else if (i < 52) {
                Lsb1 = Lsb << (i - 32);
            } else {
                Lsb1 = Exp_mask;
            }
        }
        bb2 += j;
        bd2 += j;
        bd2 += bc.scale;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2) {
            i = bs2;
        }
        if (i > 0) {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0) {
            bs  = pow5mult(bs, bb5, &TI);
            bb1 = mult(bs, bb, &TI);
            Bfree(bb, &TI);
            bb = bb1;
        }
        if (bb2 > 0) {
            bb = lshift(bb, bb2, &TI);
        }
        if (bd5 > 0) {
            bd = pow5mult(bd, bd5, &TI);
        }
        if (bd2 > 0) {
            bd = lshift(bd, bd2, &TI);
        }
        if (bs2 > 0) {
            bs = lshift(bs, bs2, &TI);
        }
        delta       = diff(bb, bd, &TI);
        bc.dsign    = delta->sign;
        delta->sign = 0;
        i           = cmp(delta, bs);
        if (bc.nd > ndigits && i <= 0) {
            if (bc.dsign) {
                // Must use bigcomp().
                req_bigcomp = true;
                break;
            }
            i = -1;  // Discarded digits make delta smaller.
        }

        if (i < 0) {
            // Error is less than half an ulp -- check for
            // special case of mantissa a power of two.
            if (bc.dsign || word1(&rv) || word0(&rv) & Bndry_mask ||
                (word0(&rv) & Exp_mask) <= (2 * P + 1) * Exp_msk1) {
                break;
            }
            if (!delta->x[0] && delta->wds <= 1) {
                // exact result
                break;
            }
            delta = lshift(delta, Log2P, &TI);
            if (cmp(delta, bs) > 0) {
                goto drop_down;
            }
            break;
        }
        if (i == 0) {
            // exactly half-way between
            if (bc.dsign) {
                if ((word0(&rv) & Bndry_mask1) == Bndry_mask1 &&
                    word1(&rv) == ((bc.scale && (y = word0(&rv) & Exp_mask) <= 2 * P * Exp_msk1)
                                           ? (0xffffffff &
                                              (0xffffffff << (2 * P + 1 - (y >> Exp_shift))))
                                           : 0xffffffff)) {
                    // boundary case -- increment exponent
                    if (rv.LL == UINT64_C(0x7fefffffffffffff)) {
                        goto overflow;
                    }
                    word0(&rv) = (word0(&rv) & Exp_mask) + Exp_msk1;
                    word1(&rv) = 0;
                    bc.dsign   = 0;
                    break;
                }
            } else if (!(word0(&rv) & Bndry_mask) && !word1(&rv)) {
            drop_down:
                // boundary case -- decrement exponent
                if (bc.scale) {
                    int32_t L = word0(&rv) & Exp_mask;
                    if (L <= (2 * P + 1) * Exp_msk1) {
                        if (L > (P + 2) * Exp_msk1) {
                            // round even ==>
                            // accept rv
                            break;
                        }
                        // rv = smallest denormal
                        if (bc.nd > ndigits) {
                            bc.uflchk = 1;
                            break;
                        }
                        goto underflow;
                    }
                }
                int32_t L  = (word0(&rv) & Exp_mask) - Exp_msk1;
                word0(&rv) = L | Bndry_mask1;
                word1(&rv) = 0xffffffff;
                if (bc.nd > ndigits) {
                    goto cont;
                }
                break;
            }
            if (Lsb1) {
                if (!(word0(&rv) & Lsb1)) {
                    break;
                }
            } else if (!(word1(&rv) & Lsb)) {
                break;
            }
            if (bc.dsign) {
                rv.d += sulp(&rv, &bc);
            } else {
                rv.d -= sulp(&rv, &bc);
                if (!rv.d) {
                    if (bc.nd > ndigits) {
                        bc.uflchk = 1;
                        break;
                    }
                    goto underflow;
                }
            }
            bc.dsign = 1 - bc.dsign;
            break;
        }

        double aadj, aadj1;
        if ((aadj = ratio(delta, bs)) <= 2.) {
            if (bc.dsign) {
                aadj = aadj1 = 1.;
            } else if (word1(&rv) || word0(&rv) & Bndry_mask) {
                if (word1(&rv) == Tiny1 && !word0(&rv)) {
                    if (bc.nd > ndigits) {
                        bc.uflchk = 1;
                        break;
                    }
                    goto underflow;
                }
                aadj  = 1.;
                aadj1 = -1.;
            } else {
                // special case -- power of FLT_RADIX to be
                // rounded down...

                if (aadj < 2. / FLT_RADIX) {
                    aadj = 1. / FLT_RADIX;
                } else {
                    aadj *= 0.5;
                }
                aadj1 = -aadj;
            }
        } else {
            aadj *= 0.5;
            aadj1 = bc.dsign ? aadj : -aadj;
        }
        y = word0(&rv) & Exp_mask;

        // Check for overflow

        if (y == Exp_msk1 * (DBL_MAX_EXP + Bias - 1)) {
            U rv0;
            rv0.d = rv.d;
            word0(&rv) -= P * Exp_msk1;
            adj.d = aadj1 * ulp(&rv);
            rv.d += adj.d;
            if ((word0(&rv) & Exp_mask) >= Exp_msk1 * (DBL_MAX_EXP + Bias - P)) {
                if (rv0.LL == UINT64_C(0x7fefffffffffffff)) {
                    goto overflow;
                }
                rv.LL = UINT64_C(0x7fefffffffffffff);
                goto cont;
            } else
                word0(&rv) += P * Exp_msk1;
        } else {
            if (bc.scale && y <= 2 * P * Exp_msk1) {
                if (aadj <= 0x7fffffff) {
                    if ((z = aadj) <= 0) {
                        z = 1;
                    }
                    aadj  = z;
                    aadj1 = bc.dsign ? aadj : -aadj;
                }
                aadj2.d = aadj1;
                word0(&aadj2) += (2 * P + 1) * Exp_msk1 - y;
                aadj1 = aadj2.d;
                adj.d = aadj1 * ulp(&rv);
                rv.d += adj.d;
                if (rv.d == 0.) {
                    req_bigcomp = true;
                    break;
                }
            } else {
                adj.d = aadj1 * ulp(&rv);
                rv.d += adj.d;
            }
        }
        z = word0(&rv) & Exp_mask;
    cont:
        Bfree(bb, &TI);
        Bfree(bd, &TI);
        Bfree(bs, &TI);
        Bfree(delta, &TI);
    }
    Bfree(bb, &TI);
    Bfree(bd, &TI);
    Bfree(bs, &TI);
    Bfree(bd0, &TI);
    Bfree(delta, &TI);
    if (req_bigcomp) {
        bd0 = 0;
        bc.e0 += nz1;
        bigcomp(&rv, s0.data, &bc, &TI);
        y = word0(&rv) & Exp_mask;
        if (y == Exp_mask) {
            goto overflow;
        }
        if (y == 0 && rv.d == 0.) {
            goto underflow;
        }
    }
    if (bc.scale) {
        U rv0;
        word0(&rv0) = Exp_1 - 2 * P * Exp_msk1;
        word1(&rv0) = 0;
        rv.d *= rv0.d;
        // try to avoid the bug of testing an 8087 register value
        if (!(word0(&rv) & Exp_mask)) {
            *error = PN_ERROR_FLOAT_OVERFLOW;
            return rv.d;
        }
    }

    return rv.d;

underflow:
    rv.d = 0.0;
    goto range_err;

overflow:
    // Can't trust HUGE_VAL
    word0(&rv) = Exp_mask;
    word1(&rv) = 0;
    goto range_err;

range_err:
    if (bd0) {
        Bfree(bb, &TI);
        Bfree(bd, &TI);
        Bfree(bs, &TI);
        Bfree(bd0, &TI);
        Bfree(delta, &TI);
    }
    *error = PN_ERROR_FLOAT_OVERFLOW;
    return rv.d;
}

static const uint64_t nan_u64 = UINT64_C(0x7ff8000000000000);
static const uint64_t inf_u64 = UINT64_C(0x7ff0000000000000);

#if (defined(__i386__) || defined(__x86_64__)) && !defined(DTOA_IS_SSE2_ARCH)
static uint16_t fpu_get_control_word() {
    uint16_t control_word;
    __asm__ __volatile__("fnstcw %0" : "=m"(control_word));
    return control_word;
}

static void fpu_set_control_word(uint16_t control_word) {
    __asm__ __volatile__("fldcw %0" : : "m"(control_word));
}
#endif

static bool pn_strtod2(const char* data, size_t size, double* f, pn_error_code_t* error) {
    if (size == 0) {
        *f     = 0.0;
        *error = PN_ERROR_INVALID_FLOAT;
        return false;
    } else if ((size == 3) && (memcmp(data, "inf", size) == 0)) {
        memcpy(f, &inf_u64, sizeof(double));
        return true;
    }
    string_view s = {data, size};

#if defined(DTOA_IS_SSE2_ARCH)
    unsigned int old_csr = _mm_getcsr();
    // Disable floating point exceptions, round nearest, allow denormals
    _mm_setcsr(_MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_OFF);
    *f = pn_strtod3(s, error);
    _mm_setcsr(old_csr);
#elif defined(__i386__) || defined(__x86_64__)
    uint16_t save_control_word    = fpu_get_control_word();
    uint16_t use_double_precision = (save_control_word & ~0x0f00) | 0x0200;
    if (save_control_word != use_double_precision) {
        fpu_set_control_word(use_double_precision);
    }
    *f = pn_strtod3(s, error);
    if (save_control_word != use_double_precision) {
        fpu_set_control_word(save_control_word);
    }
#elif defined(__arm__) || defined(__arm64__)
    *f = pn_strtod3(s, error);
#else
#error "Unknown architecture"
#endif

    return *error == PN_OK;
}

bool pn_strtod(const char* data, size_t size, double* f, pn_error_code_t* error) {
    pn_error_code_t no_error;
    error = error ? error : &no_error;
    errno = 0;
    if (size == 0) {
        *f     = 0.0;
        *error = PN_ERROR_INVALID_FLOAT;
        return false;
    } else if ((size == 3) && (memcmp(data, "nan", size) == 0)) {
        memcpy(f, &nan_u64, sizeof(double));
        return true;
    }
    bool neg = false;
    switch (*data) {
        case '-':
            neg = true;
            ++data;
            --size;
            break;
        case '+':
            ++data;
            --size;
            break;
    }
    bool result = pn_strtod2(data, size, f, error);
    if (neg) {
        *f = -*f;
    }
    return result;
}

static char* rv_alloc(int i, ThInfo** PTI) {
    int j, k, *r;

    j = sizeof(uint32_t);
    for (k = 0; sizeof(Bigint) - sizeof(uint32_t) - sizeof(int) + j <= (size_t)i; j <<= 1) {
        k++;
    }
    r  = (int*)Balloc(k, PTI);
    *r = k;
    return (char*)(r + 1);
}

static char* nrv_alloc(const char* s, char* s0, size_t s0len, char** rve, int n, ThInfo** PTI) {
    char *rv, *t;

    if (!s0) {
        s0 = rv_alloc(n, PTI);
    } else if (s0len <= (size_t)n) {
        rv = 0;
        t  = rv + n;
        goto rve_chk;
    }
    t = rv = s0;
    while ((*t = *s++)) {
        ++t;
    }
rve_chk:
    if (rve) {
        *rve = t;
    }
    return rv;
}

// freedtoa(s) must be used to free values s returned by dtoa

void freedtoa(char* s) {
    ThInfo* TI = 0;
    Bigint* b  = (Bigint*)((int*)s - 1);
    b->maxwds  = 1 << (b->k = *(int*)b);
    Bfree(b, &TI);
}

// dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
//
// Inspired by "How to Print Floating-Point Numbers Accurately" by
// Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 112-126].
//
// Modifications:
//      1. Rather than iterating, we use a simple numeric overestimate
//         to determine k = floor(log10(d)).  We scale relevant
//         quantities using O(log2(k)) rather than O(k) multiplications.
//      2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
//         try to generate digits strictly left to right.  Instead, we
//         compute with fewer bits and propagate the carry if necessary
//         when rounding the final digit up.  This is often faster.
//      3. Under the assumption that input will be rounded nearest,
//         mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
//         That is, we allow equality in stopping tests when the
//         round-nearest rule will give the same floating-point value
//         as would satisfaction of the stopping test with strict
//         inequality.
//      4. We remove common factors of powers of 2 from relevant
//         quantities.
//      5. When converting floating-point integers less than 1e16,
//         we use floating-point arithmetic rather than resorting
//         to multiple-precision integers.
//      6. When asked to produce fewer than 15 digits, we first try
//         to get by with floating-point arithmetic; we resort to
//         multiple-precision integer arithmetic only if we cannot
//         guarantee that the floating-point calculation has given
//         the correctly rounded result.  For k requested digits and
//         "uniformly" distributed input, the probability is
//         something like 10^(k-15) that we must resort to the Long
//         calculation.

char* dtoa_r(
        double dd, int mode, int ndigits, int* decpt, int* sign, char** rve, char* buf,
        size_t blen) {
    //     Arguments ndigits, decpt, sign are similar to those
    //     of ecvt and fcvt; trailing zeros are suppressed from
    //     the returned string.  If not null, *rve is set to point
    //     to the end of the return value.  If d is +-Infinity or NaN,
    //     then *decpt is set to 9999.
    //
    //     mode:
    //             0 ==> shortest string that yields d when read in
    //                     and rounded to nearest.
    //             1 ==> like 0, but with Steele & White stopping rule;
    //                     e.g. with IEEE P754 arithmetic , mode 0 gives
    //                     1e23 whereas mode 1 gives 9.999999999999999e22.
    //             2 ==> max(1,ndigits) significant digits.  This gives a
    //                     return value similar to that of ecvt, except
    //                     that trailing zeros are suppressed.
    //             3 ==> through ndigits past the decimal point.  This
    //                     gives a return value similar to that from fcvt,
    //                     except that trailing zeros are suppressed, and
    //                     ndigits can be negative.
    //             4,5 ==> similar to 2 and 3, respectively, but (in
    //                     round-nearest mode) with the tests of mode 0 to
    //                     possibly return a shorter string that rounds to d.
    //                     With IEEE arithmetic and compilation with
    //                     -DHonor_FLT_ROUNDS, modes 4 and 5 behave the same
    //                     as modes 2 and 3 when FLT_ROUNDS != 1.
    //             6-9 ==> Debugging modes similar to mode - 4:  don't try
    //                     fast floating-point estimate (if applicable).
    //
    //             Values of mode other than 0-9 are treated as mode 0.
    //
    //     When not NULL, buf is an output buffer of length blen, which must
    //     be large enough to accommodate suppressed trailing zeros and a trailing
    //     null byte.  If blen is too small, rv = NULL is returned, in which case
    //     if rve is not NULL, a subsequent call with blen >= (*rve - rv) + 1
    //     should succeed in returning buf.
    //
    //     When buf is NULL, sufficient space is allocated for the return value,
    //     which, when done using, the caller should pass to freedtoa().
    //
    //     USE_BF is automatically defined when NO_BF96 is not defined

    ThInfo* TI = 0;
    int     bbits, b2, b5, be, dig, i, ilim, ilim1, j, j1, k, leftright, m2, m5, s2, s5, spec_case;
    int     denorm;
    Bigint *b, *b1, *delta, *mlo, *mhi, *S;
    U       u;
    char*   s;
    BF96*   p10;
    uint64_t dbhi, dbits, dblo, den, hb, rb, rblo, res, res0, res3, reslo, sres, sulp, tv0, tv1,
            tv2, tv3, ulp, ulplo, ulpmask, ures, ureslo, zb;
    int eulp, k1, n2, ulpadj, ulpshift;

    u.d = dd;
    if (word0(&u) & Sign_bit) {
        // set sign for everything, including 0's and NaNs
        *sign = 1;
        word0(&u) &= ~Sign_bit;  // clear sign bit
    } else
        *sign = 0;

    if ((word0(&u) & Exp_mask) == Exp_mask) {
        // Infinity or NaN
        *decpt = 9999;
        if (!word1(&u) && !(word0(&u) & 0xfffff)) {
            return nrv_alloc("inf", buf, blen, rve, 8, &TI);
        }
        return nrv_alloc("nan", buf, blen, rve, 3, &TI);
    }
    if (!u.d) {
        *decpt = 1;
        return nrv_alloc("0", buf, blen, rve, 1, &TI);
    }

    dbits = (u.LL & 0xfffffffffffffull) << 11;  // fraction bits
    if ((be = u.LL >> 52)) /* biased exponent; nonzero ==> normal */ {
        dbits |= 0x8000000000000000ull;
        denorm = ulpadj = 0;
    } else {
        denorm = 1;
        ulpadj = be + 1;
        dbits <<= 1;
        if (!(dbits & 0xffffffff00000000ull)) {
            dbits <<= 32;
            be -= 32;
        }
        if (!(dbits & 0xffff000000000000ull)) {
            dbits <<= 16;
            be -= 16;
        }
        if (!(dbits & 0xff00000000000000ull)) {
            dbits <<= 8;
            be -= 8;
        }
        if (!(dbits & 0xf000000000000000ull)) {
            dbits <<= 4;
            be -= 4;
        }
        if (!(dbits & 0xc000000000000000ull)) {
            dbits <<= 2;
            be -= 2;
        }
        if (!(dbits & 0x8000000000000000ull)) {
            dbits <<= 1;
            be -= 1;
        }
        assert(be >= -51);
        ulpadj -= be;
    }
    j    = Lhint[be + 51];
    p10  = &pten[j];
    dbhi = dbits >> 32;
    dblo = dbits & 0xffffffffull;
    i    = be - 0x3fe;
    if (i < p10->e || (i == p10->e && (dbhi < p10->b0 || (dbhi == p10->b0 && dblo < p10->b1)))) {
        --j;
    }
    k = j - 342;

    // now 10^k <= dd < 10^(k+1)

    if (mode < 0 || mode > 9) {
        mode = 0;
    }

    if (mode > 5) {
        mode -= 4;
    }
    leftright = 1;
    ilim = ilim1 = -1;  // Values for cases 0 and 1; done here to
                        // silence erroneous "gcc -Wall" warning.
    switch (mode) {
        case 0:
        case 1:
            i       = 18;
            ndigits = 0;
            break;
        case 2: leftright = 0;
        // no break
        case 4:
            if (ndigits <= 0) {
                ndigits = 1;
            }
            ilim = ilim1 = i = ndigits;
            break;
        case 3: leftright = 0;
        // no break
        case 5:
            i     = ndigits + k + 1;
            ilim  = i;
            ilim1 = i - 1;
            if (i <= 0) {
                i = 1;
            }
    }
    if (!buf) {
        buf  = rv_alloc(i, &TI);
        blen = sizeof(Bigint) + ((1 << ((int*)buf)[-1]) - 1) * sizeof(uint32_t) - sizeof(int);
    } else if (blen <= (size_t)i) {
        buf = 0;
        if (rve) {
            *rve = buf + i;
        }
        return buf;
    }
    s = buf;

    // Check for special case that d is a normalized power of 2.

    spec_case = 0;
    if (mode < 2 || (leftright)) {
        if (!word1(&u) && !(word0(&u) & Bndry_mask) && word0(&u) & (Exp_mask & ~Exp_msk1)) {
            // The special case
            spec_case = 1;
        }
    }

    b = 0;
    if (ilim < 0 && (mode == 3 || mode == 5)) {
        S = mhi = 0;
        goto no_digits;
    }
    i        = 1;
    j        = 52 + 0x3ff - be;
    ulpshift = 0;
    ulplo    = 0;
    // Can we do an exact computation with 64-bit integer arithmetic?
    if (k < 0) {
        if (k < -25) {
            goto toobig;
        }
        res = dbits >> 11;
        n2  = pfivebits[k1 = -(k + 1)] + 53;
        j1  = j;
        if (n2 > 61) {
            ulpshift = n2 - 61;
            if (res & (ulpmask = (1ull << ulpshift) - 1)) {
                goto toobig;
            }
            j -= ulpshift;
            res >>= ulpshift;
        }
        // Yes.
        res *= ulp = pfive[k1];
        if (ulpshift) {
            ulplo = ulp;
            ulp >>= ulpshift;
        }
        j += k;
        if (ilim == 0) {
            S = mhi = 0;
            if (res > (5ull << j)) {
                goto one_digit;
            }
            goto no_digits;
        }
        goto no_div;
    }
    if (ilim == 0 && j + k >= 0) {
        S = mhi = 0;
        if ((dbits >> 11) > (pfive[k - 1] << j)) {
            goto one_digit;
        }
        goto no_digits;
    }
    if (k <= dtoa_divmax && j + k >= 0) {
    // Another "yes" case -- we will use exact integer arithmetic.
    use_exact:
        Debug(++dtoa_stats[3]);
        res = dbits >> 11;  // residual
        ulp = 1;
        if (k <= 0) {
            goto no_div;
        }
        j1  = j + k + 1;
        den = pfive[k - i] << (j1 - i);
        for (;;) {
            dig  = res / den;
            *s++ = '0' + dig;
            if (!(res -= dig * den)) {
                goto retc;
            }
            if (ilim < 0) {
                ures = den - res;
                if (2 * res <= ulp && (spec_case ? 4 * res <= ulp : (2 * res < ulp || dig & 1))) {
                    goto ulp_reached;
                }
                if (2 * ures < ulp) {
                    goto Roundup;
                }
            } else if (i == ilim) {
                ures = 2 * res;
                if (ures > den || (ures == den && dig & 1) ||
                    (spec_case && res <= ulp && 2 * res >= ulp)) {
                    goto Roundup;
                }
                goto retc;
            }
            if (j1 < ++i) {
                res *= 10;
                ulp *= 10;
            } else {
                if (i > k) {
                    break;
                }
                den = pfive[k - i] << (j1 - i);
            }
        }
    no_div:
        for (;;) {
            dig = den = res >> j;
            *s++      = '0' + dig;
            if (!(res -= den << j)) {
                goto retc;
            }
            if (ilim < 0) {
                ures = (1ull << j) - res;
                if (2 * res <= ulp && (spec_case ? 4 * res <= ulp : (2 * res < ulp || dig & 1))) {
                ulp_reached:
                    if (ures < res || (ures == res && dig & 1)) {
                        goto Roundup;
                    }
                    goto retc;
                }
                if (2 * ures < ulp) {
                    goto Roundup;
                }
            }
            --j;
            if (i == ilim) {
                hb = 1ull << j;
                if (res & hb && (dig & 1 || res & (hb - 1))) {
                    goto Roundup;
                }
                if (spec_case && res <= ulp && 2 * res >= ulp) {
                Roundup:
                    while (*--s == '9') {
                        if (s == buf) {
                            ++k;
                            *s++ = '1';
                            goto ret1;
                        }
                    }
                    ++*s++;
                    goto ret1;
                }
                goto retc;
            }
            ++i;
            res *= 5;
            if (ulpshift) {
                ulplo = 5 * (ulplo & ulpmask);
                ulp   = 5 * ulp + (ulplo >> ulpshift);
            } else {
                ulp *= 5;
            }
        }
    }
toobig:
    if (ilim > 28) {
        goto Fast_failed1;
    }
    // Scale by 10^-k
    p10  = &pten[342 - k];
    tv0  = p10->b2 * dblo;  // rarely matters, but does, e.g., for 9.862818194192001e18
    tv1  = p10->b1 * dblo + (tv0 >> 32);
    tv2  = p10->b2 * dbhi + (tv1 & 0xffffffffull);
    tv3  = p10->b0 * dblo + (tv1 >> 32) + (tv2 >> 32);
    res3 = p10->b1 * dbhi + (tv3 & 0xffffffffull);
    res  = p10->b0 * dbhi + (tv3 >> 32) + (res3 >> 32);
    be += p10->e - 0x3fe;
    eulp = j1 = be - 54 + ulpadj;
    if (!(res & 0x8000000000000000ull)) {
        --be;
        res3 <<= 1;
        res = (res << 1) | ((res3 & 0x100000000ull) >> 32);
    }
    res0 = res;                                    // save for Fast_failed
#if !defined(SET_INEXACT) && !defined(NO_DTOA_64)  // {
    if (ilim > 19) {
        goto Fast_failed;
    }
    Debug(++dtoa_stats[4]);
    assert(be >= 0 && be <= 4);  // be = 0 is rare, but possible, e.g., for 1e20
    res >>= 4 - be;
    ulp = p10->b0;  // ulp
    ulp = (ulp << 29) | (p10->b1 >> 3);
    // scaled ulp = ulp * 2^(eulp - 60)
    // We maintain 61 bits of the scaled ulp.
    if (ilim == 0) {
        if (!(res & 0x7fffffffffffffeull) || !((~res) & 0x7fffffffffffffeull)) {
            goto Fast_failed1;
        }
        S = mhi = 0;
        if (res >= 0x5000000000000000ull) {
            goto one_digit;
        }
        goto no_digits;
    }
    rb = 1;  // upper bound on rounding error
    for (;; ++i) {
        dig  = res >> 60;
        *s++ = '0' + dig;
        res &= 0xfffffffffffffffull;
        if (ilim < 0) {
            ures = 0x1000000000000000ull - res;
            if (eulp > 0) {
                assert(eulp <= 4);
                sulp = ulp << (eulp - 1);
                if (res <= ures) {
                    if (res + rb > ures - rb) {
                        goto Fast_failed;
                    }
                    if (res < sulp) {
                        goto retc;
                    }
                } else {
                    if (res - rb <= ures + rb) {
                        goto Fast_failed;
                    }
                    if (ures < sulp) {
                        goto Roundup;
                    }
                }
            } else {
                zb = -(1ull << (eulp + 63));
                if (!(zb & res)) {
                    sres = res << (1 - eulp);
                    if (sres < ulp && (!spec_case || 2 * sres < ulp)) {
                        if ((res + rb) << (1 - eulp) >= ulp) {
                            goto Fast_failed;
                        }
                        if (ures < res) {
                            if (ures + rb >= res - rb) {
                                goto Fast_failed;
                            }
                            goto Roundup;
                        }
                        if (ures - rb < res + rb) {
                            goto Fast_failed;
                        }
                        goto retc;
                    }
                }
                if (!(zb & ures) && ures << -eulp < ulp) {
                    if (ures << (1 - eulp) < ulp) {
                        goto Roundup;
                    }
                    goto Fast_failed;
                }
            }
        } else if (i == ilim) {
            ures = 0x1000000000000000ull - res;
            if (ures < res) {
                if (ures <= rb || res - rb <= ures + rb) {
                    if (j + k >= 0 && k >= 0 && k <= 27) {
                        goto use_exact1;
                    }
                    goto Fast_failed;
                }
                goto Roundup;
            }
            if (res <= rb || ures - rb <= res + rb) {
                if (j + k >= 0 && k >= 0 && k <= 27) {
                use_exact1:
                    s = buf;
                    i = 1;
                    goto use_exact;
                }
                goto Fast_failed;
            }
            goto retc;
        }
        rb *= 10;
        if (rb >= 0x1000000000000000ull) {
            goto Fast_failed;
        }
        res *= 10;
        ulp *= 5;
        if (ulp & 0x8000000000000000ull) {
            eulp += 4;
            ulp >>= 3;
        } else {
            eulp += 3;
            ulp >>= 2;
        }
    }
#endif  // }
Fast_failed:
    Debug(++dtoa_stats[5]);
    s     = buf;
    i     = 4 - be;
    res   = res0 >> i;
    reslo = 0xffffffffull & res3;
    if (i) {
        reslo = (res0 << (64 - i)) >> 32 | (reslo >> i);
    }
    rb   = 0;
    rblo = 4;        // roundoff bound
    ulp  = p10->b0;  // ulp
    ulp  = (ulp << 29) | (p10->b1 >> 3);
    eulp = j1;
    for (i = 1;; ++i) {
        dig  = res >> 60;
        *s++ = '0' + dig;
        res &= 0xfffffffffffffffull;
        if (ilim < 0) {
            ures   = 0x1000000000000000ull - res;
            ureslo = 0;
            if (reslo) {
                ureslo = 0x100000000ull - reslo;
                --ures;
            }
            if (eulp > 0) {
                assert(eulp <= 4);
                sulp = (ulp << (eulp - 1)) - rb;
                if (res <= ures) {
                    if (res < sulp) {
                        if (res + rb < ures - rb) {
                            goto retc;
                        }
                    }
                } else if (ures < sulp) {
                    if (res - rb > ures + rb) {
                        goto Roundup;
                    }
                }
                goto Fast_failed1;
            } else {
                zb = -(1ull << (eulp + 60));
                if (!(zb & (res + rb))) {
                    sres = (res - rb) << (1 - eulp);
                    if (sres < ulp && (!spec_case || 2 * sres < ulp)) {
                        sres = res << (1 - eulp);
                        if ((j = eulp + 31) > 0) {
                            sres += (rblo + reslo) >> j;
                        } else {
                            sres += (rblo + reslo) << -j;
                        }
                        if (sres + (rb << (1 - eulp)) >= ulp) {
                            goto Fast_failed1;
                        }
                        if (sres >= ulp) {
                            goto more96;
                        }
                        if (ures < res || (ures == res && ureslo < reslo)) {
                            if (ures + rb >= res - rb) {
                                goto Fast_failed1;
                            }
                            goto Roundup;
                        }
                        if (ures - rb <= res + rb) {
                            goto Fast_failed1;
                        }
                        goto retc;
                    }
                }
                if (!(zb & ures) && (ures - rb) << (1 - eulp) < ulp) {
                    if ((ures + rb) << (1 - eulp) < ulp) {
                        goto Roundup;
                    }
                    goto Fast_failed1;
                }
            }
        } else if (i == ilim) {
            ures = 0x1000000000000000ull - res;
            sres = ureslo = 0;
            if (reslo) {
                ureslo = 0x100000000ull - reslo;
                --ures;
                sres = (reslo + rblo) >> 31;
            }
            sres += 2 * rb;
            if (ures <= res) {
                if (ures <= sres || res - ures <= sres) {
                    goto Fast_failed1;
                }
                goto Roundup;
            }
            if (res <= sres || ures - res <= sres) {
                goto Fast_failed1;
            }
            goto retc;
        }
    more96:
        rblo *= 10;
        rb = 10 * rb + (rblo >> 32);
        rblo &= 0xffffffffull;
        if (rb >= 0x1000000000000000ull) {
            goto Fast_failed1;
        }
        reslo *= 10;
        res = 10 * res + (reslo >> 32);
        reslo &= 0xffffffffull;
        ulp *= 5;
        if (ulp & 0x8000000000000000ull) {
            eulp += 4;
            ulp >>= 3;
        } else {
            eulp += 3;
            ulp >>= 2;
        }
    }
Fast_failed1:
    Debug(++dtoa_stats[6]);
    S = mhi = mlo = 0;
    b             = d2b(&u, &be, &bbits, &TI);
    s             = buf;
    i             = (int)(word0(&u) >> Exp_shift1 & (Exp_mask >> Exp_shift1));
    i -= Bias;
    if (ulpadj) {
        i -= ulpadj - 1;
    }
    j = bbits - i - 1;
    if (j >= 0) {
        b2 = 0;
        s2 = j;
    } else {
        b2 = -j;
        s2 = 0;
    }
    if (k >= 0) {
        b5 = 0;
        s5 = k;
        s2 += k;
    } else {
        b2 -= k;
        b5 = -k;
        s5 = 0;
    }

    m2  = b2;
    m5  = b5;
    mhi = mlo = 0;
    if (leftright) {
        i = denorm ? be + (Bias + (P - 1) - 1 + 1) : 1 + P - bbits;
        b2 += i;
        s2 += i;
        mhi = i2b(1, &TI);
    }
    if (m2 > 0 && s2 > 0) {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
    }
    if (b5 > 0) {
        if (leftright) {
            if (m5 > 0) {
                mhi = pow5mult(mhi, m5, &TI);
                b1  = mult(mhi, b, &TI);
                Bfree(b, &TI);
                b = b1;
            }
            if ((j = b5 - m5)) {
                b = pow5mult(b, j, &TI);
            }
        } else {
            b = pow5mult(b, b5, &TI);
        }
    }
    S = i2b(1, &TI);
    if (s5 > 0) {
        S = pow5mult(S, s5, &TI);
    }

    if (spec_case) {
        b2 += Log2P;
        s2 += Log2P;
    }

    // Arrange for convenient computation of quotients:
    // shift left if necessary so divisor has 4 leading 0 bits.
    //
    // Perhaps we should just compute leading 28 bits of S once
    // and for all and pass them and a shift to quorem, so it
    // can do shifts and ors to compute the numerator for q.
    i = dshift(S, s2);
    b2 += i;
    m2 += i;
    s2 += i;
    if (b2 > 0) {
        b = lshift(b, b2, &TI);
    }
    if (s2 > 0) {
        S = lshift(S, s2, &TI);
    }
    if (ilim <= 0 && (mode == 3 || mode == 5)) {
        if (ilim < 0 || cmp(b, S = multadd(S, 5, 0, &TI)) <= 0) {
        // no digits, fcvt style
        no_digits:
            k = -1 - ndigits;
            goto ret;
        }
    one_digit:
        *s++ = '1';
        ++k;
        goto ret;
    }
    if (leftright) {
        if (m2 > 0) {
            mhi = lshift(mhi, m2, &TI);
        }

        // Compute mlo -- check for special case
        // that d is a normalized power of 2.

        mlo = mhi;
        if (spec_case) {
            mhi = Balloc(mhi->k, &TI);
            Bcopy(mhi, mlo);
            mhi = lshift(mhi, Log2P, &TI);
        }

        for (i = 1;; i++) {
            dig = quorem(b, S) + '0';
            // Do we yet have the shortest decimal string
            // that will round to d?
            j     = cmp(b, mlo);
            delta = diff(S, mhi, &TI);
            j1    = delta->sign ? 1 : cmp(b, delta);
            Bfree(delta, &TI);
            if (j1 == 0 && mode != 1 && !(word1(&u) & 1)) {
                if (dig == '9') {
                    goto round_9_up;
                }
                if (j > 0) {
                    dig++;
                }
                *s++ = dig;
                goto ret;
            }
            if (j < 0 || (j == 0 && mode != 1 && !(word1(&u) & 1))) {
                if (!b->x[0] && b->wds <= 1) {
                    goto accept_dig;
                }
                if (j1 > 0) {
                    b  = lshift(b, 1, &TI);
                    j1 = cmp(b, S);
                    if ((j1 > 0 || (j1 == 0 && dig & 1)) && dig++ == '9') {
                        goto round_9_up;
                    }
                }
            accept_dig:
                *s++ = dig;
                goto ret;
            }
            if (j1 > 0) {
                if (dig == '9') {  // possible if i == 1
                round_9_up:
                    *s++ = '9';
                    goto roundoff;
                }
                *s++ = dig + 1;
                goto ret;
            }
            *s++ = dig;
            if (i == ilim) {
                break;
            }
            b = multadd(b, 10, 0, &TI);
            if (mlo == mhi) {
                mlo = mhi = multadd(mhi, 10, 0, &TI);
            } else {
                mlo = multadd(mlo, 10, 0, &TI);
                mhi = multadd(mhi, 10, 0, &TI);
            }
        }
    } else {
        for (i = 1;; i++) {
            dig  = quorem(b, S) + '0';
            *s++ = dig;
            if (!b->x[0] && b->wds <= 1) {
                goto ret;
            }
            if (i >= ilim)
                break;
            b = multadd(b, 10, 0, &TI);
        }
    }

    // Round off last digit

    b = lshift(b, 1, &TI);
    j = cmp(b, S);
    if (j > 0 || (j == 0 && dig & 1)) {
    roundoff:
        while (*--s == '9') {
            if (s == buf) {
                k++;
                *s++ = '1';
                goto ret;
            }
        }
        ++*s++;
    }
ret:
    Bfree(S, &TI);
    if (mhi) {
        if (mlo && mlo != mhi) {
            Bfree(mlo, &TI);
        }
        Bfree(mhi, &TI);
    }
retc:
    while (s > buf && s[-1] == '0') {
        --s;
    }
ret1:
    if (b) {
        Bfree(b, &TI);
    }
    *s     = 0;
    *decpt = k + 1;
    if (rve) {
        *rve = s;
    }
    return buf;
}

char* pn_dtoa(char* b, double x) {
    register int   i, k;
    register char* s;
    int            decpt, j, sign;
    char *         b0, *s0, *se;

    b0 = b;
    if (signbit(x)) {
        *b++ = '-';
        x    = -x;
    }

    switch (fpclassify(x)) {
        case FP_INFINITE: strcpy(b, "inf"); return b0;
        case FP_NAN: strcpy(b0, "nan"); return b0;
        case FP_ZERO: strcpy(b, "0.0"); return b0;
        case FP_NORMAL: break;
        case FP_SUBNORMAL: break;
    }
    s = s0 = dtoa_r(x, 0, 0, &decpt, &sign, &se, 0, 0);
    if (decpt == 9999) /* Infinity or Nan */ {
        while ((*b++ = *s++))
            ;
        goto done;
    }

    if (decpt <= -4 || decpt > se - s + 15) {
        *b++ = *s++;
        if (*s) {
            *b++ = '.';
            while ((*b = *s++))
                b++;
        }
        *b++ = 'e';
        /* sprintf(b, "%+.2d", decpt - 1); */
        if (--decpt < 0) {
            *b++  = '-';
            decpt = -decpt;
        } else
            *b++ = '+';
        for (j = 2, k = 10; 10 * k <= decpt; j++, k *= 10)
            ;
        for (;;) {
            i    = decpt / k;
            *b++ = i + '0';
            if (--j <= 0)
                break;
            decpt -= i * k;
            decpt *= 10;
        }
        *b = 0;
    } else if (decpt <= 0) {
        *b++ = '0';
        *b++ = '.';
        for (; decpt < 0; decpt++)
            *b++ = '0';
        while ((*b++ = *s++))
            ;
    } else {
        while ((*b = *s++)) {
            b++;
            if (--decpt == 0) {
                *b++ = '.';
                if (!*s) {
                    *b++ = '0';
                }
            }
        }
        if (decpt > 0) {
            for (; decpt > 0; decpt--)
                *b++ = '0';
            *b++ = '.';
            *b++ = '0';
            *b   = 0;
        }
    }
done:
    freedtoa(s0);
    return b0;
}
