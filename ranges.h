/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef RANGES_H
#define RANGES_H

#include <functional>
#include "smartvector.h"

#ifdef _DEBUG
#include "checked_cast.h"
#endif

class ZAbstractTableModel;
class QPersistentModelIndex;

// General range structure for [first, last] ranges, where both ends are inclusive.
struct Range
{
    int first;
    int last;
};

struct Interval
{
    int index;
    int count;
};

class RangeSelection
{
public:
    RangeSelection();

    // Removes all ranges.
    void clear();

    // Returns whether there are no ranges in the selection.
    bool empty() const;

    // Returns the number of indexes covered by all ranges in the selection. If addfirst and
    // addlast are both specified (not -1), items between them will also be included in the
    // result.
    int size(int addfirst = -1, int addlast = -1) const;

    // Returns whether index is selected and it's the only selected item.
    bool singleSelected(int index) const;

    // Returns whether the item at index is marked as selected.
    bool selected(int index) const;
    // Changes the selected status of the item at index.
    void setSelected(int index, bool sel);
    // Fills result with individual items taken from the selection ranges. If addfirst and
    // addlast are both specified (not -1), items between them will also be included in the
    // result. The result list will be ordered by value.
    void getSelection(std::vector<int> &result, int addfirst = -1, int addlast = -1) const;

    // Fills result with the ranges that have parts in the range [first, last]. The
    // first and last ranges in result might only be partially in the passed range.
    void selectedRanges(int first, int last, std::vector<Range const *> &result) const;

    // Changes the selection to include or exclude the range [first, last], depending on the
    // value of sel.
    void selectRange(int first, int last, bool sel);

    // Returns whether every item in the range [first, last] is inside the selection.
    bool rangeSelected(int first, int last) const;

    // Returns the index of the selindex-th item marked as selected. If addfirst and addlast
    // are both specified (not -1), items between them are also considered selected.
    int selectedItems(int selindex, int addfirst = -1, int addlast = -1) const;

    // Number of ranges saved in the RangeSelection.
    int rangeCount() const;
    // Returns one range saved in the RangeSelection at ix position.
    const Range& ranges(int ix) const;

    // Updates the saved ranges, inserting cnt number of non-selected empty "items" at index,
    // updating the ranges coming after the inserted items, increasing their positions by cnt.
    // Returns whether the selection was changed.
    bool insert(int index, int cnt);

    // Updates the saved ranges, inserting intervals of non-selected empty "items" at the
    // passed intervals list, updating ranges as necessary. The intervals list must be ordered
    // by index and refer to coordinates before the insertion.
    // Returns whether the selection was changed.
    bool insert(const smartvector<Interval> &intervals);

    // Updates the saved ranges, removing items between first and last.
    // Returns whether the selection was changed.
    bool remove(const Range &range);

    // Removes the passed ranges from the selection. The ranges list must be ordered.
    // Returns whether the selection was changed.
    bool remove(const smartvector<Range> &ranges);

    // Updates the saved ranges, moving items between first and last to the new index. The new
    // index is a position before the move, and must be outside the moved range.
    // Returns whether the selection was changed.
    bool move(const Range &range, int newindex);

    // Moves selection between ranges to newindex. Has the effect of copying the selections of
    // ranges and merging them, removing them from the original selection, and then inserting
    // them back/ at the position that was newindex before the removal. 
    // Returns whether the selection was changed.
    bool move(const smartvector<Range> &ranges, int newindex);

    // Prepares for a layout change in model by saving every selected line into a persistent
    // model index.
    //void beginLayouting(ZAbstractTableModel *model);

    // Restores the selection ranges from the saved persistent model indexes.
    //void endLayouting(ZAbstractTableModel *model);
private:
    // Returns the iterator of the first range in ranges, that contains index. If such range
    // is not found, returns the iterator to the first range starting after index, or
    // if index comes after all the ranges, returns ranges.end().
    smartvector<Range>::iterator search(int index);

    // Returns the iterator of the first range in ranges, that contains index. If such range
    // is not found, returns the iterator to the first range starting after index, or
    // if index comes after all the ranges, returns ranges.end().
    smartvector<Range>::const_iterator search(int index) const;

    // Selection ranges. The list is ordered.
    smartvector<Range> list;

    // Used to store persistent model indexes of a model while its layout is changing. In any
    // other case this list should be empty.
    std::vector<QPersistentModelIndex> layoutindexes;

    // Speeds up searching if the access to items is linear, by saving the last iterator used
    // for looking up a position. If ranges changes, it must be set to ranges.end() to avoid
    // using it in an invalid state.
    mutable smartvector<Range>::const_iterator itcache;
};


void _rangeFromIndexes(const std::vector<int> &indexes, smartvector<Range> &ranges);

template<typename T>
void _rangeFromIndexes(const T* indexes, int indexcount, smartvector<Range> &ranges)
{
    for (int pos = 0, next = 1, siz = indexcount; pos != siz; ++next)
    {
        if (next == siz || indexes[next] - indexes[next - 1] != 1)
        {
            ranges.push_back({ indexes[pos], indexes[next - 1] });
            pos = next;
        }
    }
}

// Returns the total number of items inside the intervals.
int _intervalSize(const smartvector<Interval> &intervals);
// Returns the total number of items inside the ranges.
int _rangeSize(const smartvector<Range> &ranges);

// Returns a valid value of pos when ranges are to be moved to pos. If pos was inside a moved
// range, the return value is the first position coming after that range. Otherwise pos is
// returned unchanged.
int _fixMovePos(const smartvector<Range> &ranges, int pos);

// Returns the index of range in ranges which touches pos. A range touches a position if the
// position is same or higher as the first in a range but not higher than the last + 1 in the
// range. If such position is not found, the return value is -1.
int _rangeOfPos(const smartvector<Range> &ranges, int pos);

// Changes ranges, merging those that touch or overlap. The ranges must be ordered by their
// first value. When pos is specified, if only a single range remains which wouldn't be moved,
// because pos is inside or right after, ranges is cleared.
void _fixMoveRanges(smartvector<Range> &ranges, int pos = -1);

// Calls func for every range in ranges that need to be moved to newpos. The ranges are moved
// starting from the range above newpos down to newpos, then going backwards up to newpos.
// The move function should move one range to a passed position without altering the other
// range positions. The ranges vector must not be changed in func.
// The ranges shouldn't overlap nor touch, and must be ordered by their position.
void _moveRanges(const smartvector<Range> &ranges, int pos, const std::function<void(const Range&, int)> &func);

// Returns the new position of item if the items in ranges were moved to pos.
int _movedPosition(const smartvector<Range> &ranges, int pos, int itempos);

// Returns the new position of item if new items were inserted at the passed intervals.
int _insertedPosition(const smartvector<Interval> &intervals, int itempos);

// Returns the new value of itempos if items in ranges were removed. If item was inside one of
// the removed ranges, the new position of the first, not removed value above that range is
// returned.
// Specify itemcount to the original count of items to return 0, when itempos would become -1,
// but there would still remain items after the removal of ranges.
int _removedPosition(const smartvector<Range> &ranges, int itempos, int itemcount = -1);

// Computes how many positions each range in ranges would be moved forward or backward and
// fills the delta list with the result. The delta list will hold as many elements as ranges,
// positive numbers meaning forward move, negative numbers for backward move.
void _moveDelta(const smartvector<Range> &ranges, int pos, std::vector<int> &delta);

// Moves values in a list from the range [first, last] to pos.
template<typename LIST>
void _moveRange(LIST &list, Range range, int pos)
{
#ifdef _DEBUG
    if (range.first > range.last || range.first < 0 || std::max(range.first, range.last) >= tosigned(list.size()) || (pos >= range.first && pos <= range.last + 1))
        throw "Invalid parameters.";
#endif

    // Change parameters if needed to move the least number of items.

    if (pos < range.first && range.last - range.first + 1 > range.first - pos)
    {
        int tmp = range.first;
        range.first = pos;
        pos = range.last + 1;
        range.last = tmp - 1;
    }
    else if (pos > range.last && range.last - range.first + 1 > pos - 1 - range.last)
    {
        int tmp = range.last;
        range.last = pos - 1;
        pos = range.first;
        range.first = tmp + 1;
    }

    std::vector<typename LIST::value_type> movelist(list.begin() + range.first, list.begin() + (range.last + 1));

    auto *ldat = list.data();
    auto *mdat = movelist.data();

    if (pos < range.first)
    {
        // Move items.
        for (int ix = 0, siz = range.first - pos; ix != siz; ++ix)
            ldat[range.last - ix] = ldat[range.first - 1 - ix];

        // Copy movelist back.
        for (int ix = 0, siz = tosigned(movelist.size()); ix != siz; ++ix)
            ldat[pos + ix] = mdat[ix];
    }
    else
    {
        // Move items.
        for (int ix = 0, siz = pos - range.last - 1; ix != siz; ++ix)
            ldat[range.first + ix] = ldat[range.last + 1 + ix];

        // Copy movelist back.
        for (int ix = 0, siz = tosigned(movelist.size()); ix != siz; ++ix)
            ldat[pos - siz + ix] = mdat[ix];
    }
}

// Moves values in an array of arbitrary data type from the range [first, last] to pos.
template<typename T>
void _moveRange(T *array,
#ifdef _DEBUG
    int arraysize,
#endif
    Range range, int pos)
{
#ifdef _DEBUG
    if (range.first > range.last || range.first < 0 || std::max(range.first, range.last) >= arraysize || (pos >= range.first && pos <= range.last + 1))
        throw "Invalid parameters.";
#endif

    // Change parameters if needed to move the least number of items.

    if (pos < range.first && range.last - range.first + 1 > range.first - pos)
    {
        int tmp = range.first;
        range.first = pos;
        pos = range.last + 1;
        range.last = tmp - 1;
    }
    else if (pos > range.last && range.last - range.first + 1 > pos - 1 - range.last)
    {
        int tmp = range.last;
        range.last = pos - 1;
        pos = range.first;
        range.first = tmp + 1;
    }

    std::vector</*typename*/ T> movelist(array + range.first, array + (range.last + 1));

    //auto *ldat = list.data();
    auto *mdat = movelist.data();

    if (pos < range.first)
    {
        // Move items.
        for (int ix = 0, siz = range.first - pos; ix != siz; ++ix)
            array[range.last - ix] = array[range.first - 1 - ix];

        // Copy movelist back.
        for (int ix = 0, siz = range.last - range.first + 1 /*movelist.size()*/; ix != siz; ++ix)
            array[pos + ix] = mdat[ix];
    }
    else
    {
        // Move items.
        for (int ix = 0, siz = pos - range.last - 1; ix != siz; ++ix)
            array[range.first + ix] = array[range.last + 1 + ix];

        // Copy movelist back.
        for (int ix = 0, siz = range.last - range.first + 1 /*movelist.size()*/; ix != siz; ++ix)
            array[pos - siz + ix] = mdat[ix];
    }
}

// Updates list by moving its members in ranges to the pos position. The function uses
// _moveRange, passing the list whose members are to be moved as first parameter. Returns
// whether anything was moved.
template<typename LIST>
bool _moveRanges(const smartvector<Range> &ranges, int pos, LIST &moved)
{
    if (ranges.empty())
        return false;

    // Index of first range after pos.
    int ix = tosigned(std::upper_bound(ranges.begin(), ranges.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    }) - ranges.begin());

    // Moving ranges above pos:

    bool changed = false;

    int movepos = ix == 0 ? pos : std::max(ranges[ix - 1]->last + 1, pos);
    for (int iy = ix, siz = tosigned(ranges.size()); iy != siz; ++iy)
    {
        const Range *r = ranges[iy];
        _moveRange(moved, *r, movepos);
        movepos += r->last - r->first + 1;

        changed = true;
    }

    // Moving ranges below pos:

    --ix;

    // Skip range touching or containing pos.
    if (ix != -1 && ranges[ix]->last + 1 >= pos)
    {
        movepos = ranges[ix]->first;
        --ix;
    }
    else
        movepos = pos;

    for (; ix != -1; --ix)
    {
        const Range *r = ranges[ix];
        _moveRange(moved, *r, movepos);
        movepos -= r->last - r->first + 1;

        changed = true;
    }

    return changed;
}

template<typename T>
void _moveRanges(const smartvector<Range> &ranges, int pos, T *array
#ifdef _DEBUG
    , int arraysize
#endif
    )
{
    if (ranges.empty())
        return;

    // Index of first range after pos.
    int ix = std::upper_bound(ranges.begin(), ranges.end(), pos, [](int pos, const Range *r) {
        return pos < r->first;
    }) - ranges.begin();

    // Moving ranges above pos:

    int movepos = ix == 0 ? pos : std::max(ranges[ix - 1]->last + 1, pos);
    for (int iy = ix, siz = ranges.size(); iy != siz; ++iy)
    {
        const Range *r = ranges[iy];
        _moveRange(array,
#ifdef _DEBUG
            arraysize,
#endif
            *r, movepos);
        movepos += r->last - r->first + 1;
    }

    // Moving ranges below pos:

    --ix;

    // Skip range touching or containing pos.
    if (ix != -1 && ranges[ix]->last + 1 >= pos)
    {
        movepos = ranges[ix]->first;
        --ix;
    }
    else
        movepos = pos;

    for (; ix != -1; --ix)
    {
        const Range *r = ranges[ix];
        _moveRange(array, 
#ifdef _DEBUG
            arraysize,
#endif
            *r, movepos);
        movepos -= r->last - r->first + 1;
    }
}



#endif // RANGES_H

