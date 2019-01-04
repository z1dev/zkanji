/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GRAMMAR_ENUMS_H
#define GRAMMAR_ENUMS_H

//#include <QChar>

// Possible recognized inflection types.
enum class InfTypes
{
    Masu,
    Mashita,
    Masen,
    Mashou,
    Nai,
    Reru,
    Seru,
    Serareru,
    I,
    Tai,
    Tagaru,
    Sou,
    Te,
    Ta,
    Tara,
    Tari,
    Eba,
    Eru,
    E,
    Ou,
    Suru,
    Rareru,
    Ku,
    Sa,
    Na,
    Teru,
    Kya,
    Chau,
    Naide,
    Zu,
    Nu,
    Tenasai,
    Teoku,
};

// Information about a word. Usually its kana or kanji usage.
// If added more than 8 values, make sure any place that saves/loads or stores this have enough space
enum class WordInfo
{
    Ateji,
    Gikun,
    IrregKanji,
    IrregKana,
    IrregOku,
    OutdatedKanji,
    OutdatedKana,

    Count,

    //// Not part of the word's information field and it shouldn't be saved. Bit set for a word
    //// which has been added to at least one group.
    //InGroup = 31,
};

// Changes from OldWordTypes:
// Removed AuxVerb and AuxAdj. They are replaced with Aux and the correct
// verb or adjective form.
enum class WordTypes
{
    Noun,

    /* Verbs */
    IchidanVerb,
    GodanVerb,
    TakesSuru, /* Adding suru makes it into a verb. */
    SuruVerb, /* Verb ending in Suru*/
    AruVerb,
    KuruVerb,
    IkuVerb,
    RiVerb, /* 2015 - Irregular -ru verb ending in -ri. */
    ZuruVerb, /* 2015 - Verb ending in -zuru. Alternative to -jiru endings. */
    Transitive,
    Intransitive,

    /* Adjectives */
    TrueAdj,
    NaAdj,
    Taru, /* Takes -to */
    PrenounAdj, /* Adjective before noun */
    
    /* Other */
    MayTakeNo,
    Adverb,
    Aux, /* 2015 - Auxiliary, can be verb or adjective or other */
    Prefix,
    Suffix,
    Conjunction,
    Interjection,
    Expression,

    Pronoun, /*  2015 */
    Particle, /* 2015 */
    Counter, /* 2015 */
    // TODO: desu doesn't have it
    CopulaDa, /* 2015 - da or desu */

    /* Archaic */
    ArchaicVerb, /* 2015 - Nidan and Yodan verbs*/
    ArchaicAdj, /* 2015 - archaic adjectives: ku, kari, shiku */
    ArchaicNa, /* 2015 - archaic or formal na-adj */

    Count
};

// Removed KanjiOnly, changed XRated to vulgar
enum class WordNotes
{
    /* Others */
    KanaOnly,
    Abbrev,
    FourCharIdiom, /* 2015 */
    Idiomatic,
    Obscure,
    Obsolete,
    Onomatopoeia, /* 2015 */
    Proverb, /* 2015 */
    Rare,
    Sensitive,

    /* Relation */
    Colloquial,
    Familiar,
    Honorific,
    Humble,
    Polite,

    /* Style */
    Archaic,
    Poetical, /* 2015 */
    ChildLang, /* 2015 */
    Male,
    Female,
    Joking, /* 2015 */
    Slang,
    MangaSlang,
    Derogatory,
    Vulgar,

    Count
};

enum class WordFields
{
    Architecture, /* 2015 */
    Business, /* 2015 */
    Computing,
    Economics, /* 2015 */
    Engineering, /* 2015 */
    Finance, /* 2015 */
    Food,
    Law, /* 2015 */
    Military, /* 2015 */
    Music, /* 2015 */

    // Sciences
    Anatomy, /* 2015 */
    Astronomy, /* 2015 */
    Biology, /* 2015 */
    Botany, /* 2015 */
    Chemistry, /* 2015 */
    Geology, /* 2015 */
    Geometry,
    Linguistics,
    Math,
    Medicine, /* 2015 */
    Physics,
    Zoology, /* 2015 */

    // Religion
    Buddhism,
    Shinto, /* 2015 */

    // Sports
    Baseball, /* 2015 */
    Mahjong, /* 2015 */
    MartialArts,
    Shogi, /* 2015 */
    Sports, /* 2015 */
    Sumo, /* 2015 */

    /* end */
    Count
};

// Since 2015
// If added more than 16 values, make sure any place that saves/loads or stores this have enough space
enum class WordDialects : ushort
{
    HokkaidouBen,
    KansaiBen,
    KantouBen,
    KyotoBen,
    KyuushuuBen,
    NaganoBen,
    OsakaBen,
    RyuukyuuBen,
    TosaBen,
    TouhokuBen,
    TsugaruBen,

    Count
};

// Values used in word attribute selectors, I.e filter or word editor.
// If added more than 16 values, make sure any place that saves/loads or stores this have enough space
enum class WordAttribs
{
    WordType,
    Verb,
    Adjective,
    Archaic,

    WordNote,
    Relation,
    Style,

    WordField,
    Science,
    Religion,
    Sport,

    WordDialect,

    WordInfo,
    WordJLPT,

    Count
};




#endif // GRAMMAR_ENUMS_H

