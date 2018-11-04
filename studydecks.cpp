/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QMessageBox>
#include <QTimeZone>
#include <QSet>
#include "studydecks.h"
#include "zkanjimain.h"
#include "zui.h"
#include "studysettings.h"

#include "checked_cast.h"

namespace
{
    const char  ZKANJI_PROFILE_FILE_VERSION[] = "001";

    //static const quint64 ms_1_minute = 60 * 1000;
    //static const quint64 ms_1_day = 24 * 60 * 60 * 1000;
    //static const quint64 ms_1_month = ms_1_day * 30.5;

    const quint32 s_1_day = 24 * 60 * 60;
    const quint32 s_1_month = s_1_day * 30.5;

    // File version of the dictionary groups data being loaded at the moment, to be used by
    // streaming operators that can't access the parent objects.
    int studyloadversion = -1;
}

//-------------------------------------------------------------


QDataStream& operator<<(QDataStream &stream, const StudyCardStat &stat)
{
    stream << make_zdate(stat.day);
    stream << (quint8)stat.level;
    stream << stat.multiplier;
    stream << (quint16)stat.timespent;
    //stream << (quint8)stat.status;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, StudyCardStat &stat)
{
    quint8 b;
    quint16 s;
    stream >> make_zdate(stat.day);
    stream >> b;
    stat.level = b;
    stream >> stat.multiplier;
    stream >> s;
    stat.timespent = s;
    //stream >> b;
    //stat.status = b;

    return stream;
}


//-------------------------------------------------------------


// Unique id given to each study card by their deck. Only unique within its own deck.
//bool operator==(const CardId &a, const CardId &b);
//bool operator!=(const CardId &a, const CardId &b);
//bool operator<(const CardId &a, const CardId &b);
//QDataStream& operator<<(QDataStream &stream, const CardId &id);
//QDataStream& operator>>(QDataStream &stream, CardId &id);

//CardId::CardId() : data(-1)
//{
//    ;
//}

//bool CardId::isValid()
//{
//    return data != -1;
//}

CardId::CardId(int data) : data(data)
{
    ;
}

//bool operator==(const CardId &a, const CardId &b)
//{
//    return a.data == b.data;
//}
//
//bool operator!=(const CardId &a, const CardId &b)
//{
//    return a.data != b.data;
//}
//
//bool operator<(const CardId &a, const CardId &b)
//{
//    return a.data < b.data;
//}

//QDataStream& operator<<(QDataStream &stream, const CardId &id)
//{
//    stream << (qint32)id.data;
//
//    return stream;
//}
//
//QDataStream& operator>>(QDataStream &stream, CardId &id)
//{
//    qint32 i;
//    stream >> i;
//    id.data = i;
//
//    return stream;
//}


//-------------------------------------------------------------


QDataStream& operator<<(QDataStream& stream, const StudyCard &c)
{
    stream << make_zvec<qint32, StudyCardStat>(c.stats);
    stream << make_zdate(c.testdate);
    stream << make_zdate(c.itemdate);
    stream.writeRawData((const char*)c.answers, 4);
    stream << (quint8)c.level;
    stream << c.multiplier;
    //stream << (quint16)c.inclusion;
    stream << (quint32)c.spacing;
    //stream << (quint16)c.answercnt;
    //stream << (quint16)c.wrongcnt;
    //stream << (qint8)c.problematic;
    stream << (qint8)c.learned;
    stream << (quint8)c.repeats;
    stream << (quint8)c.testlevel;
    stream << (quint32)c.timespent;

    //stream << *c.id.get();

    return stream;
}

QDataStream& operator>>(QDataStream& stream, StudyCard &c)
{
    quint8 b;
    qint8 ch;
    //quint16 s;
    quint32 ui;

    stream >> make_zvec<qint32, StudyCardStat>(c.stats);
    stream >> make_zdate(c.testdate);
    stream >> make_zdate(c.itemdate);
    stream.readRawData((char*)c.answers, 4);

    stream >> b;
    c.level = b;
    stream >> c.multiplier;
    //stream >> s;
    //c.inclusion = s;
    stream >> ui;
    c.spacing = ui;
    c.nexttestdate = QDateTime();
    //stream >> s;
    //c.answercnt = s;
    //stream >> s;
    //c.wrongcnt = s;
    //stream >> ch;
    //c.problematic = ch;

    if (studyloadversion >= 3)
    {
        stream >> ch;
        c.learned = ch;
    }
    stream >> b;
    c.repeats = b;
    stream >> b;
    c.testlevel = b;
    stream >> ui;
    c.timespent = ui;

    return stream;
}

QDataStream& operator<<(QDataStream &stream, const DeckTimeStatItem &i)
{
    stream << (quint8)i.used;
    for (int ix = 0; ix != i.used; ++ix)
        stream << (quint32)i.time[ix];

    return stream;
}

QDataStream& operator>>(QDataStream &stream, DeckTimeStatItem &i)
{
    quint8 b;
    quint32 ui;
    stream >> b;
    i.used = b;
    for (int ix = 0; ix != i.used; ++ix)
    {
        stream >> ui;
        i.time[ix] = ui;
    }

    return stream;
}


QDataStream& operator<<(QDataStream &stream, const DeckTimeStat &s)
{
    stream << (quint8)s.used;
    for (int ix = 0; ix != s.used; ++ix)
        stream << (quint8)s.repeat[ix];
    stream << make_zvec<qint32, const DeckTimeStatItem>(s.timestats);

    return stream;
}

QDataStream& operator>>(QDataStream &stream, DeckTimeStat &s)
{
    quint8 b;
    stream >> b;
    s.used = b;
    for (int ix = 0; ix != s.used; ++ix)
    {
        stream >> b;
        *(quint8*)&(s.repeat[ix]) = b;
    }
    stream >> make_zvec<qint32, DeckTimeStatItem>(s.timestats);

    return stream;
}


//-------------------------------------------------------------


DeckTimeStatList::DeckTimeStatList()
{

}

void DeckTimeStatList::load(QDataStream &stream)
{
    stream >> make_zvec<qint32, DeckTimeStat>(items);
}

void DeckTimeStatList::save(QDataStream &stream) const
{
    stream << make_zvec<qint32, DeckTimeStat>(items);
}

void DeckTimeStatList::createUndo(uchar level)
{
    undocount = tosigned(items.size());
    if (items.size() > level)
        undostat = items[level];
}

void DeckTimeStatList::revertUndo(uchar level)
{
    if (level >= undocount)
        items.resize(undocount);
    else
        items[level] = undostat;
    //DeckTimeStat &stat = items[level];
    //memmove(stat.repeat, stat.repeat + 1, sizeof(uchar) * 100);
    //--stat.used;
}

void DeckTimeStatList::addTime(uchar level, uchar repeats, quint32 time)
{
    if (items.size() <= level)
    {
        int s = tosigned(items.size());
        items.resize(level + 1);
        for (int ix = s; ix != level + 1; ++ix)
            items[ix].used = 0;
    }
    DeckTimeStat &stat = items[level];

    repeats = std::min<uchar>(254, repeats);
    if (stat.timestats.size() <= repeats)
    {
        int s = tosigned(stat.timestats.size());
        stat.timestats.resize(repeats + 1);
        for (int ix = s; ix != repeats + 1; ++ix)
            stat.timestats[ix].used = 0;
    }

    DeckTimeStatItem &tstat = stat.timestats[repeats];
    memmove(tstat.time + 1, tstat.time, sizeof(quint32) * 254);
    tstat.time[0] = time;
    tstat.used = std::min(tstat.used + 1, 255);
}

void DeckTimeStatList::addRepeat(uchar level, uchar repeats)
{
#ifdef _DEBUG
    if (items.size() <= level)
        throw "Use addTime first to add the level to items.";
#endif

    DeckTimeStat &stat = items[level];
    memmove(stat.repeat + 1, stat.repeat, sizeof(uchar) * 99);
    stat.repeat[0] = std::min<uchar>(254, repeats);
    stat.used = std::min(stat.used + 1, 100);
}

quint32 DeckTimeStatList::estimate(uchar level, uchar repeats) const
{
    if (items.size() <= level || items[level].used < 10)
    {
        if (level > 0)
            return estimate(level - 1, repeats) * 0.9;

        if (items.empty() || items[0].timestats.empty())
        {
            // Estimate 5 repeats for each new item when there's no data at all, with 1
            // minute test time for the 0th repeat, and 30 seconds for the rest.
            if (repeats == 0)
                return 3 * 60 * 10;
            // If we are at a higher than average repeat, return 40 seconds.
            if (repeats >= 5)
                return 40 * 10;
            return 30 * 10 * repeats;
        }

        if (repeats >= 5)
            return estimate(level, repeats - 1) * 1.2;

        // Calculate real time from the existing data and estimate 3 minutes to fill it up to
        // 5 repeats like above.
        qint64 r = 0;
        for (int ix = repeats, siz = std::min<int>(5, tosigned(items[0].timestats.size())); ix < siz; ++ix)
        {
            qint64 avg = _estimate(0, ix);

            const DeckTimeStatItem &tstat = items[0].timestats[ix];
            for (int iy = tstat.used; iy < 10; ++iy)
                avg -= (avg - (ix == 0 ? 60 * 10 : 30 * 10)) / (iy + 1);

            r += avg;
        }
        
        if (items[0].timestats.size() < 5)
            r += 30 * 10 * (5 - items[0].timestats.size());

        return std::max<qint64>(0, r);
    }

    const DeckTimeStat &stat = items[level];
    // Number of times an item at level is repeated in average.
    int repeatavg = 0;
    for (int ix = 0; ix != stat.used; ++ix)
        repeatavg += stat.repeat[ix];
    repeatavg = (repeatavg / stat.used) + 1;

    if (repeats >= repeatavg)
    {
        if (stat.timestats.size() <= repeats)
            return estimate(level, repeats - 1);

        qint64 avg = _estimate(level, repeats);
        const DeckTimeStatItem &tstat = stat.timestats[repeats];
        if (tstat.used < 10)
        {
            quint32 prevstimate = estimate(level, repeats - 1);
            for (int ix = tstat.used; ix != 10; ++ix)
                avg -= (avg - prevstimate) / (ix + 1);
        }

        return std::max<qint64>(0, avg);
    }

    qint64 r = 0;
    for (int ix = repeats, siz = std::min<int>(repeatavg, tosigned(stat.timestats.size())); ix < siz; ++ix)
    {
        qint64 avg = _estimate(level, ix);
        const DeckTimeStatItem &tstat = stat.timestats[ix];

        if (tstat.used < 10)
        {
            quint32 prevstimate = _estimate(level, repeats - 1);
            for (int iy = tstat.used; iy != 10; ++iy)
                avg -= (avg - prevstimate) / (iy + 1);
        }

        r += avg;
    }

    int ssiz = tosigned(stat.timestats.size());
    if (ssiz <= repeatavg)
    {
        for (int ix = ssiz; ix != repeatavg; ++ix)
            r += _estimate(level, ssiz - 1) * 0.8;
    }

    return std::max<qint64>(0, r);

}

quint32 DeckTimeStatList::_estimate(uchar level, uchar repeats) const
{
    const DeckTimeStat &stat = items[level];
    const DeckTimeStatItem &tstat = stat.timestats[repeats];

    if (tstat.used == 0)
        return 0;

    qint64 avg = tstat.time[0];
    for (int ix = 1, siz = tstat.used; ix != siz; ++ix)
        avg -= (avg - tstat.time[ix]) / (ix + 1);

    return std::max<qint64>(0, avg);
}

//-------------------------------------------------------------


QDataStream& operator<<(QDataStream &stream, const DeckDayStat &s)
{
    stream << make_zdate(s.day);
    stream << (qint32)s.timespent;
    stream << (qint32)s.testcount;
    stream << (qint32)s.testednew;
    stream << (qint32)s.testwrong;
    stream << (qint32)s.testlearned;
    //stream << (qint32)s.testproblematic;
    stream << (qint32)s.itemcount;
    stream << (qint32)s.groupcount;
    stream << (qint32)s.itemlearned;
    //stream << (qint32)s.itemproblematic;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, DeckDayStat &s)
{
    qint32 i;
    stream >> make_zdate(s.day);
    stream >> i;
    s.timespent = i;
    stream >> i;
    s.testcount = i;
    stream >> i;
    s.testednew = i;
    stream >> i;
    s.testwrong = i;
    stream >> i;
    s.testlearned = i;
    //stream >> i;
    //s.testproblematic = i;
    stream >> i;
    s.itemcount = i;
    stream >> i;
    s.groupcount = i;
    stream >> i;
    s.itemlearned = i;
    //stream >> i;
    //s.itemproblematic = i;

    return stream;
}

//-------------------------------------------------------------


DeckDayStatList::DeckDayStatList()
{
    ;
}

void DeckDayStatList::load(QDataStream &stream)
{
    stream >> make_zvec<qint32, DeckDayStat>(list);
}

void DeckDayStatList::save(QDataStream &stream) const
{
    stream << make_zvec<qint32, DeckDayStat>(list);
}

void DeckDayStatList::fixStats(const std::vector<std::tuple<StudyCard*, int, bool>> &cards)
{
    // Recreates the daily stats from scratch, only bringing over the time spent testing from
    // the old statistics.

    std::vector<DeckDayStat> tmp;
    std::swap(tmp, list);

    QSet<StudyCard*> added;
    int statpos = 0;
    int timespent = 0;
    for (int ix = 0, siz = tosigned(cards.size()); ix != siz; ++ix)
    {
        StudyCard *card = std::get<0>(cards[ix]);
        int cardstatix = std::get<1>(cards[ix]);
        QDate carddate = card->stats[cardstatix].day;
        bool cardgood = std::get<2>(cards[ix]);
        while (statpos < tosigned(tmp.size()) && tmp[statpos].day <= carddate)
        {
            timespent += tmp[statpos].timespent;
            ++statpos;
        }

        if (list.empty() || list.back().day != carddate)
        {
            DeckDayStat stat;
            stat.day = carddate;
            stat.timespent = timespent;
            timespent = 0;
            stat.testcount = 0;
            stat.testednew = 0;
            stat.testwrong = 0;
            stat.testlearned = 0;
            stat.itemcount = list.empty() ? 0 : list.back().itemcount;
            stat.groupcount = list.empty() ? 0 : list.back().groupcount;
            stat.itemlearned = list.empty() ? 0 : list.back().itemlearned;
            list.push_back(stat);
        }

        DeckDayStat &stat = list.back();
        if (cardstatix == 0)
        {
            ++stat.itemcount;
            ++stat.testednew;

            StudyCard *pos = card;
            bool newgroup = true;
            while (newgroup && pos->next != card)
            {
                pos = pos->next;
                if (added.contains(pos))
                    newgroup = false;
            }
            if (newgroup)
                ++stat.groupcount;

            added << card;
        }

        ++stat.testcount;
        if (!cardgood)
            ++stat.testwrong;

        bool newlearned = cardgood && cardstatix != 0 && card->stats[cardstatix - 1].day.daysTo(card->stats[cardstatix].day) >= 61;
        if (!card->learned && newlearned)
        {
            ++stat.testlearned;
            ++stat.itemlearned;
        }
        else if (card->learned && !newlearned)
            --stat.itemlearned;
        card->learned = newlearned;
    }
}

bool DeckDayStatList::empty() const
{
    return list.empty();
}

DeckDayStat& DeckDayStatList::back()
{
    return list.back();
}

const DeckDayStat& DeckDayStatList::back() const
{
    return list.back();
}

DeckDayStatList::size_type DeckDayStatList::size() const
{
    return list.size();
}

const DeckDayStat& DeckDayStatList::items(int index) const
{
    return list[index];
}

DeckDayStat& DeckDayStatList::items(int index)
{
    return list[index];
}

void DeckDayStatList::clear()
{
    list.clear();
}

void DeckDayStatList::createUndo(QDate testdate)
{
    addDay(testdate);
    undo = list.back();
}

void DeckDayStatList::revertUndo()
{
#ifdef _DEBUG
    if (list.empty() || list.back().day != undo.day)
        throw "Undo data not saved.";
#endif
    list.back() = undo;
}

void DeckDayStatList::newCard(QDate testdate, bool newgroup)
{
    addDay(testdate);
    DeckDayStat &stat = list.back();
    ++stat.itemcount;
    if (newgroup)
        ++stat.groupcount;
}

void DeckDayStatList::cardDeleted(QDate testdate, bool grouplast, bool learned)
{
    //if (list.empty() || list.back().day < testdate)
    addDay(testdate);
    DeckDayStat &stat = list.back();
    --stat.itemcount;
    if (grouplast)
        --stat.groupcount;
    if (learned)
        --stat.itemlearned;
}

void DeckDayStatList::cardTested(QDate testdate, int timespent, bool wrong, bool newcard, bool orilearned, bool newlearned)
{
    addDay(testdate);

    DeckDayStat &stat = list.back();
    stat.timespent += timespent;
    ++stat.testcount;
    if (newcard)
        ++stat.testednew;
    if (!newcard && wrong)
        ++stat.testwrong;
    if (orilearned && !newlearned)
        --stat.itemlearned;
    else if (!orilearned && newlearned)
    {
        ++stat.itemlearned;
        ++stat.testlearned;
    }
}

void DeckDayStatList::groupsMerged()
{
    if (list.empty())
        return;
    --list.back().groupcount;
}

void DeckDayStatList::addDay(QDate testdate)
{
#ifdef _DEBUG
    if (!list.empty() && list.back().day > testdate)
        throw "Invalid test date. The date shouldn't be for an earlier day.";
#endif

    if (!list.empty() && list.back().day == testdate)
        return;

    DeckDayStat stat;
    stat.day = testdate;
    stat.timespent = 0;
    stat.testcount = 0;
    stat.testednew = 0;
    stat.testwrong = 0;
    stat.testlearned = 0;
    stat.itemcount = 0;
    stat.groupcount = 0;
    stat.itemlearned = 0;

    if (!list.empty())
    {
        DeckDayStat &past = list.back();
        stat.itemcount = past.itemcount;
        stat.groupcount = past.groupcount;
        stat.itemlearned = past.itemlearned;
    }

    list.push_back(stat);
}


//-------------------------------------------------------------


/* static */ StudyDeckId StudyDeckId::next(StudyDeckId a, StudyDeckId b)
{
    return StudyDeckId(std::max(a.id, b.id) + 1);
}

/* static */ StudyDeckId StudyDeckId::next(StudyDeckId a)
{
    return StudyDeckId(a.id + 1);
}

StudyDeckId::StudyDeckId() : id(0)
{
    ;
}

StudyDeckId::StudyDeckId(const StudyDeckId &orig) : id(orig.id)
{

}

StudyDeckId::StudyDeckId(StudyDeckId &&orig) : id(orig.id)
{

}

StudyDeckId& StudyDeckId::operator=(const StudyDeckId &orig)
{
    id = orig.id;
    return *this;
}

StudyDeckId& StudyDeckId::operator=(StudyDeckId &&orig)
{
    id = orig.id;
    return *this;
}

bool StudyDeckId::valid() const
{
    return id != 0;
}

StudyDeckId::StudyDeckId(intptr_t id) : id(id) 
{
    ;
}

bool operator==(const StudyDeckId &a, const StudyDeckId &b)
{
    return a.id == b.id;
}

QDataStream& operator<<(QDataStream &stream, const StudyDeckId &id)
{
    stream << (quint32)id.id;
    return stream;
}

QDataStream& operator>>(QDataStream &stream, StudyDeckId &id)
{
    quint32 uval;
    stream >> uval;
    id.id = uval;
    return stream;
}


//-------------------------------------------------------------


size_t StudyDeckIdHash::operator()(const StudyDeckId &id) const
{
    return std::hash<int>()(id.id);
}

//-------------------------------------------------------------


StudyDeck::StudyDeck(StudyDeckList *owner, StudyDeckId id) : owner(owner), id(id)
{

}

StudyDeck::~StudyDeck()
{
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        StudyCard *card = list[ix];
        if (card->level >= 3)
            ZKanji::profile().removeMultiplier(card->multiplier);
        
    }
}

void StudyDeck::load(QDataStream &stream)
{
    //stream >> id;
    stream >> make_zdate(testdate);

    qint32 i;
    stream >> i;
    list.reserve(i);
    ids.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(new StudyCard);
        list.back()->index = ix;// .reset(new CardId(ix));
        stream >> *list.back();
        ids.push_back(new CardId(ix));
    }

    timestats.load(stream);
    daystats.load(stream);

    for (StudyCard *c : list)
    {
        qint32 idindex;
        stream >> idindex;

        if (c->index /*->data*/ == idindex)
            c->next = c;
        else
            c->next = list[idindex];
    }

    stream >> make_zvec<qint32, qint32>(testcards);
}

void StudyDeck::save(QDataStream &stream) const
{
    //stream << id;
    stream << make_zdate(testdate);

    stream << (qint32)list.size();
    for (auto *sc : list)
        stream << *sc;

    timestats.save(stream);
    daystats.save(stream);

    for (const StudyCard *c : list)
        stream << (qint32)c->next->index /*->data*/;

    stream << make_zvec<qint32, qint32>(testcards);
}

void StudyDeck::copy(StudyDeck *src, std::map<CardId*, CardId*> &map)
{
    testdate = src->testdate;
    testcards = src->testcards;
    timestats = src->timestats;
    daystats = src->daystats;

    list.clear();
    ids.clear();

    list.reserve(src->list.size());
    ids.reserve(src->ids.size());

    for (int ix = 0, siz = tosigned(src->ids.size()); ix != siz; ++ix)
    {
        CardId *cid = new CardId(ix);
        ids.push_back(cid);
        map.insert(std::make_pair(src->ids[ix], cid));
    }

    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        StudyCard *c = new StudyCard;
        StudyCard *csrc = src->list[ix];
        *c = *csrc;
        c->data = 0;
        c->next = nullptr;

        list.push_back(c);
    }

    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        StudyCard *c = list[ix];
        StudyCard *csrc = src->list[ix];

        if (csrc->next != nullptr)
        {
            if (csrc->next == csrc)
                c->next = c;
            else
                c->next = list[csrc->next->index];
        }
    }
}

const DeckDayStatList& StudyDeck::dayStats() const
{
    return daystats;
}

//void StudyDeck::fixDayStats()
//{
//    if (daystats.empty() || daystats.back().itemcount == list.size())
//        return;
//
//    DeckDayStat &back = daystats.back();
//    back.itemcount = list.size();
//    back.itemlearned = 0;
//
//    for (int ix = 0; ix != back.itemcount; ++ix)
//    {
//        if (list[ix]->spacing > s_1_month * 2)
//            ++back.itemlearned;
//    }
//}

void StudyDeck::fixDayStats()
{
    if (list.empty())
    {
        daystats.clear();
        return;
    }

    testcards.clear();

    // This function simulates answering the cards one by one on each study date.

    // [ card, card stat index, answer was correct]
    std::vector<std::tuple<StudyCard*, int, bool>> tmp;
    //std::map<StudyCard*, int> incl;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        StudyCard *c = list[ix];

        //c->answercnt = 0;
        //c->wrongcnt = 0;

        //incl[c] = 0;
        for (int iy = 0, sizy = tosigned(c->stats.size()); iy != sizy; ++iy)
        {
            if (iy != 0 && c->stats[iy - 1].day == c->stats[iy].day)
                continue;
            tmp.push_back(std::make_tuple(c, iy, iy == 0 || c->stats[iy].level > c->stats[iy - 1].level));
        }

        c->learned = false;
    }

    std::sort(tmp.begin(), tmp.end(), [](const std::tuple<StudyCard*, int, bool> &a, const std::tuple<StudyCard*, int, bool> &b) {
        StudyCard *ca = std::get<0>(a);
        StudyCard *cb = std::get<0>(b);
        return ca->stats[std::get<1>(a)].day < cb->stats[std::get<1>(b)].day;
    });

    if (!tmp.empty())
    {
        QDate cdate = std::get<0>(tmp.back())->stats.back().day;
        auto it = tmp.end();
        while (it != tmp.begin())
        {
            auto pit = std::prev(it);
            if (std::get<0>(*pit)->stats[std::get<1>(*pit)].day == cdate)
                testcards.push_back(std::get<0>(*pit)->index);
            it = pit;
        }
    }

    std::sort(testcards.begin(), testcards.end(), [this](int a, int b) { return list[a]->itemdate < list[b]->itemdate; });

    daystats.fixStats(tmp);

}

#ifdef _DEBUG
void StudyDeck::checkDayStats() const
{
    if (daystats.empty())
        return;

    const DeckDayStat &stat = daystats.back();

    bool error = false;
    if (stat.itemcount != tosigned(list.size()))
        error = true;

    int lcnt = 0;
    int gcnt = 0;
    QSet<const StudyCard*> added;
    for (const StudyCard *c : list)
    {
        if (c->learned)
            ++lcnt;

        const StudyCard *pos = c;
        bool newgrp = true;
        while (newgrp && pos->next != c)
        {
            pos = pos->next;
            if (added.contains(pos))
                newgrp = false;
        }
        if (newgrp)
            ++gcnt;
        added << c;
    }

    if (stat.itemlearned != lcnt)
        error = true;

    if (stat.groupcount != gcnt)
        error = true;

    if (error)
        throw "Day stat check failed.";
}
#endif

StudyDeckId StudyDeck::deckId() const
{
    return id;
}

CardId* StudyDeck::cardId(int index) const
{
    return const_cast<CardId*>(ids[index]); //list[index]->id.get();
}

int StudyDeck::cardIndex(CardId *cardid) const
{
    return posFromId(cardid);
}

void StudyDeck::fixResizeCardId(int size)
{
    if (tosigned(ids.size()) == size)
        return;
    QString errormsg = qApp->translate("", "The study data is corrupted. There's a high chance that the cards in the long-term study list will have invalid intervals and score.");
    QMessageBox::warning(nullptr, "zkanji", errormsg);
    while (tosigned(ids.size()) > size)
        deleteCard(ids.back());
    while (tosigned(ids.size()) < size)
        createCard(nullptr, 0);
}

CardId* StudyDeck::loadCardId(QDataStream &stream)
{
    qint32 val;
    stream >> val;
    if (val == -1)
        return nullptr;
    return ids[val];
}

void StudyDeck::saveCardId(QDataStream &stream, CardId *cid) const
{
    if (cid == nullptr)
        stream << (qint32)-1;
    else
        stream << (qint32)cid->data;
}

void StudyDeck::setCardData(CardId *cardid, intptr_t data)
{
    StudyCard *card = fromId(cardid);
    card->data = data;
}

intptr_t StudyDeck::cardData(const CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    return card->data;
}

CardId* StudyDeck::nextCard(CardId *cardid)
{
    StudyCard *card = fromId(cardid);
    return ids[card->next->index];
}

void StudyDeck::groupData(CardId *cardid, std::vector<intptr_t> &result)
{
    StudyCard *card = fromId(cardid);
    StudyCard *first = card;
    do
    {
        result.push_back(card->data);
        card = card->next;
    } while (card != first);
}

CardId* StudyDeck::createCard(CardId *cardid_group, intptr_t data)
{
    StudyCard *group = fromId(cardid_group);
    StudyCard *card = new StudyCard;
    card->data = data;
    memset(card->answers, 0, sizeof(uchar) * 4);

    //card->problematic = false;
    //card->wrongcnt = 0;
    //card->answercnt = 0;

    //card->level = 0;
    //card->inclusion = 0;
    card->spacing = 0;
    card->repeats = 0;
    card->level = 0;
    card->multiplier = ZKanji::profile().baseMultiplier();
    card->testlevel = 0;
    card->timespent = 0;
    card->next = card;
    card->learned = false;
    //if (list.empty())
    //    card->id = CardId(0);
    //else
    //    card->id = CardId(list.back()->id.data + 1);
    card->index = tosigned(list.size()); // id.reset(new CardId(list.size()));

    if (group != nullptr)
    {
        // Card is placed as the second item in the group.
        card->next = group->next;
        group->next = card;
    }
    daystats.newCard(ltDay(testdate), group == nullptr);

    list.push_back(card);
    ids.push_back(new CardId(tosigned(ids.size())));

    return ids.back(); //card->id.get();
}

CardId* StudyDeck::deleteCard(CardId *cardid)
{
    // IMPORTANT: when changing, update deleteCardGroup() too (below.) which does the same
    // thing but does the bookkeeping only once.

    int cardix = cardid == nullptr ? -1 : cardid->data; // posFromId(cardid);
    if (cardix == -1)
#ifdef _DEBUG
        throw "No such card.";
#else
        return nullptr;
#endif

    StudyCard *card = list[cardix];

    // Result to be returned. The next card in the same group.
    int rindex;

    if (card->next == card)
        rindex = -1;
    else
    {
        rindex = card->next->index;

        // Remove card from its group.
        StudyCard *pos = card->next;
        while (pos->next != card)
            pos = pos->next;
        pos->next = card->next;
    }

    if (card->level >= 3)
        ZKanji::profile().removeMultiplier(card->multiplier);

    if (rindex > cardix)
        --rindex;

    daystats.cardDeleted(ltDay(QDateTime::currentDateTimeUtc()), rindex == -1, card->learned);

    auto it = testcards.begin();
    while (it != testcards.end())
    {
        if (*it == cardix)
        {
            it = testcards.erase(it);
            continue;
        }
        else if (*it > cardix)
            --*it;
        ++it;
    }

    list.erase(list.begin() + cardix);
    ids.erase(ids.begin() + cardix);
    for (int siz = tosigned(list.size()); cardix != siz; ++cardix)
    {
        --list[cardix]->index; //id->data;
        --ids[cardix]->data;
    }

    return rindex == -1 ? nullptr : ids[rindex]; //r;
}

void StudyDeck::deleteCardGroup(CardId *cardid)
{
#ifdef _DEBUG
    if (cardid == nullptr || cardid->data < 0 || cardid->data >= tosigned(list.size()))
        throw "Invalid card id.";
#endif

    StudyCard *card = list[cardid->data];
    StudyCard *first = card;
    auto listdata = list.data();
    auto iddata = ids.data();

    std::vector<int> dellist;

    int removed = 0;
    do
    {
        int cardix = card->index /*->data*/;
        dellist.push_back(cardix);
        //owner->changeAnswerRatio(-card->answercnt, -card->wrongcnt);
        if (card->level >= 3)
            ZKanji::profile().removeMultiplier(card->multiplier);

        daystats.cardDeleted(ltDay(QDateTime::currentDateTimeUtc()), card->next == first, card->learned);

        card = card->next;
        delete listdata[cardix];
        listdata[cardix] = nullptr;
        delete iddata[cardix];
        iddata[cardix] = nullptr;

        auto it = testcards.begin();
        while (it != testcards.end())
        {
            if (*it == cardix)
            {
                it = testcards.erase(it);
                continue;
            }
            else if (*it > cardix)
                --*it;
            ++it;
        }

        ++removed;
    } while (first != card);


    std::sort(dellist.begin(), dellist.end());

    // Removing the cards from list and updating the id of the rest.
    int ix = 0;
    for (int pos = 0, siz = tosigned(list.size()); pos != siz; ++pos)
    {
        if (listdata[pos] == nullptr)
            continue;

        if (listdata[ix] == nullptr)
        {
            listdata[ix] = listdata[pos];
            listdata[pos] = nullptr;
            iddata[ix] = iddata[pos];
            iddata[pos] = nullptr;
        }

        listdata[ix]->index /*->data*/ = ix;
        iddata[ix]->data = ix;

        ++ix;
    }
    list.resizeAddNull(ix);

}

bool StudyDeck::sameGroup(CardId *g1, CardId *g2) const
{
    if (g1 == g2)
        return true;

    const StudyCard *first = fromId(g1);
    const StudyCard *other = fromId(g2);
    const StudyCard *pos = first;
    while (pos->next != first)
    {
        if (pos->next == other)
            return true;
        pos = pos->next;
    }
    return false;
}

void StudyDeck::mergeGroups(CardId *g1, CardId *g2)
{
    if (sameGroup(g1, g2))
        return;

    StudyCard *first = fromId(g2);
    StudyCard *last = first;
    while (last->next != first)
        last = last->next;

    StudyCard *g = fromId(g1);
    last->next = g->next;
    g->next = first;

    daystats.groupsMerged();
}

ushort StudyDeck::dayStatSize() const
{
    return tounsigned<ushort>(daystats.size());
}

const DeckDayStat& StudyDeck::dayStat(int index) const
{
    return daystats.items(index);
}


//int StudyDeck::cardCount()
//{
//    return list.size();
//}

//StudyCard* StudyDeck::cards(int index)
//{
//    return list[index];
//}

uchar StudyDeck::cardLevelOld(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr || card->stats.empty())
        return 0;

    return card->stats.size() > 1 ? std::prev(card->stats.end(), 2)->level : card->level;
}

uchar StudyDeck::cardLevel(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;
    return card->level;
}

const uchar* StudyDeck::cardTries(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return nullptr;
    return card->answers;
}

uchar StudyDeck::cardTestLevel(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;
    return card->testlevel;
}

bool StudyDeck::cardFirstShow(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return false;
    return card->repeats == 1;
}

ushort StudyDeck::cardInclusion(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;

    return tounsigned<ushort>(card->stats.size());// inclusion;
}

QDateTime StudyDeck::cardTestDate(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return QDateTime();

    return card->testdate;
}

QDate StudyDeck::cardFirstStatDate(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr || card->stats.empty())
        return QDate();

    return card->stats[0].day;
}

QDateTime StudyDeck::cardItemDate(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return QDateTime();

    return card->itemdate;
}

QDateTime StudyDeck::cardNextTestDate(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr || !card->testdate.isValid())
        return QDateTime();

    if (!card->nexttestdate.isValid())
        card->nexttestdate = card->testdate.addSecs(card->spacing);
    return card->nexttestdate;
}

quint32 StudyDeck::cardSpacing(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;

    return card->spacing;
}

quint32 StudyDeck::increasedSpacing(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;

    if (card->stats.empty())
        return card->spacing;

    quint32 spacing = card->spacing * card->multiplier;
    fixCardSpacing(card, card->testdate, card->level + 1, spacing);
    return spacing;
}

quint32 StudyDeck::decreasedSpacing(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;

    if (card->stats.empty())
        return card->spacing;

    if (card->level < 2)
        return card->spacing;
    quint32 spacing = std::max<quint32>(card->spacing / card->multiplier, 24 * 60 * 60);
    fixCardSpacing(card, card->testdate, card->level - 1, spacing);
    return spacing;
}

void StudyDeck::increaseSpacingLevel(CardId *cardid)
{
    StudyCard *card = fromId(cardid);
    if (card == nullptr || card->stats.empty())
        return;

    if (card->level >= 3)
        ZKanji::profile().removeMultiplier(card->multiplier);

    quint32 spacing = card->spacing * card->multiplier;
    fixCardSpacing(card, card->testdate, card->level + 1, spacing);
    card->spacing = spacing;
    ++card->level;
    card->nexttestdate = QDateTime();

    if (card->level >= 3)
        ZKanji::profile().addMultiplier(card->multiplier);

    if (!card->stats.empty())
        updateCardStat(card, 0);
}

void StudyDeck::decreaseSpacingLevel(CardId *cardid)
{
    StudyCard *card = fromId(cardid);
    if (card == nullptr || card->level < 2 || card->stats.empty())
        return;

    if (card->level >= 3)
        ZKanji::profile().removeMultiplier(card->multiplier);

    quint32 spacing = std::max<quint32>(card->spacing / card->multiplier, 24 * 60 * 60);
    fixCardSpacing(card, card->testdate, card->level - 1, spacing);
    card->spacing = spacing;
    --card->level;
    card->nexttestdate = QDateTime();

    if (card->level >= 3)
        ZKanji::profile().addMultiplier(card->multiplier);

    if (!card->stats.empty())
        updateCardStat(card, 0);
}

void StudyDeck::resetCardStudyData(CardId *cardid)
{
    StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return;

    if (card->level >= 3)
        ZKanji::profile().removeMultiplier(card->multiplier);
    if (card->learned)
        --daystats.back().itemlearned;

    card->stats.clear();
    memset(card->answers, 0, sizeof(uchar) * 4);
    card->spacing = 0;
    card->repeats = 0;
    card->level = 0;
    card->multiplier = ZKanji::profile().baseMultiplier();
    card->testlevel = 0;
    card->timespent = 0;
    card->learned = false;
    card->nexttestdate = QDateTime();
    card->itemdate = QDateTime();
    card->testdate = QDateTime();

    auto it = std::find(testcards.begin(), testcards.end(), card->index);
    if (it != testcards.end())
    {
        testcards.erase(it);
    }
}

float StudyDeck::cardMultiplierOld(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr || card->stats.empty())
        return 0;
    return card->stats.size() > 1 ? std::prev(card->stats.end(), 2)->multiplier : card->multiplier;
}

float StudyDeck::cardMultiplier(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return 0;
    return card->multiplier;
}

quint32 StudyDeck::cardEta(CardId *cardid) const
{
    const StudyCard *card = fromId(cardid);
    if (card == nullptr)
        return newCardEta();

    return timestats.estimate(card->repeats == 0 ? card->level : card->testlevel, card->repeats);
}

quint32 StudyDeck::newCardEta() const
{
    return timestats.estimate(0, 0);
}

bool StudyDeck::startTestDay()
{
    // TODO: don't allow testing if the dates are invalid compared to past statistics.

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDate testday = ltDay(now);
    if (testdate.isValid() && ltDay(testdate).daysTo(testday) <= 0)
        return false;

    undodata.card = nullptr;

    testdate = now;

    for (StudyCard *c : list)
        c->repeats = 0;

    testcards.clear();

    return true;
}

int StudyDeck::daysSinceLastInclude() const
{
    if (daystats.empty())
        return -1;

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDate day = ltDay(now);

    for (int ix = tosigned(daystats.size()) - 1; ix != -1; --ix)
    {
        const DeckDayStat &stat = daystats.items(ix);
        if (stat.testednew == 0)
            continue;
        return std::max<qint64>(0, stat.day.daysTo(day));
    }
    return -1;
}

bool StudyDeck::firstTest() const
{
    if (!testdate.isValid())
        return true;
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDate testday = ltDay(now);
    return ltDay(testdate).daysTo(testday) > 0;
}

QDate StudyDeck::testDay() const
{
    // TODO: when user changes test day start/end, update the testdate and anything else that can screw things up.
    return ltDay(testdate);
}

int StudyDeck::testSize() const
{
    return tosigned(testcards.size());
}

CardId* StudyDeck::testCard(int index) const
{
    return const_cast<CardId*>(ids[testcards[index]]);
}

quint32 StudyDeck::answer(CardId *cardid, StudyCard::AnswerType a, qint64 answertime, /*bool &postponed,*/ bool simulate)
{
    if (simulate && (a == StudyCard::Wrong || a == StudyCard::Retry))
        throw "Don't simulate in case of negative answer.";

    //postponed = false;
    StudyCard *card = fromId(cardid);

    QDate testday = ltDay(testdate);
    QDateTime oldtestdate = card->stats.size() >= 2 && testdate == card->testdate ? QDateTime(card->stats[card->stats.size() - 2].day, QTime(12, 01, 01), QTimeZone::utc()) : card->testdate;
    //qint64 oldinterval = card->interval;
    //uchar oldlevel = card->level;

    if (!simulate)
    {
        if (card->level >= 3)
            ZKanji::profile().removeMultiplier(card->multiplier);

        // First time a card is shown for the day.
        if (card->repeats == 0)
        {
            // Only reset card stats for the day here and not in startTestDay() because the
            // card might show up in the previous test day list.
            card->testlevel = card->level;
            memset(card->answers, 0, sizeof(uchar) * 4);
        }

        // The card's testlevel is set above, which is the only data that must be changed
        // before saving the card as undo data.
        createUndo(card, a, answertime);

        card->answers[(int)a] = std::min(255, card->answers[(int)a] + 1);

        card->itemdate = QDateTime::currentDateTimeUtc();

        timestats.addTime(card->testlevel, card->repeats, answertime / 100);
        if (a == StudyCard::Easy || a == StudyCard::Correct)
            timestats.addRepeat(card->testlevel, card->repeats);

        if (card->repeats != 254)
            ++card->repeats;

        if (card->repeats == 1)
        {
            // Add new empty stats that will be updated.
            card->stats.resize(card->stats.size() + 1);
            StudyCardStat &cardstat = card->stats[card->stats.size() - 1];
            cardstat.day = testday;
            cardstat.level = card->level;
            cardstat.multiplier = card->multiplier;
            cardstat.timespent = 0;
            //cardstat.status = /*card->problematic ? (int)StudyCardStatus::Problematic :*/ 0;

            daystats.cardTested(testday, answertime / 100, a == StudyCard::Wrong || a == StudyCard::Retry, card->testlevel == 0, card->learned, oldtestdate.secsTo(QDateTime(testday, QTime(12, 01, 01), QTimeZone::utc())) >= s_1_month * 2);
        }

        int idpos = posFromId(cardid);
        testcards.resize(std::remove(testcards.begin(), testcards.end(), idpos) - testcards.begin());
        testcards.push_back(idpos);
    }

    uchar repeats = card->repeats + (simulate && card->repeats != 254 ? 1 : 0);

    // New items have a testlevel of 0.
    if (card->testlevel == 0)
    {
        quint32 cardspacing = s_1_day;
        uchar cardlevel = 1;
        float cardmulti = card->multiplier;

        if (repeats != 1 && (a == StudyCard::Wrong || a == StudyCard::Retry))
            return cardspacing;

        if (a == StudyCard::Easy)
        {
            cardmulti = ZKanji::profile().easyMultiplier(cardmulti);
            cardspacing += s_1_day * 2;
        }

        //if (a == StudyCard::Wrong || a == StudyCard::Retry)
        //    cardinterval = Settings::study.delaywrong * ms_1_minute;
        //else
        fixCardSpacing(card, testdate, cardlevel, cardspacing);

        if (!simulate)
        {
            card->testdate = testdate;
            //card->inclusion = 1;
            card->level = 1;
            card->spacing = cardspacing;
            card->multiplier = cardmulti;
            card->nexttestdate = QDateTime();

            updateCardStat(card, /*a,*/ answertime / 100);

            //card->answercnt = 0;
            //owner->changeAnswerRatio(1, 0);

        }

        return cardspacing;
    }

    //if (!simulate && repeats == 1)
    //{
    //    ++card->inclusion;
    //    if (card->inclusion > 3)
    //        ++card->answercnt;
    //}

    if (a == StudyCard::Easy || a == StudyCard::Correct)
    {
        quint32 cardspacing = card->spacing;
        uchar cardlevel = card->level;
        float cardmulti = card->multiplier;

        // Card was previously been marked as incorrect or retried. All the values were set
        // already at previous answers.
        if (repeats != 1)
        {
            //cardspacing = ZKanji::profile().defaultInterval(cardlevel);
            //cardlevel = ZKanji::profile().levelFromInterval(ltDay(testdate), cardinterval);

            //fixCardInterval(card, testdate, cardinterval, cardlevel);

            if (!simulate)
            {
                //card->testdate = testdate;
                //card->spacing = cardspacing;
                card->nexttestdate = QDateTime();
                //card->level = cardlevel;
            }
            return cardspacing;
        }

        int seconds = oldtestdate.secsTo(testdate);
        // If the answer to this item was late, first increase its multiplier. If it was so
        // late to fit in a latter level, increase the level too.
        if (seconds > cardspacing * (2 - ZKanji::profile().acceptRate(cardlevel) / 1.035))
        {
            while (cardspacing * cardmulti < seconds)
            {
                cardspacing *= cardmulti;
                ++cardlevel;
            }
            cardspacing = seconds;
            cardmulti = ZKanji::profile().easyMultiplier(cardmulti);

            //cardlevel = ZKanji::profile().levelFromInterval(ltDay(oldtestdate), oldtestdate.secsTo(testdate));
            //cardinterval = ZKanji::profile().defaultInterval(cardlevel);
        }

        cardspacing *= cardmulti;
        if (a == StudyCard::Easy)
        {
            cardmulti = ZKanji::profile().easyMultiplier(cardmulti);
            cardspacing *= 1.3;
        }
        ++cardlevel;

        //if (!simulate && cardspacing >= s_1_month)
        //    card->problematic = false;

        //if (a == StudyCard::Easy)
        //    cardinterval *= ZKanji::profile().multiplier(cardlevel + 1);

        //cardlevel = ZKanji::profile().levelFromInterval(ltDay(testdate), cardinterval);

        fixCardSpacing(card, testdate, cardlevel, cardspacing);

        if (!simulate)
        {
            card->testdate = testdate;
            card->spacing = cardspacing;
            card->multiplier = cardmulti;
            card->nexttestdate = QDateTime();
            card->level = cardlevel;

            if (card->level >= 3)
                ZKanji::profile().addMultiplier(cardmulti);

            if (oldtestdate.secsTo(QDateTime(testday, QTime(12, 01, 01), QTimeZone::utc())) >= s_1_month * 2)
                card->learned = true;

            updateCardStat(card, /*a,*/ answertime / 100);

            //if (card->inclusion > 3)
            //    owner->changeAnswerRatio(1, 0);
        }

        return cardspacing;
    }

    //// Hard or retry. Only postponed is important when simulating.
    //if (repeats == 1)
    //{
    //    if (!simulate)
    //    {
    //        ++card->wrongcnt;
    //        if (card->inclusion > 3)
    //            owner->changeAnswerRatio(1, 1);
    //    }

    //    // When a card's answer-to-wrong-answer ratio becomes too bad compared to the average,
    //    // postpone it for the set days. It's not asked again in the same test. When the
    //    // student selected Retry but the card gets postponed, it's handled like the answer
    //    // was incorrect.
    //    if (/*a != StudyCard::Retry &&*/ Settings::study.problematic && card->answercnt > 3 && card->inclusion > 5 && ZKanji::profile().hasProblematicAnswers(card->answercnt, card->wrongcnt))
    //    {
    //        if (!simulate)
    //        {
    //            card->testdate = testdate;
    //            card->problematic = true;
    //            card->level = 1;
    //            card->interval = ms_1_day * Settings::study.postponedays;
    //            card->nexttestdate = QDateTime();

    //            // If the card becomes problematic, its data must be removed from
    //            // the student's answer-wrong-answer count, as well as it must be
    //            // set to 0.
    //            owner->changeAnswerRatio(-card->answercnt, -card->wrongcnt);
    //            card->answercnt = 0;
    //            card->wrongcnt = 0;

    //            if (a != StudyCard::Retry)
    //            {
    //                fixCardInterval(card, testdate, card->interval, 1);
    //                card->nexttestdate = QDateTime();
    //            }
    //        }

    //        postponed = true;

    //        return card->interval;
    //    }

    //    // When simulating, we don't care for the actual interval, only for postponed or not.
    //    if (simulate)
    //        return 0;

    //    // The test was taken too late. A retry counts as negative answer.
    //    if (card->testdate.msecsTo(testdate) > card->interval * (2 - ZKanji::profile().acceptRate(card->level) / 1.035))
    //        a = StudyCard::Wrong;
    //}

    // // When simulating, we don't care for the actual interval, only for postponed or not.
    //if (simulate)
    //    return 0;

    card->learned = false;
    card->testdate = testdate;
    if (a == StudyCard::Retry)
    {
        card->spacing /= card->multiplier;
        card->level = std::max(1, card->level - 1);

        if (card->spacing >= s_1_day * 2)
            card->multiplier = ZKanji::profile().retryMultiplier(card->multiplier);
    }
    if (a == StudyCard::Wrong)
    {
        // If the last answer wasn't "Wrong", decrease the spacing and level, otherwise reset
        // both of them.
        if (card->repeats == 1 /*&& (card->stats.size() == 1 || (card->stats[card->stats.size() - 2].status & (int)StudyCardStatus::Finished) != 0)*/)
        {
            card->spacing /= card->multiplier * card->multiplier;
            card->level = std::max(1, card->level - 2);
            while (card->level > 5)
            {
                card->spacing /= card->multiplier;
                --card->level;
            }
        }
        else
        {
            card->spacing = s_1_day;
            card->level = 1;
        }
        if (card->spacing >= s_1_day * 2)
            card->multiplier = ZKanji::profile().wrongMultiplier(card->multiplier);
    }

    if (card->level == 1 || card->spacing < s_1_day * 2)
    {
        card->level = 1;
        card->spacing = s_1_day;
    }

    fixCardSpacing(card, testdate, card->level, card->spacing);

    card->nexttestdate = QDateTime();

    updateCardStat(card, /*a,*/ answertime / 100);

    if (card->level >= 3)
        ZKanji::profile().addMultiplier(card->multiplier);

    //if (repeats != 1 && (a != StudyCard::Retry || repeats != 2))
    //{
    //    card->level = 1;
    //    return card->interval;
    //}

    //if (a == StudyCard::Retry && repeats == 1)
    //    card->level = std::max(1, card->level - 1);
    //else if (a == StudyCard::Wrong || repeats == 2 /* when Retried */)
    //    card->level = std::max(1, card->level - 2);

    //if (repeats == 1 && !card->problematic)
    //    ZKanji::profile().updateMultiplier(oldlevel, oldinterval, oldtestdate, ltDay(testdate), false);

    return card->spacing;
}

StudyCard::AnswerType StudyDeck::lastAnswer() const
{
    return undodata.lastanswer;
}

void StudyDeck::changeLastAnswer(StudyCard::AnswerType a)
{
    revertUndo();
    answer(ids[undodata.card->index] /*d.get()*/, a, undodata.answertime);
}

const CardId* StudyDeck::lastCard() const
{
    return ids[undodata.card->index]; /*d.get()*/;
}

const StudyCard* StudyDeck::fromId(const CardId *cardid) const
{
    int ix = posFromId(cardid);
    if (ix == -1)
        return nullptr;
    return list[ix];
}

StudyCard* StudyDeck::fromId(const CardId *cardid)
{
    int ix = posFromId(cardid);
    if (ix == -1)
        return nullptr;
    return list[ix];
}

int StudyDeck::posFromId(const CardId *cardid) const
{
    if (cardid == nullptr)
        return -1;

    return cardid->data;

    //auto listdata = list.data();
    //int ix = std::lower_bound(listdata, listdata + list.size(), cardid, [](const StudyCard *card, const CardId &id){
    //    return card->id < id;
    //}) - listdata;

    //if (ix == list.size())
    //    return -1;

    //const StudyCard *c = list[ix];
    //if (c->id != cardid)
    //    return -1;

    //return ix;
}

void StudyDeck::createUndo(StudyCard *card, StudyCard::AnswerType a, qint64 answertime)
{
    ZKanji::profile().createUndo();

    timestats.createUndo(card->testlevel);
    daystats.createUndo(ltDay(testdate));

    undodata.card = card;
    undodata.cardundo = *card;
    undodata.answertime = answertime;
    undodata.lastanswer = a;
}

void StudyDeck::revertUndo()
{
    ZKanji::profile().revertUndo();

    timestats.revertUndo(undodata.card->testlevel);
    daystats.revertUndo();

    *undodata.card = undodata.cardundo;
}

void StudyDeck::updateCardStat(StudyCard *card, /*StudyCard::AnswerType a,*/ int time)
{
    StudyCardStat &cardstat = card->stats[card->stats.size() - 1];
    //if (a == StudyCard::Correct || a == StudyCard::Easy)
    //    cardstat.status |= (int)StudyCardStatus::Finished;
    //if (card->problematic)
    //    cardstat.status |= (int)StudyCardStatus::Problematic;
    cardstat.level = card->level;
    cardstat.multiplier = card->multiplier;
    cardstat.timespent = std::min<ushort>(65535, cardstat.timespent + time);
}

void StudyDeck::fixCardSpacing(const StudyCard *card, QDateTime cardtestdate, uchar cardlevel, quint32 &cardspacing) const
{
    if (card->next == card || card->next == nullptr)
        return;

    // Determine the acceptable period when the card can be tested without
    // making it too easy or too difficult to remember.
    QDateTime duedate = cardtestdate.addSecs(cardspacing);
    QDate dueday = ltDay(duedate);

    // Acceptable difference in seconds +- from duedate.
    qint64 diff = std::max<quint32>(s_1_day, cardspacing * (1 - ZKanji::profile().acceptRate(cardlevel) / 1.035) / 1.8);

    QDateTime firstdate = duedate.addMSecs(-diff);
    QDateTime lastdate = duedate.addMSecs(diff);
    if (ltDay(firstdate) <= ltDay(testdate))
    {
        firstdate = testdate.addSecs(s_1_day);
        if (lastdate <= firstdate) // Should never happen, just for safety.
        {
            cardspacing = s_1_day;
            return;
        }
    }

    // List of acceptable dates when test might take place.
    std::vector<QDateTime> gooddates;
    gooddates.push_back(duedate);
    // List of periods when tests shouldn't take place if possible. The days of
    // the period limits are excluded.
    std::vector<std::pair<QDate, QDate>> badint;

    StudyCard *pos = card->next;

    // Fill the gooddates list with all the acceptable dates around other
    // items in the period of [firstdate, lastdate].
    while (pos && pos != card)
    {
        QDateTime posdue = pos->testdate.addSecs(pos->spacing);
        qint64 posdiff = pos->spacing * (1 - ZKanji::profile().acceptRate(pos->level) / 1.035) / 1.8;

        QDateTime posfirst = posdue.addMSecs(-posdiff);
        QDateTime poslast = posdue.addMSecs(+posdiff);
        if (ltDay(posfirst) == ltDay(posdue))
            posfirst = posfirst.addSecs(-(qint64)s_1_day);
        if (ltDay(poslast) == ltDay(posdue))
            poslast = poslast.addSecs(s_1_day);

        if (ltDay(posfirst) <= ltDay(lastdate) && ltDay(poslast) >= ltDay(firstdate))
        {
            // Overlap found.
            if (ltDay(posfirst) >= ltDay(firstdate))
                gooddates.push_back(posfirst);
            if (ltDay(poslast) <= ltDay(lastdate))
                gooddates.push_back(poslast);
        }
        badint.push_back(std::make_pair(ltDay(posfirst), ltDay(poslast)));
        
        pos = pos->next;
    }

    qint64 daysto = -1;
    QDateTime bestdate;

    // Remove dates that overlap with bad intervals from gooddates.
    // If anything remains, find the one that's closest to dueday.
    for (auto &p : badint)
    {
        auto it = gooddates.begin();
        while (it != gooddates.end())
        {
            QDate d = ltDay(*it);
            if (d > p.first && d < p.second)
            {
                it = gooddates.erase(it);
                continue;
            }
            
            if (daysto == -1 || d.daysTo(dueday) < daysto)
            {
                daysto = d.daysTo(dueday);
                bestdate = *it;
            }

            ++it;
        }
    }

    // We found a date when the card can be safely tested.
    if (daysto != -1)
    {
        cardspacing = cardtestdate.secsTo(bestdate);
        return;
    }

    // No date found. Try day +- 1 from the due day to see if anything
    // overlaps, and if not use that. Otherwise no changes are made.
    pos = card->next;
    bool pregood = true;
    bool nextgood = true;
    bool daygood = true;
    while (pos && pos != card && (pregood || nextgood))
    {
        QDate posdate = ltDay(pos->testdate.addSecs(pos->spacing));
        if (posdate == dueday.addDays(1))
            nextgood = false;
        if (posdate == dueday.addDays(-1))
            pregood = false;
        if (posdate == dueday)
            daygood = false;
        pos = pos->next;
    }

    if (daygood || (!nextgood && !pregood))
        return;

    if (nextgood)
        cardspacing += s_1_day;
    else
        cardspacing = std::max(s_1_day, cardspacing - s_1_day);
}


//-------------------------------------------------------------


StudyDeckList::StudyDeckList() : nextid(1) //: cardanswercnt(0), cardwrongcnt(0)
{
}

StudyDeckList::~StudyDeckList()
{
    clear();
}

void StudyDeckList::load(QDataStream &stream, int version)
{
    studyloadversion = version;

    //bool mod = ZKanji::profile().isModified();
    //ZKanji::profile().changeAnswerRatio(-cardanswercnt, -cardwrongcnt);

    qint32 i;
    //quint8 b;

    //stream >> i;
    //cardanswercnt = i;
    //stream >> i;
    //cardwrongcnt = i;

    clear();

    stream >> i;
    list.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        StudyDeckId id;
        stream >> id;
        list.push_back(new StudyDeck(this, id));
        list.back()->load(stream);
        mapping.insert({ id, list.back() });
        nextid = StudyDeckId::next(id, nextid);
    }

    //ZKanji::profile().changeAnswerRatio(cardanswercnt, cardwrongcnt);
    //ZKanji::profile().setModified(mod);
}

void StudyDeckList::save(QDataStream &stream) const
{
    //stream << (qint32)cardanswercnt;
    //stream << (qint32)cardwrongcnt;

    stream << (qint32)list.size();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        stream << list[ix]->deckId();
        list[ix]->save(stream);
    }
}

void StudyDeckList::clear()
{
    list.clear();
    nextid = StudyDeckId(1);
}

void StudyDeckList::fixStats()
{
    //changeAnswerRatio(-cardanswercnt, -cardwrongcnt);
    //cardanswercnt = 0;
    //cardwrongcnt = 0;
    for (auto deck : list)
        deck->fixDayStats();
}

StudyDeck* StudyDeckList::getDeck(StudyDeckId id)
{
    auto it = mapping.find(id);
    if (it != mapping.end())
        return it->second;
    //for (int ix = 0; ix != list.size(); ++ix)
    //    if (list[ix]->deckId() == id)
    //        return list[ix];
    return nullptr;
}

StudyDeckId StudyDeckList::createDeck()
{
    list.push_back(new StudyDeck(this, nextid));
    mapping.insert({ nextid, list.back() });
    nextid = StudyDeckId::next(nextid);
    return list.back()->deckId();
}

void StudyDeckList::removeDeck(StudyDeckId id)
{
    auto it = std::find_if(list.begin(), list.end(), [&id](StudyDeck *deck) { return deck->deckId() == id; });
    if (it != list.end())
        list.erase(it);
    mapping.erase(id);
}

StudyDeckList::size_type StudyDeckList::size() const
{
    return list.size();
}

StudyDeck* StudyDeckList::decks(int index)
{
    return list[index];
}

//void StudyDeckList::createUndo()
//{
//    //undodata.cardanswercnt = cardanswercnt;
//    //undodata.cardwrongcnt = cardwrongcnt;
//    
//    ZKanji::profile().createUndo();
//}
//
//void StudyDeckList::revertUndo()
//{
//    cardanswercnt = undodata.cardanswercnt;
//    cardwrongcnt = undodata.cardwrongcnt;
//
//    ZKanji::profile().revertUndo();
//}
//
//void StudyDeckList::changeAnswerRatio(ushort answercnt, ushort wrongcnt)
//{
//    cardanswercnt += answercnt;
//    cardwrongcnt += wrongcnt;
//
//    ZKanji::profile().changeAnswerRatio(answercnt, wrongcnt);
//}


//-------------------------------------------------------------


StudentProfile::StudentProfile() : mod(false), cardcount(0), multiaverage(2.5) //: modified(false), cardanswercnt(0), cardwrongcnt(0)
{
    //const double arate[12] = { 0.8, 0.83, 0.86, 0.89, 0.91, 0.92, 0.93, 0.933, 0.94, 0.95, 0.96, 0.961 };

    //QDate nowday = ltDay(QDateTime::currentDateTimeUtc());

    // We assume no more than 12 levels are ever reached for the items.
    //for (int ix = 0; ix != 12; ++ix)
    //{
    //    //multi[ix + 1].push_back(std::make_pair(nowday, 2.5));
    //    acceptrate[ix] = arate[ix];
    //}
}

void StudentProfile::load(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    char tmp[7];
    tmp[6] = 0;
    stream.readRawData(tmp, 6);
    if (strncmp(tmp, "zpf", 3))
        return;

    //int version = strtol(tmp + 3, nullptr, 10);

    qint32 i;
    stream >> i;
    cardcount = i;
    stream >> multiaverage;
//
//    memset(acceptrate, 0, sizeof(double) * 12);
//    multi.clear();
//
//    for (int ix = 0; ix != 12; ++ix)
//        stream >> acceptrate[ix];
//
//    int cnt;
//    qint32 i;
//    stream >> i;
//    cnt = i;
//
//    quint8 b;
//    double d;
//
//    for (int ix = 0; ix != cnt; ++ix)
//    {
//        stream >> b;
//        stream >> i;
//
//        std::vector<std::pair<QDate, double>> &mvec = multi[b];
//
//        for (int iy = 0; iy != i; ++iy)
//        {
//            std::pair<QDate, double> p;
//            stream >> make_zdate(p.first);
//            stream >> p.second;
//            mvec.push_back(p);
//        }
//    }
//    
    mod = false;
}

void StudentProfile::save(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly))
        return;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("zpf", 3);
    stream.writeRawData(ZKANJI_PROFILE_FILE_VERSION, 3);

    stream << (qint32)cardcount;
    stream << multiaverage;
//
//    for (int ix = 0; ix != 12; ++ix)
//        stream << acceptrate[ix];
//
//    stream << multi.size();
//    for (const auto &p : multi)
//    {
//        stream << (quint8)p.first;
//        const std::vector<std::pair<QDate, double>> &mvec = p.second;
//        stream << (qint32)mvec.size();
//        for (int ix = 0; ix != mvec.size(); ++ix)
//        {
//            stream << make_zdate(mvec[ix].first);
//            stream << mvec[ix].second;
//        }
//    }
//
    mod = false;
}

void StudentProfile::clear()
{
    if (cardcount == 0)
        return;
    cardcount = 0;
    multiaverage = 2.5;
    mod = true;
}

float StudentProfile::baseMultiplier() const
{
    if (cardcount < 10)
        return 2.5;
    return multiaverage;
}

void StudentProfile::removeMultiplier(float mul)
{
    multiaverage = cardcount > 1 ? ((multiaverage * cardcount) - (double)mul) / double(cardcount - 1) : 2.5;
    --cardcount;

    if (cardcount < 0)
    {
        cardcount = 0;
        multiaverage = 2.5;
    }

    mod = true;
}

void StudentProfile::addMultiplier(float mul)
{
    multiaverage = ((multiaverage * cardcount) + (double)mul) / double(cardcount + 1);
    ++cardcount;

    mod = true;
}

float StudentProfile::easyMultiplier(float mul) const
{
    // When changed: update numbers in StudyDeck::loadLegacy().
    return mul + 0.15;
}

float StudentProfile::wrongMultiplier(float mul) const
{
    // When changed: update numbers in StudyDeck::loadLegacy().
    return std::max(1.3, mul - 0.32);
}

float StudentProfile::retryMultiplier(float mul) const
{
    // When changed: update numbers in StudyDeck::loadLegacy().
    return std::max(1.3, mul - 0.14);
}

bool StudentProfile::isModified() const
{
    return mod;
}

//void StudentProfile::setModified(bool mod)
//{
//    modified = mod;
//}

void StudentProfile::createUndo()
{
    //undodata.cardanswercnt = cardanswercnt;
    //undodata.cardwrongcnt = cardwrongcnt;
    //undodata.multi.clear();
    //for (auto p : multi)
    //    undodata.multi.insert({ p.first, { p.second.back().second, uint(p.second.size()) } });
    undodata.cardcount = cardcount;
    undodata.multiaverage = multiaverage;
}

void StudentProfile::revertUndo()
{
    cardcount = undodata.cardcount;
    multiaverage = undodata.multiaverage;

    mod = true;
    //cardanswercnt = undodata.cardanswercnt;
    //cardwrongcnt = undodata.cardwrongcnt;

    //// A copy of multi is used so when multi is reverted, it won't have data for levels it
    //// didn't have before the undo.
    //std::map<uchar, std::vector<std::pair<QDate, double>>> multicpy;
    //std::swap(multi, multicpy);
    //for (auto p : undodata.multi)
    //{
    //    auto it = multicpy.find(p.first);
    //    it->second.resize(p.second.second);
    //    it->second.back().second = p.second.first;
    //    multi.insert({ p.first, it->second });
    //}
}

double StudentProfile::acceptRate(uchar level)
{
    const double arate[12] = { 0.8, 0.83, 0.86, 0.89, 0.91, 0.92, 0.93, 0.933, 0.94, 0.95, 0.96, 0.961 };
    if (level < 1)
        level = 1;
    if (level > 12)
        level = 12;
    return arate[level - 1];
}

//uchar StudentProfile::levelFromInterval(QDate testdate, qint64 interval)
//{
//    qint64 i = ms_1_day * 0.99 * multiplierOn(testdate, 1);
//    int level = 0;
//
//    while (interval >= i)
//    {
//        ++level;
//        i *= multiplierOn(testdate, level + 1);
//    }
//
//    return level;
//}
//
//qint64 StudentProfile::defaultInterval(uchar level)
//{
//    qint64 interval = ms_1_day;
//    for (int ix = 0; ix != level; ++ix)
//        interval *= multiplier(ix + 1);
//    return interval;
//}
//
//double StudentProfile::multiplierOn(QDate date, uchar level)
//{
//    std::vector<std::pair<QDate, double>> &mlist = multi[level];
//    double m = mlist.front().second;
//
//    for (auto &p : mlist)
//    {
//        if (p.first > date)
//            break;
//        m = p.second;
//    }
//
//    return m;
//}
//
//double StudentProfile::multiplier(uchar level)
//{
//    return multi[level].back().second;
//}
//
//void StudentProfile::updateMultiplier(uchar level, qint64 interval, QDateTime cardtestdate, QDate testdate, bool correct)
//{
//    modified = true;
//
//    double rate = 1 - acceptrate[level - 1];
//
//    qint64 realinterval = cardtestdate.msecsTo(QDateTime(testdate));
//
//    // Lot longer time passed than the acceptable interval. We assume this
//    // means the tested item was harder to remember, so the acceptable
//    // forgetting rate for this one time is increased.
//    if (realinterval > interval * (1 + rate))
//    {
//        // Too long time passed. Any answer is meaningless and shouldn't
//        // influence student data.
//        if (realinterval - interval * (1 + rate) > interval * rate * 2)
//            return;
//        rate *= (double)realinterval / (interval * (1 + rate));
//    }
//
//    // We assume the past 99 answers exactly match the acceptable
//    // success/fail ratio.
//    double goodanswer = 99. * (1. - rate);
//    double badanswer = 99 - goodanswer;
//
//    // Add the new answer as the 100th.
//    if (correct)
//        ++goodanswer;
//    else
//        ++badanswer;
//
//    // Because intervals are determined at the time of testing, the multipliers
//    // used are also from that time, not the latest one.
//    double m = multiplierOn(ltDay(cardtestdate), level);
//    m = m - m * ((badanswer / 100. - rate) * 2);
//    m = (multi[level].back().second + m) / 2;
//    if (multi[level].back().first < testdate)
//        multi[level].push_back(std::make_pair(testdate, m));
//    else
//        multi[level].back().second = m;
//}
//
//void StudentProfile::changeAnswerRatio(ushort answercnt, ushort wrongcnt)
//{
//    modified = true;
//
//    cardanswercnt += answercnt;
//    cardwrongcnt += wrongcnt;
//}
//
//bool StudentProfile::hasProblematicAnswers(ushort answercnt, ushort wrongcnt)
//{
//    if (answercnt == 0 || cardanswercnt == 0)
//        throw "Programmer error. Items with no answers should never reach here.";
//
//    if (cardanswercnt - cardwrongcnt < 20)
//        return false;
//
//    double ratio = (double)wrongcnt / answercnt;
//    double cardratio = (double)cardwrongcnt / cardanswercnt;
//
//    bool problematic = ratio > (cardratio + 1) / 2;
//
//    return problematic;
//}



//-------------------------------------------------------------


namespace ZKanji
{
    static StudentProfile _zkzk_profile;
    StudentProfile& profile()
    {
        return _zkzk_profile;
    }
}


//-------------------------------------------------------------

