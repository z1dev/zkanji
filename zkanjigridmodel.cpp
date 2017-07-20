/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMimeData>
#include "zkanjigridmodel.h"

#include "zkanjimain.h"
#include "words.h"
#include "groups.h"
#include "kanji.h"
#include "ranges.h"
#include "colorsettings.h"

//-------------------------------------------------------------


KanjiGridModel::KanjiGridModel(QObject *parent) : base(parent), permapindex(-1)
{

}

KanjiGridModel::~KanjiGridModel()
{

}

bool KanjiGridModel::empty() const
{
    return size() == 0;
}

KanjiGroup* KanjiGridModel::kanjiGroup() const
{
    return nullptr;
}

QMimeData* KanjiGridModel::mimeData(const std::vector<int> &indexes) const
{
    // Format of zkanji/kanji mime data:
    // Pointer to dictionary. intptr_t
    // Pointer to kanji group. intptr_t
    // Array of kanji indexes. There's no count just the values. ushort each.

    QMimeData *dat = new QMimeData();
    QByteArray arr;
    arr.resize(indexes.size() * sizeof(ushort) + sizeof(intptr_t) * 2);

    Dictionary *dict = kanjiGroup() == nullptr ? nullptr : kanjiGroup()->dictionary();
    *(intptr_t*)arr.data() = (intptr_t)dict;
    *((intptr_t*)arr.data() + 1) = (intptr_t)kanjiGroup();
    ushort *arrdat = (ushort*)(arr.data() + sizeof(intptr_t) * 2);

    for (int ix : indexes)
    {
        *arrdat = kanjiAt(ix);
        ++arrdat;
    }

    dat->setData("zkanji/kanji", arr);

    return dat;
}

bool KanjiGridModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const
{
    return false;
}

bool KanjiGridModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int index)
{
    return false;
}

Qt::DropActions KanjiGridModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions KanjiGridModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    return 0;
}

int KanjiGridModel::savePersistentList(std::vector<int> &indexes)
{
    ++permapindex;

    persistents[permapindex] = indexes;
    return permapindex;
}

void KanjiGridModel::restorePersistentList(int id, std::vector<int> &indexes)
{
    indexes = persistents[id];
    persistents.erase(id);

    if (persistents.empty())
        permapindex = -1;
}

void KanjiGridModel::getPersistentIndexes(std::vector<int> &result)
{
    for (const auto &perpair : persistents)
        result.insert(result.end(), perpair.second.begin(), perpair.second.end());
    std::sort(result.begin(), result.end());
    result.resize(std::unique(result.begin(), result.end()) - result.begin());
}

void KanjiGridModel::changePersistentLists(const std::vector<int> &from, const std::vector<int> &to)
{
    int fsiz = std::min(from.size(), to.size());
    for (auto &perpair : persistents)
    {
        std::vector<int> &list = perpair.second;
        std::vector<int> order;
        order.reserve(list.size());
        for (int ix = 0, siz = list.size(); ix != siz; ++ix)
            order.push_back(ix);
        std::sort(order.begin(), order.end(), [&list](int a, int b) { return list[a] < list[b]; });

        int fpos = 0;
        int opos = 0;
        int osiz = order.size();

        while (fpos != fsiz && opos != osiz)
        {
            while (fpos != fsiz && from[fpos] < list[order[opos]])
                ++fpos;
            if (fpos == fsiz)
                break;
            while (opos != osiz && from[fpos] > list[order[opos]])
                ++opos;

            while (opos != osiz && from[fpos] == list[order[opos]])
            {
                list[order[opos]] = to[fpos];
                ++opos;
            }
        }
    }
}

void KanjiGridModel::signalModelReset()
{
    emit modelReset();
}

void KanjiGridModel::signalItemsInserted(const smartvector<Interval> &intervals)
{
    emit itemsInserted(intervals);
}

void KanjiGridModel::signalItemsRemoved(const smartvector<Range> &ranges)
{
    emit itemsRemoved(ranges);
}

void KanjiGridModel::signalItemsMoved(const smartvector<Range> &ranges, int pos)
{
    emit itemsMoved(ranges, pos);
}

void KanjiGridModel::signalLayoutToChange()
{
    emit layoutToChange();
}

void KanjiGridModel::signalLayoutChanged()
{
    emit layoutChanged();
}



//-------------------------------------------------------------

MainKanjiListModel::MainKanjiListModel(QObject *parent) : base(parent)
{
    ;
}

MainKanjiListModel::~MainKanjiListModel()
{

}

int MainKanjiListModel::size() const
{
    return ZKanji::kanjicount;
}

// Returns the index of the kanji in the main kanji list at pos position.
ushort MainKanjiListModel::kanjiAt(int pos) const
{
    if (pos < 0 || pos >= size())
        return -1;
    return pos;
}


//-------------------------------------------------------------


KanjiListModel::KanjiListModel(QObject *parent) : base(parent), candrop(false)
{

}

KanjiListModel::~KanjiListModel()
{

}

void KanjiListModel::setList(const std::vector<ushort> &newlist)
{
    list = newlist;
    signalModelReset();
}

void KanjiListModel::setList(std::vector<ushort> &&newlist)
{
    list.swap(std::vector<ushort>());
    list.swap(newlist);
    signalModelReset();
}

const std::vector<ushort>& KanjiListModel::getList() const
{
    return list;
}

bool KanjiListModel::acceptDrop()
{
    return candrop;
}

void KanjiListModel::setAcceptDrop(bool acceptdrop)
{
    candrop = acceptdrop;
}

int KanjiListModel::size() const
{
    return list.size();
}

ushort KanjiListModel::kanjiAt(int pos) const
{
    return list[pos];
}

bool KanjiListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const
{
    return candrop && index == -1 && data->hasFormat("zkanji/kanji") && (((data->data("zkanji/kanji")).size() - sizeof(intptr_t) * 2) % sizeof(ushort)) == 0;
}

bool KanjiListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int index)
{
    if (!canDropMimeData(data, action, index))
        return false;

    // TODO: (later) when implementing dropping simple text, always test if the copied data has
    // duplicate kanji, and remove them before anything else is done.

    QByteArray arr = data->data("zkanji/kanji");
    const ushort *dat = (const ushort*)(arr.constData() + sizeof(intptr_t) * 2);
    int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(ushort);

    if (cnt * sizeof(ushort) != arr.size() - sizeof(intptr_t) * 2 || cnt == 0)
        return false;

    // Check which kanji are already in list and only add the rest.

    std::vector<int> lorder;
    lorder.reserve(list.size());
    while (lorder.size() != list.size())
        lorder.push_back(lorder.size());

    std::vector<int> aorder;
    aorder.reserve(cnt);
    while (aorder.size() != cnt)
        aorder.push_back(aorder.size());

    std::sort(lorder.begin(), lorder.end(), [this](int a, int b) { return list[a] < list[b]; });
    std::sort(aorder.begin(), aorder.end(), [dat](int a, int b) { return dat[a] < dat[b]; });

    int skipped = 0;
    for (int lsiz = lorder.size(), lpos = 0, asiz = aorder.size(), apos = 0; lpos != lsiz && apos != asiz;)
    {
        while (lpos != lsiz && list[lorder[lpos]] < dat[aorder[apos]])
            ++lpos;
        if (lpos == lsiz)
            break;
        while (apos != asiz && list[lorder[lpos]] > dat[aorder[apos]])
            ++apos;
        if (apos == asiz)
            break;

        while (lpos != lsiz && apos != asiz && list[lorder[lpos]] == dat[aorder[apos]])
        {
            aorder[apos] = -1;
            ++skipped;
            ++lpos;
            ++apos;
        }
    }

    if (skipped == aorder.size())
        return true;

    list.reserve(list.size() + aorder.size() - skipped);
    std::sort(aorder.begin(), aorder.end(), [](int a, int b) {
        return a != -1 && (b == -1 || a < b);
    });


    for (int ix = 0, siz = aorder.size(); ix != siz && aorder[ix] != -1; ++ix)
        list.push_back(dat[aorder[ix]]);

    smartvector<Interval> intervals;
    intervals.push_back({ (int)list.size() - (int)(aorder.size() - skipped), (int)aorder.size() - skipped });
    signalItemsInserted(intervals);

    return true;
}

Qt::DropActions KanjiListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (candrop && mime->hasFormat("zkanji/kanji") && (((mime->data("zkanji/kanji")).size() - sizeof(intptr_t) * 2) % sizeof(ushort)) == 0 && !samesource)
        return Qt::CopyAction | Qt::MoveAction;
    return 0;
}



//-------------------------------------------------------------


KanjiGroupModel::KanjiGroupModel(QObject *parent) : base(parent), group(nullptr)
{

}

KanjiGroupModel::~KanjiGroupModel()
{

}

void KanjiGroupModel::setKanjiGroup(KanjiGroup *g)
{
    if (g == group)
        return;

    if (group != nullptr)
        disconnect(group, nullptr, this, nullptr);

    if (g != nullptr)
    {
        connect(&g->dictionary()->kanjiGroups(), &GroupCategoryBase::itemsInserted, this, &KanjiGroupModel::itemsInserted);
        connect(&g->dictionary()->kanjiGroups(), &GroupCategoryBase::itemsRemoved, this, &KanjiGroupModel::itemsRemoved);
        connect(&g->dictionary()->kanjiGroups(), &GroupCategoryBase::itemsMoved, this, &KanjiGroupModel::itemsMoved);
    }

    group = g;
    signalModelReset();
}

KanjiGroup* KanjiGroupModel::kanjiGroup() const
{
    return group;
}

int KanjiGroupModel::size() const
{
    return group->size();
}

ushort KanjiGroupModel::kanjiAt(int pos) const
{
    return group->getIndexes()[pos];
}

bool KanjiGroupModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const
{
    return group != nullptr && data->hasFormat("zkanji/kanji");
}

bool KanjiGroupModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int index)
{
    if (!canDropMimeData(data, action, index))
        return false;

    // Extract the indexes from the mime data.

    QByteArray arr = data->data("zkanji/kanji");
    ushort *dat = (ushort*)(arr.constData() + sizeof(intptr_t) * 2);
    int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(ushort);

    if (cnt * sizeof(ushort) != arr.size() - sizeof(intptr_t) * 2 || cnt == 0)
        return false;

    std::vector<ushort> kindexes;
    kindexes.reserve(cnt);

    while (kindexes.size() != cnt)
    {
        kindexes.push_back(*dat);
        ++dat;
    }

    if (index == -1)
        index = size();

    if (group == (KanjiGroup*)*((intptr_t*)arr.constData() + 1))
    {
        smartvector<Range> ranges;

        std::vector<int> positions;
        group->indexOf(kindexes, positions);

        for (int pos = 0, next = 1, siz = positions.size(); pos != siz; ++next)
        {
            if (next == siz || positions[next] - positions[next - 1] != 1)
            {
                ranges.push_back({ positions[pos], positions[next - 1] });
                pos = next;
            }
        }

        group->move(ranges, index);
    }
    else
        group->insert(kindexes, index);

    return true;
}

Qt::DropActions KanjiGroupModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions KanjiGroupModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    QByteArray arr;
    if (mime->hasFormat("zkanji/kanji") &&
        (((arr = mime->data("zkanji/kanji")).size() - sizeof(intptr_t) * 2) % sizeof(ushort)) == 0 &&
        (*(intptr_t*)arr.constData() == 0 || *(intptr_t*)arr.constData() == (intptr_t)group->dictionary()) &&
        (*((intptr_t*)arr.constData() + 1) == 0 || samesource || (intptr_t)group != *((intptr_t*)arr.constData() + 1)))
        return Qt::CopyAction | Qt::MoveAction;
    return 0;
}

void KanjiGroupModel::itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals)
{
    if (group != parent)
        return;

    signalItemsInserted(intervals);
}

void KanjiGroupModel::itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges)
{
    if (group != parent)
        return;
    signalItemsRemoved(ranges);
}

void KanjiGroupModel::itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos)
{
    if (group != parent)
        return;

    signalItemsMoved(ranges, pos);
}


//-------------------------------------------------------------


KanjiGridSortModel::KanjiGridSortModel(KanjiGridModel *basemodel, KanjiGridSortOrder order, Dictionary *dict, QObject *parent) : base(parent), basemodel(basemodel), order(KanjiGridSortOrder::NoSort), sortcount(-1)
{
    list.reserve(basemodel->size());
    for (int ix = 0; ix != basemodel->size(); ++ix)
        list.push_back(ix);

    sort(order, dict);
}

KanjiGridSortModel::KanjiGridSortModel(KanjiGridModel *basemodel, const std::vector<ushort> &filter, KanjiGridSortOrder order, Dictionary *dict, QObject *parent) : base(parent), basemodel(basemodel), order(KanjiGridSortOrder::NoSort)
{
    list = filter;
    sort(order, dict);
}

KanjiGridSortModel::KanjiGridSortModel(KanjiGridModel *basemodel, std::vector<ushort> &&filter, KanjiGridSortOrder order, Dictionary *dict, QObject *parent) : base(parent), basemodel(basemodel), order(KanjiGridSortOrder::NoSort)
{
    std::swap(list, filter);
    sort(order, dict);
}

KanjiGridSortModel::~KanjiGridSortModel()
{

}

void KanjiGridSortModel::sort(KanjiGridSortOrder sorder, Dictionary *dict)
{
    if (order == sorder)
        return;
    order = sorder;
    sortcount = -1;

    signalLayoutToChange();

    std::vector<int> perix;
    getPersistentIndexes(perix);

    std::vector<int> newix = perix;
    for (int ix = 0, siz = newix.size(); ix != siz; ++ix)
        newix[ix] = list[newix[ix]];

    switch (order)
    {
    case KanjiGridSortOrder::Jouyou:
    {
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) -> bool {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->jouyou;
            int valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jlpt;
            valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            vala = ka->word_freq;
            valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            if (vala != valb)
                return vala > valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            return ka->ch.unicode() < kb->ch.unicode();
        });

        auto it = std::lower_bound(list.begin(), list.end(), 0, [this](int a, int b) {
            a = ZKanji::kanjis[basemodel->kanjiAt(a)]->jouyou;
            return a != 0;
        });
        if (it != list.end())
            sortcount = it - list.begin();
        break;
    }
    case KanjiGridSortOrder::JLPT:
    {
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->jlpt;
            int valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jouyou;
            valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            vala = ka->word_freq;
            valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            if (vala != valb)
                return vala > valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });

        auto it = std::lower_bound(list.begin(), list.end(), 0, [this](int a, int b) {
            a = ZKanji::kanjis[basemodel->kanjiAt(a)]->jlpt;
            return a != 0;
        });
        if (it != list.end())
            sortcount = it - list.begin();
        break;
    }
    case KanjiGridSortOrder::Freq:
    {
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->frequency;
            int valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jouyou;
            valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jlpt;
            valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            vala = ka->word_freq;
            valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            if (vala != valb)
                return vala > valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });
        auto it = std::lower_bound(list.begin(), list.end(), 0, [this](int a, int b) {
            a = ZKanji::kanjis[basemodel->kanjiAt(a)]->frequency;
            return a != 0;
        });
        if (it != list.end())
            sortcount = it - list.begin();
        break;
    }
    case KanjiGridSortOrder::Words:
    {
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = dict->kanjiWordCount(basemodel->kanjiAt(a));
            int valb = dict->kanjiWordCount(basemodel->kanjiAt(b));
            if (vala != valb)
                return vala > valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });
        auto it = std::lower_bound(list.begin(), list.end(), 0, [this, dict](int a, int b) {
            a = dict->kanjiWordCount(basemodel->kanjiAt(a));
            return a != 0;
        });
        if (it != list.end())
            sortcount = it - list.begin();
        break;
    }
    case KanjiGridSortOrder::WordFreq:
    {
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->word_freq;
            int valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a));
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b));
            if (vala != valb)
                return vala > valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jouyou;
            valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jlpt;
            valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            //vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            //valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            //if (vala != valb)
            //    return vala > valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });
        auto it = std::lower_bound(list.begin(), list.end(), 0, [this](int a, int b) {
            a = ZKanji::kanjis[basemodel->kanjiAt(a)]->word_freq;
            return a != 0;
        });
        if (it != list.end())
            sortcount = it - list.begin();
        break;
    }
    case KanjiGridSortOrder::Stroke:
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->strokes;
            int valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jouyou;
            valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jlpt;
            valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->rad;
            valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->word_freq;
            valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            if (vala != valb)
                return vala > valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });
        break;
    case KanjiGridSortOrder::Rad:
        std::sort(list.begin(), list.end(), [this, dict](int a, int b) {
            KanjiEntry *ka = ZKanji::kanjis[basemodel->kanjiAt(a)];
            KanjiEntry *kb = ZKanji::kanjis[basemodel->kanjiAt(b)];
            int vala = ka->rad;
            int valb = kb->rad;
            if (vala != valb)
                return vala < valb;
            vala = ka->frequency;
            valb = kb->frequency;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jouyou;
            valb = kb->jouyou;
            if (vala != valb)
                return !valb || (vala && vala < valb);
            vala = ka->jlpt;
            valb = kb->jlpt;
            if (vala != valb)
                return vala > valb;
            vala = ka->strokes;
            valb = kb->strokes;
            if (vala != valb)
                return vala < valb;
            vala = ka->word_freq;
            valb = kb->word_freq;
            if (vala != valb)
                return vala > valb;
            vala = dict->kanjiWordCount(basemodel->kanjiAt(a))/*->words.size()*/;
            valb = dict->kanjiWordCount(basemodel->kanjiAt(b))/*->words.size()*/;
            if (vala != valb)
                return vala > valb;
            return ka->ch.unicode() < kb->ch.unicode();
        });
        break;
    case KanjiGridSortOrder::Unicode:
        std::sort(list.begin(), list.end(), [this](int a, int b) {
            return ZKanji::kanjis[basemodel->kanjiAt(a)]->ch.unicode() < ZKanji::kanjis[basemodel->kanjiAt(b)]->ch.unicode();
        });
        break;
    case KanjiGridSortOrder::JISX0208:
        std::sort(list.begin(), list.end(), [this](int a, int b) {
            return ZKanji::kanjis[basemodel->kanjiAt(a)]->jis < ZKanji::kanjis[basemodel->kanjiAt(b)]->jis;
        });
        break;
    default: // Original order.
        std::sort(list.begin(), list.end());
    }

    std::vector<int> lorder;
    lorder.reserve(list.size());
    while (lorder.size() != list.size())
        lorder.push_back(lorder.size());
    std::sort(lorder.begin(), lorder.end(), [this](int a, int b) { return list[a] < list[b]; });

    std::vector<int> xorder;
    xorder.reserve(newix.size());
    while (xorder.size() != newix.size())
        xorder.push_back(xorder.size());
    std::sort(xorder.begin(), xorder.end(), [&newix](int a, int b) { return newix[a] < newix[b]; });

    int lsiz = lorder.size();
    int xsiz = xorder.size();
    int lpos = 0;
    int xpos = 0;
    while (lpos != lsiz && xpos != xsiz)
    {
        while (lpos != lsiz && list[lorder[lpos]] < newix[xorder[xpos]])
            ++lpos;
        if (lpos == lsiz)
            break;

#ifdef _DEBUG
        if (list[lorder[lpos]] > newix[xorder[xpos]])
            throw "DEBUG error. newix must only hold items found in list.";
#endif

        while (lpos != lsiz && xpos != xsiz && list[lorder[lpos]] == newix[xorder[xpos]])
        {
            newix[xorder[xpos]] = lorder[lpos];
            ++xpos;
            ++lpos;
        }

    }

    changePersistentLists(perix, newix);

    signalLayoutChanged();
}

//KanjiGridModel* KanjiGridSortModel::baseModel()
//{
//    return basemodel;
//}

int KanjiGridSortModel::size() const
{
    return list.size();
}

ushort KanjiGridSortModel::kanjiAt(int pos) const
{
    return basemodel->kanjiAt(list[pos]);
}

QColor KanjiGridSortModel::textColorAt(int pos) const
{
    return basemodel->textColorAt(list[pos]);
}

QColor KanjiGridSortModel::backColorAt(int pos) const
{
    if (sortcount == -1 || pos < sortcount)
        return QColor();
    return Settings::colors.kanjiunsorted.isValid() ? Settings::colors.kanjiunsorted : Settings::colors.unsorteddef;
}

KanjiGroup* KanjiGridSortModel::kanjiGroup() const
{
    return basemodel->kanjiGroup();
}


//-------------------------------------------------------------
