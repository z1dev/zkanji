/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYSETTINGS_H
#define DICTIONARYSETTINGS_H


enum class BrowseOrder : uchar { ABCDE, AIUEO };
enum class ResultOrder : uchar { Relevance, Frequency, JLPTfrom1, JLPTfrom5 };

struct DictionarySettings
{
    enum InflectionShow { Never, CurrentRow, Everywhere };
    enum JlptColumn { Frequency, Definition, Both };

    bool autosize = false;
    InflectionShow inflection = CurrentRow;
    ResultOrder resultorder = ResultOrder::Relevance;
    BrowseOrder browseorder = BrowseOrder::ABCDE;

    bool showingroup = false;
    bool showjlpt = true;
    JlptColumn jlptcolumn = Frequency;

    // Number of last searches stored.
    int historylimit = 1000;
    // Whether an entered string should be saved if the user doesn't touch the search field.
    bool historytimer = true;
    // Seconds timeout after last keypress in search field when its text is saved to history.
    int historytimeout = 4;
};


namespace Settings
{
    extern DictionarySettings dictionary;
}

#endif // DICTIONARYSETTINGS_H

