/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include "wordtestsettingsform.h"
#include "ui_wordtestsettingsform.h"
#include "zkanjimain.h"
#include "zlistview.h"
#include "groups.h"
#include "words.h"
#include "wordstudyform.h"
#include "ranges.h"
#include "zui.h"
#include "colorsettings.h"
#include "romajizer.h"
#include "generalsettings.h"
#include "globalui.h"

#include "checked_cast.h"

//-------------------------------------------------------------


TestWordsItemModel::TestWordsItemModel(WordGroup *group, QObject *parent) : base(parent), group(group), display(TestWordsDisplay::All), showexcluded(true)
{
    connect();
    insertColumn(0, { (int)TestWordsColumnTypes::Order, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(30), tr("#") });
    setColumn(1, { (int)TestWordsColumnTypes::Score, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(50), tr("Score") });

    items = group->studyData().indexes();
    if (items.empty())
        group->studyData().correctedIndexes(items);
    reset();
}

TestWordsItemModel::~TestWordsItemModel()
{

}

void TestWordsItemModel::refresh()
{
    reset(true);
}

TestWordsDisplay TestWordsItemModel::displayed() const
{
    return display;
}

void TestWordsItemModel::setDisplayed(TestWordsDisplay disp)
{
    if (display == disp)
        return;

    display = disp;
    reset();
}

void TestWordsItemModel::reset(bool dorefresh)
{
    beginResetModel();
    
    if (dorefresh)
        group->studyData().correctedIndexes(items);

    list.clear();

    if (display == TestWordsDisplay::All)
    {
        for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
            if (showexcluded || !items[ix].excluded)
                list.push_back(ix);
    }
    else
    {
        std::vector<int> order(items.size(), 0);
        for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
            order[ix] = ix;

        auto func = [&order, this](const std::function<const QChar* (int windex)> &wfunc) {
            std::sort(order.begin(), order.end(), [&wfunc, this](int a, int b) {
                if (a == b)
                    return false;
                if (!showexcluded)
                {
                    if (items[a].excluded)
                    {
                        if (items[b].excluded)
                            return a < b;
                        return false;
                    }
                    else if (items[b].excluded)
                        return true;
                }
                int cmp = qcharcmp(wfunc(items[a].windex), wfunc(items[b].windex));
                if (cmp == 0)
                    return a < b;
                return cmp < 0;
            });

            while (!showexcluded && !order.empty() && items[order.back()].excluded)
                order.pop_back();

            // Throw out unique words.
            for (int ix = tosigned(order.size()) - 1; ix != -1; --ix)
            {
                int wix = items[order[ix]].windex; 
                int iy = ix;
                while (iy != 0 && qcharcmp(wfunc(wix), wfunc(items[order[iy - 1]].windex)) == 0)
                    --iy;
                if (iy == ix)
                    order.erase(order.begin() + ix);
                else
                    ix = iy;
            }
        };

        if (display == TestWordsDisplay::WrittenConflict)
        {
            func([this](int windex) -> const QChar* {
                return group->dictionary()->wordEntry(windex)->kanji.data();
            });
        }
        else if (display == TestWordsDisplay::KanaConflict)
        {
            func([this](int windex) -> const QChar* {
                return group->dictionary()->wordEntry(windex)->kana.data();
            });
        }
        else if (display == TestWordsDisplay::DefinitionConflict)
        {
            std::map<int, QString> cache;
            func([this, &cache](int windex) -> const QChar*{
                return meaningString(windex, cache).constData();
            });
        }

        list = order;
    }

    endResetModel();

}

bool TestWordsItemModel::showExcluded() const
{
    return showexcluded;
}

void TestWordsItemModel::setShowExcluded(bool shown)
{
    if (showexcluded == shown)
        return;

    showexcluded = shown;

    if (!showexcluded)
    {
        // Remove excluded items and signal the view.

        std::vector<int> ex;
        std::map<int, QString> cache;

        for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
            if (items[list[ix]].excluded)
            {
                int pre = 0;
                int suf = 0;

                int first = -1;
                int last = -1;
                if (display != TestWordsDisplay::All)
                {
                    // Check surrounding items in case they must be removed as well.

                    int windex = items[list[ix]].windex;

                    for (int iy = ix - 1, end = ex.empty() ? -1 : ex.back(); iy != end; --iy)
                    {
                        if (!comparedEqual(display, windex, items[list[iy]].windex, cache))
                            break;
                        if (!items[list[iy]].excluded)
                            ++pre;
                        first = iy;
                    }
                    for (int iy = ix + 1, end = tosigned(list.size()); iy != end; ++iy)
                    {
                        if (!comparedEqual(display, windex, items[list[iy]].windex, cache))
                            break;
                        if (!items[list[iy]].excluded)
                            ++suf;
                        last = iy;
                    }

                    if (first == -1)
                        first = ix;
                    if (last == -1)
                        last = ix;

                    // Only one conflicting word was found, which will be hidden as well.
                    if (pre + suf == 1)
                        for (int iy = first; iy != ix; ++iy)
                            ex.push_back(iy);

                }
                ex.push_back(ix);

                // Only one conflicting word was found, which will be hidden as well.
                if (pre + suf == 1)
                {
                    for (int iy = ix + 1; iy != last + 1; ++iy)
                        ex.push_back(iy);
                    if (last != -1)
                        ix = last;
                }
            }

        if (ex.empty())
            return;

        smartvector<Range> ranges;
        _rangeFromIndexes(ex, ranges);

        for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
            list.erase(list.begin() + ranges[ix]->first, list.begin() + ranges[ix]->last + 1);
        signalRowsRemoved(ranges);
        return;
    }

    smartvector<Interval> intervals;

    if (display == TestWordsDisplay::All)
    {
        for (int lpos = tosigned(list.size()) - 1, pos = tosigned(items.size()) - 1, prev = pos; prev != -1;)
        {
            while (prev != -1 && !items[prev].excluded)
            {
                --lpos;
                --prev;
            }
            pos = prev;
            if (pos == -1)
                break;
            while (prev != -1 && items[prev].excluded)
            {
                list.insert(list.begin() + (lpos + 1), prev);
                --prev;
            }

            intervals.insert(intervals.begin(), { lpos + 1, pos - prev });
        }
    }
    else
    {
        std::vector<int> order(items.size(), 0);
        for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
            order[ix] = ix;

        auto func = [&order, this](const std::function<const QChar* (int windex)> &wfunc) {
            std::sort(order.begin(), order.end(), [&wfunc, this](int a, int b) {
                if (a == b)
                    return false;
                int cmp = qcharcmp(wfunc(items[a].windex), wfunc(items[b].windex));
                if (cmp == 0)
                    return a < b;
                return cmp < 0;
            });

            // Throw out unique words.
            for (int ix = tosigned(order.size()) - 1; ix != -1; --ix)
            {
                int wix = items[order[ix]].windex;
                int iy = ix;
                while (iy != 0 && qcharcmp(wfunc(wix), wfunc(items[order[iy - 1]].windex)) == 0)
                    --iy;
                if (iy == ix)
                    order.erase(order.begin() + ix);
                else
                    ix = iy;
            }
        };

        if (display == TestWordsDisplay::WrittenConflict)
        {
            func([this](int windex) -> const QChar*{
                return group->dictionary()->wordEntry(windex)->kanji.data();
            });
        }
        else if (display == TestWordsDisplay::KanaConflict)
        {
            func([this](int windex) -> const QChar*{
                return group->dictionary()->wordEntry(windex)->kana.data();
            });
        }
        else if (display == TestWordsDisplay::DefinitionConflict)
        {
            std::map<int, QString> cache;
            func([this, &cache](int windex) -> const QChar*{
                return meaningString(windex, cache).constData();
            });
        }

        // Compare order with list and add the missing parts to interval.

        for (int pos = 0, lpos = 0, siz = tosigned(order.size()), lsiz = tosigned(list.size()); pos != siz;)
        {
            while (pos != siz && lpos != lsiz && order[pos] == list[lpos])
                ++pos, ++lpos;

            if (pos == siz)
                break;

            Interval i;
            i.index = lpos;
            i.count = 0;

            while (pos != siz && (lpos == lsiz || order[pos] != list[lpos]))
                ++pos, ++i.count;

            intervals.push_back(i);
        }

        std::swap(list, order);
    }

    if (!intervals.empty())
        signalRowsInserted(intervals);
}

void TestWordsItemModel::setExcluded(const std::vector<int> &rows, bool exclude)
{
    std::vector<int> changed;
    for (int ix = 0, siz = tosigned(rows.size()); ix != siz; ++ix)
    {
        int row = rows[ix];
        int pos = list[row];
        if (items[pos].excluded != exclude)
        {
            items[pos].excluded = exclude;
            changed.push_back(row);
        }
    }

    if (changed.empty())
        return;

    smartvector<Range> ranges;

    if (showexcluded)
    {
        _rangeFromIndexes(changed, ranges);
        for (int ix = 0, siz = tosigned(ranges.size()); ix != siz; ++ix)
            emit dataChanged(index(ranges[ix]->first, 0), index(ranges[ix]->last, columnCount() - 1));
        return;
    }

    if (display != TestWordsDisplay::All)
    {
        std::map<int, QString> cache;

        std::vector<int> tmp;
        for (int ix = 0, siz = tosigned(changed.size()); ix != siz; ++ix)
        {
            int row = changed[ix];

            int pre = 0;
            int suf = 0;

            int windex = items[list[row]].windex;
            for (int iy = row - 1, last = tmp.empty() ? -1 : tmp.back(); iy != last; --iy)
            {
                if (items[list[iy]].excluded)
                {
                    --iy;
                    continue;
                }
                if (!comparedEqual(display, windex, items[list[iy]].windex, cache))
                    break;
                ++pre;
            }

            int sufrow = -1;
            for (int iy = row + 1, sizy = tosigned(list.size()); iy != sizy; ++iy)
            {
                if (items[list[iy]].excluded)
                {
                    ++iy;
                    continue;
                }
                if (!comparedEqual(display, windex, items[list[iy]].windex, cache))
                    break;
                if (sufrow == -1)
                    sufrow = iy;
                ++suf;
            }
            if (pre == 1 && suf == 0)
                tmp.push_back(row - 1);
            tmp.push_back(row);
            if (sufrow != -1)
            {
                while (row != sufrow - 1)
                    tmp.push_back(++row);
            }
            if (pre == 0 && suf == 1)
                tmp.push_back(sufrow);
            row += suf;
        }
        std::swap(tmp, changed);
    }

    // Only included words are in list. Those in changed must be removed.

    _rangeFromIndexes(changed, ranges);

    for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
        list.erase(list.begin() + ranges[ix]->first, list.begin() + ranges[ix]->last + 1);
    signalRowsRemoved(ranges);
}

void TestWordsItemModel::resetScore(const std::vector<int> &rows)
{
    std::vector<int> changed;
    for (int ix = 0, siz = tosigned(rows.size()); ix != siz; ++ix)
    {
        int row = rows[ix];
        int pos = list[row];
        WordStudyItem &item = items[pos];
        if (item.correct + item.incorrect != 0)
        {
            items[pos].correct = 0;
            items[pos].incorrect = 0;
            changed.push_back(row);
        }
    }

    if (changed.empty())
        return;

    smartvector<Range> ranges;
    _rangeFromIndexes(changed, ranges);

    for (int ix = 0, siz = tosigned(ranges.size()); ix != siz; ++ix)
        emit dataChanged(index(ranges[ix]->first, 0), index(ranges[ix]->last, columnCount() - 1));
}

const std::vector<WordStudyItem>& TestWordsItemModel::getItems() const
{
    return items;
}

bool TestWordsItemModel::isRowExcluded(int row) const
{
    return items[list[row]].excluded;
}

bool TestWordsItemModel::rowScored(int row) const
{
    return items[list[row]].correct + items[list[row]].incorrect != 0;
}

int TestWordsItemModel::indexes(int pos) const
{
    return items[list[pos]].windex;
}

Dictionary* TestWordsItemModel::dictionary() const
{
    return group->dictionary();
}

int TestWordsItemModel::rowCount(const QModelIndex &/*parent*/) const
{
    return tosigned(list.size());
}

QVariant TestWordsItemModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    if (headerData(col, Qt::Horizontal, (int)DictColumnRoles::Type).toInt() == (int)TestWordsColumnTypes::Score && role == Qt::DisplayRole)
    {
        int correct = items[list[row]].correct;
        int incorrect = items[list[row]].incorrect;
        return QString("+%1/-%2 (%3%4)").arg(correct).arg(incorrect).arg(correct > incorrect ? "+" : "").arg(correct - incorrect);
    }
    if (headerData(col, Qt::Horizontal, (int)DictColumnRoles::Type).toInt() == (int)TestWordsColumnTypes::Order && role == Qt::DisplayRole)
        return QString::number(list[row] + 1);

    if (!index.isValid() || (role != (int)CellRoles::CellColor) || !items[list[row]].excluded)
        return base::data(index, role);

    return mixColors(QColor(160, 160, 160), Settings::textColor((QWidget*)parent(), ColorSettings::Bg), 0.2);
}

Qt::ItemFlags TestWordsItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags r = base::flags(index);
    int row = index.row();
    if (index.isValid() && items[list[row]].excluded)
        r &= ~Qt::ItemIsEnabled;
    return r;
}

bool TestWordsItemModel::sortOrder(int c, int a, int b) const
{
    const WordStudyItem &ia = items[list[a]];
    const WordStudyItem &ib = items[list[b]];

    // Score. Ordered by higher score, then number of times it was tested.
    if (c == 1)
    {
        if (ia.correct - ia.incorrect != ib.correct - ib.incorrect)
            return ia.correct - ia.incorrect > ib.correct - ib.incorrect;
        if (ia.correct + ia.incorrect != ib.correct + ib.incorrect)
            return ia.correct + ia.incorrect > ib.correct + ib.incorrect;
        return list[a] < list[b];
    }

    Dictionary *d = group->dictionary();

    // Order by kanji
    if (c == 2)
    {
        WordEntry *wa = d->wordEntry(ia.windex);
        WordEntry *wb = d->wordEntry(ib.windex);

        int val = qcharcmp(wa->kanji.data(), wb->kanji.data());
        if (val != 0)
            return val < 0;
        return list[a] < list[b];
    }

    // Order by kana
    if (c == 3)
    {
        WordEntry *wa = d->wordEntry(ia.windex);
        WordEntry *wb = d->wordEntry(ib.windex);

        int val = qcharcmp(wa->kana.data(), wb->kana.data());
        if (val != 0)
            return val < 0;
        return list[a] < list[b];
    }

    // Row index
    return list[a] < list[b];
}

void TestWordsItemModel::entryRemoved(int /*windex*/, int /*abcdeindex*/, int /*aiueoindex*/)
{
    ;
}

//void TestWordsItemModel::entryChanged(int windex, bool studydef)
//{
//    ;
//}

void TestWordsItemModel::entryAdded(int /*windex*/)
{
    ;
}

//void TestWordsItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
//{
//    int wpos = -1;
//    for (int ix = 0, siz = items.size(); ix != siz; ++ix)
//        if (items[ix].windex == windex)
//            wpos = ix;
//        else if (items[ix].windex > windex)
//            --items[ix].windex;
//
//    if (wpos == -1)
//        return;
//
//    items.erase(items.begin() + wpos);
//
//    int lpos = -1;
//    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
//        if (list[ix] == wpos)
//            lpos = ix;
//        else if (list[ix] > wpos)
//            --list[ix];
//
//    list.erase(list.begin() + lpos);
//
//    if (lpos != -1)
//        signalRowsRemoved({ { lpos, lpos } });
//}

void TestWordsItemModel::entryChanged(int windex, bool /*studydef*/)
{
    int wpos = std::find_if(items.begin(), items.end(), [windex](const WordStudyItem &item) {
        return item.windex == windex;
    }) - items.begin();

    if (wpos == tosigned(items.size()))
        return;

    if (!showexcluded && /*display != TestWordsDisplay::DefinitionConflict && */items[wpos].excluded)
        return;

    int lpos = std::find(list.begin(), list.end(), wpos) - list.begin();
    if (lpos == tosigned(list.size()))
        lpos = -1;

    if (display != TestWordsDisplay::DefinitionConflict)
    {
        if (lpos != -1)
            emit dataChanged(index(lpos, 0), index(lpos, columnCount() - 1));
        return;
    }

    std::vector<int> order;
    order.reserve(items.size());
    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
        if (showexcluded || !items[ix].excluded)
            order.push_back(ix);

    std::vector<QString> defs;
    defs.reserve(items.size());
    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
    {
        if (showexcluded || !items[ix].excluded)
            defs.push_back(group->dictionary()->displayedStudyDefinition(items[ix].windex).toLower());
        else
            defs.push_back(QString());
    }

    std::sort(order.begin(), order.end(), [this, &defs](int a, int b) {
        if (a == b)
            return false;
        int cmp = qcharcmp(defs[a].constData(), defs[b].constData());
        if (cmp == 0)
            return a < b;
        return cmp < 0;
    });

    // First position in ordered definitions with the same definition as word at wpos.
    int first = std::lower_bound(order.begin(), order.end(), wpos, [&defs](int ix, int wpos) {
        if (ix == wpos)
            return false;
        int cmp = qcharcmp(defs[ix].constData(), defs[wpos].constData());
        return cmp < 0;
    }) - order.begin();

    // Position after the last position with the same definition as the word at wpos.
    int next = first + 1;
    int osiz = tosigned(order.size());
    while (next != osiz && qcharcmp(defs[order[first]].constData(), defs[order[next]].constData()) == 0)
        ++next;

    // Number of words to insert if the word at wpos conflicts with them, including wpos.
    int insertcnt = next - first;

    if (lpos == -1)
    {
        // Word wasn't displayed, but it's possible that it should be, if a match is found. 

        // One or no words found, there's nothing to display.
        if (insertcnt <= 1)
            return;

        // Finding insert position in list, which should be sorted already.
        lpos = std::lower_bound(list.begin(), list.end(), wpos, [&defs](int ix, int wpos) {
            if (ix == wpos)
                return false;
            int cmp = qcharcmp(defs[ix].constData(), defs[wpos].constData());
            if (cmp == 0)
                return ix < wpos;
            return cmp < 0;
        }) - list.begin();

        // If insertcnt is larger than 2, other conflicting words were already displayed. In
        // that case wpos is inserted alone. Otherwise the other word to display is added as
        // well.

        if (insertcnt > 2)
        {
            list.insert(list.begin() + lpos, wpos);
            signalRowsInserted({ { lpos, 1} });
        }
        else
        {
            list.insert(list.begin() + lpos, order.begin() + first, order.begin() + next);
            signalRowsInserted({ { lpos, 2 } });
        }

        return;
    }

    // Word is displayed at lpos. Check if words before and after it have the right to stay.

    // First word in list to be removed if it's alone.
    int front = lpos - 1;
    while (front > 0 && qcharcmp(defs[list[front]].constData(), defs[list[front - 1]].constData()) == 0)
        --front;
    front = std::max(front, 0);

    // Last word in list to be removed if it's alone.
    int back = lpos + 1;
    while (back < tosigned(list.size()) - 1 && qcharcmp(defs[list[back]].constData(), defs[list[back + 1]].constData()) == 0)
        ++back;
    back = std::min<int>(back, tosigned(list.size()) - 1);

    bool delfront = front == lpos - 1 && qcharcmp(defs[list[front]].constData(), defs[list[lpos]].constData()) != 0;
    bool delback = back == lpos + 1 && qcharcmp(defs[list[back]].constData(), defs[list[lpos]].constData()) != 0;

    // The word in list at lpos changed, but the previous and next words still conflict and
    // should be kept.
    if (delfront && delback && qcharcmp(defs[list[front]].constData(), defs[list[back]].constData()) == 0)
        delfront = delback = false;

    if (delfront && delback && insertcnt == 1)
    {
        // Both the words before and after the word at lpos should be deleted and lpos too.
        list.erase(list.begin() + front, list.begin() + (back + 1));
        signalRowsRemoved({ { front, back } });
        return;
    }
    else
    {
        smartvector<Range> ranges;

        if (delback)
        {
            int pos = back;
            if (insertcnt == 1)
                pos = lpos;

            // Erase the single word after lpos. Erase lpos too if it's not to be inserted
            // anywhere
            list.erase(list.begin() + pos, list.begin() + (back + 1));

            ranges.push_back({ pos, back });
        }
        if (delfront)
        {
            int pos = front;
            if (insertcnt == 1)
                pos = lpos;

            // Erase the single word before lpos. Erase lpos too if it's not to be inserted
            // anywhere
            list.erase(list.begin() + front, list.begin() + (pos + 1));
            ranges.insert(ranges.begin(), { front, pos });

            --lpos;
        }

        if (!delback && !delfront && insertcnt == 1)
        {
            list.erase(list.begin() + lpos);
            ranges.insert(ranges.begin(), { lpos, lpos });
        }

        if (!ranges.empty())
            signalRowsRemoved(ranges);

        if (insertcnt == 1)
            return;
    }


    // Remove wpos from list (at lpos) because it has changed and list might not be sorted.
    list.erase(list.begin() + lpos);

    // Finding insert position in list, which should be sorted already.
    int insertpos = std::lower_bound(list.begin(), list.end(), wpos, [&defs](int ix, int wpos) {
        if (ix == wpos)
            return false;
        int cmp = qcharcmp(defs[ix].constData(), defs[wpos].constData());
        if (cmp == 0)
            return ix < wpos;
        return cmp < 0;
    }) - list.begin();

    // wpos is put back to list to keep the model in a consistent state before signals.
    list.insert(list.begin() + lpos, wpos);
    if (lpos <= insertpos)
        ++insertpos;

    // Find wpos in order.
    int opos = -1;
    for (int ix = first; ix != next && opos == -1; ++ix)
        if (order[ix] == wpos)
            opos = ix;

    // Insert words before and after wpos to list.

    // Insert words coming after wpos in the ordered items list. If the words are already
    // inserted (because they conflict with other words,) there's nothing to do.
    if (next > opos + 1 && (insertpos == tosigned(list.size()) || order[opos + 1] != list[insertpos]))
    {
        list.insert(list.begin() + insertpos, order.begin() + (opos + 1), order.begin() + next);
        signalRowsInserted({ { insertpos, next - opos - 1 } });

        if (lpos >= insertpos)
            lpos += next - opos - 1;
    }

    // Move word at lpos to insertpos if it's not already there.
    if (lpos != insertpos - 1)
    {
        list.erase(list.begin() + lpos);
        if (insertpos > lpos)
            --insertpos;
        list.insert(list.begin() + insertpos, wpos);

        signalRowsMoved({ { lpos, lpos } }, insertpos >= lpos ? insertpos + 1 : insertpos);
    }
    else
        --insertpos;

    // Insert words in front if they are not already found.
    if (front < opos && (insertpos == 0 || order[opos - 1] != list[insertpos - 1]))
    {
        list.insert(list.begin() + insertpos, order.begin() + first, order.begin() + opos);
        signalRowsInserted({ { insertpos,  opos - first } });
    }
}

const QString& TestWordsItemModel::meaningString(int windex, std::map<int, QString> &cache) const
{
    auto it = cache.find(windex);
    if (it != cache.end())
        return it->second;
    return (cache[windex] = group->dictionary()->displayedStudyDefinition(windex).toLower());
};

bool TestWordsItemModel::comparedEqual(TestWordsDisplay disp, int windex1, int windex2, std::map<int, QString> &cache) const
{
    switch (disp)
    {
    case TestWordsDisplay::WrittenConflict:
        return qcharcmp(group->dictionary()->wordEntry(windex1)->kanji.data(), group->dictionary()->wordEntry(windex2)->kanji.data()) == 0;
    case TestWordsDisplay::KanaConflict:
        return qcharcmp(group->dictionary()->wordEntry(windex1)->kana.data(), group->dictionary()->wordEntry(windex2)->kana.data()) == 0;
    case TestWordsDisplay::DefinitionConflict:
        return qcharcmp(meaningString(windex1, cache).constData(), meaningString(windex2, cache).constData()) == 0;
    default:
        return false;
    }
}


//-------------------------------------------------------------


WordTestSettingsForm::WordTestSettingsForm(WordGroup *group, QWidget *parent) :
    base(parent), ui(new Ui::WordTestSettingsForm), group(group), model(nullptr), scolumn(0), sorder(Qt::AscendingOrder)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    //setAttribute(Qt::WA_DontShowOnScreen);
    //show();
    for (int ix = 0, siz = ui->settingsTabs->count(); ix != siz; ++ix)
    {
        ui->settingsTabs->setCurrentIndex(ix);
        fixWrapLabelsHeight(this, -1/*500*/);
        //updateWindowGeometry(this);
    }
    //hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    group->studyData().applyScore();
    model = new TestWordsItemModel(group, ui->dictWidget);

    connect(ui->askKanjiBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);
    connect(ui->askKanaBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);
    connect(ui->askMeaningBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);

    connect(ui->askKanjiKanaBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);
    connect(ui->askMeaningKanjiBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);
    connect(ui->askMeaningKanaBox, &QCheckBox::toggled, this, &WordTestSettingsForm::updateStartSave);

    connect(ui->dictWidget, &DictionaryWidget::rowSelectionChanged, this, &WordTestSettingsForm::selectionChanged);
    connect(ui->dictWidget, &DictionaryWidget::sortIndicatorChanged, this, &WordTestSettingsForm::headerSortChanged);
    ui->dictWidget->setDictionary(group->dictionary());
}

WordTestSettingsForm::~WordTestSettingsForm()
{
    delete ui;
}

void WordTestSettingsForm::exec()
{
    // Modifies sizes of some widgets and shows the window.

    //setAttribute(Qt::WA_DontShowOnScreen);
    //base::show();

    ui->settingsTabs->setCurrentIndex(0);
    updateWindowGeometry(this);
    //layout()->update();
    //layout()->activate();

    ui->settingsTabs->setCurrentIndex(1);
    ui->methodStack->setCurrentWidget(ui->gradualPage);
    updateWindowGeometry(this);

    //layout()->update();
    //layout()->activate();

    int w = mmax(ui->timerBox->sizeHint().width(), ui->timeLimitBox->sizeHint().width(), ui->wordsBox->sizeHint().width());

    ui->timerBox->setMinimumWidth(w);
    ui->timeLimitBox->setMinimumWidth(w);
    ui->wordsBox->setMinimumWidth(w);

    w = mmax(ui->initCBox->sizeHint().width(), ui->increaseCBox->sizeHint().width(), ui->roundCBox->sizeHint().width()/*, ui->timerCBox->width(), ui->timeLimitEdit->width(), ui->wordsEdit->width()*/);

    ui->initCBox->setMinimumWidth(w);
    ui->increaseCBox->setMinimumWidth(w);
    ui->roundCBox->setMinimumWidth(w);
    ui->timerCBox->setMinimumWidth(w);
    ui->timeLimitEdit->setMinimumWidth(w);
    ui->wordsEdit->setMinimumWidth(w);

    //ui->singleSpacer->changeSize(ui->methodLabel->width(), 1, QSizePolicy::Fixed, QSizePolicy::Fixed);

    w = mmax(ui->wordLabel->sizeHint().width(), ui->minLabel->sizeHint().width(), ui->secLabel->sizeHint().width());
    ui->wordLabel->setMinimumWidth(w);
    ui->minLabel->setMinimumWidth(w);
    ui->secLabel->setMinimumWidth(w);

    const WordStudySettings &settings = group->studyData().studySettings();

    // Set up form:
    ui->askKanjiBox->setChecked((settings.tested & (int)WordStudyQuestion::Kanji) != 0);
    ui->askKanaBox->setChecked((settings.tested & (int)WordStudyQuestion::Kana) != 0);
    ui->askMeaningBox->setChecked((settings.tested & (int)WordStudyQuestion::Definition) != 0);
    ui->askKanjiKanaBox->setChecked((settings.tested & (int)WordStudyQuestion::KanjiKana) != 0);
    ui->askMeaningKanjiBox->setChecked((settings.tested & (int)WordStudyQuestion::DefKanji) != 0);
    ui->askMeaningKanaBox->setChecked((settings.tested & (int)WordStudyQuestion::DefKana) != 0);

    ui->hideKanaBox->setChecked(settings.hidekana);

    ui->answerCBox->setCurrentIndex((int)settings.answer);

    ui->typeKanjiBox->setChecked((settings.typed & (int)WordPartBits::Kanji) != 0);
    ui->typeKanaBox->setChecked((settings.typed & (int)WordPartBits::Kana) != 0);
    ui->typeMeaningBox->setChecked((settings.typed & (int)WordPartBits::Definition) != 0);

    ui->kanaUsageBox->setChecked(settings.matchkana);

    ui->methodCBox->setCurrentIndex(settings.method == WordStudyMethod::Gradual ? 0 : 1);
    //on_methodCBox_currentIndexChanged(ui->methodCBox->currentIndex());

    ui->initCBox->setCurrentText(QString::number(settings.gradual.initnum));
    ui->increaseCBox->setCurrentText(QString::number(settings.gradual.incnum));
    ui->roundCBox->setCurrentText(QString::number(settings.gradual.roundlimit));

    ui->prefNewBox->setChecked(settings.single.prefernew);
    ui->prefDifBox->setChecked(settings.single.preferhard);

    ui->shuffleRoundBox->setChecked(settings.gradual.roundshuffle);
    ui->timerCBox->setCurrentText(QString::number(settings.timer));
    ui->timerBox->setChecked(settings.usetimer);

    ui->timeLimitEdit->setText(QString::number(settings.limit));
    ui->timeLimitBox->setChecked(settings.uselimit);

    ui->wordsEdit->setText(QString::number(settings.wordlimit));
    ui->wordsBox->setChecked(settings.usewordlimit);

    ui->shuffleTestBox->setChecked(settings.shuffle);

    ui->dictWidget->setStudyDefinitionUsed(true);
    ui->dictWidget->setMultilineVisible(false);
    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setModel(model);
    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->setSortIndicatorVisible(true);
    ui->dictWidget->setInflButtonVisible(false);
    ui->dictWidget->setSortFunction([this](DictionaryItemModel * /*d*/, int c, int a, int b){ return model->sortOrder(c, a, b); });
    ui->dictWidget->setSortIndicator(0, Qt::AscendingOrder);

    adjustSize();
    //setFixedSize(size());

    ui->settingsTabs->setCurrentIndex(0);

    //base::hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    setAttribute(Qt::WA_DeleteOnClose);

    setWidgetTexts();

    // Warning: if changed to show(), handle word and dictionary changes.
    base::showModal();
}

bool WordTestSettingsForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        setWidgetTexts();
    }

    return base::event(e);
}

void WordTestSettingsForm::closeEvent(QCloseEvent *e)
{
    e->ignore();

    if (modalResult() == ModalResult::Cancel)
        ;// onclose(result, group, WordStudySettings());
    else
    {
        if (ui->methodCBox->currentIndex() == 0 && ui->initCBox->currentText().toInt() > ui->roundCBox->currentText().toInt())
        {
            QMessageBox::information(this, tr("Invalid settings"), tr("The initial number of words can't be higher than the number of words from previous rounds, when the study method is gradual inclusion."), QMessageBox::Ok);
            base::closeEvent(e);
            return;
        }

        WordStudySettings settings;
        memset(&settings, 0, sizeof(WordStudySettings));
        if (ui->askKanjiBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::Kanji;
        if (ui->askKanaBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::Kana;
        if (ui->askMeaningBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::Definition;
        if (ui->askKanjiKanaBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::KanjiKana;
        if (ui->askMeaningKanjiBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::DefKanji;
        if (ui->askMeaningKanaBox->isChecked())
            settings.tested |= (int)WordStudyQuestion::DefKana;

        settings.hidekana = ui->hideKanaBox->isChecked();

        if (ui->answerCBox->currentIndex() == -1)
            ui->answerCBox->setCurrentIndex(0);
        settings.answer = WordStudyAnswering(ui->answerCBox->currentIndex());

        if (ui->typeKanjiBox->isChecked())
            settings.typed |= (int)WordPartBits::Kanji;
        if (ui->typeKanaBox->isChecked())
            settings.typed |= (int)WordPartBits::Kana;
        if (ui->typeMeaningBox->isChecked())
            settings.typed |= (int)WordPartBits::Definition;

        settings.matchkana = ui->kanaUsageBox->isChecked();

        if (ui->methodCBox->currentIndex() == 0)
            settings.method = WordStudyMethod::Gradual;
        if (ui->methodCBox->currentIndex() == 1)
            settings.method = WordStudyMethod::Single;

        settings.gradual.initnum = ui->initCBox->currentText().toInt();
        settings.gradual.incnum = ui->increaseCBox->currentText().toInt();
        settings.gradual.roundlimit = ui->roundCBox->currentText().toInt();

        settings.single.prefernew = ui->prefNewBox->isChecked();
        settings.single.preferhard = ui->prefDifBox->isChecked();


        settings.gradual.roundshuffle = ui->shuffleRoundBox->isChecked();
        settings.timer = ui->timerCBox->currentText().toInt();
        settings.usetimer = ui->timerBox->isChecked();

        settings.limit = ui->timeLimitEdit->text().toInt();
        settings.uselimit = ui->timeLimitBox->isChecked();

        settings.wordlimit = ui->wordsEdit->text().toInt();
        settings.usewordlimit = ui->wordsBox->isChecked();

        settings.shuffle = ui->shuffleTestBox->isChecked();

        if (ui->defaultBox->isChecked())
            ZKanji::setDefaultWordStudySettings(settings);

        // Count the number of testable items, and check if there are enough for the test to
        // start. Update WordStudy::initTest() too if this part changes.
        const std::vector<WordStudyItem> &items = model->getItems();
        bool onlykanjikana = (settings.tested & ~((int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana)) == 0;
        int sum = tosigned(items.size());
        for (int ix = sum - 1; ix != -1; --ix)
            if (items[ix].excluded || (onlykanjikana && hiraganize(group->dictionary()->wordEntry(items[ix].windex)->kanji) == hiraganize(group->dictionary()->wordEntry(items[ix].windex)->kana)))
                --sum;
        if ((settings.answer == WordStudyAnswering::Choices5 && sum < 5) || (settings.answer == WordStudyAnswering::Choices8 && sum < 8))
        {
            QMessageBox::information(this, tr("Invalid settings"), tr("There are too few items to be tested for the number of choices."), QMessageBox::Ok);
            base::closeEvent(e);
            return;
        }

        if (settings.method == WordStudyMethod::Gradual)
        {
            if (std::max(settings.gradual.roundlimit, settings.gradual.initnum + settings.gradual.incnum) > sum)
            {
                QMessageBox::information(this, tr("Invalid settings"), tr("The number of words to test must be at least %1, with the current gradual inclusion test settings.\n\nDepending on your test settings, words having the same written and kana forms can't be in the test.").arg(std::max(settings.gradual.roundlimit, settings.gradual.initnum + settings.gradual.incnum)), QMessageBox::Ok);
                base::closeEvent(e);
                return;
            }
        }
        else if (settings.method == WordStudyMethod::Single && sum < 10)
        {
            QMessageBox::information(this, tr("Invalid settings"), tr("At least 10 words must be present for the single round test.\n\nDepending on your test settings, words having the same written and kana forms can't be in the test."), QMessageBox::Ok);
            base::closeEvent(e);
            return;
        }

        group->studyData().setup(settings, model->getItems());

        if (modalResult() == ModalResult::Start)
        {
            WordStudyForm *study = new WordStudyForm();
            study->exec(&group->studyData());
        }
    }

    e->accept();
    base::closeEvent(e);
}

void WordTestSettingsForm::setWidgetTexts()
{
    setWindowTitle(QString("zkanji - %1: %2").arg(tr("Word test settings")).arg(group->name()));
    on_methodCBox_currentIndexChanged(ui->methodCBox->currentIndex());
}

void WordTestSettingsForm::updateStartSave()
{
    bool good = (ui->askKanjiBox->isChecked() || ui->askKanaBox->isChecked() || ui->askMeaningBox->isChecked() ||
        ui->askKanjiKanaBox->isChecked() || ui->askMeaningKanaBox->isChecked() || ui->askMeaningKanjiBox->isChecked());
    ui->startButton->setEnabled(good);
    ui->saveButton->setEnabled(good);
}

//bool WordTestSettingsForm::hasExclude(int index)
//{
//    if (index == -1)
//        return ui->displayBox->currentIndex() != ui->displayBox->count() - 1;
//    return index != ui->displayBox->count() - 1;
//}

void WordTestSettingsForm::on_methodCBox_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->methodStack->setCurrentWidget(ui->gradualPage);
        ui->infoLabel->setText(tr("<p><b>Gradual inclusion:</b><br><br>The test has multiple rounds.<br><br>In each round, only a few words are shown. In the first round the initial number of words appear.<br><br>In all following rounds, words from the past rounds and at most the above set number of new words are shown. Words shown from past rounds are also limited to the set amount. If you make a mistake in a round, the next round will have the same words without new words added.<br><br>The test ends, when all words had the chance to be tested the same number of times.</p>"));
    }
    else
    {
        ui->methodStack->setCurrentWidget(ui->singlePage);
        ui->infoLabel->setText(tr("<p><b>Single round:</b><br><br>Each word is shown once, until the last one is shown. If you make a mistake and can't answer a word correctly, it's shown again at the end of the test.</p>"));
    }

    updateStartSave();
}

void WordTestSettingsForm::on_timeCBox_currentIndexChanged(int /*index*/)
{
    ui->timerBox->setChecked(true);
}

void WordTestSettingsForm::on_timeLimitEdit_textChanged(QString t)
{
    ui->timeLimitBox->setChecked(!t.isEmpty());
}

void WordTestSettingsForm::on_wordsEdit_textChanged(QString t)
{
    ui->wordsBox->setChecked(!t.isEmpty());
}

void WordTestSettingsForm::on_saveButton_clicked()
{
    closeSave();
}

void WordTestSettingsForm::on_startButton_clicked()
{
    closeStart();
}

void WordTestSettingsForm::on_displayBox_currentIndexChanged(int index)
{
    //ui->excludedButton->setEnabled(index == 0);

    ui->dictWidget->setSortIndicatorVisible(index == 0);
    if (index == 0)
    {
        ui->dictWidget->setSortIndicator(scolumn, sorder);
        ui->dictWidget->setSortFunction([this](DictionaryItemModel * /*d*/, int c, int a, int b){ return model->sortOrder(c, a, b); });
    }
    else
        ui->dictWidget->setSortFunction(nullptr);
    model->setDisplayed(TestWordsDisplay(index));
    ui->dictWidget->resetScrollPosition();
}

void WordTestSettingsForm::on_excludedButton_clicked()
{
    model->setShowExcluded(ui->excludedButton->isChecked());
}

void WordTestSettingsForm::on_excludeButton_clicked()
{
    std::vector<int> rows;
    ui->dictWidget->selectedRows(rows);
    if (rows.empty())
        return;
    ui->dictWidget->unfilteredRows(rows, rows);
    std::sort(rows.begin(), rows.end());
    model->setExcluded(rows, true);

    ui->excludeButton->setEnabled(false);
    ui->includeButton->setEnabled(true);
}

void WordTestSettingsForm::on_includeButton_clicked()
{
    std::vector<int> rows;
    ui->dictWidget->selectedRows(rows);
    if (rows.empty())
        return;
    ui->dictWidget->unfilteredRows(rows, rows);
    std::sort(rows.begin(), rows.end());
    model->setExcluded(rows, false);

    ui->excludeButton->setEnabled(true);
    ui->includeButton->setEnabled(false);
}

void WordTestSettingsForm::on_resetButton_clicked()
{
    std::vector<int> rows;
    ui->dictWidget->selectedRows(rows);
    if (rows.empty())
        return;

    if (QMessageBox::question(this, "zkanji", tr("Do you want to reset the score of the selected items? This operation only affects this group."), QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    ui->dictWidget->unfilteredRows(rows, rows);
    std::sort(rows.begin(), rows.end());
    model->resetScore(rows);

    ui->resetButton->setEnabled(false);
}

void WordTestSettingsForm::on_refreshButton_clicked()
{
    model->refresh();
}

void WordTestSettingsForm::on_answerCBox_currentIndexChanged(int index)
{
    ui->typeKanjiBox->setEnabled(index == 0);
    ui->typeKanaBox->setEnabled(index == 0);
    ui->typeMeaningBox->setEnabled(index == 0);
    ui->kanaUsageBox->setEnabled(index == 0);
}

void WordTestSettingsForm::selectionChanged()
{
    std::vector<int> rows;
    ui->dictWidget->selectedRows(rows);
    ui->dictWidget->unfilteredRows(rows, rows);
    std::sort(rows.begin(), rows.end());

    std::vector<int> indexes;
    ui->dictWidget->selectedIndexes(indexes);
    ui->defEditor->setWords(ui->dictWidget->dictionary(), indexes);

    bool hasex = false;
    bool hasinc = false;
    bool hasscore = false;
    for (int ix = 0, siz = tosigned(rows.size()); ix != siz && (!hasex || !hasinc); ++ix)
    {
        if (model->isRowExcluded(rows[ix]))
            hasex = true;
        else
            hasinc = true;

        hasscore = hasscore || model->rowScored(rows[ix]);
    }
    ui->excludeButton->setEnabled(hasinc);
    ui->includeButton->setEnabled(hasex);

    ui->resetButton->setEnabled(hasscore);
}

void WordTestSettingsForm::headerSortChanged(int column, Qt::SortOrder order)
{
    if ((order == sorder && column == scolumn) || !ui->dictWidget->isSortIndicatorVisible())
        return;

    if (column == model->columnCount() - 1)
    {
        // The last column shows the definitions of words and cannot be used for sorting. The
        // original column should be restored.
        ui->dictWidget->setSortIndicator(scolumn, sorder);
        return;
    }

    scolumn = column;
    sorder = order;

    ui->dictWidget->sortByIndicator();
}


//-------------------------------------------------------------

