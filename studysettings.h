/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef STUDYSETTINGS_H
#define STUDYSETTINGS_H

enum class WordParts : uchar;

struct StudySettings
{
    enum IncludeDays { One, Two, Three, Five, Seven };
    enum SuspendWait { WaitOne, WaitOneHalf, WaitTwo, WaitThree, WaitFive };

    enum ReadingType { ON, Kun, ONKun, None };
    enum ReadingFrom { Both, NewOnly, MistakeOnly, EveryWord };

    int starthour = 4;

    IncludeDays includedays = One;
    int includecount = 0;
    bool onlyunique = true;

    bool limitnew = true;
    int limitcount = 60;

    // These rely on the WordParts enum when loading/saving settings. Don't use WordPartBits.

    WordParts kanjihint = (WordParts)2; //WordParts::Definition;
    WordParts kanahint = (WordParts)2; //WordParts::Definition;
    WordParts defhint = (WordParts)0; //WordParts::Kanji;

    bool showestimate = true;
    bool hidekanjikana = false;
    bool writekanji = false;
    bool writekana = false;
    bool writedef = false;
    // Whether typing hiragana instead of katakana is invalid (or the reverse.)
    bool matchkana = true;

    int delaywrong = 10;

    bool review = true;

    bool idlesuspend = false;
    SuspendWait idletime = WaitThree;

    bool itemsuspend = false;
    int itemsuspendnum = 60;
    bool timesuspend = false;
    int timesuspendnum = 20;

    ReadingType readings = ONKun;
    ReadingFrom readingsfrom = Both;
};

namespace Settings
{
    extern StudySettings study;
}

#endif // STUDYSETTINGS_H
