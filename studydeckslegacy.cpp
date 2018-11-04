/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>

#include "studydecks.h"
#include "zkanjimain.h"

static const quint32 s_1_day = 24 * 60 * 60;


void DeckTimeStatList::loadLegacy(QDataStream &stream, int /*version*/)
{
    double d;
    //qint32 val;
    quint8 b;

    qint32 cnt;
    stream >> cnt;
    items.resize(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        DeckTimeStat &s = items[ix];
        s.used = 0;

        stream >> b;
        s.used = b;
        if (s.used)
        {
            if (s.used > 100)
            {
                stream.readRawData((char*)s.repeat, 100);
                stream.skipRawData(s.used - 100);
                s.used = 100;
            }
            else
                stream.readRawData((char*)s.repeat, s.used);
        }

        stream >> b;
        s.timestats.resize(b);

        for (int j = 0, siz = tosigned(s.timestats.size()); j != siz; ++j)
        {
            DeckTimeStatItem &i = s.timestats[j];

            stream >> b;
            i.used = b;
            for (int k = 0; k != i.used; ++k)
            {
                stream >> d;
                i.time[k] = d * quint32(24) * quint32(60) * quint32(60) * quint32(10);
            }
        }
    }
}

void DeckDayStatList::loadLegacy(QDataStream &stream, int /*version*/)
{
    qint32 cnt;
    stream >> cnt;
    list.reserve(cnt);

    qint32 val;
    double d;

    for (int ix = 0; ix != cnt; ++ix)
    {
        DeckDayStat item;
        stream >> val;
        item.day = ZKanji::QDateTimeUTCFromTDateTime(val).date();

        stream >> d;
        item.timespent = d * quint32(24) * quint32(60) * quint32(60) * quint32(10);
        stream >> val;
        item.testcount = val;
        stream >> val;
        item.testednew = val;
        stream >> val;
        item.testwrong = val;
        stream >> val;
        item.testlearned = val;

        stream >> val;
        item.itemcount = val;
        stream >> val;
        item.groupcount = val;
        stream >> val;
        item.itemlearned = val;

        //item.itemproblematic = 0;

        list.push_back(item);
    }
}

void StudyDeck::loadLegacy(QDataStream &stream, int version)
{
    //quint32 uval;
    //stream >> uval;
    //id = StudyDeckId(uval);

    //stream >> id;

    qint32 val;
    stream >> val;
    list.reserve(val);
    ids.reserve(val);

    std::vector<int> nextvec(val);

    //int *nexts = nextvec.data();

    double d;

    for (int ix = 0, siz = tosigned(nextvec.size()); ix != siz; ++ix)
    {
        StudyCard *item = new StudyCard;
        list.push_back(item);
        ids.push_back(new CardId(ix));

        item->index = ix; // id.reset(new CardId(ix));

        //item->problematic = false;
        //item->wrongcnt = 0;
        //item->answercnt = 0;

        stream >> val;
        //fread(&p, sizeof(int), 1, f);
        item->data = val;

        stream >> d;
        item->testdate = ZKanji::QDateTimeUTCFromTDateTime(d);

        stream >> d; // Skip score.
        //item->difficulty = fixDifficultyFromScore(d);


        quint8 b;
        stream >> b;
        item->level = b;

        quint16 w;
        stream >> w;
        // Inclusion is now the same as the number of daily statistics the card has.
        // SKIP: item->inclusion = w;

        stream >> d;
        item->spacing = d * quint32(24) * quint32(60) * quint32(60) /** qint64(1000)*/;
        if (item->spacing < s_1_day * 2)
        {
            item->spacing = s_1_day;
            item->level = 1;
        }

        stream >> d;
        item->timespent = d * quint32(24) * quint32(60) * quint32(60) * quint32(10/*00*/);

        stream >> b;
        item->repeats = std::min<quint8>(254, b);

        stream >> b;
        item->testlevel = b;

        stream >> val;
        nextvec[ix] = val;

        stream.readRawData((char*)item->answers, 4);

        //fread(&item->answers.uncertain, sizeof(byte), 1, f);
        //fread(&item->answers.good, sizeof(byte), 1, f);
        //fread(&item->answers.hard, sizeof(byte), 1, f);
        //fread(&item->answers.easy, sizeof(byte), 1, f);

        stream >> d;
        item->itemdate = ZKanji::QDateTimeUTCFromTDateTime(d);

        qint32 cnt;
        stream >> cnt;
        item->stats.resize(cnt);

        // The time spent on each test day on a card is not saved originally, so it'll just be
        // the same value for each day in the imported stats.
        ushort timeavg = std::min<ushort>(65535, item->timespent / cnt);
        float multipl = ZKanji::profile().baseMultiplier();
        for (int j = 0; j != cnt; ++j)
        {
            StudyCardStat &stat = item->stats[j];

            stream >> val;
            stat.day = ZKanji::QDateTimeUTCFromTDateTime(val).date();

            stream >> b;
            //stat.score = b;

            stream >> b;
            stat.level = b;
            stat.timespent = timeavg;

            if (j != 0)
            {
                const StudyCardStat &prev = item->stats[j - 1];

                //if (prev.level + 1 < stat.level)
                //    multipl += 0.15;
                /*else*/ if (prev.level < stat.level)
                    multipl += 0.1f;
                else if (prev.level > stat.level + 1 && prev.day.daysTo(stat.day) > 1)
                {
                    // Wrong answer
                    if (prev.level > stat.level + 2 && stat.level == 1)
                        multipl = std::max(1.3, multipl - 0.32);
                    else
                        multipl = std::max(1.3, multipl - 0.14);
                }

                if (j > 1 && prev.level < stat.level)
                    multipl = std::max(1.3f, std::min(multipl, float(prev.day.daysTo(stat.day)) / float(item->stats[j - 2].day.daysTo(prev.day))));
            }

            stat.multiplier = multipl;
            //stat.status = 0;
        }
        item->multiplier = multipl;
        if (item->level >= 3)
            ZKanji::profile().addMultiplier(item->multiplier);
    }

    for (int ix = 0, siz = tosigned(nextvec.size()); ix != siz; ++ix)
    {
        if (nextvec[ix] >= 0)
            list[ix]->next = list[nextvec[ix]];
        else
        {
            list[ix]->next = list[ix];
        }
    }

    // Freeing up memory.
    std::vector<int>().swap(nextvec);

    timestats.loadLegacy(stream, version);
    daystats.loadLegacy(stream, version);
}

void StudyDeckList::loadDecksLegacy(QDataStream &stream, int version)
{
    list.clear();

    qint32 cnt;
    stream >> cnt;

    list.reserve(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        StudyDeckId id;
        stream >> id;
        id.id = ix + 1;
        list.push_back(new StudyDeck(this, id));
        list.back()->loadLegacy(stream, version);
        mapping.insert({ id, list.back() });
    }
    nextid.id = cnt + 1;
}

StudyDeckId StudyDeckList::skipIdLegacy(QDataStream &stream)
{
    StudyDeckId id;
    stream >> id;
    id.id = 1;
    return id;
}

