/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>

#include "groupstudy.h"
#include "groups.h"
#include "words.h"
#include "kanji.h"

void WordStudy::loadLegacy(QDataStream &stream, WordStudyMethod method, int version, const std::vector<int> &mapping)
{
    if (method != WordStudyMethod::Gradual && method != WordStudyMethod::Single)
        throw "No study data to be loaded, and don't use these throws already.";

    qint32 val;

    memset(&settings, 0, sizeof(WordStudySettings));

    settings.method = method;

    // Question type.
    stream >> val;
    switch (val)
    {
    case 0: // Meaning
        settings.tested = (int)WordStudyQuestion::Definition;
        break;
    case 1: // Kana
        settings.tested = (int)WordStudyQuestion::Kana;
        break;
    case 2: // Kanji
        settings.tested = (int)WordStudyQuestion::Kanji;
        break;
    case 3: // Kanji and Kana
        settings.tested = (int)WordStudyQuestion::KanjiKana;
        break;
    case 4: // Meaning and Kana
        settings.tested = (int)WordStudyQuestion::DefKana;
        break;
    case 5: // Meaning OR Kana
        settings.tested = (int)WordStudyQuestion::Definition | (int)WordStudyQuestion::Kana;
        break;
    case 6: // Meaning OR Kanji
        settings.tested = (int)WordStudyQuestion::Definition | (int)WordStudyQuestion::Kanji;
        break;
    case 7: // Meaning OR Kanji and Kana
        settings.tested = (int)WordStudyQuestion::Definition | (int)WordStudyQuestion::KanjiKana;
        break;
    case 8: // Kanji OR Kana
        settings.tested = (int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana;
        break;
    case 9: // Meaning OR Kanji OR Kana
        settings.tested = (int)WordStudyQuestion::Definition | (int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana;
        break;
    }

    // Answer type.
    stream >> val;
    switch (val)
    {
    case 0: // CorrectWrong
        settings.answer = WordStudyAnswering::CorrectWrong;
        break;
    case 1: // 5 Choices
        settings.answer = WordStudyAnswering::Choices5;
        break;
    case 2: // Type Kana
        settings.answer = WordStudyAnswering::CorrectWrong;
        settings.typed = (int)WordPartBits::Kana;
        break;
    case 3: // Type Kanji
        settings.answer = WordStudyAnswering::CorrectWrong;
        settings.typed = (int)WordPartBits::Kana;
        break;
    case 4: // Type Kanji and Kana
        settings.answer = WordStudyAnswering::CorrectWrong;
        settings.typed = (int)WordPartBits::Kanji | (int)WordPartBits::Kana;
        break;
    }

    qint8 c;
    stream >> c;
    settings.usetimer = c != 0;
    if (c != 0)
        settings.timer = c;

    stream >> c;
    settings.hidekana = c;

    settings.matchkana = false;

    stream >> c;
    //bool inited = c;

    qint16 w;
    stream >> w;
    //round = w;

    stream >> val;
    //current = val;
    stream >> val;
    //correct = val;
    stream >> val;
    //wrong = val;

    qint32 cnt;
    stream >> cnt;

    list.clear();
    list.reserve(cnt);

    const std::vector<int> &ownerindexes = owner->getIndexes();

    for (int ix = 0; ix != cnt; ++ix)
    {
        stream >> val;
        //TestItem item;
        //item.windex = indexes[val];
        //item.tested = (WordStudyQuestion)createRandomTested(settings.tested);
        //items.push_back(item);
        if (mapping[val] == -1)
            continue;
        list.push_back(WordStudyItem());
        list.back().windex = ownerindexes[mapping[val]];
    }

    // Skipping study statistics for simple study groups, because of future redesign.
    stream >> cnt;
    stream.skipRawData(23 * cnt);
    //fanswers->LoadFromFile(f, ver);

    //settings->load(stream, version);


    settings.single.prefernew = true;
    settings.single.preferhard = false;

    if (settings.method == WordStudyMethod::Gradual)
    {
        quint8 b;
        settings.shuffle = true;

        stream >> b;
        settings.gradual.initnum = b;

        quint32 uval;
        stream >> uval;
        // Originally: 0 - no shuffle, 1 - test backwards, 2 - shuffle round.
        settings.gradual.roundshuffle = uval == 2;

        stream >> b;
        settings.shuffle = b;

        stream >> b;
        settings.gradual.incnum = b;

        // Number of tested words at most in a single round:
        // 0 - all, 1 - 10, 2 - 20, 3 - 30
        // In the new test most choices make no sense.
        stream >> b;
        if (b == 0 || b == 1)
            b = 10;
        else
            b = 20;
        settings.gradual.roundlimit = 10;

        //if (settings.g_roundlimit < 5)
        //    settings.g_roundlimit = 3;
        //else if (settings.g_roundlimit < 8)
        //    settings.g_roundlimit = 5;
        //else
        //    settings.g_roundlimit = 10;

        stream >> b;
        //globals = b; // Global score affects which words are tested in a round


        state.reset(new WordStudyGradual(settings.gradual));
        state->loadLegacy(stream, version);
    }
    else if (settings.method == WordStudyMethod::Single)
    {
        settings.shuffle = true;

        state.reset(new WordStudySingle);
        state->loadLegacy(stream, version);

        quint32 uval;
        stream >> uval;
        settings.wordlimit = uval;
        settings.usewordlimit = true;
    }
    else
        throw "No method";

    state.reset();
}

void WordStudyGradual::loadLegacy(QDataStream &stream, int /*version*/)
{
    // Skips all data of old format suspended tests.

    //quint8 b;
    quint32 val;

    stream >> val;
    //included = val;

    stream >> val;
    //pos = val;

    stream >> val;
    //startpos = val;

    //wordstore.clear();
    stream >> val;
    //wordstore.reserve(val);

    while (val-- != 0)
    {
        quint32 windex;
        stream >> windex;
        //wordstore.push_back(windex);
    }
}

void WordStudySingle::loadLegacy(QDataStream &stream, int /*version*/)
{
    quint32 val;

    stream >> val;
    //inclusion = val;

    //stream >> val;
    //number = val;
}
