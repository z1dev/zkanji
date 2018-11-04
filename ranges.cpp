/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <algorithm>
#include "ranges.h"
#include "zdictionarymodel.h"
#include "zkanjimain.h"


//-------------------------------------------------------------


RangeSelection::RangeSelection() : itcache(list.cend())
{
    ;
}

void RangeSelection::clear()
{
    list.clear();
    itcache = list.cend();
}

bool RangeSelection::empty() const
{
    return list.empty();
}

int RangeSelection::size(int addfirst, int addlast) const
{
    //int cnt = 0;
    //for (const Range *r : ranges)
    //    cnt += r->last - r->first + 1;

    //return cnt;

    bool validadd = addfirst != -1 && addlast != -1 && addfirst <= addlast;
    if (list.empty())
        return validadd ? addlast - addfirst + 1 : 0;

    int cnt = 0;

    // Last item in the previously counted  range.
    int bottom = -1;

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        const Range *r = list[ix];

        int rtop = r->first;
        int rbottom = r->last;

        if (validadd && addfirst > bottom && addlast < rtop)
        {
            // The added range is between two ranges, not overlapping with them.
            cnt += addlast - addfirst + 1;
            validadd = false;
        }
        if (validadd && addlast < rtop)
        {
            // The added range overlapped the previous range and reaches below it, but doesn't
            // overlap with the next range.

            cnt += addlast - bottom;

            validadd = false;
        }

        // The added range might start above the current range and overlaps with it
        if (validadd && addlast >= rtop)
            rtop = std::max(bottom + 1, std::min(rtop, addfirst));

        cnt += rbottom - rtop + 1;

        bottom = rbottom;

        // Invalidate added range once we are finished with it;
        if (validadd && addlast <= bottom)
            validadd = false;
    }

    if (validadd)
    {
        // The added range reaches below the last range.

        cnt += addlast - std::max(bottom + 1, addfirst) + 1;

        validadd = false;
    }

    return cnt;
}

bool RangeSelection::singleSelected(int index) const
{
    if (list.size() != 1)
        return false;

    const Range *r = list.front();
    return r->first == index && r->last == index;
}


bool RangeSelection::selected(int index) const
{
    auto it = search(index);

    if (it == list.end())
        return false;

    return (*it)->first <= index;
}

void RangeSelection::setSelected(int index, bool sel)
{
    auto it = search(index);

    // Already selected or unselected.
    if ((it == list.end() && !sel) || (it != list.end() && ((*it)->first <= index) == sel))
        return;

    // Affected range containing or directly after index.
    Range *r = it != list.end() ? *it : nullptr;
    // Affected range directly in front of index.
    Range *r2 = !sel ? r : it != list.begin() ? *std::prev(it) : nullptr;

    // Null out ranges if they don't touch index.

    if ((sel && it != list.end() && r->first != index + 1) || (!sel && r->last == index))
        r = nullptr;

    if ((sel && it != list.begin() && r2->last != index - 1) || (!sel && r != nullptr))
        r2 = nullptr;

    if (sel)
    {
        // Select.

        if (r != nullptr && r2 != nullptr)
        {
            // Both ranges touch the new index. Make one fill the whole range and delete the
            // other one.

            r2->last = r->last;
            itcache = list.erase(it);
            return;
        }

        if (r != nullptr)
            --r->first;
        else if (r2 != nullptr)
            ++r2->last;
        else
        {
            r = new Range;
            r->first = r->last = index;
            itcache = list.insert(it, r);
        }

        return;
    }

    // Deselect.

    if (r2 != nullptr)
    {
        // Both sides equal to index here.
        if (r2->first == r2->last)
            itcache = list.erase(it);
        else
            --r2->last;
        return;
    }

    if (r->first == index)
    {
        // Range after index touched it from the front.
        ++r->first;
        return;
    }

    // Range must be split in the middle at index.

    r2 = new Range;
    r2->first = r->first;
    r2->last = index - 1;

    r->first = index + 1;

    itcache = list.insert(it, r2);
}

void RangeSelection::getSelection(std::vector<int> &result, int addfirst, int addlast) const
{
    bool validadd = addfirst != -1 && addlast != -1 && addfirst <= addlast;
    if (list.empty())
    {
        for (int iy = addfirst; validadd && iy != addlast + 1; ++iy)
            result.push_back(iy);
        return;
    }

    result.reserve(size());

    // Last item in the previously added range.
    int bottom = -1;

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        const Range *r = list[ix];

        int rtop = r->first;
        int rbottom = r->last;

        if (validadd && addfirst > bottom && addlast < rtop)
        {
            // The added range is between two ranges, not overlapping with them.
            for (int iy = addfirst; iy != addlast + 1; ++iy)
                result.push_back(iy);

            validadd = false;
        }
        if (validadd && addlast < rtop)
        {
            // The added range overlapped the previous range and reaches below it, but doesn't
            // overlap with the next range.

            for (int iy = bottom + 1; iy != addlast + 1; ++iy)
                result.push_back(iy);

            validadd = false;
        }

        // The added range might start above the current range and overlaps with it
        if (validadd && addlast >= rtop)
            rtop = std::max(bottom + 1, std::min(rtop, addfirst));

        for (int iy = rtop; iy != rbottom + 1; ++iy)
            result.push_back(iy);

        bottom = rbottom;

        // Invalidate added range once we are finished with it;
        if (validadd && addlast <= bottom)
            validadd = false;
    }

    if (validadd)
    {
        // The added range reaches below the last range.

        for (int iy = std::max(bottom + 1, addfirst); iy != addlast + 1; ++iy)
            result.push_back(iy);

        validadd = false;
    }
}

void RangeSelection::selectedRanges(int first, int last, std::vector<Range const *> &result) const
{
    auto it = search(first);

    while (it != list.end())
    {
        const Range *r = *it;
        if (r->first > last)
            return;
        result.push_back(r);
        ++it;
    }
}

void RangeSelection::selectRange(int first, int last, bool sel)
{
    // Get the range touching index if selecting, so it can be extended. Either that, or the
    // next range will be returned here.
    auto it = search(sel ? std::max(0, first - 1) : first);

    if (sel)
    {
        if (it == list.end())
        {
            // The new selection will come after the existing selection, which won't be
            // affected.

            Range *r = new Range;
            r->first = first;
            r->last = last;

            // itcache is not invalidated as it's the same as it, already set to list.end().
            list.push_back(r);
            itcache = list.cend();
            return;
        }

        Range *r = *it;

        if (r->first <= last + 1)
            r->first = std::min(r->first, first);
        else
        {
            // The found range doesn't touch the new selection. A new range must be created.

            r = new Range;
            r->first = first;
            r->last = last;
            itcache = list.insert(it, r);
            return;
        }

        // The found range touches the new selection from the front. Extend its back.

        r->last = std::max(r->last, last);

        // If a selection comes after r that also touches the new selection, it must be
        // joined with r.

        ++it;
        while (it != list.end())
        {
            Range *r2 = *it;

            // The next range is not touching the new selection.
            if (r2->first > last + 1)
                break;

            r->last = std::max(r->last, r2->last);
            it = list.erase(it);
        }

        itcache = it;

        return;
    }

    // Nothing to deselect.
    if (it == list.end())
        return;

    Range *r = *it;
    if (r->first > last)
        return;

    if (r->first < first && r->last > last)
    {
        // The found range contains the whole deselection. It must be cut in two parts.

        Range *r2 = new Range;
        r2->first = last + 1;
        r2->last = r->last;
        r->last = first - 1;

        itcache = list.insert(std::next(it), r2);
        return;
    }

    if (r->first < first)
    {
        // Found range touches the deselection from the back. Remove that part.

        r->last = first - 1;
        ++it;
        if (it == list.end())
            return;
        r = *it;
    }

    while (r->last <= last)
    {
        // Remove every range fully deselected.

        it = list.erase(it);
        if (it == list.end())
        {
            itcache = list.cend();
            return;
        }
        r = *it;
    }

    itcache = it;

    // Found range touches the deselection from the front. Remove that part.
    if (r->first <= last)
        r->first = last + 1;
}

bool RangeSelection::rangeSelected(int first, int last) const
{
    auto it = search(first);
    if (it == list.cend() || (*it)->first > first)
        return false;
    return (*it)->last >= last;
}

int RangeSelection::selectedItems(int selindex, int addfirst, int addlast)  const
{
    if (selindex < 0)
        return -1;

    bool validadd = addfirst != -1 && addlast != -1 && addfirst <= addlast;
    if (list.empty())
    {
        if (!validadd || selindex > addlast - addfirst)
            return -1;
        return addfirst + selindex;
    }

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        const Range *r = list[ix];

        if (validadd && addfirst < r->first)
        {
            if (selindex <= std::min(addlast, r->first - 1) - addfirst)
                return selindex + addfirst;
            selindex -= std::min(addlast, r->first - 1) - addfirst + 1;
            addfirst = r->last + 1;
        }

        if (validadd && addlast <= r->last)
            validadd = false;

        if (r->last - r->first + 1 <= selindex)
            selindex -= r->last - r->first + 1;
        else
            return r->first + selindex;
    }

    if (validadd && selindex <= addlast - addfirst)
        return addfirst + selindex;

    return -1;
}

int RangeSelection::rangeCount() const
{
    return tosigned(list.size());
}

const Range& RangeSelection::ranges(int ix) const
{
    return *list[ix];
}

bool RangeSelection::insert(int index, int cnt)
{
    auto it = search(index);

    // Every range was below index.
    if (it == list.end() || cnt == 0)
        return false;

    Range *r = *it;
    if (r->first < index)
    {
        // Found a range that must be cut in two, with cnt number of empty items inserted in
        // its middle.

        Range *r2 = new Range;
        r2->first = index + cnt;
        r2->last = r->last + cnt;
        r->last = index - 1;

        itcache = it = std::next(list.insert(it, r2));
    }

    // Every range at and above it are to be moved up by cnt.
    for (; it != list.end(); ++it)
    {
        r = *it;
        r->first += cnt;
        r->last += cnt;
    }
    return true;
}

bool RangeSelection::insert(const smartvector<Interval> &intervals)
{
    bool changed = false;
    for (int ix = tosigned(intervals.size()) - 1; ix != -1; --ix)
        changed = insert(intervals[ix]->index, intervals[ix]->count) | changed;
    return changed;
}

bool RangeSelection::remove(const Range &range)
{
    auto it = search(range.first);

    // Every range was below index.
    if (it == list.end())
        return false;

    Range *r = *it;

    int cnt = range.last - range.first + 1;
    if (cnt <= 0)
        return false;

    // Shrink range by the overlapping removed part.

    if (r->first < range.first)
    {
        r->last = std::max(range.first - 1, r->last - cnt);

        ++it;
        if (it == list.end())
            return true;
        r = *it;
    }

    // Remove every range that fully falls inside [index, index + cnt).

    auto it2 = it;
    while (r->first >= range.first && r->last <= range.last)
    {
        ++it2;
        if (it2 != list.end())
            r = *it2;
        else
            break;
    }

    if (it != it2)
        itcache = it = list.erase(it, it2);

    if (it == list.end())
        return true;

    // Remove part of next range overlapping with [index, index + cnt).

    r = *it;
    if (r->first < range.last + 1)
    {
        r->first = range.first;
        r->last -= cnt;
        ++it;
    }

    // Move every range at and above it down by cnt.
    for (; it != list.end(); ++it)
    {
        r = *it;
        r->first -= cnt;
        r->last -= cnt;
    }

    return true;
}

bool RangeSelection::remove(const smartvector<Range> &rngs)
{
    bool changed = false;
    for (int ix = tosigned(rngs.size()) - 1; ix != -1; --ix)
        changed = remove(*rngs[ix]) | changed;
    return changed;
}

bool RangeSelection::move(const Range &range, int newindex)
{
    // No move. New position would be the same as the old one.
    if (newindex <= range.last + 1 && newindex >= range.first)
        return false;

    // Getting the first range at or after the moved part.

    auto it = search(range.first);

    if (it == list.end())
    {
        // No ranges are affected by the move. Inserting empty space at newindex.
        return insert(newindex, range.last - range.first + 1);
    }

    Range *r = *it;

    if (r->first > range.last)
    {
        // The first range affected comes after the moved part. No ranges are moved, just
        // empty space. Remove the empty space between [first, last] and insert it at the
        // modified newindex.

        if (newindex > range.last)
            newindex -= range.last - range.first + 1;

        bool changed = remove(range);
        changed = insert(newindex, range.last - range.first + 1) | changed;
        return changed;
    }


    // Create a vector of ranges to hold ranges moved to newindex. The ranges' positions will
    // be updated to place them where they are to be moved to.

    std::vector<Range*> moved;

    // Number of places the moved ranges are shifted in either direction. Negative means
    // shifting backwards.
    int moveamount = newindex > range.last ? newindex - range.last - 1 : newindex - range.first;

    // The found range is bigger than the moved area.
    if (r->first < range.first && r->last > range.last)
    {
        r = new Range;
        r->first = range.first + moveamount;
        r->last = range.last + moveamount;
        moved.push_back(r);
    }
    else
    {
        // The first range is partially in the moved part. Update it and create a range to be
        // moved.
        if (r->first < range.first)
        {
            Range *r2 = new Range;
            r2->first = range.first + moveamount;
            r2->last = r->last + moveamount;
            r->last = range.first - 1;

            moved.push_back(r2);
            ++it;
        }

        while (it != list.end())
        {
            r = *it;

            // If the range is at least partially outside the moved range, it'll be processed
            // separately after the loop.
            if (r->last > range.last)
                break;

            // The r range is fully moved.
            it = list.removeAt(it/*, it*/);
            r->first += moveamount;
            r->last += moveamount;
            moved.push_back(r);
        }

        itcache = it;

        // Last range affected is partially in the moved part. Update it and create a range to
        // be moved.
        if (it != list.end() && r->first <= range.last)
        {
            Range *r2 = new Range;
            r2->first = r->first + moveamount;
            r2->last = range.last + moveamount;
            r->first = range.last + 1;

            moved.push_back(r2);
        }
    }

    // The range where the moved ranges were is now empty. Remove the empty space.
    remove(range);

    // Update the newindex to point to the insert position after the space has been removed.
    if (newindex > range.last)
        newindex -= range.last - range.first + 1;

    // Insert empty space where the moved ranges will be put.
    insert(newindex, range.last - range.first + 1);

    // Find the iterator where the moved list is to be inserted, then insert. Because an empty
    // space has been inserted there already, the range we get will not overlap with the
    // inserted list.
    it = search(newindex);

#ifdef _DEBUG
    if (it != list.end() && (*it)->first <= newindex)
        throw "Something went very wrong!";
#endif

    auto it2 = list.insert(it, moved.begin(), moved.end());

    // It's still possible that the new inserted ranges touch old list. They will be joined.

    it = it2 + moved.size();

    if (it != list.end())
    {
        // Last inserted range might touch the range after it.

        // Range after the last inserted.
        Range *r2 = *it;
        // Last inserted range.
        r = *std::prev(it);

        if (r2->first == r->last + 1)
        {
            // The two ranges touch which is invalid. They must be joined.

            // it2 will be possibly invalidated here. Its position can be saved to restore it.
            int it2pos = it2 - list.begin();

            r->last = r2->last;
            list.erase(it);

            // Restoring the invalidated iterator.
            it2 = list.begin() + it2pos;
        }
    }

    if (it2 != list.begin())
    {
        // First inserted range might touch the range before it.

        Range *r2 = *std::prev(it2);
        r = *it2;

        if (r->first == r2->last + 1)
        {
            r2->last = r->last;
            it2 = list.erase(it2);
        }
    }

    itcache = it2;

    return true;
}

bool RangeSelection::move(const smartvector<Range> &rngs, int newindex)
{
    bool changed = false;
    _moveRanges(rngs, newindex, [this, &changed](const Range &r, int pos) { changed = move(r, pos) | changed; });
    return changed;
}

//void RangeSelection::beginLayouting(ZAbstractTableModel *model)
//{
//    layoutindexes.reserve(size());
//
//    for (Range *r : list)
//    {
//        for (int ix = r->first; ix != r->last + 1; ++ix)
//            layoutindexes.emplace_back(model->index(ix, 0));
//    }
//
//    list.clear();
//    itcache = list.end();
//}
//
//void RangeSelection::endLayouting(ZAbstractTableModel *model)
//{
//    // The rows of all valid indexes in layoutindexes sorted by value.
//    std::vector<int> order;
//    order.reserve(layoutindexes.size());
//    for (int ix = 0, siz = layoutindexes.size(); ix != siz; ++ix)
//    {
//        if (layoutindexes[ix].isValid())
//            order.push_back(layoutindexes[ix].row());
//    }
//    layoutindexes.swap(std::vector<QPersistentModelIndex>());
//
//    std::sort(order.begin(), order.end());
//    order.resize(std::unique(order.begin(), order.end()) - order.begin());
//
//    // Creating ranges from consecutive rows.
//    for (int ix = 1, top = 0, siz = order.size(); ix != siz + 1; ++ix)
//    {
//        if (ix == siz || order[ix] - order[ix - 1] != 1)
//        {
//            Range *r = new Range;
//            r->first = order[top];
//            r->last = order[ix - 1];
//            list.push_back(r);
//            top = ix;
//        }
//    }
//
//    itcache = list.end();
//}

smartvector<Range>::iterator RangeSelection::search(int index)
{
    auto itfrom = list.cbegin();
    auto itto = list.cend();

    if (itcache != list.cend())
    {
        const Range *r = *itcache;
        if (r->first > index)
            itto = itcache;
        else if (r->last < index)
            itfrom = std::next(itcache);
        else
            return list.begin() + (itcache - list.cbegin());
    }

    itcache = std::lower_bound(itfrom, itto, index, [](const Range *r, int val) {
        return r->last < val;
    });

    return list.begin() + (itcache - list.cbegin());
}

auto RangeSelection::search(int index) const -> smartvector<Range>::const_iterator
{
    //smartvector<Range> &r = *const_cast<smartvector<Range>*>(&list);

    auto itfrom = list.cbegin();
    auto itto = list.cend();

    if (itcache != list.cend())
    {
        const Range *r = *itcache;
        if (r->first > index)
            itto = itcache;
        else if (r->last < index)
            itfrom = std::next(itcache);
        else
            return itcache;
    }

    itcache = std::lower_bound(itfrom, itto, index, [](const Range *r, int val) {
        return r->last < val;
    });

    return itcache;
}


//-------------------------------------------------------------


void _rangeFromIndexes(const std::vector<int> &indexes, smartvector<Range> &ranges)
{
    for (int pos = 0, next = 1, siz = tosigned(indexes.size()); pos != siz; ++next)
    {
        if (next == siz || indexes[next] - indexes[next - 1] != 1)
        {
            ranges.push_back({ indexes[pos], indexes[next - 1] });
            pos = next;
        }
    }
}

int _intervalSize(const smartvector<Interval> &intervals)
{
    int cnt = 0;
    for (const Interval *i : intervals)
        cnt += i->count;
    return cnt;
}

int _rangeSize(const smartvector<Range> &list)
{
    int cnt = 0;
    for (const Range *r : list)
        cnt += r->last - r->first + 1;
    return cnt;
}

int _fixMovePos(const smartvector<Range> &list, int pos)
{
    auto it = std::upper_bound(list.begin(), list.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    });
    if (it == list.begin())
        return pos;
    return std::max((*std::prev(it))->last + 1, pos);
}

int _rangeOfPos(const smartvector<Range> &list, int pos)
{
    auto it = std::upper_bound(list.begin(), list.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    });
    if (it == list.begin())
        return -1;
    --it;
    if (pos > (*it)->last + 1)
        return -1;
    return it - list.begin();
}

void _fixMoveRanges(smartvector<Range> &list, int pos)
{
    if (list.empty())
        return;

    for (int ix = tosigned(list.size()) - 1; ix != 0; --ix)
    {
        if (list[ix - 1]->last >= list[ix]->first - 1)
        {
            // Overlapping ranges, "merge" them by extending the previous range and deleting
            // the next one.

            list[ix - 1]->last = std::max(list[ix - 1]->last, list[ix]->last);

            delete list[ix];
            list[ix] = nullptr;
        }
    }

    // std::remove() leaves duplicate values in ranges after the new end-iterator, that are
    // also found inside the ranges list. Shrinking the list would delete these and
    // cause access violation later. They have to be manually nulled out.
    auto newend = std::remove(list.begin(), list.end(), nullptr);
    for (auto it = newend; it != list.end(); ++it)
        *it = nullptr;

    if (pos != -1 && newend == std::next(list.begin()))
    {
        const Range *r = list[0];

        // Only one range remains, and it wouldn't be moved. Ranges should be cleared.
        if (pos >= r->first && pos <= r->last + 1)
            newend = list.begin();
    }

    list.shrink(newend - list.begin());
}

void _moveRanges(const smartvector<Range> &list, int pos, const std::function<void(const Range&, int)> &func)
{
    if (list.empty())
        return;

    // Moves the passed not overlapping ranges to pos. The result is the same as removing the
    // items found in ranges, and inserting them back in the same order in front of the item
    // originally at pos.

    // The ranges in front of pos are moved to pos backwards, then the ranges after pos are
    // moved there forward, so the indexes in ranges stay the same.

    // Index of first range after pos.
    int ix = std::upper_bound(list.begin(), list.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    }) - list.begin();

    // Moving ranges above pos:

    int movepos = ix == 0 ? pos : std::max(list[ix - 1]->last + 1, pos);
    for (int iy = ix, siz = tosigned(list.size()); iy != siz; ++iy)
    {
        const Range *r = list[iy];
        func(*r, movepos);
        movepos += r->last - r->first + 1;
    }

    // Moving ranges below pos:

    --ix;

    // Skip range touching or containing pos.
    if (ix != -1 && list[ix]->last + 1 >= pos)
    {
        movepos = list[ix]->first;
        --ix;
    }
    else
        movepos = pos;

    for (; ix != -1; --ix)
    {
        const Range *r = list[ix];
        func(*r, movepos);
        movepos -= r->last - r->first + 1;
    }
}

int _movedPosition(const smartvector<Range> &list, int pos, int itempos)
{
    if (list.empty() || (itempos < list.front()->first && pos > itempos) || (itempos > list.back()->last && pos <= itempos))
        return itempos;

    // Index of first range after pos.
    int ix = std::upper_bound(list.begin(), list.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    }) - list.begin();

    if (ix != 0)
    {
        const Range *r = list[ix - 1];

        // if itempos is within a range holding or touching pos, it wouldn't be moved.
        if (pos <= r->last + 1 && itempos >= r->first && itempos <= r->last)
            return itempos;
    }

    if (itempos >= pos)
    {
        // Emulating ranges moved down to pos.

        pos = ix == 0 ? pos : std::max(list[ix - 1]->last + 1, pos);

        for (int siz = tosigned(list.size()); ix != siz; ++ix)
        {
            const Range *r = list[ix];
            int cnt = r->last - r->first + 1;

            if (r->last < itempos)
            {
                pos += cnt;
                continue;
            }

            if (itempos < r->first)
                itempos += cnt;
            else
            {
                itempos -= r->first - pos;
                break;
            }

            pos += cnt;
        }
    }
    else
    {
        // itempos < pos

        // Making ix mark the range before pos.
        --ix;

        // Skip range touching or containing pos.
        if (ix != -1 && list[ix]->last + 1 >= pos)
        {
            pos = list[ix]->first;
            --ix;
        }

        for (; ix != -1; --ix)
        {
            const Range *r = list[ix];
            int cnt = r->last - r->first + 1;

            if (r->first > itempos)
            {
                pos -= cnt;
                continue;
            }

            if (itempos > r->last)
                itempos -= cnt;
            else
            {
                itempos += pos - r->last - 1;
                break;
            }

            pos -= cnt;
        }
    }

    return itempos;
}

int _insertedPosition(const smartvector<Interval> &intervals, int itempos)
{
    // First interval above itempos.
    int ix = std::upper_bound(intervals.begin(), intervals.end(), itempos, [](int itempos, const Interval *i) {
        return itempos < i->index;
    }) - intervals.begin();

    for (ix = ix - 1; ix != -1; --ix)
        itempos += intervals[ix]->count;
    return itempos;
}

int _removedPosition(const smartvector<Range> &list, int itempos, int itemcount)
{
    if (list.empty())
    {
        if (itemcount != -1 && itempos >= 0)
            return std::min(itempos, itemcount - 1);
        return itempos;
    }

    // First range above itempos.
    int ix = std::upper_bound(list.begin(), list.end(), itempos, [](int itempos, const Range *r) {
        return itempos < r->first;
    }) - list.begin();

    for (ix = ix - 1; ix != -1; --ix)
    {
        if (list[ix]->last >= itempos)
            itempos = list[ix]->first - 1;
        else
            itempos -= list[ix]->last - list[ix]->first + 1;
    }
    if (itemcount != -1 && itempos == -1 && list.back()->last < itemcount - 1)
        return 0;

    return itempos;
}

void _moveDelta(const smartvector<Range> &list, int pos, std::vector<int> &delta)
{
    if (list.empty())
        return;

    delta.resize(list.size());

    // Index of first range after pos.
    int ix = std::upper_bound(list.begin(), list.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    }) - list.begin();

    // Ranges above pos:

    int movepos = ix == 0 ? pos : std::max(list[ix - 1]->last + 1, pos);
    for (int iy = ix, siz = tosigned(list.size()); iy != siz; ++iy)
    {
        const Range *r = list[iy];
        delta[iy] = movepos - r->first;
        movepos += r->last - r->first + 1;
    }

    // Moving ranges below pos:

    --ix;

    // Skip range touching or containing pos.
    if (ix != -1 && list[ix]->last + 1 >= pos)
    {
        delta[ix] = 0;
        movepos = list[ix]->first;
        --ix;
    }
    else
        movepos = pos;

    for (; ix != -1; --ix)
    {
        const Range *r = list[ix];
        delta[ix] = movepos - r->last - 1;
        movepos -= r->last - r->first + 1;
    }
}


//-------------------------------------------------------------
