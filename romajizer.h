/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ROMAJIZER_H
#define ROMAJIZER_H

#include <QString>
#include <QValidator>
#include <QChar>

// Converts HIRAGANA to KATAKANA.
QString toKatakana(const QString &str, int len = -1);
// Converts HIRAGANA to KATAKANA.
QString toKatakana(const QCharString &str, int len = -1);
// Converts HIRAGANA to KATAKANA.
QString toKatakana(const QChar *str, int len = -1);
// Converts japanese kana string to a form of romaji that the
// program understands. It is not intended to be legible by humans.
QString romanize(const QString &str, int len = -1);
// Converts japanese kana string to a form of romaji that the
// program understands. It is not intended to be legible by humans.
QString romanize(const QCharString &str, int len = -1);
// Converts japanese kana string to a form of romaji that the
// program understands. It is not intended to be legible by humans.
QString romanize(const QChar *str, int len = -1);

// Converts KATAKANA to HIRAGANA. To convert romaji use toKana().
QString hiraganize(const QString &str, int len = -1);
// Converts KATAKANA to HIRAGANA. To convert romaji use toKana().
QString hiraganize(const QCharString &str, int len = -1);
// Converts KATAKANA to HIRAGANA. To convert romaji use toKana().
QString hiraganize(const QChar *str, int len = -1);

// Stores the unicode of a kana character in ch, which would be romanized as the
// first few bytes of str. Sets chlen to the number of romaji characters needed for
// the conversion. Returns false and sets chlen to 0 when the start of str cannot be
// converted to a kana character. Len is the length of str that can be used.
// Only strings which are in the form that romanize would produce are correct input.
bool kanavowelize(ushort &ch, int &chlen, const QChar *str, int len = -1);

// Converts human readable romaji to hiragana, leaving out any invalid part of the input.
QString toKana(QString str, bool uppertokata = false);
// Converts human readable romaji to hiragana, leaving out any invalid part of the input.
// Set uppertokata to true to convert syllables with upper case romaji to katakana.
QString toKana(const QChar *str, int len = -1, bool uppertokata = false);
// Converts human readable romaji to hiragana, leaving out any invalid part of the input.
// Set uppertokata to true to convert syllables with upper case romaji to katakana.
QString toKana(const char *str, int len = -1, bool uppertokata = false);

// Returns the hiragana equivalent of a katakana character, or the character
// itself if it's not katakana. The function wants the whole string and the
// index of the character to convert because of dashes.
QChar hiraganaCh(const QChar *c, int ix);
// Returns whether two characters in two words' kana forms might be pronounced the
// same way. It's not bulletproof because for example small katakana ke is
// sometimes read as ka, which makes it a possible match.
bool kanaMatch(const QChar *c1, int pos1, const QChar *c2, int pos2);

extern const char* kanainput[];
extern const ushort kanaoutput[][3];
// Sets pos and size to a range in kanainput and kanaoutput, depending on ch. The range
// corresponds to syllables starting with ch.
void findKanaArrays(QChar ch, /*const char** &input, const ushort* output[3],*/ int &pos, int &size);

#endif
