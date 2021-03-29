/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPainter>
#include <QStylePainter>
#include <QtEvents>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QMenu>
#include <QDrag>
#include <QMimeData>
#include <QPixmap>
#include <QPainterPath>

#include <cmath>

#include "zkanjigridview.h"
#include "ranges.h"
#include "zkanjimain.h"
#include "groups.h"
#include "zkanjigridmodel.h"
#include "words.h"
#include "kanji.h"
#include "zui.h"
#include "dialogs.h"
#include "collectwordsform.h"
#include "kanjidefform.h"
#include "fontsettings.h"
#include "colorsettings.h"
#include "globalui.h"
#include "ztooltip.h"
#include "kanjisettings.h"
#include "zkanjiform.h"
#include "zkanjiwidget.h"
#include "generalsettings.h"
#include "zstatusbar.h"


//-------------------------------------------------------------


ZKanjiGridView::ZKanjiGridView(QWidget *parent) : base(parent), itemmodel(nullptr), connected(false), dict(ZKanji::dictionary(0))/*, popup(nullptr)*/, status(nullptr),
        state(State::None), cellsize(Settings::scaled(std::ceil(Settings::fonts.kanjifontsize / 0.7))), autoscrollmargin(24), cols(0), rows(0), mousedown(false),
        current(-1), selpivot(-1), selection(new RangeSelection), kanjitipcell(-1), kanjitipkanji(-1), dragind(-1)
{
    setAcceptDrops(true);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    recompute(viewport()->size());
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);

    setMouseTracking(true);

    //popup = new QMenu(this);
    //QAction *a;

    //a = popup->addAction(tr("Kanji information..."));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::showKanjiInfo);
    //popup->addSeparator();
    //a = popup->addAction(tr("Add to group..."));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::kanjiToGroup);
    //a = popup->addAction(tr("Collect words..."));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::collectWords);
    //popup->addSeparator();
    //a = popup->addAction(tr("Edit definition..."));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::definitionEdit);
    //popup->addSeparator();
    //a = popup->addAction(tr("Copy to clipboard"));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::copyKanji);
    //a = popup->addAction(tr("Append to clipboard"));
    //connect(a, &QAction::triggered, this, &ZKanjiGridView::appendKanji);

    connect(gUI, &GlobalUI::settingsChanged, this, &ZKanjiGridView::settingsChanged);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &ZKanjiGridView::dictionaryToBeRemoved);

    Settings::updatePalette(this);
}

ZKanjiGridView::~ZKanjiGridView()
{
    if (itemmodel != nullptr && connected)
        disconnect(itemmodel, 0, this, 0);
}

void ZKanjiGridView::setModel(KanjiGridModel *newmodel)
{
    if (itemmodel == newmodel)
        return;

    if (connected && itemmodel != nullptr)
        disconnect(itemmodel, 0, this, 0);

    itemmodel = newmodel;
    connected = false;

    if (itemmodel != nullptr)
    {
        connect(itemmodel, &KanjiGridModel::modelReset, this, &ZKanjiGridView::reset);
        connect(itemmodel, &KanjiGridModel::itemsInserted, this, &ZKanjiGridView::itemsInserted);
        connect(itemmodel, &KanjiGridModel::itemsRemoved, this, &ZKanjiGridView::itemsRemoved);
        connect(itemmodel, &KanjiGridModel::itemsMoved, this, &ZKanjiGridView::itemsMoved);
        connect(itemmodel, &KanjiGridModel::layoutToChange, this, &ZKanjiGridView::layoutToChange);
        connect(itemmodel, &KanjiGridModel::layoutChanged, this, &ZKanjiGridView::layoutChanged);
        connect(itemmodel, &QObject::destroyed, this, &ZKanjiGridView::modelDestroyed);

        connected = true;
    }

    reset();
}

Dictionary* ZKanjiGridView::dictionary() const
{
    return dict;
}

void ZKanjiGridView::setDictionary(Dictionary *newdict)
{
    if (dict == newdict)
        return;
    dict = newdict;
    viewport()->update();
}

void ZKanjiGridView::executeCommand(int command)
{
    if (command < (int)CommandLimits::KanjiBegin || command >= (int)CommandLimits::KanjiEnd)
        return;

    switch (command)
    {
    case (int)Commands::ShowKanjiInfo:
        showKanjiInfo();
        break;
    case (int)Commands::KanjiToGroup:
        kanjiToGroup();
        break;
    case (int)Commands::EditKanjiDef:
        definitionEdit();
        break;
    case (int)Commands::CollectKanjiWords:
        collectWords();
        break;
    case (int)Commands::CopyKanji:
        copyKanji();
        break;
    case (int)Commands::AppendKanji:
        appendKanji();
        break;
    }
}

void ZKanjiGridView::commandState(int command, bool &enabled, bool &checked, bool &visible) const
{
    if (command < (int)CommandLimits::KanjiBegin || command >= (int)CommandLimits::KanjiEnd)
        return;

    enabled = true;
    checked = false;
    visible = true;

    switch (command)
    {
    case (int)Commands::ShowKanjiInfo:
        enabled = selCount() == 1;
        break;
    case (int)Commands::KanjiToGroup:
        enabled = selCount() != 0;
        break;
    case (int)Commands::EditKanjiDef:
        enabled = selCount() != 0 && dict != ZKanji::dictionary(0);
        break;
    case (int)Commands::CollectKanjiWords:
        enabled = selCount() != 0;
        break;
    case (int)Commands::CopyKanji:
        enabled = selCount() != 0;
        break;
    case (int)Commands::AppendKanji:
        enabled = selCount() != 0;
        break;
    }
}

int ZKanjiGridView::cellSize()
{
    return cellsize;
}

//void ZKanjiGridView::setCellSize(int newsize)
//{
//    if (cellsize == newsize || newsize < 25)
//        return;
//
//    cellsize = newsize;
//
//    recompute(viewport()->size());
//    recomputeScrollbar(viewport()->size());
//
//    viewport()->update();
//}

void ZKanjiGridView::settingsChanged()
{
    Settings::updatePalette(this);

    cellsize = Settings::scaled(std::ceil(Settings::fonts.kanjifontsize / 0.7));
    recompute(viewport()->size());
    recompute(viewport()->size());
    recomputeScrollbar(viewport()->size());

    viewport()->update();
}

void ZKanjiGridView::dictionaryToBeRemoved(int /*oldix*/, int /*newix*/, Dictionary *d)
{
    if (connected && dict == d && itemmodel != nullptr)
    {
        disconnect(itemmodel, 0, this, 0);
        connected = false;
    }
}

void ZKanjiGridView::reset()
{
    selection->clear();
    selpivot = -1;

    //int oldcurrent = current;
    int newcurrent;

    //lastkeyindex = -1;
    //_clearsel();
    // TODO: (later?) allow no select mode in grid.
    if (itemmodel != nullptr && !itemmodel->empty())
    {
        selection->setSelected(0, true);
        newcurrent = 0;
    }
    else
        newcurrent = -1;

    recompute(viewport()->size());
    recomputeScrollbar(viewport()->size());

    if (status != nullptr)
        status->clear();

    current = -1;
    setAsCurrent(newcurrent);
    if (newcurrent == -1)
        updateStatus();

    updateCommands();

    //emit currentChanged(current, oldcurrent);
    emit selectionChanged();

    viewport()->update();
}

void ZKanjiGridView::itemsInserted(const smartvector<Interval> &intervals)
{
//#ifdef _DEBUG
//    if (first > last)
//        throw "Invalid range for itemsAdded.";
//#endif

    recompute(viewport()->size());
    recomputeScrollbar(viewport()->size());

    bool selchange = selection->insert(intervals);

    int oldcurrent = current;
    if (current != -1)
        current = _insertedPosition(intervals, current);

    selpivot = -1;

    updateCommands();
    if (oldcurrent != current)
        emit currentChanged(current, oldcurrent);

    if (selchange)
        emit selectionChanged();

    viewport()->update();
}

//void ZKanjiGridView::beginItemsRemove(int first, int last)
//{
//#ifdef _DEBUG
//    if (first > last)
//        throw "Invalid range for itemsRemoved.";
//    if (removefirstpos != -1 || removelastpos != -1)
//        throw "Nested calls not supported.";
//#endif
//
//    removefirstpos = first;
//    removelastpos = last;
//}

void ZKanjiGridView::itemsRemoved(const smartvector<Range> &ranges)
{
//#ifdef _DEBUG
//    if (removefirstpos == -1 || removelastpos == -1)
//        throw "Call beginItemsRemove() first.";
//#endif

    // Saving and resetting the values to avoid nesting call of these functions.
    //int first = removefirstpos;
    //removefirstpos = -1;
    //int last = removelastpos;
    //removelastpos = -1;

    //selection->remove(first, last);

    //selpivot = -1;
    //if (current > last)
    //    current -= last - first + 1;
    //else if (current >= first)
    //{
    //    if (model()->empty())
    //        current = -1;
    //    else
    //        current = std::max(0, first - 1);
    //}

    bool selchange = selection->remove(ranges);
    selpivot = -1;

    //int oldcurrent = current;
    int newcurrent = current;

    if (current != -1)
        newcurrent = _removedPosition(ranges, current);

    if (current == -1 && !model()->empty())
        newcurrent = 0;

    //if (oldcurrent != current)
    //    emit currentChanged(current, oldcurrent);
    setAsCurrent(newcurrent);

    updateCommands();

    if (selchange)
        emit selectionChanged();

    viewport()->update();
}

//void ZKanjiGridView::beginItemsMove(int first, int last, int pos)
//{
//#ifdef _DEBUG
//    if (first > last)
//        throw "Invalid range for itemsRemoved.";
//    if (removefirstpos != -1 || removelastpos != -1 || movepos != -1)
//        throw "Nested calls not supported.";
//    if (pos > first && pos <= last)
//        throw "Move position must be outside move range";
//#endif
//
//    removefirstpos = first;
//    removelastpos = last;
//    movepos = pos;
//}

void ZKanjiGridView::itemsMoved(const smartvector<Range> &ranges, int pos)
{
//#ifdef _DEBUG
//    if (removefirstpos == -1 || removelastpos == -1 || movepos == -1)
//        throw "Call beginItemsMove() first.";
//#endif

    // Saving and resetting the values to avoid nesting call of these functions.
    //int first = removefirstpos;
    //removefirstpos = -1;
    //int last = removelastpos;
    //removelastpos = -1;
    //int pos = movepos;
    //movepos = -1;

    bool selchange = selection->move(ranges, pos);

    selpivot = -1;

    int oldcurrent = current;

    if (current != -1)
        current = _movedPosition(ranges, pos, current);

    if (oldcurrent != current)
        emit currentChanged(current, oldcurrent);

    if (selchange)
        emit selectionChanged();

    viewport()->update();
}

void ZKanjiGridView::layoutToChange()
{
    selpivot = -1;
    
    std::vector<int> persel;
    persel.reserve(selection->size() + 1);
    for (int ix = 0, siz = selection->rangeCount(); ix != siz; ++ix)
    {
        const Range &r = selection->ranges(ix);
        for (int iy = r.first, sizy = r.last + 1; iy != sizy; ++iy)
            persel.push_back(iy);
    }
    persel.push_back(current);
    persistentid = model()->savePersistentList(persel);
    selection->clear();
}

void ZKanjiGridView::layoutChanged()
{
    std::vector<int> persel;
    model()->restorePersistentList(persistentid, persel);

    int oldcurrent = current;
    current = persel.back();
    persel.pop_back();

    std::sort(persel.begin(), persel.end());

    for (int pos = 0, next = 1, siz = tosigned(persel.size()); pos != siz; ++next)
    {
        if (next == siz || persel[next] - persel[next - 1] > 1)
        {
            selection->selectRange(persel[pos], persel[next - 1], true);
            pos = next;
        }
    }

    scrollTo(current);

    updateCommands();

    if (oldcurrent != current)
        emit currentChanged(current, oldcurrent);

    emit selectionChanged();

    viewport()->update();
}

void ZKanjiGridView::selectAll()
{
    if (model() == nullptr || selection->rangeSelected(0, tosigned(model()->size()) - 1))
        return;

    selpivot = -1;
    selection->selectRange(0, tosigned(model()->size()) - 1, true);

    viewport()->update();

    updateCommands();

    emit selectionChanged();
}

void ZKanjiGridView::clearSelection()
{
    if (model() == nullptr || selection->empty())
        return;

    selpivot = -1;
    selection->clear();

    viewport()->update();

    updateCommands();

    emit selectionChanged();
}

void ZKanjiGridView::modelDestroyed()
{
    itemmodel = nullptr;
    connected = false;
    reset();
}

KanjiGridModel* ZKanjiGridView::model() const
{
    return itemmodel;
}

void ZKanjiGridView::assignStatusBar(ZStatusBar *bar)
{
    if (bar == status)
        return;
    if (status != nullptr)
        disconnect(status, nullptr, this, nullptr);
    status = bar;
    if (status != nullptr)
    {
        connect(status, &QObject::destroyed, this, &ZKanjiGridView::statusDestroyed);
        status->assignTo(this);
        connect(status, &ZStatusBar::assigned, this, &ZKanjiGridView::statusDestroyed);
    }

    updateStatus();
}

ZStatusBar* ZKanjiGridView::statusBar() const
{
    return status;
}

int ZKanjiGridView::selCount() const
{
    return selection->size();
}

bool ZKanjiGridView::selected(int index) const
{
    return selection->selected(index);
}

void ZKanjiGridView::setSelected(int index, bool sel)
{
    if (selection->selected(index) == sel)
        return;

    selection->setSelected(index, sel);

    updateCell(index);

    updateCommands();

    emit selectionChanged();
}

bool ZKanjiGridView::continuousSelection() const
{
    return selection->rangeCount() == 1;
}

int ZKanjiGridView::cellAt(const QPoint &p) const
{
    return cellAt(p.x(), p.y());
}

int ZKanjiGridView::cellAt(int x, int y) const
{
    int vpos = verticalScrollBar()->value();
    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top row. y <= 0.
    int yshift = top * cellsize - vpos;

    int col = x / cellsize;
    int row = (y - yshift) / cellsize + top;

    if (col >= cols || row >= rows || (cols * row) + col >= tosigned(itemmodel->size()))
        return -1;

    return (cols * row) + col;
}

QRect ZKanjiGridView::cellRect(int index) const
{
    if (index < 0 || index >= tosigned(itemmodel->size()))
        return QRect();

    int vpos = verticalScrollBar()->value();
    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top row. y <= 0.
    int yshift = top * cellsize - vpos;

    QSize size = viewport()->size();

    if (index < top * cols || index >= (top + ((size.height() - yshift + cellsize - 1) / cellsize)) * cols)
        return QRect();

    int row = index / cols;
    int col = index % cols;

    return QRect(col * cellsize, (row - top) * cellsize + yshift, cellsize, cellsize);
}

int ZKanjiGridView::cellRowAt(int y) const
{
    int vpos = verticalScrollBar()->value();
    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top row. y <= 0.
    int yshift = top * cellsize - vpos;

    int row = (y - yshift) / cellsize + top;

    if (row >= rows)
        return -1;

    return row;
}

void ZKanjiGridView::select(int index)
{
#ifdef _DEBUG
    if (index < 0 || model() == nullptr || index >= tosigned(model()->size()))
        throw "Invalid index to select.";
#endif

    updateSelected();

    selection->clear();
    selection->setSelected(index, true);

    //int oldcurrent = current;
    int newcurrent = index;
    scrollIn(index);
    //updateCell(oldcurrent);
    //updateCell(index);

    //if (oldcurrent != current)
    //    emit currentChanged(current, oldcurrent);
    setAsCurrent(newcurrent);

    updateCommands();

    emit selectionChanged();
}

bool ZKanjiGridView::toggleSelect(int index)
{
#ifdef _DEBUG
    if (index < 0 || model() == nullptr || index >= tosigned(model()->size()))
        throw "Invalid index to select.";
#endif

    if (selection->size() == 1 && selection->selected(index))
        return true;

    //int oldcurrent = current;
    int newcurrent = index;

    bool sel = !selection->selected(index);
    selection->setSelected(index, sel);
    scrollIn(index);
    //updateCell(oldcurrent);
    //updateCell(index);

    //if (oldcurrent != current)
    //    emit currentChanged(current, oldcurrent);
    if (current != -1)
        updateCell(current);
    if (current != newcurrent)
        setAsCurrent(newcurrent);
    else
    {
        updateCommands();

        if (current != -1)
            updateCell(current);
    }

    emit selectionChanged();

    return sel;
}

void ZKanjiGridView::multiSelect(int endindex, bool deselect)
{
    if (selpivot == -1)
        selpivot = current;

    int first = std::min(selpivot, endindex);
    int last = std::max(selpivot, endindex);

    std::vector<const Range *> r;

    scrollIn(endindex);

    if (deselect)
    {
        // Everything outside the selected area will be deselected. The cells currently
        // selected there must be repainted when clearing the selection. Go through the
        // selected ranges and update the cells inside them.

        if (first > 0)
        {
            selection->selectedRanges(0, first - 1, r);
            if (!r.empty())
            {
                for (int ix = 0, siz = tosigned(r.size()); ix != siz; ++ix)
                {
                    const Range *range = r[ix];
                    int to = std::min(first, range->last + 1);
                    for (int iy = range->first; iy != to; ++iy)
                        updateCell(iy);
                }

                r.clear();
            }
        }

        if (last < tosigned(model()->size()) - 1)
        {
            selection->selectedRanges(last + 1, tosigned(model()->size()) - 1, r);
            if (!r.empty())
            {
                for (int ix = 0, siz = tosigned(r.size()); ix != siz; ++ix)
                {
                    const Range *range = r[ix];
                    int from = std::max(last + 1, range->first);
                    for (int iy = from; iy != range->last + 1; ++iy)
                        updateCell(iy);
                }

                r.clear();
            }
        }
    }

    // Update the cells in the new selection that were not selected before.
    selection->selectedRanges(first, last, r);

    if (r.empty())
    {
        for (int ix = first; ix != last + 1; ++ix)
            updateCell(ix);
    }
    else
    {
        int pos = first;
        for (int ix = 0, siz = tosigned(r.size()); ix != siz && pos <= last; ++ix)
        {
            const Range *range = r[ix];
            for (int iy = pos; iy < range->first; ++iy)
                updateCell(iy);
            pos = range->last + 1;
        }
        if (pos <= last)
        {
            for (int iy = pos; iy != last + 1; ++iy)
                updateCell(iy);
        }
    }

    if (deselect)
        selection->clear();
    selection->selectRange(first, last, true);

    int oldcurrent = current;
    int newcurrent = endindex;
    //updateCell(oldcurrent);
    //updateCell(current);

    //if (oldcurrent != current)
    //    emit currentChanged(current, oldcurrent);
    setAsCurrent(newcurrent);

    updateCommands();
    if (oldcurrent != newcurrent)
        updateStatus();

    emit selectionChanged();
}

int ZKanjiGridView::selectedCell(int pos) const
{
    return selection->selectedItems(pos);
}

void ZKanjiGridView::selectedCells(std::vector<int> &result) const
{
    //result.reserve(selection->size());
    //int pos = 0;
    //for (int ix = 0; ix != selection->rangeCount(); ++ix)
    //{
    //    auto &r = selection->ranges(ix);
    //    for (int iy = r.from; iy != r.to + 1; ++iy)
    //        result.push_back(iy);
    //}

    selection->getSelection(result);
}

void ZKanjiGridView::selKanji(std::vector<ushort> &result) const
{
    result.reserve(selection->size());
    //int pos = 0;
    for (int ix = 0; ix != selection->rangeCount(); ++ix)
    {
        auto &r = selection->ranges(ix);
        for (int iy = r.first, last = r.last + 1; iy != last; ++iy)
            result.push_back(itemmodel->kanjiAt(iy));
    }
}

QString ZKanjiGridView::selString() const
{
    std::vector<ushort> selkanji;
    selKanji(selkanji);

    QString str;
    str.resize(tosigned(selkanji.size()));
    for (int ix = 0, siz = tosigned(selkanji.size()); ix != siz; ++ix)
        str[ix] = ZKanji::kanjis[selkanji[ix]]->ch;

    return str;
}

void ZKanjiGridView::updateCell(int index)
{
    QRect cr = cellRect(index);

    if (cr.isEmpty())
        return;

    viewport()->update(cr);
}

void ZKanjiGridView::updateSelected()
{
    for (int ix = 0; ix != selection->rangeCount(); ++ix)
    {
        auto &range = selection->ranges(ix);
        for (int iy = range.first, last = range.last + 1; iy != last; ++iy)
        {
            QRect r = cellRect(iy);
            if (!r.isEmpty())
                viewport()->update(r);
        }
    }
}

void ZKanjiGridView::scrollTo(int index, bool forcemid)
{
    int vpos = verticalScrollBar()->value();
    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top row. y <= 0.
    int yshift = top * cellsize - vpos;

    QSize size = viewport()->size();

    int row = index / cols;

    if ((row < top && yshift < 0) || ((size.height() - yshift) / cellsize + top < row))
        forcemid = true;

    if (!forcemid)
    {
        if ((row == top && yshift == 0) || (row > top && row < top + (size.height() - yshift) / cellsize))
            return;

        if (row <= top)
            verticalScrollBar()->setValue(row * cellsize);
        else
            verticalScrollBar()->setValue(row * cellsize - viewport()->height() + cellsize);
    }
    else
        verticalScrollBar()->setValue(row * cellsize - (viewport()->height() - cellsize) / 2);
}

void ZKanjiGridView::scrollIn(int index)
{
    int vpos = verticalScrollBar()->value();
    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top row. y <= 0.
    int yshift = top * cellsize - vpos;

    QSize size = viewport()->size();

    int row = index / cols;

    if ((row == top && yshift == 0) || (row > top && row < top + (size.height() - yshift) / cellsize))
        return;

    if (row <= top)
        verticalScrollBar()->setValue(row * cellsize);
    else
        verticalScrollBar()->setValue(row * cellsize - viewport()->height() + cellsize);
}

int ZKanjiGridView::autoScrollMargin() const
{
    return autoscrollmargin;
}

void ZKanjiGridView::setAutoScrollMargin(int val)
{
    autoscrollmargin = val;
}

void ZKanjiGridView::startAutoScroll()
{
    if (autoScrollTimer.isActive())
        return;

    int scrollInterval = /*(verticalScrollMode() == QAbstractItemView::ScrollPerItem) ? 150 :*/ 50;
    autoScrollTimer.start(scrollInterval, this);
    autoScrollCount = 0;
}

void ZKanjiGridView::stopAutoScroll()
{
    autoScrollTimer.stop();
    autoScrollCount = 0;
}

void ZKanjiGridView::doAutoScroll()
{
    QRect vr = viewport()->rect();
    QPoint cp = viewport()->mapFromGlobal(QCursor::pos());

    if (cp.y() >= vr.top() + autoScrollMargin() && cp.y() <= vr.bottom() - autoScrollMargin())
    {
        stopAutoScroll();
        return;
    }

    int vertpos = verticalScrollBar()->sliderPosition();

    int change = cp.y() < vr.top() + autoScrollMargin() ? -1 : 1;

    if (autoScrollCount < verticalScrollBar()->pageStep() / 5)
        autoScrollCount += 2;

    autoScrollCount = std::max(1, autoScrollCount);

    verticalScrollBar()->setSliderPosition(vertpos + change * autoScrollCount);

    if (vertpos == verticalScrollBar()->sliderPosition())
        stopAutoScroll();
    else
        viewport()->update();
}

void ZKanjiGridView::removeSelected()
{
    std::vector<int> indexes;
    selectedCells(indexes);

    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);

    model()->kanjiGroup()->remove(ranges);
}

bool ZKanjiGridView::cancelActions()
{
    bool cancelled = state == State::CanDrag || mousedown;
    if (state == State::CanDrag)
        state = State::None;
    mousedown = false;

    return cancelled;
}

void ZKanjiGridView::scrollContentsBy(int dx, int dy)
{
    base::scrollContentsBy(dx, dy);

    if (state != State::Dragging)
        return;

    QDragMoveEvent e = QDragMoveEvent(viewport()->mapFromGlobal(QCursor::pos()), savedDropActions, savedMimeData, savedMouseButtons, savedKeyboardModifiers);
    dragMoveEvent(&e);
}

bool ZKanjiGridView::viewportEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::Leave:
    case QEvent::HoverLeave:
        ZToolTip::startHide();
        kanjitipcell = -1;
        kanjitipkanji = -1;
        break;
    case QEvent::LanguageChange:
        if (status != nullptr)
            status->clear();
        updateStatus();
        break;
    default:
        break;
    }

    return base::viewportEvent(e);
}

void ZKanjiGridView::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == autoScrollTimer.timerId())
    {
        doAutoScroll();
        return;
    }

    base::timerEvent(e);
}

void ZKanjiGridView::paintEvent(QPaintEvent *event)
{
    QStylePainter p(viewport());
    const QSize &size = viewport()->size();
    QStyleOptionViewItem opts;
    opts.initFrom(this);
    opts.showDecorationSelected = true;
    QBrush oldbrush = p.brush();

    if (itemmodel == nullptr || itemmodel->empty())
    {
        p.setBrush(Settings::textColor(this, ColorSettings::Bg));
        p.fillRect(event->rect(), p.brush());
        p.setBrush(oldbrush);
        return;
    }

    QPen oldpen = p.pen();
    QFont oldfont = p.font();

    QColor gridColor = Settings::uiColor(ColorSettings::Grid);
    QColor textcolor = Settings::textColor(this, ColorSettings::Text);
    QColor seltextcolor = Settings::textColor(this, ColorSettings::SelText);
    QColor bgcolor = Settings::textColor(this, ColorSettings::Bg);
    QColor selcolor = Settings::textColor(this, ColorSettings::SelBg);

    p.setPen(gridColor);
    
    int vpos = verticalScrollBar()->value();

    // First grid row partially or fully visible at the top at the current scroll position.
    int top = vpos / cellsize;
    // Upper Y coordinate of visible top cells to be drawn. y <= 0.
    int y = top * cellsize - vpos;
    // Lower Y coordinate of visible bottom cell to be drawn. 0 <= y2 <= size.height()
    int y2 = std::min((rows - top) * cellsize + y, size.height());

    // Number of columns at the bottom-most visible row.
    int bcols = ((rows - top - 1) * cellsize + y > size.height()) ? cols : cols - (cols * rows - tosigned(itemmodel->size()));

    // Draw the vertical grid lines till the lowest visible row's bottom.
    int x = cellsize - 1;
    for (int ix = 0; ix != bcols; ++ix, x += cellsize)
        p.drawLine(x, 0, x, y2 - 1);

    // Draws the rest of the vertical grid lines.
    if (bcols != cols)
    {
        y2 = std::min((rows - top - 1) * cellsize + y, size.height());
        for (int ix = bcols; ix != cols; ++ix, x += cellsize)
            p.drawLine(x, 0, x, y2 - 1);
    }

    // Fill background to the right of the last grid line.
    p.setBrush(bgcolor);
    if (cols * cellsize < size.width())
        p.fillRect(QRect(QPoint(cols * cellsize, 0), QPoint(size.width(), y2)), p.brush());

    if (bcols != cols)
    {
        p.fillRect(QRect(QPoint(bcols * cellsize, y2), QPoint(size.width(), y2 + cellsize)), p.brush());
        y2 += cellsize;
    }
    if (y2 < size.height())
        p.fillRect(QRect(QPoint(0, y2), QPoint(size.width(), size.height())), p.brush());

    // Draw the horizontal grid lines.
    x -= cellsize;
    y += cellsize;
    for (int ix = top; ix != rows - 1 && y - 1 < size.height(); ++ix, y += cellsize)
        p.drawLine(0, y - 1, x, y - 1);
    if (y - 1 < size.height())
        p.drawLine(0, y - 1, bcols * cellsize - 1, y - 1);

    // Reset positions for cell drawing.

    y = top * cellsize - vpos;
    x = 0;
    // Current element of the group.
    int drawpos = top * cols;

    QFont kfont = Settings::kanjiFont();

    p.setFont(kfont);

    // Fill the backgrounds and draw the kanji.
    for (int isiz = tosigned(itemmodel->size()); drawpos != isiz && y < size.height(); ++drawpos, x += cellsize)
    {
        if (x != 0 && x + cellsize > size.width())
        {
            x = 0;
            y += cellsize;
        }


        if (!event->rect().intersects(QRect(x, y, cellsize - 1, cellsize - 1)))
            continue;

        bool sel = selected(drawpos);
        if (!sel)
        {
            QColor c = itemmodel->textColorAt(drawpos);
            if (!c.isValid())
            {
                if (dict->kanjiWordCount(itemmodel->kanjiAt(drawpos)) == 0)
                    c = Settings::uiColor(ColorSettings::KanjiNoWords);
                else
                    c = textcolor; // opts.palette.color(colorgrp, QPalette::Text);
            }
            p.setPen(c);
            c = itemmodel->backColorAt(drawpos);
            if (!c.isValid())
                c = bgcolor; // opts.palette.color(colorgrp, QPalette::Base);
            p.setBrush(c);
        }
        else
        {
            QColor cc = itemmodel->textColorAt(drawpos);
            QColor c = seltextcolor;
            if (!cc.isValid() && dict->kanjiWordCount(itemmodel->kanjiAt(drawpos)) == 0)
                cc = Settings::uiColor(ColorSettings::KanjiNoWords);
            if (cc.isValid())
                c = colorFromBase(textcolor, c, cc);
            p.setPen(c);

            c = selcolor;
            cc = itemmodel->backColorAt(drawpos);
            if (cc.isValid())
                c = colorFromBase(bgcolor, c, cc);

            p.setBrush(c);
        }
        p.fillRect(QRect(x, y, cellsize - 1, cellsize - 1), p.brush());

        // Draw a small triangle at the bottom right corner of a cell to show the kanji
        // doesn't have user defined meaning. Only needed for user dictionaries.
        if (dict != ZKanji::dictionary(0) && !dict->hasKanjiMeaning(itemmodel->kanjiAt(drawpos)))
        {
            QColor ccc = Settings::uiColor(ColorSettings::KanjiNoTranslation);
            if (sel)
                ccc = colorFromBase(bgcolor, p.brush().color(), ccc);
            QLinearGradient grad(QPointF(x + cellsize * 0.8, y + cellsize * 0.8), QPointF(x + cellsize, y + cellsize));
            grad.setColorAt(0, p.brush().color());
            grad.setColorAt(0.5, p.brush().color());
            grad.setColorAt(1, ccc);
            p.fillRect(QRect(x + cellsize * 0.8, y + cellsize * 0.8, cellsize * 0.2 - 1, cellsize * 0.2 - 1), QBrush(grad));
        }

        drawTextBaseline(&p, x, y + cellsize * 0.86, true, QRect(x, y, cellsize - 1, cellsize - 1), ZKanji::kanjis[itemmodel->kanjiAt(drawpos)]->ch);

        if (current == drawpos && hasFocus())
        {
            p.save();
            // Draw the focus rectangle.
            QStyleOptionFocusRect fop;
            fop.backgroundColor = p.brush().color();
            fop.palette = opts.palette;
            fop.direction = opts.direction;
            fop.rect = QRect(x, y, cellsize - 1, cellsize - 1);
            fop.state = QStyle::State(QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange | (sel ? QStyle::State_Selected : QStyle::State_None));
            p.drawPrimitive(QStyle::PE_FrameFocusRect, fop);
            p.restore();
        }
    }

    if (state == State::Dragging && dragind != -1)
    {
        // Paint the drag indicator.

        p.setBrush(selcolor);

        QRect r = dragind < tosigned(model()->size()) ? cellRect(dragind) : QRect();
        QRect r2 = dragind > 0 ? cellRect(dragind - 1) : QRect();
        if (!r.isEmpty())
        {
            // Item on the right of the insert position.
            p.fillRect(QRect(r.left(), r.top(), 1, r.height()), p.brush());

            QPainterPath pp;
            pp.moveTo(r.left() + 1, r.top());
            pp.lineTo(r.left() + cellsize / 12, r.top());
            pp.lineTo(r.left() + 1, r.top() + cellsize / 12 - 1);
            p.fillPath(pp, p.brush());

            pp = QPainterPath();
            pp.moveTo(r.left() + 1, r.bottom() - cellsize / 12 + 1);
            pp.lineTo(r.left() + cellsize / 12, r.bottom() + 1);
            pp.lineTo(r.left() + 1, r.bottom() + 1);
            p.fillPath(pp, p.brush());
        }
        if (!r2.isEmpty())
        {
            // Item on the left of the insert position.
            p.fillRect(QRect(r2.right() - 1, r2.top(), 2, r2.height()), p.brush());

            QPainterPath pp;
            pp.moveTo(r2.right() - 1 - cellsize / 12, r2.top());
            pp.lineTo(r2.right() - 1, r2.top());
            pp.lineTo(r2.right() - 1, r2.top() + cellsize / 12);
            p.fillPath(pp, p.brush());

            pp = QPainterPath();
            pp.moveTo(r2.right() - 1, r2.bottom() - cellsize / 12 + 1);
            pp.lineTo(r2.right() - 1, r2.bottom() + 1);
            pp.lineTo(r2.right() - 1 - cellsize / 12, r2.bottom() + 1);
            p.fillPath(pp, p.brush());
        }
    }

    // Cleanup.
    p.setBrush(oldbrush);
    p.setPen(oldpen);
    p.setFont(oldfont);
}

void ZKanjiGridView::resizeEvent(QResizeEvent *event)
{
    if (std::max(1, event->size().width() / cellsize) != cols && itemmodel != nullptr && !itemmodel->empty())
    {
        recompute(event->size());
        viewport()->update();
    }
    recomputeScrollbar(event->size());
}

void ZKanjiGridView::keyPressEvent(QKeyEvent *e)
{
    if (model() == nullptr || model()->empty())
    {
        QFrame::keyPressEvent(e);
        return;
    }

    if (e == QKeySequence::SelectAll)
    {
        selectAll();
        QFrame::keyPressEvent(e);
        return;
    }

    if (e == QKeySequence::Deselect
#ifndef Q_OS_LINUX
        // Qt is missing this important key combination for Windows and Mac.
        || e == QKeySequence(tr("Ctrl+D", "Deselect")) || e == QKeySequence(tr("Ctrl+Shift+A", "Deselect"))
#endif
        )
    {
        clearSelection();
        QFrame::keyPressEvent(e);
        return;
    }

    //int index;
    bool multi = e->modifiers().testFlag(Qt::ShiftModifier);
    bool toggle = e->modifiers().testFlag(Qt::ControlModifier);

    if (multi && selpivot == -1)
        selpivot = current;

    //if (lastkeyindex != -1 && multi)
    //    index = lastkeyindex;
    //else
    //    index = sellist.back();

    // The value 'current' will end up to when the keypress is handled.
    int endindex = -1;
    QSize size = viewport()->size();

    if (e->key() == Qt::Key_PageUp)
    {
        endindex = std::max(current % cols, current - std::max(1, size.height() / cellsize) * cols);
    }
    else if (e->key() == Qt::Key_PageDown)
    {
        int change = std::max(1, size.height() / cellsize) * cols;
        while (current + change >= tosigned(itemmodel->size()))
            change -= cols;
        endindex = current + change;
    }
    else if (e->key() == Qt::Key_Home && toggle)
    {
        toggle = false;

        endindex = 0;
    }
    else if (e->key() == Qt::Key_End && toggle)
    {
        toggle = false;

        endindex = tosigned(itemmodel->size()) - 1;
    }
    else if (e->key() == Qt::Key_Home && !toggle)
    {
        toggle = false;

        endindex = current - (current % cols);
    }
    else if (e->key() == Qt::Key_End && !toggle)
    {
        toggle = false;

        endindex = std::min(tosigned(itemmodel->size()) - 1, current + (cols - (current % cols)) - 1);
    }
    else if (e->key() == Qt::Key_Up)
    {
        endindex = std::max(current % cols, current - cols);
    }
    else if (e->key() == Qt::Key_Down)
    {
        if (current + cols < tosigned(itemmodel->size()))
            endindex = current + cols;
        else
            endindex = current;
    }
    else if (e->key() == Qt::Key_Left)
    {
        endindex = std::max(0, current - 1);
    }
    else if (e->key() == Qt::Key_Right)
    {
        endindex = std::min(tosigned(itemmodel->size()) - 1, current + 1);
    }
    else
    {
        if (current != -1 && (e->key() == Qt::Key_Space || e->key() == Qt::Key_Select))
        {
            // Toggle selection of the current item.
            toggleSelect(current);
        }
        //lastkeyindex = -1;

        QFrame::keyPressEvent(e);
        return;
    }

    if (!multi)
    {
        selpivot = -1;
        if (!toggle)
            select(endindex);
        else
        {
            //int oldcurrent = current;
            int newcurrent = endindex;
            scrollIn(newcurrent);
            //updateCell(oldcurrent);
            //updateCell(current);

            //if (oldcurrent != current)
            //    emit currentChanged(current, oldcurrent);
            setAsCurrent(newcurrent);
        }
    }
    else
        multiSelect(endindex, !toggle);

    QFrame::keyPressEvent(e);
}

void ZKanjiGridView::mousePressEvent(QMouseEvent *e)
{
    if (mousedown)
    {
        base::mousePressEvent(e);
        e->accept();
        return;
    }

    state = State::None;

    int cell;
    if ((e->button() != Qt::LeftButton && e->button() != Qt::RightButton) || (cell = cellAt(e->pos())) == -1)
    {
        base::mousePressEvent(e);
        return;
    }

    if (e->button() == Qt::LeftButton)
        mousedown = true;

    bool multi = e->modifiers().testFlag(Qt::ShiftModifier);
    bool toggle = e->modifiers().testFlag(Qt::ControlModifier);

    if (!multi)
        selpivot = -1;

    if (!multi && !toggle)
    {
        if (e->button() == Qt::LeftButton || !selected(cell))
        {
            if (e->button() == Qt::LeftButton)
                state = State::CanDrag;
            mousedownpos = e->pos();

            // Allow drag and drop of kanji if the user clicked inside a bigger selection.
            // The kanji under the mouse can still be simply selected on mouse up if the drag
            // and drop hasn't started.
            if (e->button() != Qt::LeftButton || !selected(cell))
                select(cell);
        }
        base::mousePressEvent(e);
        return;
    }

    if (!multi && toggle)
        toggleSelect(cell);
    else
        multiSelect(cell, !toggle);

    base::mousePressEvent(e);
    e->accept();
}

void ZKanjiGridView::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);

    int ix = cellAt(e->pos());
    if (ix != -1)
        gUI->showKanjiInfo(dictionary(), model()->kanjiAt(ix));
    e->accept();
}

void ZKanjiGridView::mouseMoveEvent(QMouseEvent *e)
{
    if (state == State::CanDrag)
    {
        if ((mousedownpos - e->pos()).manhattanLength() > QApplication::startDragDistance())
        {
            e->accept();
            QMimeData *mdat = dragMimeData(/*mousedownpos*/);
            if (mdat == nullptr)
                return;

            mousedown = false;

            QDrag *drag = new QDrag(this);
            drag->setMimeData(mdat);

            state = State::None;

            if (drag->exec(model()->supportedDragActions(), Qt::CopyAction) == Qt::MoveAction)
                removeSelected();


            stopAutoScroll();
            return;
        }
    }

    base::mouseMoveEvent(e);
    e->accept();

    if (!Settings::kanji.tooltip || itemmodel == nullptr || e->buttons() != Qt::NoButton)
        return;

    int cell = cellAt(e->pos());
    if (cell == -1)
    {
        kanjitipcell = -1;
        kanjitipkanji = -1;
        return;
    }
    int kanji = itemmodel->kanjiAt(cell);

    if (kanjitipcell != cell || kanjitipkanji != kanji)
    {
        kanjitipcell = cell;
        kanjitipkanji = kanji;

        ZToolTip::show(ZKanji::kanjiTooltipWidget(dictionary(), kanji), this, cellRect(cell).adjusted(0, 0, 1, 1), Settings::kanji.hidetooltip ? Settings::kanji.tooltipdelay * 1000 : 2000000000);
    }
}

void ZKanjiGridView::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    // Don't forget viewportEvent() when changing this.
    ZToolTip::startHide();
    kanjitipcell = -1;
    kanjitipkanji = -1;
}

void ZKanjiGridView::mouseReleaseEvent(QMouseEvent *e)
{
    if (!mousedown)
    {
        base::mouseReleaseEvent(e);
        e->accept();
        return;
    }

    mousedown = false;

    if (state == State::CanDrag)
    {
        // The mouse hasn't been moved enough to start a drag operation, so the cell clicked
        // should be selected instead.

        int cell = cellAt(mousedownpos);
        select(cell);
    }

    state = State::None;

    base::mouseReleaseEvent(e);
    e->accept();
}

void ZKanjiGridView::contextMenuEvent(QContextMenuEvent *e)
{
    //int cell;
    if (e->reason() != QContextMenuEvent::Mouse || selCount() == 0)
        return;

    QMenu popup;
    QAction *a;

    a = popup.addAction(tr("Kanji information..."));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::showKanjiInfo);
    popup.addSeparator();
    a = popup.addAction(tr("Add to group..."));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::kanjiToGroup);
    a = popup.addAction(tr("Collect words..."));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::collectWords);
    popup.addSeparator();
    a = popup.addAction(tr("Edit definition..."));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::definitionEdit);
    popup.addSeparator();
    a = popup.addAction(tr("Copy to clipboard"));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::copyKanji);
    a = popup.addAction(tr("Append to clipboard"));
    connect(a, &QAction::triggered, this, &ZKanjiGridView::appendKanji);


    popup.actions().at(0)->setEnabled(selCount() == 1);

    popup.exec(e->globalPos());

    e->accept();
}

void ZKanjiGridView::focusInEvent(QFocusEvent * /*e*/)
{
    viewport()->update();
}

void ZKanjiGridView::focusOutEvent(QFocusEvent * /*e*/)
{
    cancelActions();
    viewport()->update();
}

void ZKanjiGridView::dragEnterEvent(QDragEnterEvent *e)
{
    // TODO: for sorted/filtered groups, multiple group kanji the drag drop should always fail.

    if (model() == nullptr || (e->possibleActions() & model()->supportedDropActions(e->source() == this, e->mimeData())) == 0)
        //!e->mimeData()->hasFormat("zkanji/kanji") || model()->kanjiGroup() == nullptr || (e->possibleActions() & (Qt::MoveAction | Qt::CopyAction)) == 0 ||
        //(hasSameGroup(e->source()) && ((e->possibleActions() & Qt::MoveAction) == 0 || e->source() != this)) ||
        //)
    {
        e->ignore();
        return;
    }

    e->accept();

    dragind = -1;
    state = State::Dragging;

    savedDropActions = e->dropAction();
    savedMimeData = e->mimeData();
    savedMouseButtons = e->mouseButtons();
    savedKeyboardModifiers = e->keyboardModifiers();

    viewport()->update();
}

void ZKanjiGridView::dragLeaveEvent(QDragLeaveEvent * /*event*/)
{
    if (state != State::Dragging)
        return;

    stopAutoScroll();

    dragind = -1;
    state = State::None;

    viewport()->update();
}

void ZKanjiGridView::dragMoveEvent(QDragMoveEvent *e)
{
    e->ignore();

    if (state != State::Dragging)
    {
        dragind = -1;
        return;
    }

    savedDropActions = e->dropAction();
    savedMimeData = e->mimeData();
    savedMouseButtons = e->mouseButtons();
    savedKeyboardModifiers = e->keyboardModifiers();

    // Compute the position for the drop indicator.

    int cell = dragCell(e->pos());

    Qt::DropAction action = e->source() == this ? Qt::MoveAction : e->dropAction();
    Qt::DropActions supaction = model()->supportedDropActions(e->source() == this, e->mimeData());
    if ((action & supaction) == 0)
    {
        action = (e->source() != this && (supaction & Qt::CopyAction) != 0) ? Qt::CopyAction : (supaction & Qt::MoveAction) != 0 ? Qt::MoveAction : Qt::IgnoreAction;
        if (action == Qt::IgnoreAction)
            return;
    }


    QRect vr = viewport()->rect();
    QPoint cp = e->pos();
    if (cp.y() < vr.top() + autoScrollMargin() || cp.y() > vr.bottom() - autoScrollMargin())
        startAutoScroll();

    if (cell != -1 && !model()->canDropMimeData(e->mimeData(), action, cell))
        cell = -1;
    if (cell == -1 && !model()->canDropMimeData(e->mimeData(), action, -1))
    {
        if (dragind != -1)
            updateDragIndicator();

        dragind = -1;
        return;
    }

    if (action != e->dropAction())
    {
        e->setDropAction(action);
        e->accept();
    }
    else
        e->acceptProposedAction();

    if (cell != dragind)
    {
        updateDragIndicator();
        dragind = cell;
        updateDragIndicator();
    }
}

void ZKanjiGridView::dropEvent(QDropEvent *e)
{
    e->ignore();
    stopAutoScroll();

    if (state == State::Dragging && dragind != -1)
        updateDragIndicator();
    dragind = -1;

    if (state != State::Dragging)
    {
        state = State::None;
        viewport()->update();
        return;
    }

    state = State::None;
    viewport()->update();

    int cell = dragCell(e->pos());

    Qt::DropAction action = this == e->source() ? Qt::MoveAction : e->dropAction();
    Qt::DropActions supaction = model()->supportedDropActions(e->source() == this, e->mimeData());
    if ((action & supaction) == 0)
    {
        action = (this != e->source() && (supaction & Qt::CopyAction) != 0) ? Qt::CopyAction : (supaction & Qt::MoveAction) != 0 ? Qt::MoveAction : Qt::IgnoreAction;
        if (action == Qt::IgnoreAction)
            return;
    }

    if (cell != -1 && !model()->dropMimeData(e->mimeData(), action, cell))
        cell = -1;
    if (cell == -1 && !model()->dropMimeData(e->mimeData(), action, -1))
    {
        dragind = -1;
        return;
    }

    if (this == e->source() && e->dropAction() != Qt::CopyAction)
    {
        // Prevent deletion of the moved data as it was handled by the model above.
        e->setDropAction(Qt::CopyAction);
        e->accept();
    }
    else
        e->acceptProposedAction();


    //if (hasSameGroup(event->source()))
    //{
    //    while (dragind != model()->size() && std::find(kindexes.begin(), kindexes.end(), model()->kanjiGroup()->getIndexes()[dragind]) != kindexes.end())
    //        ++dragind;

    //    std::vector<int> positions;
    //    model()->kanjiGroup()->indexOf(kindexes, positions);
    //    smartvector<Range> ranges;
    //    _rangeFromIndexes(positions, ranges);

    //    model()->kanjiGroup()->move(ranges, dragind);
    //}
    //else
    //    model()->kanjiGroup()->insert(kindexes, dragind);

}

void ZKanjiGridView::updateStatus()
{
    if (status == nullptr || itemmodel == nullptr)
    {
        if (status != nullptr)
            status->clear();
        return;
    }

    if ((itemmodel->statusCount() == 0 && status->size() != 2) || (itemmodel->statusCount() != 0 && tosigned(status->size()) != itemmodel->statusCount() + 1))
    {
        status->clear();

        status->add(QString(), 0, "0 :", 7, "0", 7);
       
        if (itemmodel->statusCount() == 0)
            status->add(QString(), 0);
        else
        {
            for (int ix = 0, siz = itemmodel->statusCount(); ix != siz; ++ix)
            {
                switch (itemmodel->statusType(ix))
                {
                case StatusTypes::TitleValue:
                    status->add(itemmodel->statusText(ix, -1, -1), itemmodel->statusSize(ix, -1), itemmodel->statusText(ix, 0, current), itemmodel->statusSize(ix, 0), itemmodel->statusAlignRight(ix));
                    break;
                case StatusTypes::TitleDouble:
                    status->add(itemmodel->statusText(ix, -1, -1), itemmodel->statusSize(ix, -1), itemmodel->statusText(ix, 0, current), itemmodel->statusSize(ix, 0), itemmodel->statusText(ix, 1, current), itemmodel->statusSize(ix, 1));
                    break;
                case StatusTypes::DoubleValue:
                    status->add("", 0, itemmodel->statusText(ix, 0, current), itemmodel->statusSize(ix, 0), itemmodel->statusText(ix, 1, current), itemmodel->statusSize(ix, 1));
                    break;
                case StatusTypes::SingleValue:
                    status->add(itemmodel->statusText(ix, 0, current), itemmodel->statusSize(ix, 0));
                    break;
                }
            }
        }
    }

    status->setValues(0, QString::number(itemmodel->size()) + " /", QString::number(current + 1));

    if (itemmodel->statusCount() == 0)
        status->setValue(1, current == -1 ? "-" : dictionary()->kanjiMeaning(itemmodel->kanjiAt(current)) );
    else
    {
        for (int ix = 0, siz = itemmodel->statusCount(); ix != siz; ++ix)
        {
            switch (itemmodel->statusType(ix))
            {
            case StatusTypes::TitleValue:
                status->setValue(ix + 1, itemmodel->statusText(ix, 0, current));
                break;
            case StatusTypes::TitleDouble:
                status->setValues(ix + 1, itemmodel->statusText(ix, 0, current), itemmodel->statusText(ix, 1, current));
                break;
            case StatusTypes::DoubleValue:
                status->setValues(ix + 1, itemmodel->statusText(ix, 0, current), itemmodel->statusText(ix, 1, current));
                break;
            case StatusTypes::SingleValue:
                status->setValue(ix + 1, itemmodel->statusText(ix, 0, current));
                break;
            }
        }
    }
}

void ZKanjiGridView::statusDestroyed()
{
    if (status != nullptr)
        assignStatusBar(nullptr);
}

void ZKanjiGridView::recompute(const QSize &size)
{
    if (itemmodel == nullptr || itemmodel->empty())
    {
        cols = 0;
        rows = 0;
    }
    else
    {
        cols = std::max(1, size.width() / cellsize);
        rows = (tosigned(itemmodel->size()) + cols - 1) / cols;
    }
}

void ZKanjiGridView::recomputeScrollbar(const QSize &size)
{
    // If switching to scroll per item, change the auto scroll timer interval.
    // See startAutoScroll().

    verticalScrollBar()->setMaximum(std::max(0, rows * cellsize - size.height()));
    verticalScrollBar()->setSingleStep(cellsize * 0.7);
    verticalScrollBar()->setPageStep(std::max(size.height() - cellsize * 1.2, cellsize * 0.9));
}

QMimeData* ZKanjiGridView::dragMimeData()
{
    std::vector<int> indexes;
    selectedCells(indexes);
    if (indexes.empty())
        return nullptr;

    return model()->mimeData(indexes);
}

void ZKanjiGridView::updateDragIndicator()
{
    if (dragind == -1)
        return;

    QRect r = dragind < tosigned(model()->size()) ? cellRect(dragind) : QRect();
    QRect r2 = dragind > 0 ? cellRect(dragind - 1) : QRect();
    if (!r.isEmpty())
    {
        // Item on the right of the insert position.
        viewport()->update(r.left(), r.top(), 1, r.height());
        viewport()->update(r.left() + 1, r.top(), cellsize / 12, cellsize / 12);
        viewport()->update(r.left() + 1, r.bottom() - cellsize / 12 + 1, cellsize / 12, cellsize / 12);
    }
    if (!r2.isEmpty())
    {
        // Item on the left of the insert position.
        viewport()->update(r2.right() - 1, r2.top(), 2, r2.height());
        viewport()->update(r2.right() - 1 - cellsize / 12, r2.top(), cellsize / 12, cellsize / 12);
        viewport()->update(r2.right() - 1 - cellsize / 12, r2.bottom() - cellsize / 12 + 1, cellsize / 12, cellsize / 12);
    }
}

bool ZKanjiGridView::hasSameGroup(QObject *obj)
{
    ZKanjiGridView *v;

#ifdef _DEBUG
    v = dynamic_cast<ZKanjiGridView*>(obj);
    if (v == nullptr)
        return false;
#else
    v = (ZKanjiGridView*)obj;
#endif

    return v->model()->kanjiGroup() == model()->kanjiGroup();
}

int ZKanjiGridView::dragCell(QPoint pt) const
{
    int cell = cellAt(pt);
    if (cell == -1)
    {
        // The mouse might be to the right side of the last column.
        int row = cellRowAt(pt.y());
        if (row == -1)
            cell = tosigned(model()->size()) - 1;
        else
            cell = std::min(tosigned(model()->size()) - 1, (row + 1) * cols - 1);
    }

    QRect crect = cellRect(cell);
    if ((cell == tosigned(model()->size()) - 1 && pt.y() > crect.bottom()) || pt.x() - crect.left() > crect.width() / 2)
        ++cell;

    return cell;
}

void ZKanjiGridView::setAsCurrent(int index)
{
    if (current == index)
        return;
    if (current != -1)
        updateCell(current);
    std::swap(current, index);
    if (current != -1)
        updateCell(current);

    updateCommands();

    emit currentChanged(current, index);

    if (current != -1)
        gUI->setInfoKanji(dictionary(), model()->kanjiAt(current));

    updateStatus();
}

CommandCategories ZKanjiGridView::activeCategory() const
{
    if (!isVisibleTo(window()) || dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return CommandCategories::NoCateg;

    const QWidget *p = parentWidget();
    while (p != nullptr && dynamic_cast<const ZKanjiWidget*>(p) == nullptr)
        p = p->parentWidget();

    if (p == nullptr)
        return CommandCategories::NoCateg;

    ZKanjiWidget *zw = ((ZKanjiWidget*)p);
    if (zw->isActiveWidget())
        return zw->mode() == ViewModes::KanjiSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg;

    QList<ZKanjiWidget*> wlist = ((ZKanjiForm*)window())->findChildren<ZKanjiWidget*>();
    for (ZKanjiWidget *w : wlist)
    {
        if (w == zw || w->window() != window())
            continue;
        else if (w->mode() == zw->mode())
            return CommandCategories::NoCateg;
    }

    return zw->mode() == ViewModes::KanjiSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg;
}

void ZKanjiGridView::updateCommands() const
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
    {
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ShowKanjiInfo, categ), selCount() == 1);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::KanjiToGroup, categ), selCount() != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::EditKanjiDef, categ), selCount() != 0 && dict != ZKanji::dictionary(0));
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CollectKanjiWords, categ), selCount() != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CopyKanji, categ), selCount() != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::AppendKanji, categ), selCount() != 0);
    }
}

void ZKanjiGridView::showKanjiInfo() const
{
    gUI->showKanjiInfo(dict, itemmodel->kanjiAt(selectedCell(0)));
}

void ZKanjiGridView::kanjiToGroup() const
{
    std::vector<ushort> selkanji;
    selKanji(selkanji);
    if (selkanji.empty())
        return;
    kanjiToGroupSelect(dictionary(), selkanji/*, window()*/);
}

void ZKanjiGridView::collectWords() const
{
    std::vector<ushort> selkanji;
    selKanji(selkanji);
    if (selkanji.empty())
        return;
    collectKanjiWords(dict, selkanji);
}

void ZKanjiGridView::definitionEdit() const
{
    std::vector<ushort> selkanji;
    selKanji(selkanji);
    if (selkanji.empty())
        return;
    if (selkanji.size() == 1)
    {
        selkanji.reserve(itemmodel->size() - selection->ranges(0).first);
        for (int ix = selection->ranges(0).first + 1, siz = tosigned(itemmodel->size()); ix != siz; ++ix)
            selkanji.push_back(itemmodel->kanjiAt(ix));
    }
    editKanjiDefinition(dict, selkanji);
}

void ZKanjiGridView::copyKanji() const
{
    if (selCount() == 0)
        return;
    gUI->clipCopy(selString());
}

void ZKanjiGridView::appendKanji() const
{
    if (selCount() == 0)
        return;
    gUI->clipAppend(selString());
}


//-------------------------------------------------------------
