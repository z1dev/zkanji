/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FURIGANA_H
#define FURIGANA_H

#include <vector>
#include "qcharstring.h"

struct KanjiEntry;

// Length of kanji kun reading without okurigana and its separator character.
int kunLen(const QChar *kun);
// Length of kanji on reading without the leading dash.
int onLen(const QChar *on);

// Kanji and kana part of a word that correspond to each other. Marks the position and number
// of characters in the word both in the kanji and kana.
struct FuriganaData
{
    struct Part
    {
        ushort pos;
        ushort len;
    };

    Part kanji;
    Part kana;
};

// Fills the furigana list with data of furigana for a word with the passed kanji and kana.
// The contents of the list will be overwritten with positions in kanji and kana.
// See grammar.cpp above furiganaStep() for a detailed explanation how it works.
void findFurigana(const QCharString &kanji, const QCharString &kana, std::vector<FuriganaData> &furigana);

// Returns the compact index of the reading for the kanji in the word of kanji/kana at
// kanjipos. If the kanji is not specified, it's looked up. To avoid that, if available,
// specify the correct kanji in k. If available also pass the furigana data that resulted from
// a call of findFurigana() with the same word kanji and kana values.
int findKanjiReading(const QCharString &kanji, const QCharString &kana, int kanjipos, KanjiEntry *k = nullptr, std::vector<FuriganaData> *furigana = nullptr);

// Returns whether the word contains the passed kanji entry with the specified reading. If the
// kanji appears in the word multiple times, they are checked until a match is first found.
bool matchKanjiReading(const QCharString &kanji, const QCharString &kana, KanjiEntry *k, int reading);



#endif // FURIGANA_H
