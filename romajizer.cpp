/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QTextStream>

#include "zkanjimain.h"
#include "romajizer.h"

#include "checked_cast.h"

#define KANAVOWEL(c) (c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o')
#define KANACONSONANT(x) (x == 'k' || x == 'g' || x == 's' || x == 'z' || x == 't' || \
                          x == 'd' || x == 'm' || x == 'h' || x == 'b' || x == 'p' || \
                          x == 'r' || x == 'y' || x == 'w' || x == 'c' || x == 'n' || \
                          x == 'v' || x == 'f')


const char* kanainput[] =
{
    "a",
    "i",
    "u",
    "e",
    "o",

    "yi", "ye", "ya", "yu", "yo",

    "whu", "wu", "wha", "wi", "whi", "we",
    "whe", "who", "wa", "wo",

    "la", "li", "lyi", "lu", "le", "lye", "lo", "lka", "lke",
    "ltsu", "ltu", "lya", "lyu", "lyo", "lwa",

    "xa", "xi", "xyi", "xu", "xe", "xye", "xo", "xka", "xke",
    "xtsu", "xtu", "xya", "xyu", "xyo", "xwa", "xn",

    "vu", "va", "vi", "vyi", "ve", "vye",
    "vo", "vya", "vyu", "vyo",

    "ca", "co", "cu", "ci", "ce",
    "chi", "cha", "cya", "chu", "cyu", "cho", "cyo",
    "che", "cye", "tye",

    "qu", "qa", "qwa", "qi", "qwi", "qwu",
    "qe", "qwe", "qo", "qwo",

    "ka", "ki", "ku", "ke", "ko",
    "kya", "kyu", "kyo", "kyi", "kye",

    "ga", "gi", "gu", "ge", "go",
    "gya", "gyu", "gyo",
    "gyi", "gye",
    "gwa", "gwi", "gwu", "gwe", "gwo",

    "sa", "shi", "si", "su", "se", "so",
    "sha", "sya", "shu", "syu", "sho", "syo",
    "she",
    "swa", "swi", "swu", "swe", "swo",

    "za", "zi", "zu", "ze", "zo", "zya", "zyu", "zyo",
    "zyi", "zye",

    "ji", "ja", "jya", "ju", "jyu", "jo", "jyo",
    "jyi", "je", "jye",

    "ta", "ti", "tsu", "tu", "te", "to", "tya",
    "tyu", "tyo",
    "tha", "thu", "tho",
    "thi", "the",
    "twa", "twi", "twu", "twe", "two",
    "tsa", "tsi", "tse", "tso",

    "da", "di", "dzi", "du", "dzu", "de", "do",
    "dya", "dyu", "dyo",
    "dyi", "dye",
    "dha", "dhu", "dho",
    "dhi", "dhe",

    "na", "ni", "nu", "ne", "no",
    "nya", "nyu", "nyo",
    "nyi", "nye", "n'",

    "ha", "hi", "hu", "he", "ho",
    "hya", "hyu", "hyo",
    "hyi", "hye",

    "ba", "bi", "bu", "be", "bo",
    "bya", "byu", "byo",
    "byi", "bye",

    "fwa", "fa", "fwi", "fi", "fwu", "fu",
    "fwe", "fe", "fo", "fwo",

    "pa", "pi", "pu", "pe", "po",
    "pya", "pyu", "pyo",
    "pyi", "pye",

    "ma", "mi", "mu", "me", "mo",
    "mya", "myu", "myo",
    "myi", "mye",

    "ra", "ri", "ru", "re", "ro",
    "ryi", "rye",
    "rya", "ryu", "ryo",

    // Dash replacement
    "-",
    // Middle dot replacement
    "/"
};

const ushort kanaoutput[][3] =
{
    { 0x3042, 0 }, /* a */
    { 0x3044, 0 }, /* i */
    { 0x3046, 0 }, /* u */
    { 0x3048, 0 }, /* e */
    { 0x304A, 0 }, /* o */
    // y
    { 0x3044, 0 }, { 0x3044, 0x3047, 0 }, { 0x3084, 0 }, { 0x3086, 0 }, { 0x3088, 0 },
    // w
    { 0x3046, 0 }, { 0x3046, 0 }, { 0x3046, 0x3041, 0 }, { 0x3046, 0x3043, 0 }, { 0x3046, 0x3043, 0 }, { 0x3046, 0x3047, 0 },
    { 0x3046, 0x3047, 0 }, { 0x3046, 0x3049, 0 }, { 0x308f, 0 }, { 0x3092, 0 },
    // l
    { 0x3041, 0 }, { 0x3043, 0 }, { 0x3043, 0 }, { 0x3045, 0 }, { 0x3047, 0 }, { 0x3047, 0 }, { 0x3049, 0 }, { 0x30f6, 0 }, { 0x30f5, 0 },
    { 0x3063, 0 }, { 0x3063, 0 }, { 0x3083, 0 }, { 0x3085, 0 }, { 0x3087, 0 }, { 0x308e, 0 },
    // x
    { 0x3041, 0 }, { 0x3043, 0 }, { 0x3043, 0 }, { 0x3045, 0 }, { 0x3047, 0 }, { 0x3047, 0 }, { 0x3049, 0 }, { 0x30f5, 0 }, { 0x30f6, 0 },
    { 0x3063, 0 }, { 0x3063, 0 }, { 0x3083, 0 }, { 0x3085, 0 }, { 0x3087, 0 }, { 0x308e, 0 }, { 0x3093, 0 },
    // v
    { 0x30f4, 0 }, { 0x30f4, 0x3041, 0 }, { 0x30f4, 0x3043, 0 }, { 0x30f4, 0x3043, 0 }, { 0x30f4, 0x3047, 0 }, { 0x30f4, 0x3047, 0 },
    { 0x30f4, 0x3049, 0 }, { 0x30f4, 0x3083, 0 }, { 0x30f4, 0x3085, 0 }, { 0x30f4, 0x3087, 0 },
    // c
    { 0x304b, 0 }, { 0x3053, 0 }, { 0x304f, 0 }, { 0x3057, 0 }, { 0x305b, 0 },
    { 0x3061, 0 }, { 0x3061, 0x3083, 0 }, { 0x3061, 0x3083, 0 }, { 0x3061, 0x3085, 0 }, { 0x3061, 0x3085, 0 }, { 0x3061, 0x3087, 0 }, { 0x3061, 0x3087, 0 },
    { 0x3061, 0x3047, 0 }, { 0x3061, 0x3047, 0 }, { 0x3061, 0x3047, 0 },
    // q
    { 0x304f, 0 }, { 0x304f, 0x3041, 0 }, { 0x304f, 0x3041, 0 }, { 0x304f, 0x3043, 0 }, { 0x304f, 0x3043, 0 }, { 0x304f, 0x3045, 0 },
    { 0x304f, 0x3047, 0 }, { 0x304f, 0x3047, 0 }, { 0x304f, 0x3049, 0 }, { 0x304f, 0x3049, 0 },
    // k
    { 0x304b, 0 }, { 0x304d, 0 }, { 0x304f, 0 }, { 0x3051, 0 }, { 0x3053, 0 },
    { 0x304d, 0x3083, 0 }, { 0x304d, 0x3085, 0 }, { 0x304d, 0x3087, 0 }, { 0x304d, 0x3043, 0 }, { 0x304d, 0x3047, 0 },
    // g
    { 0x304c, 0 }, { 0x304e, 0 }, { 0x3050, 0 }, { 0x3052, 0 }, { 0x3054, 0 },
    { 0x304e, 0x3083, 0 }, { 0x304e, 0x3085, 0 }, { 0x304e, 0x3087, 0 },
    { 0x304e, 0x3043, 0 }, { 0x304e, 0x3047, 0 },
    { 0x3050, 0x3041, 0 }, { 0x3050, 0x3043, 0 }, { 0x3050, 0x3045, 0 }, { 0x3050, 0x3047, 0 }, { 0x3050, 0x3049, 0 },
    // s
    { 0x3055, 0 }, { 0x3057, 0 }, { 0x3057, 0 }, { 0x3059, 0 }, { 0x305b, 0 }, { 0x305d, 0 },
    { 0x3057, 0x3083, 0 }, { 0x3057, 0x3083, 0 }, { 0x3057, 0x3085, 0 }, { 0x3057, 0x3085, 0 }, { 0x3057, 0x3087, 0 }, { 0x3057, 0x3087, 0 },
    { 0x3057, 0x3047, 0 },
    { 0x3059, 0x3041, 0 }, { 0x3059, 0x3043, 0 }, { 0x3059, 0x3045, 0 }, { 0x3059, 0x3047, 0 }, { 0x3059, 0x3049, 0 },
    // z
    { 0x3056, 0 }, { 0x3058, 0 }, { 0x305a, 0 }, { 0x305c, 0 }, { 0x305e, 0 }, { 0x3058, 0x3083, 0 }, { 0x3058, 0x3085, 0 }, { 0x3058, 0x3087, 0 },
    { 0x3058, 0x3043, 0 }, { 0x3058, 0x3047, 0 },
    // j
    { 0x3058, 0 }, { 0x3058, 0x3083, 0 }, { 0x3058, 0x3083, 0 }, { 0x3058, 0x3085, 0 }, { 0x3058, 0x3085, 0 }, { 0x3058, 0x3087, 0 }, { 0x3058, 0x3087, 0 },
    { 0x3058, 0x3043, 0 }, { 0x3058, 0x3047, 0 }, { 0x3058, 0x3047, 0 },
    // t
    { 0x305f, 0 }, { 0x3061, 0 }, { 0x3064, 0 }, { 0x3064, 0 }, { 0x3066, 0 }, { 0x3068, 0 }, { 0x3061, 0x3083, 0 },
    { 0x3061, 0x3085, 0 }, { 0x3061, 0x3087, 0 },
    { 0x3066, 0x3083, 0 }, { 0x3066, 0x3085, 0 }, { 0x3066, 0x3087, 0 },
    { 0x3066, 0x3043, 0 }, { 0x3066, 0x3047, 0 },
    { 0x3068, 0x3041, 0 }, { 0x3068, 0x3043, 0 }, { 0x3068, 0x3045, 0 }, { 0x3068, 0x3047, 0 }, { 0x3068, 0x3049, 0 },
    { 0x3064, 0x3041, 0 }, { 0x3064, 0x3043, 0 }, { 0x3064, 0x3047, 0 }, { 0x3064, 0x3049, 0 },
    // d
    { 0x3060, 0 }, { 0x3062, 0 }, { 0x3062, 0 }, { 0x3065, 0 }, { 0x3065, 0 }, { 0x3067, 0 }, { 0x3069, 0 },
    { 0x3062, 0x3083, 0 }, { 0x3062, 0x3085, 0 }, { 0x3062, 0x3087, 0 },
    { 0x3062, 0x3043, 0 }, { 0x3062, 0x3047, 0 },
    { 0x3067, 0x3083, 0 }, { 0x3067, 0x3085, 0 }, { 0x3067, 0x3087, 0 },
    { 0x3067, 0x3043, 0 }, { 0x3067, 0x3047, 0 },
    // n
    { 0x306a, 0 }, { 0x306b, 0 }, { 0x306c, 0 }, { 0x306d, 0 }, { 0x306e, 0 },
    { 0x306b, 0x3083, 0 }, { 0x306b, 0x3085, 0 }, { 0x306b, 0x3087, 0 },
    { 0x306b, 0x3043, 0 }, { 0x306b, 0x3047, 0 }, { 0x3093, 0 },
    // h
    { 0x306f, 0 }, { 0x3072, 0 }, { 0x3075, 0 }, { 0x3078, 0 }, { 0x307b, 0 },
    { 0x3072, 0x3083, 0 }, { 0x3072, 0x3085, 0 }, { 0x3072, 0x3087, 0 },
    { 0x3072, 0x3043, 0 }, { 0x3072, 0x3047, 0 },
    // b
    { 0x3070, 0 }, { 0x3073, 0 }, { 0x3076, 0 }, { 0x3079, 0 }, { 0x307c, 0 },
    { 0x3073, 0x3083, 0 }, { 0x3073, 0x3085, 0 }, { 0x3073, 0x3087, 0 },
    { 0x3073, 0x3043, 0 }, { 0x3073, 0x3047, 0 },
    // f
    { 0x3075, 0x3041, 0 }, { 0x3075, 0x3041, 0 }, { 0x3075, 0x3043, 0 }, { 0x3075, 0x3043, 0 }, { 0x3075, 0x3045, 0 }, { 0x3075, 0 },
    { 0x3075, 0x3047, 0 }, { 0x3075, 0x3047, 0 }, { 0x3075, 0x3049, 0 }, { 0x3075, 0x3049, 0 },
    // p
    { 0x3071, 0 }, { 0x3074, 0 }, { 0x3077, 0 }, { 0x307a, 0 }, { 0x307d, 0 },
    { 0x3074, 0x3083, 0 }, { 0x3074, 0x3085, 0 }, { 0x3074, 0x3087, 0 },
    { 0x3074, 0x3043, 0 }, { 0x3074, 0x3047, 0 },
    // m
    { 0x307e, 0 }, { 0x307f, 0 }, { 0x3080, 0 }, { 0x3081, 0 }, { 0x3082, 0 },
    { 0x307f, 0x3083, 0 }, { 0x307f, 0x3085, 0 }, { 0x307f, 0x3087, 0 },
    { 0x307f, 0x3043, 0 }, { 0x307f, 0x3047, 0 },
    // r
    { 0x3089, 0 }, { 0x308a, 0 }, { 0x308b, 0 }, { 0x308c, 0 }, { 0x308d, 0 },
    { 0x308a, 0x3043, 0 }, { 0x308a, 0x3047, 0 },
    { 0x308a, 0x3083, 0 }, { 0x308a, 0x3085, 0 }, { 0x308a, 0x3087, 0 },

    { KDASH, 0 },

    { MIDDLEDOT, 0 }
};

namespace
{
    const int kanaalen = 1;
    const int kanaapos = 0;

    const int kanailen = 1;
    const int kanaipos = 1;

    const int kanaulen = 1;
    const int kanaupos = 2;

    const int kanaelen = 1;
    const int kanaepos = 3;

    const int kanaolen = 1;
    const int kanaopos = 4;

    const int kanaylen = 5;
    const int kanaypos = kanaopos + kanaolen;

    const int kanawlen = 10;
    const int kanawpos = kanaypos + kanaylen;

    const int kanallen = 15;
    const int kanalpos = kanawpos + kanawlen;

    const int kanaxlen = 16;
    const int kanaxpos = kanalpos + kanallen;

    const int kanavlen = 10;
    const int kanavpos = kanaxpos + kanaxlen;

    const int kanaclen = 15;
    const int kanacpos = kanavpos + kanavlen;

    const int kanaqlen = 10;
    const int kanaqpos = kanacpos + kanaclen;

    const int kanaklen = 10;
    const int kanakpos = kanaqpos + kanaqlen;

    const int kanaglen = 15;
    const int kanagpos = kanakpos + kanaklen;

    const int kanaslen = 18;
    const int kanaspos = kanagpos + kanaglen;

    const int kanazlen = 10;
    const int kanazpos = kanaspos + kanaslen;

    const int kanajlen = 10;
    const int kanajpos = kanazpos + kanazlen;

    const int kanatlen = 23;
    const int kanatpos = kanajpos + kanajlen;

    const int kanadlen = 17;
    const int kanadpos = kanatpos + kanatlen;

    const int kananlen = 11;
    const int kananpos = kanadpos + kanadlen;

    const int kanahlen = 10;
    const int kanahpos = kananpos + kananlen;

    const int kanablen = 10;
    const int kanabpos = kanahpos + kanahlen;

    const int kanaflen = 10;
    const int kanafpos = kanabpos + kanablen;

    const int kanaplen = 10;
    const int kanappos = kanafpos + kanaflen;

    const int kanamlen = 10;
    const int kanampos = kanappos + kanaplen;

    const int kanarlen = 10;
    const int kanarpos = kanampos + kanamlen;

    const int kanadashlen = 1;
    const int kanadashpos = kanarpos + kanarlen;

    const int kanadotlen = 1;
    const int kanadotpos = kanadashpos + kanadashlen;


    const char *kanatable[86] =
    {
        "xa", "a", "xi", "i", "xu", "u", "xe", "e", "xo", "o", "ka", "ga", "ki", "gi", "ku", //14
        "gu", "ke", "ge", "ko", "go", "sa", "za", "si", "zi", "su", "zu", "se", "ze", "so", "zo", "ta", //30
        "da", "ti", "di", "+", "tu", "du", "te", "de", "to", "do", "na", "ni", "nu", "ne", "no", "ha", //46 '+' is small tsu.
        "ba", "pa", "hi", "bi", "pi", "hu", "bu", "pu", "he", "be", "pe", "ho", "bo", "po", "ma", "mi", //62
        "mu", "me", "mo", "xya", "ya", "xyu", "yu", "xyo", "yo", "ra", "ri", "ru", "re", "ro", "xwa", "wa", //78
        "whi", "whe", "wo", "n'", "vu", "xka", "xke"
    };

    //const char *kanatableU[86] =
    //{
    //    "XA", "A", "XI", "I", "XU", "U", "XE", "E", "XO", "O", "KA", "GA", "KI", "GI", "KU", //14
    //    "GU", "KE", "GE", "KO", "GO", "SA", "ZA", "SI", "ZI", "SU", "ZU", "SE", "ZE", "SO", "ZO", "TA", //30
    //    "DA", "TI", "DI", "+", "TU", "DU", "TE", "DE", "TO", "DO", "NA", "NI", "NU", "NE", "NO", "HA", //46
    //    "BA", "PA", "HI", "BI", "PI", "HU", "BU", "PU", "HE", "BE", "PE", "HO", "BO", "PO", "MA", "MI", //62
    //    "MU", "ME", "MO", "XYA", "YA", "XYU", "YU", "XYO", "YO", "RA", "RI", "RU", "RE", "RO", "XWA", "WA", //78
    //    "WHI", "WHE", "WO", "N'", "VU", "XKA", "XKE"
    //};
    //
    //const char *kanatable_ext[84] =
    //{
    //    "a", "a", "i", "i", "u", "u", "e", "e", "o", "o", "ka", "ga", "ki", "gi", "ku", //14
    //    "gu", "ke", "ge", "ko", "go", "sa", "za", "shi", "ji", "su", "zu", "se", "ze", "so", "zo", "ta", //30
    //    "da", "chi", "ji", "+", "tsu", "zu", "te", "de", "to", "do", "na", "ni", "nu", "ne", "no", "ha", //46
    //    "ba", "pa", "hi", "bi", "pi", "fu", "bu", "pu", "he", "be", "pe", "ho", "bo", "po", "ma", "mi", //62
    //    "mu", "me", "mo", "ya", "ya", "yu", "yu", "yo", "yo", "ra", "ri", "ru", "re", "ro", "wa", "wa", //78
    //    "wi", "we", "wo", "n", "vu"
    //};
    //
    //const char *kanatable_extU[84] =
    //{
    //    "A", "A", "I", "I", "U", "U", "E", "E", "O", "O", "KA", "GA", "KI", "GI", "KU", //14
    //    "GU", "KE", "GE", "KO", "GO", "SA", "ZA", "SHI", "JI", "SU", "ZU", "SE", "ZE", "SO", "ZO", "TA", //30
    //    "DA", "CHI", "JI", "+", "TSU", "ZU", "TE", "DE", "TO", "DO", "NA", "NI", "NU", "NE", "NO", "HA", //46
    //    "BA", "PA", "HI", "BI", "PI", "FU", "BU", "PU", "HE", "BE", "PE", "HO", "BO", "PO", "MA", "MI", //62
    //    "MU", "ME", "MO", "YA", "YA", "YU", "YU", "YO", "YO", "RA", "RI", "RU", "RE", "RO", "WA", "WA", //78
    //    "WI", "WE", "WO", "N", "VU"
    //};

    const ushort kanavowel[5] = { 0x3042, 0x3044, 0x3046, 0x3048, 0x304A, };

    const char vowelcolumn[84] =
    {
        0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 0, 0, 1, 1, 2, //14
        2, 3, 3, 4, 4, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 0, //30
        0, 1, 1, -1, 2, 2, 3, 3, 4, 4, 0, 1, 2, 3, 4, 0, //46
        0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 0, 1, //62
        2, 3, 4, 0, 0, 2, 2, 4, 4, 0, 1, 2, 3, 4, -1, 0, //78
        -1, -1, 4, -1, 3
    };

    const ushort HiraganaBase = 0x3041;
    const ushort HiraganaEnd = 0x3093;
    const ushort KatakanaBase = 0x30A1;
    const ushort KatakanaEnd = 0x30F6;
    const ushort KanjiBase = 0x4E00;
    const ushort KanjiEnd = 0x9FAF;

}

QString toKatakana(const QString &str, int len)
{
    return toKatakana(str.constData(), len == -1 ? str.size() : len);
}

QString toKatakana(const QCharString &str, int len)
{
    return toKatakana(str.data(), len == -1 ? str.size() : len);
}

QString toKatakana(const QChar *str, int len)
{
    QString r;
    ushort c;
    for (int ix = 0, slen = len == -1 ? tosigned(qcharlen(str)) : len; ix != slen; ++ix)
    {
        c = str[ix].unicode();
        if (HIRAGANA(c))
        {
            c += 0x30a1 - 0x3041;
            r += QString::fromUtf16(&c, 1);
        }
        else
            r += str[ix];
    }
    return r;
}


QString romanize(const QString &str, int len)
{
    return romanize(str.constData(), len == -1 ? str.size() : len);
}

QString romanize(const QCharString &str, int len)
{
    return romanize(str.data(), len == -1 ? str.size() : len);
}

QString romanize(const QChar *str, int len)
{
    //if (len == 1 && KANA(str[0]))
    //    return romanize(str[0]);

    int i;

    uint clen = len == -1 ? tounsigned(qcharlen(str)) : tounsigned(len);
    char *conv = new char[clen * 3 + 1];
    memset(conv, 0, clen * 3 + 1);

    int convlen = 0;
    for (unsigned int ix = 0; ix < clen; ++ix)
    {
        if (DASH(str[ix].unicode()) && convlen && KANAVOWEL(conv[convlen - 1]))
        {
            conv[convlen] = conv[convlen - 1];
            convlen++;
            continue;
        }

        if (MIDDOT(str[ix].unicode()))
        {
            //conv[convlen] = '/';
            //convlen++;
            continue;
        }

        if (!KANA(str[ix].unicode())) // Skip unknown chars
        {
            if ((str[ix].unicode() >= 'A' && str[ix].unicode() <= 'Z') || (str[ix].unicode() >= 'a' && str[ix].unicode() <= 'z'))
            {
                conv[convlen] = (char)str[ix].unicode(); // Leave romaji there, maybe we will need it.
                convlen++;
            }
            continue;
        }
        if (KATAKANA(str[ix].unicode()))
            i = str[ix].unicode() - 0x30A1;
        else
            i = str[ix].unicode() - 0x3041;

        if (kanatable[i][0] != '*')
        {
            if (convlen && conv[convlen - 1] == '+')
            {
                // Error, remove doubling too.
                if (kanatable[i][0] == '+' || kanatable[i][0] == 'y' || kanatable[i][0] == 'Y' || kanatable[i][0] == 'w' ||
                    kanatable[i][0] == 'v' || kanatable[i][0] == 'a' || kanatable[i][0] == 'i' || kanatable[i][0] == 'u' ||
                    kanatable[i][0] == 'e' || kanatable[i][0] == 'o' || kanatable[i][0] == 'x' || kanatable[i][0] == 'W')
                {
                    convlen--;
                    conv[convlen] = 0;
                }
                else
                    conv[convlen - 1] = kanatable[i][0];
            }
            int cl = 0;
            while (kanatable[i][cl])
            {
                conv[convlen] = kanatable[i][cl++];
                convlen++;
            }
        }
    }
    if (convlen && conv[convlen - 1] == '+')
        conv[--convlen] = 0;

    QString s(conv);

    delete[] conv;

    return s;
}

QString hiraganize(const QString &str, int len)
{
    return hiraganize(str.constData(), len == -1 ? str.size() : len);
}

QString hiraganize(const QCharString &str, int len)
{
    return hiraganize(str.data(), len == -1 ? str.size() : len);
}

QString hiraganize(const QChar *str, int len)
{
    QString s;
    if (len == -1)
        len = tosigned(qcharlen(str));

    for (int i = 0; i < len; i++)
    {
        if (DASH(str[i].unicode()))
        {
            if (!s.isEmpty() && HIRAGANA(s[s.size() - 1].unicode()))
            {
                ushort d = s[s.size() - 1].unicode() - 0x3041;
                if (vowelcolumn[d] >= 0)
                    s += QChar(kanavowel[(unsigned char)vowelcolumn[d]]);
            }
            continue;
        }

        if (KATAKANA(str[i].unicode()) && str[i].unicode() <= 0x30F4)
            s += QChar(str[i].unicode() - 0x60);
        else
            s += str[i];
    }

    return s;
}

namespace
{
    //const char* vowelsyllables[] = {
    //    "a", "i", "u", "e", "o"
    //};

    //const int vowelsyllableslen = 5;

    const ushort vowelunicodes[] = {
        0x3042, 0x3044, 0x3046, 0x3048, 0x304A
    };

    const char* xsyllables[] = {
        "xa", "xi", "xu", "xe", "xo",  "xya", "xyu", "xyo",  "xwa", "xka", "xke",
    };

    const int xsyllableslen = 11;
    const int xsyllablesshortlen = 5;

    const ushort xunicodes[] = {
        0x3041, 0x3043, 0x3045, 0x3047, 0x3049, 0x3083, 0x3085, 0x3087, 0x308E, 0x30F5, 0x30F6
    };

    const char* kgsyllables[] = {
        "ka", "ga", "ki", "gi", "ku", "gu", "ke", "ge", "ko", "go",
    };

    const int kgsyllableslen = 10;

    const ushort kgunicodes[] = {
        0x304B, 0x304C, 0x304D, 0x304E, 0x304F, 0x3050, 0x3051, 0x3052, 0x3053, 0x3054,
    };

    const char* szsyllables[] = {
        "sa", "za", "si", "zi", "su", "zu", "se", "ze", "so", "zo",
    };

    const int szsyllableslen = 10;

    const ushort szunicodes[] = {
        0x3055, 0x3056, 0x3057, 0x3058, 0x3059, 0x305A, 0x305B, 0x305C, 0x305D, 0x305E,
    };

    const char* tdsyllables[] = {
        "ta", "da", "ti", "di", "tu", "du", "te", "de", "to", "do",
    };

    const int tdsyllableslen = 10;

    const ushort tdunicodes[] = {
        0x305F, 0x3060, 0x3061, 0x3062, 0x3064, 0x3065, 0x3066, 0x3067, 0x3068, 0x3069,
    };

    const char* nsyllables[] = {
        "na", "ni", "nu", "ne", "no", "n'",
    };

    const int nsyllableslen = 6;

    const ushort nunicodes[] = {
        0x306A, 0x306B, 0x306C, 0x306D, 0x306E, 0x3093,
    };

    const char* hbpsyllables[] = {
        "ha", "ba", "pa", "hi", "bi", "pi", "hu", "bu", "pu", "he", "be", "pe", "ho", "bo", "po",
    };

    const int hbpsyllableslen = 15;

    const ushort hbpunicodes[] = {
        0x306F, 0x3070, 0x3071, 0x3072, 0x3073, 0x3074, 0x3075, 0x3076, 0x3077, 0x3078, 0x3079, 0x307A, 0x307B, 0x307C, 0x307D,
    };

    const char* msyllables[] = {
        "ma", "mi", "mu", "me", "mo",
    };

    const int msyllableslen = 5;

    const ushort municodes[] = {
        0x307E, 0x307F, 0x3080, 0x3081, 0x3082,
    };

    const char* ysyllables[] = {
        "ya", "yu", "yo",
    };

    const int ysyllableslen = 3;

    const ushort yunicodes[] = {
        0x3084, 0x3086, 0x3088,
    };

    const char* rsyllables[] = {
        "ra", "ri", "ru", "re", "ro",
    };

    const int rsyllableslen = 5;

    const ushort runicodes[] = {
        0x3089, 0x308A, 0x308B, 0x308C, 0x308D,
    };

    const char* wvsyllables[] = {
        "wa", "wo", "vu", "whi", "whe"
    };

    const int wvsyllableslen = 5;
    const int wvsyllablesshortlen = 3;

    const ushort wvunicodes[] = {
        0x308F, 0x3092, 0x30F4, 0x3090, 0x3091
    };
}


bool kanavowelize(ushort &ch, int &chlen, const QChar *str, int len)
{
    if (len == -1)
        len = tosigned(qcharlen(str));

    chlen = 0;

    if (len == 0)
        return false;

    chlen = 1;

    const char** syllables;
    int syllableslen;
    int shortlen;
    const ushort *unicodes;

    switch (str[0].unicode())
    {
    case '/':
        ch = MIDDLEDOT;
        return true;
    case 'a':
        ch = vowelunicodes[0];
        return true;
    case 'i':
        ch = vowelunicodes[1];
        return true;
    case 'u':
        ch = vowelunicodes[2];
        return true;
    case 'e':
        ch = vowelunicodes[3];
        return true;
    case 'o':
        ch = vowelunicodes[4];
        return true;
    case 'x':
        syllables = xsyllables;
        syllableslen = xsyllableslen;
        shortlen = xsyllablesshortlen;
        unicodes = xunicodes;
        break;
    case 'k':
    case 'g':
        syllables = kgsyllables;
        shortlen = syllableslen = kgsyllableslen;
        unicodes = kgunicodes;
        break;
    case 's':
    case 'z':
        syllables = szsyllables;
        shortlen = syllableslen = szsyllableslen;
        unicodes = szunicodes;
        break;
    case 't':
    case 'd':
        syllables = tdsyllables;
        shortlen = syllableslen = tdsyllableslen;
        unicodes = tdunicodes;
        break;
    case 'n':
        syllables = nsyllables;
        shortlen = syllableslen = nsyllableslen;
        unicodes = nunicodes;
        break;
    case 'h':
    case 'b':
    case 'p':
        syllables = hbpsyllables;
        shortlen = syllableslen = hbpsyllableslen;
        unicodes = hbpunicodes;
        break;
    case 'm':
        syllables = msyllables;
        shortlen = syllableslen = msyllableslen;
        unicodes = municodes;
        break;
    case 'y':
        syllables = ysyllables;
        shortlen = syllableslen = ysyllableslen;
        unicodes = yunicodes;
        break;
    case 'r':
        syllables = rsyllables;
        shortlen = syllableslen = rsyllableslen;
        unicodes = runicodes;
        break;
    case 'w':
    case 'v':
        syllables = wvsyllables;
        syllableslen = wvsyllableslen;
        shortlen = wvsyllablesshortlen;
        unicodes = wvunicodes;
        break;
    default:
        return false;
    }

    if (len == 1)
    {
        chlen = 0;
        return false;
    }

    if (str[0] == str[1])
    {
        ch = MINITSU;
        return true;
    }

    chlen = 2;

    for (int ix = 0; ix != shortlen; ++ix)
    {
        if (str[0] == syllables[ix][0] && str[1] == syllables[ix][1])
        {
            ch = unicodes[ix];
            return true;
        }
    }

    if (len == 2)
    {
        chlen = 0;
        return false;
    }

    chlen = 3;

    for (int ix = shortlen; ix != syllableslen; ++ix)
    {
        if (str[0] == syllables[ix][0] && str[1] == syllables[ix][1] && str[2] == syllables[ix][2])
        {
            ch = unicodes[ix];
            return true;
        }
    }

    chlen = 0;
    return false;
}

QString toKana(QString str, bool uppertokata)
{
    return toKana(str.constData(), str.size(), uppertokata);
}

void findKanaArrays(QChar ch, /*const char** &input, const ushort* output[3],*/ int &pos, int &size)
{
#define CASEQ(x) #x
#define ADDCASE(x)\
                pos = kana ## x ## pos; \
                size = kana ## x ## len

    switch (ch.unicode())
    {
    case 'a':
        ADDCASE(a);
        break;
    case 'i':
        ADDCASE(i);
        break;
    case 'u':
        ADDCASE(u);
        break;
    case 'e':
        ADDCASE(e);
        break;
    case 'o':
        ADDCASE(o);
        break;
    case '-':
        ADDCASE(dash);
        break;
    case '/':
        ADDCASE(dot);
        break;
    case 'y':
        ADDCASE(y);
        break;
    case 'w':
        ADDCASE(w);
        break;
    case 'l':
        ADDCASE(l);
        break;
    case 'x':
        ADDCASE(x);
        break;
    case 'v':
        ADDCASE(v);
        break;
    case 'c':
        ADDCASE(c);
        break;
    case 'q':
        ADDCASE(q);
        break;
    case 'k':
        ADDCASE(k);
        break;
    case 'g':
        ADDCASE(g);
        break;
    case 's':
        ADDCASE(s);
        break;
    case 'z':
        ADDCASE(z);
        break;
    case 'j':
        ADDCASE(j);
        break;
    case 't':
        ADDCASE(t);
        break;
    case 'd':
        ADDCASE(d);
        break;
    case 'n':
        ADDCASE(n);
        break;
    case 'h':
        ADDCASE(h);
        break;
    case 'b':
        ADDCASE(b);
        break;
    case 'f':
        ADDCASE(f);
        break;
    case 'p':
        ADDCASE(p);
        break;
    case 'm':
        ADDCASE(m);
        break;
    case 'r':
        ADDCASE(r);
        break;
    default:
        pos = -1;
        size = 0;
    }

#undef CASEQ
#undef ADDCASE
}

namespace
{
    inline QChar _toKanaPicker(const QChar* str, int pos, bool useupper, bool &upper)
    {
        if (useupper)
            upper = str[pos].isUpper();
        else
            upper = false;
        return str[pos].toLower().unicode();
    }

    inline QChar _toKanaPicker(const char* str, int pos, bool useupper, bool &upper)
    {
        QChar ch = str[pos];
        return _toKanaPicker(&ch, 0, useupper, upper);
    }

    template<typename T>
    QString _toKana(const T *str, int len, bool uppertokata)
    {
        QString result;

        bool isupper;

        int apos;
        int asize;

        bool kata;
        bool found;

        while (len != 0)
        {
            QChar ch = _toKanaPicker(str, 0, uppertokata, isupper);
            kata = isupper;

            switch (ch.unicode())
            {
            case 'a':
                result += !isupper ? QChar(0x3042) : QChar(0x30A2);
                ++str;
                --len;
                continue;
            case 'i':
                result += !isupper ? QChar(0x3044) : QChar(0x30A4);
                ++str;
                --len;
                continue;
            case 'u':
                result += !isupper ? QChar(0x3046) : QChar(0x30A6);
                ++str;
                --len;
                continue;
            case 'e':
                result += !isupper ? QChar(0x3048) : QChar(0x30A8);
                ++str;
                --len;
                continue;
            case 'o':
                result += !isupper ? QChar(0x304A) : QChar(0x30AA);
                ++str;
                --len;
                continue;
            case '-':
                result += QChar(KDASH);
                ++str;
                --len;
                continue;
            case '/':
                result += QChar(MIDDLEDOT);
                ++str;
                --len;
                continue;
            }

            findKanaArrays(ch, apos, asize);

            ++str;
            --len;

            if (asize == 0)
                continue;

            if (len == 0)
            {
                //if (ch.unicode() == 'n')
                //    result += !isupper ? QString::fromUtf16(kanaoutput[apos + asize - 1]) : toKatakana(QString::fromUtf16(kanaoutput[apos + asize - 1]));
                break;
            }

            bool isupper2;
            if (_toKanaPicker(str, -1, uppertokata, isupper) == _toKanaPicker(str, 0, uppertokata, isupper2) && ch.unicode() != 'n')
            {
                kata = kata || isupper || isupper2;
                if (result.isEmpty() || result.at(result.size() - 1) != QChar(MINITSU))
                    result += kata ? QChar(MINITSUKATA) : QChar(MINITSU);
                continue;
            }

            found = false;
            for (int ix = apos; ix != apos + asize; ++ix)
            {

                int pos = 0;
                for (; pos != len && kanainput[ix][pos + 1] != 0 && kanainput[ix][pos + 1] == _toKanaPicker(str, pos, uppertokata, isupper); ++pos)
                    kata = kata || isupper;
                if (kanainput[ix][pos + 1] == 0)
                {
                    found = true;

                    result += !kata ? QString::fromUtf16(kanaoutput[ix]) : toKatakana(QString::fromUtf16(kanaoutput[ix]));
                    
                    //kata = false;
                    str += pos;
                    len -= pos;
                    break;
                }
            }
            if (!found && ch.unicode() == 'n')
            {
                ++str;
                --len;
                result += !isupper ? QString::fromUtf16(kanaoutput[apos + asize - 1]) : toKatakana(QString::fromUtf16(kanaoutput[apos + asize - 1]));
            }
        }
        return result;
    }
}


QString toKana(const char *str, int len, bool uppertokata)
{
    if (len == -1)
        len = tosigned(strlen(str));

    return _toKana<char>(str, len, uppertokata);
}

QString toKana(const QChar *str, int len, bool uppertokata)
{
    if (len == -1)
        len = tosigned(qcharlen(str));

    return _toKana<QChar>(str, len, uppertokata);
}

QChar hiraganaCh(const QChar *c, int ix)
{
    if (DASH(c[ix].unicode()))
    {
        if (ix > 0 && HIRAGANA(hiraganaCh(c, ix - 1)) && vowelcolumn[hiraganaCh(c, ix - 1).unicode() - 0x3041] >= 0)
            return kanavowel[(unsigned char)vowelcolumn[hiraganaCh(c, ix - 1).unicode() - 0x3041]];
        return c[ix];
    }

    if (KATAKANA(c[ix]))
        return c[ix].unicode() - 0x60;

    return c[ix];
}

bool kanaMatch(const QChar *c1, int pos1, const QChar *c2, int pos2)
{
    if (hiraganaCh(c1, pos1) == hiraganaCh(c2, pos2))
        return true;

    QChar hc1 = hiraganaCh(c1, pos1);
    QChar hc2 = hiraganaCh(c2, pos2);

    if ((hc1 == 0x3072 /* HI */ && hc2 == 0x3044 /* I */) || (hc1 == 0x3075 /* FU */ && hc2 == 0x3046 /* U */) ||
        (hc1 == 0x3078 /* HE */ && hc2 == 0x3048 /* E */) || (hc1 == 0x306f /* HA */ && hc2 == 0x308f /* WA */) ||
        (hc1 == 0x3065 /* DZU */ && hc2 == 0x305A /* ZU */))
        return true;

    if (((c1[pos1] == 0x30B1 /* KE */ || c1[pos1] == 0x30F6 /*XKE */ || c1[pos1] == 0x30F5 /* XKA */) &&
        (hc2 == 0x304B /* ka */ || hc2 == 0x3051 /*ke*/ || hc2 == 0x304C /* ga */)) ||
        ((c2[pos2] == 0x30B1 /* KE */ || c2[pos2] == 0x30F6 /* XKE */ || c2[pos2] == 0x30F5 /* XKA */) &&
        (hc1 == 0x304B /* ka */ || hc1 == 0x3051 /* ke */ || hc1 == 0x304C /* ga */)))
        return true;

    if ((pos1 > 0 && c1[pos1] == KDASH && (hc2 == 0x304A /* o */ || hc2 == 0x3046 /* u */) && vowelcolumn[hiraganaCh(c1, pos1 - 1).unicode() - 0x3041] == 4) ||
        (pos2 > 0 && c2[pos2] == KDASH && (hc1 == 0x304A /* o */ || hc1 == 0x3046 /* u */) && vowelcolumn[hiraganaCh(c2, pos2 - 1).unicode() - 0x3041] == 4) ||
        (pos1 > 0 && c1[pos1] == KDASH && (hc2 == 0x3044 /* i */ || hc2 == 0x3048 /* e */) && vowelcolumn[hiraganaCh(c1, pos1 - 1).unicode() - 0x3041] == 3) ||
        (pos2 > 0 && c2[pos2] == KDASH && (hc1 == 0x3044 /* i */ || hc1 == 0x3048 /* e */) && vowelcolumn[hiraganaCh(c2, pos2 - 1).unicode() - 0x3041] == 3))
        return true;

    return false;
}



//-------------------------------------------------------------

