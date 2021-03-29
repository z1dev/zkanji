/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QScrollBar>
#include <QPainter>
#include <QHeaderView>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QLayout>
#include <QStringBuilder>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QPainterPath>

#include "zlistview.h"
#include "zstatusbar.h"
#include "zabstracttablemodel.h"
#include "zlistviewitemdelegate.h"
#include "zevents.h"
#include "zui.h"
#include "ranges.h"
#include "zkanjimain.h"
#include "fontsettings.h"
#include "colorsettings.h"
#include "globalui.h"
#include "formstates.h"
#include "generalsettings.h"

//-------------------------------------------------------------


ZListView::ZListView(QWidget *parent) : base(parent), selection(new RangeSelection), currentrow(-1), seltype(ListSelectionType::Single),
        selpivot(-1), status(nullptr), autosize(true), sizebase(ListSizeBase::Custom), firstrow(-1), lastrow(-1), checkcol(-1), state(State::None),
        doubleclick(false), mousedown(false), pressed(false), hover(false), canclickedit(false), editcolumn(0), ignorechange(false), dragpos(-1)
{
    setAttribute(Qt::WA_MouseTracking);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setTabKeyNavigation(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setWordWrap(false);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setMinimumSectionSize(Settings::scaled(12));
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(Settings::scaled(20));

    connect(gUI, &GlobalUI::settingsChanged, this, &ZListView::settingsChanged);
    Settings::updatePalette(this);

    //qApp->postEvent(this, new ConstructedEvent);

    setItemDelegate(new ZListViewItemDelegate(this));

    //connect(horizontalHeader(), &QHeaderView::sectionResized, this, &ZListView::sectionResized);
}

ZListView::~ZListView()
{
    doubleclicktimer.stop();
}

void ZListView::saveXMLSettings(QXmlStreamWriter &writer) const
{
    // Only saves the sizes of the resizable columns.
    ZAbstractTableModel *m = model();
    if (m == nullptr)
        return;

    int cc = m->columnCount();
    if (cc == 0)
        return;

    QString colstr;
    QString sizstr;
    //QString hidstr;

    for (int ix = 0; ix != cc; ++ix)
    {
        if (!m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
            continue;
        if (colstr.isEmpty())
        {
            colstr = QString::number(ix);
            sizstr = QString::number(columnWidth(ix));
        }
        else
        {
            colstr = colstr % "," % QString::number(ix);
            sizstr = sizstr % "," % QString::number(columnWidth(ix));
        }

        //if (isColumnHidden(ix))
        //{
        //    if (hidstr.isEmpty())
        //        hidstr = QString::number(ix);
        //    else
        //        hidstr = hidstr % "," % QString::number(ix);
        //}
    }

    if (!colstr.isEmpty())
    {
        writer.writeAttribute("columns", colstr);
        writer.writeAttribute("sizes", sizstr);
    }
    //if (!hidstr.isEmpty())
    //    writer.writeAttribute("hidden", hidstr);
}

void ZListView::loadXMLSettings(QXmlStreamReader &reader)
{
    ZAbstractTableModel *m = model();
    if (m == nullptr)
    {
        reader.skipCurrentElement();
        return;
    }

    int cc = m->columnCount();
    if (cc == 0)
    {
        reader.skipCurrentElement();
        return;
    }

    QStringRef colstr = reader.attributes().value("columns");
    QStringRef sizstr = reader.attributes().value("sizes");
    if (!colstr.isEmpty() && !sizstr.isEmpty())
    {
        QVector<QStringRef> cols = colstr.split(",");
        QVector<QStringRef> sizes = sizstr.split(",");

        if (cols.size() == sizes.size())
        {
            // Only do anything if the whole data is correct.
            std::vector<std::pair<int, int>> colsizes;
            colsizes.reserve(cols.size());

            bool ok = true;
            for (int ix = 0, siz = cols.size(); ok && ix != siz; ++ix)
            {
                int c = cols.at(ix).trimmed().toInt(&ok);
                if (!m->headerData(c, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
                    ok = false;

                int s = ok ? sizes.at(ix).trimmed().toInt(&ok) : -1;
                ok = ok && c >= 0 && c < cc && s >= 0 && s < 999999;

                if (ok)
                    colsizes.push_back(std::make_pair(c, s));
            }

            if (ok)
            {
                for (int ix = 0, siz = tosigned(colsizes.size()); ix != siz; ++ix)
                    setColumnWidth(colsizes[ix].first, colsizes[ix].second);
            }
        }
    }


    //QStringRef hidstr = reader.attributes().value("hidden");
    //QVector<QStringRef> hids = colstr.split(",");

    //std::vector<int> hidden;
    //hidden.reserve(hids.size());
    //bool ok = true;

    //for (int ix = 0, siz = hids.size(); ok && ix != siz; ++ix)
    //{
    //    int c = hids.at(ix).trimmed().toInt(&ok);
    //    ok = ok && c >= 0 && c < cc;

    //    if (ok)
    //        hidden.push_back(c);
    //}

    //if (ok)
    //{
    //    for (int ix = 0, siz = hidden.size(); ix != siz; ++ix)
    //        hideColumn(hidden[ix]);
    //}

    reader.skipCurrentElement();
}

void ZListView::saveColumnState(ListStateData &dat) const
{
    dat.columns.clear();

    ZAbstractTableModel *m = model();
    if (m == nullptr)
        return;

    int cc = m->columnCount();
    if (cc == 0)
        return;

    for (int ix = 0; ix != cc; ++ix)
    {
        if (!m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
            continue;

        dat.columns.push_back(std::make_pair(ix, columnWidth(ix)));
    }
}

void ZListView::restoreColumnState(const ListStateData &dat, ZAbstractTableModel *columnmodel)
{
    bool selfmodel = model() != nullptr;
    ZAbstractTableModel *m = selfmodel ? model() : columnmodel;
    if (m == nullptr)
        return;

    int cc = m->columnCount();
    if (cc == 0)
        return;

    if (!selfmodel)
        saveColumnData(m);

    for (int ix = 0, siz = tosigned(dat.columns.size()); ix != siz; ++ix)
    {
        int c = dat.columns[ix].first;
        int w = dat.columns[ix].second;
        if (c < 0 || c >= cc || w < 0 || w > 999999 || !m->headerData(c, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
            continue;
        if (selfmodel)
            setColumnWidth(c, w);
        else
            tmpcolsize[c].width = w;
    }
}

void ZListView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    // Disable auto horizontal scrolling by restoring the original scrollbar's position.
    // Relies on that no drawing takes place before the function returns.
    int v = horizontalScrollBar()->value();
    base::scrollTo(index, hint);
    horizontalScrollBar()->setValue(v);
}

//QItemSelectionModel::SelectionFlags ZListView::selectionCommand(const QModelIndex &index, const QEvent *e) const
//{
//    QItemSelectionModel::SelectionFlags flags = base::selectionCommand(index, e);
//    flags |= QItemSelectionModel::Rows;
//    return flags;
//}

//QModelIndex ZListView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
//{
//    if (cursorAction == MoveLeft || cursorAction == MoveRight)
//    {
//        if (cursorAction == MoveLeft)
//            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - 15);
//        if (cursorAction == MoveRight)
//            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 15);
//        return currentIndex();
//    }
//
//    return base::moveCursor(cursorAction, modifiers);
//}

bool ZListView::autoSizeColumns()
{
    return autosize;
}

void ZListView::setAutoSizeColumns(bool value)
{
    if (autosize == value)
        return;
    autosize = value;

    autoResizeColumns();
}

CheckBoxMouseState ZListView::checkBoxMouseState(const QModelIndex &index) const
{
    if (mousecell != index || (!hover && !pressed))
        return CheckBoxMouseState::None;

    if (hover && !pressed)
        return CheckBoxMouseState::Hover;
    if (pressed && !hover)
        return CheckBoxMouseState::Down;

    return CheckBoxMouseState::DownHover;
}

int ZListView::currentRow() const
{
    return currentrow;
    //if (!currentIndex().isValid())
    //    return -1;

    //return currentIndex().row();
}

void ZListView::setCurrentRow(int rowindex)
{
    if (!model())
        return;

    //if (rowindex < 0 && seltype == ListSelectionType::Single)
    //    rowindex = 0;
    if (rowindex < 0)
        rowindex = -1;

    if (rowindex >= model()->rowCount())
        rowindex = model()->rowCount() - 1;

    int row = currentrow;

    bool selchange = false;
    if (rowindex != currentrow)
        updateRow(row);

    int selindex = rowindex != -1 ? mapToSelection(rowindex) : -1;
    int selcurrent = mapToSelection(currentrow);

    if (selindex == -1)
    {
        selchange = (selpivot != -1 && currentrow != -1) || !selection->empty();
        if (selchange)
        {
            updateSelection();
            selpivot = -1;
            selection->clear();
        }
    }
    else if (seltype != ListSelectionType::None && (!selection->singleSelected(selindex) || (selpivot != -1 && (selpivot != selindex || selcurrent != selindex))))
    {
        updateSelection();
        selpivot = -1;
        selection->clear();
        selection->setSelected(selindex, true);
        updateSelection();

        selchange = true;
    }
    else if (rowindex != currentrow)
        updateRow(currentrow);

    currentrow = rowindex;
    if (currentrow != -1)
    {
        updateRow(currentrow);
        scrollToRow(currentrow);
    }

    updateStatus();

    if (row != currentrow)
        emit currentRowChanged(currentrow, row);
    if (selchange)
        signalSelectionChanged();

    //QModelIndex index = model()->index(rowindex, 0);
    //selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate | QItemSelectionModel::Rows);
    //selectionModel()->select(index, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void ZListView::changeCurrentRow(int rowindex)
{
    if (!model())
        return;

    if (rowindex < 0)
        rowindex = -1;

    if (rowindex >= model()->rowCount())
        rowindex = model()->rowCount() - 1;

    if (rowindex == currentrow)
        return;

    commitPivotSelection();
    if (currentrow != -1)
        updateRow(currentrow);
    std::swap(currentrow, rowindex);
    if (currentrow != -1)
    {
        updateRow(currentrow);
        scrollToRow(currentrow);
    }

    emit currentRowChanged(currentrow, rowindex);
}

void ZListView::selectedRows(std::vector<int> &result) const
{
    int pivotfirst = currentrow == -1 ? -1 : selpivot;
    int pivotlast = pivotfirst == -1 ? -1 : mapToSelection(currentrow);
    if (pivotlast < pivotfirst)
        std::swap(pivotfirst, pivotlast);

    selection->getSelection(result, pivotfirst, pivotlast);

    //auto list = selectionModel()->selectedRows();
    //result.reserve(list.size());

    //for (int ix = 0; ix != list.size(); ++ix)
    //    result.push_back(list[ix].row());
}

QModelIndexList ZListView::selectedIndexes() const
{
    std::vector<int> rows;
    selectedRows(rows);

    auto m = model();

    QModelIndexList result;
    result.reserve(tosigned(rows.size()));
    for (int r : rows)
        result << m->index(r, 0);

    return result;
}

//void ZListView::clearSelection()
//{
//    if (selpivot == -1 && selection->empty())
//        return;
//
//    updateSelection();
//    selpivot = -1;
//    selection->clear();
//
//    signalSelectionChanged();
//}

bool ZListView::hasSelection() const
{
    return (selpivot != -1 && currentrow != -1) || !selection->empty();
}


bool ZListView::rowSelected(int rowindex) const
{
    if (rowindex < 0 || rowindex >= model()->rowCount())
        return false;

    rowindex = mapToSelection(rowindex);
    if (selpivot != -1 && currentrow != -1)
    {
        int first = selpivot;
        int last = mapToSelection(currentrow);
        if (last < first)
            std::swap(last, first);
        if (first <= rowindex && last >= rowindex)
            return true;
    }

    return selection->selected(rowindex);
}

int ZListView::selectionSize() const
{
    int pivotfirst = currentrow == -1 ? -1 : selpivot;
    int pivotlast = pivotfirst == -1 ? -1 : mapToSelection(currentrow);
    if (pivotlast < pivotfirst)
        std::swap(pivotfirst, pivotlast);

    return selection->size(pivotfirst, pivotlast);
}

int ZListView::selectedRow(int index) const
{
    int pivotfirst = currentrow == -1 ? -1 : selpivot;
    int pivotlast = pivotfirst == -1 ? -1 : mapToSelection(currentrow);
    if (pivotlast < pivotfirst)
        std::swap(pivotfirst, pivotlast);

    return selection->selectedItems(index, pivotfirst, pivotlast);
}

ListSelectionType ZListView::selectionType() const
{
    return seltype;
}

void ZListView::setSelectionType(ListSelectionType newtype)
{
    if (seltype == newtype)
        return;

    seltype = newtype;
    if (seltype == ListSelectionType::None || seltype == ListSelectionType::Single)
    {
        clearSelection();
        if (seltype == ListSelectionType::Single && currentrow != -1)
        {
            selection->setSelected(mapToSelection(currentrow), true);
            updateSelection();
        }
        signalSelectionChanged();
    }
}

bool ZListView::continuousSelection() const
{
    if (seltype == ListSelectionType::None)
        return false;
    if (seltype == ListSelectionType::Single)
        return hasSelection();

    if (selection->rangeCount() == 0)
        return hasSelection();

    int pivotfirst = currentrow == -1 ? -1 : selpivot;
    int pivotlast = pivotfirst == -1 ? -1 : mapToSelection(currentrow);
    if (pivotlast < pivotfirst)
        std::swap(pivotfirst, pivotlast);
    if (pivotfirst == -1 || pivotlast == -1)
        return selection->rangeCount() == 1;

    for (int ix = 0, siz = selection->rangeCount(); ix != siz; ++ix)
    {
        const Range &r = selection->ranges(ix);
        if (r.first > pivotlast + 1 || r.last < pivotfirst - 1)
            return false;
    }
    return true;
}

void ZListView::scrollToRow(int row, ScrollHint hint)
{
    if (model() == nullptr)
        return;

    scrollTo(model()->index(row, 0), hint);
}

QRect ZListView::visualRowRect(int row) const
{
    ZAbstractTableModel *m = model();
    if (m == nullptr || m->rowCount() <= row || row < 0)
        return QRect();

    QRect r = visualRect(m->index(row, 0));

    r.setLeft(0);
    r.setRight(viewport()->width());

    return r;
}

QRect ZListView::rowRect(int row) const
{
    ZAbstractTableModel *m = model();
    if (m == nullptr || m->rowCount() <= row || row < 0)
        return QRect();

    QRect r = visualRect(m->index(row, 0));

    int w = 0;
    for (int ix = 1; ix != m->columnCount(); ++ix)
        if (!isColumnHidden(ix))
            w += columnWidth(ix) + (showGrid() ? 1 : 0);
    r.setRight(r.right() + w);

    return r;
}

void ZListView::updateRow(int row)
{
    ZAbstractTableModel *m = model();
    if (m == nullptr || m->rowCount() <= row)
        return;

    viewport()->update(visualRowRect(row));
}

void ZListView::updateRows(int first, int last)
{
    if (firstrow == -1 || lastrow == -1 || first > last)
        return;

    first = std::max(firstrow, first);
    last = std::min(lastrow, last);

    int w = viewport()->width();

    int top = rowViewportPosition(first);
    int bottom = rowViewportPosition(last) + rowHeight(last);

    viewport()->update(0, top, w, bottom - top);
}

void ZListView::updateSelection(bool range)
{
    if (firstrow == -1 || lastrow == -1)
        return;

    std::vector<const Range *> sel;
    selection->selectedRanges(mapToSelection(firstrow), mapToSelection(lastrow), sel);

    int w = viewport()->width();
    
    if (!sel.empty())
    {
        // Update first range within the view.

        int top = rowViewportPosition(std::max(firstrow, mapFromSelection(sel.front()->first, true)));

        int r = std::min(lastrow, mapFromSelection(sel.front()->last, false));
        int bottom = rowViewportPosition(r) + rowHeight(r);

        viewport()->update(0, top, w, bottom - top);
    }
    if (sel.size() > 1)
    {
        // Update last range within the view.

        int top = rowViewportPosition(std::max(firstrow, mapFromSelection(sel.back()->first, true)));

        int r = std::min(lastrow, mapFromSelection(sel.back()->last, false));
        int bottom = rowViewportPosition(r) + rowHeight(r);

        viewport()->update(0, top, w, bottom - top);
    }
    if (sel.size() > 2)
    {
        // Update the selections fully within the view.

        for (int ix = 1, siz = tosigned(sel.size()); ix != siz - 1; ++ix)
        {
            int top = rowViewportPosition(mapFromSelection(sel[ix]->first, true));

            int r = mapFromSelection(sel[ix]->last, false);
            int bottom = rowViewportPosition(r) + rowHeight(r);

            viewport()->update(0, top, w, bottom - top);
        }
    }

    if (range && selpivot != -1 && currentrow != -1)
    {
        int first = selpivot;
        int last = mapToSelection(currentrow);
        if (last < first)
            std::swap(first, last);
        first = mapFromSelection(first, true);
        last = mapFromSelection(last, false);

        // Update possible temporary range selection.

        int top = rowViewportPosition(std::max(firstrow, first));

        int r = std::min(lastrow, last);
        int bottom = rowViewportPosition(r) + rowHeight(r);

        viewport()->update(0, top, w, bottom - top);
    }
}

void ZListView::commitPivotSelection()
{
    if (selpivot == -1 || currentrow == -1)
        return;

    int first = selpivot;
    int last = mapToSelection(currentrow);
    if (last < first)
        std::swap(first, last);

    selection->selectRange(first, last, true);

    selpivot = -1;
}

bool ZListView::cancelActions()
{
    bool cancelled = state == State::CanDrag || state == State::CanDragOrSelect || mousedown || doubleclick || pressed;
    if (state == State::CanDrag || state == State::CanDragOrSelect)
        state = State::None;
    mousedown = false;
    doubleclick = false;
    if (pressed && mousecell.isValid())
        viewport()->update(itemDelegate()->checkBoxRect(mousecell));
    pressed = false;
    return cancelled;
}

void ZListView::signalSelectionChanged()
{
    emit rowSelectionChanged();
}

void ZListView::setModel(ZAbstractTableModel *newmodel)
{
    doubleclicktimer.stop();

    if (newmodel == model())
        return;

    selpivot = -1;

    QItemSelectionModel *oldselmodel = selectionModel();

    ZAbstractTableModel *m = model();
    if (m != nullptr)
    {
        disconnect(m, &ZAbstractTableModel::rowsWereInserted, this, &ZListView::rowsInserted);
        disconnect(m, &ZAbstractTableModel::rowsWereRemoved, this, &ZListView::rowsRemoved);
        disconnect(m, &ZAbstractTableModel::rowsWereMoved, this, &ZListView::rowsMoved);

        disconnect(m, &ZAbstractTableModel::layoutAboutToBeChanged, this, &ZListView::layoutAboutToBeChanged);
        disconnect(m, &ZAbstractTableModel::layoutChanged, this, &ZListView::layoutChanged);
        disconnect(m, &ZAbstractTableModel::modelAboutToBeReset, this, &ZListView::aboutToBeReset);
        disconnect(m, &ZAbstractTableModel::statusChanged, this, &ZListView::updateStatus);
    }

    bool selchange = !selection->empty() || (selpivot != -1 && currentrow != -1);
    selection->clear();
    selpivot = -1;
    int prevcur = currentrow;
    currentrow = -1;

    saveColumnData();

    base::setModel(newmodel);

    if (newmodel != nullptr)
    {
        connect(newmodel, &ZAbstractTableModel::rowsWereInserted, this, &ZListView::rowsInserted);
        connect(newmodel, &ZAbstractTableModel::rowsWereRemoved, this, &ZListView::rowsRemoved);
        connect(newmodel, &ZAbstractTableModel::rowsWereMoved, this, &ZListView::rowsMoved);

        connect(newmodel, &ZAbstractTableModel::layoutAboutToBeChanged, this, &ZListView::layoutAboutToBeChanged);
        connect(newmodel, &ZAbstractTableModel::layoutChanged, this, &ZListView::layoutChanged);
        connect(newmodel, &ZAbstractTableModel::modelAboutToBeReset, this, &ZListView::aboutToBeReset);
        connect(newmodel, &ZAbstractTableModel::statusChanged, this, &ZListView::updateStatus);
    }

    hover = false;
    pressed = false;

    mousecell = QModelIndex();

    // Unnecessary because base::setModel() calls reset() which in turn calls resetColumnData().
        //if (newmodel != nullptr)
        //    resetColumnData();

    if (oldselmodel != nullptr && selectionModel() != oldselmodel)
    {
        oldselmodel->deleteLater();

        // Our virtual setSelectionModel() override makes sure this happens when the model is set.
        //connect(selectionModel(), &QItemSelectionModel::currentRowChanged, this, &ZListView::currentRowChanged);
    }

    updateStatus();

    if (prevcur != currentrow)
        emit currentRowChanged(currentrow, prevcur);
    if (selchange)
        signalSelectionChanged();
}

ZAbstractTableModel* ZListView::model() const
{
    return (ZAbstractTableModel*)base::model();
}

void ZListView::assignStatusBar(ZStatusBar *bar)
{
    if (bar == status)
        return;
    if (status != nullptr)
    {
        disconnect(status, &QObject::destroyed, this, &ZListView::statusDestroyed);
        disconnect(status, &ZStatusBar::assigned, this, &ZListView::statusDestroyed);
    }

    status = bar;
    if (status != nullptr)
    {
        connect(status, &QObject::destroyed, this, &ZListView::statusDestroyed);
        status->assignTo(this);
        connect(status, &ZStatusBar::assigned, this, &ZListView::statusDestroyed);
    }

    updateStatus();
}

ZStatusBar* ZListView::statusBar() const
{
    return status;
}

QSize ZListView::sizeHint() const
{
    if (sizhint.isEmpty())
        return base::sizeHint();

    return sizhint;
}

void ZListView::setSizeHint(QSize newsizehint)
{
    sizhint = newsizehint;
    _invalidate();
}

void ZListView::setFontSizeHint(int charwidth, int charheight)
{
    QFontMetrics fm(font());
    sizhint = QSize(fm.averageCharWidth() * charwidth, fm.height() * charheight);
    _invalidate();
}

ListSizeBase ZListView::sizeBase() const
{
    return sizebase;
}

void ZListView::setSizeBase(ListSizeBase newbase)
{
    if (newbase == sizebase)
        return;

    sizebase = newbase;

    if (sizebase == ListSizeBase::Custom)
        return;

    LineSize siz = sizebase == ListSizeBase::Main ? Settings::fonts.mainsize : Settings::fonts.popsize;
    verticalHeader()->setDefaultSectionSize(Settings::scaled(siz == LineSize::Medium ? 19 : siz == LineSize::Small ? 17 : siz == LineSize::Large ? 24 : 28));
}

int ZListView::checkBoxColumn() const
{
    return checkcol;
}

void ZListView::setCheckBoxColumn(int newcol)
{
    checkcol = newcol;
}

void ZListView::clickCheckBox(const QModelIndex &index)
{
    if (!index.isValid() || !index.flags().testFlag(Qt::ItemIsUserCheckable))
        return;

    ZAbstractTableModel *m = model();

    Qt::CheckState newstate = m->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked;
    if (!rowSelected(index.row()))
    {
        m->setData(index, newstate, Qt::CheckStateRole);
        viewport()->update(itemDelegate()->checkBoxRect(index));
        return;
    }

    int col = index.column();
    std::vector<int> rows;
    selectedRows(rows);
    for (int ix = 0, siz = tosigned(rows.size()); ix != siz; ++ix)
    {
        QModelIndex ind = m->index(rows[ix], col);
        if (!ind.flags().testFlag(Qt::ItemIsUserCheckable))
            continue;
        m->setData(ind, newstate, Qt::CheckStateRole);
        if (rows[ix] >= firstrow && rows[ix] <= lastrow)
            viewport()->update(itemDelegate()->checkBoxRect(ind));
    }
    //viewport()->update();
}

int ZListView::editColumn() const
{
    return editcolumn;
}

void ZListView::setEditColumn(int col)
{
    editcolumn = std::max(-1, col);
}

bool ZListView::editing() const
{
    return base::state() == EditingState;
}

ZListViewItemDelegate* ZListView::itemDelegate()
{
    if (dynamic_cast<ZListViewItemDelegate*>(base::itemDelegate()) == nullptr)
    {
        base::itemDelegate()->deleteLater();
        setItemDelegate(/*createItemDelegate()*/ new ZListViewItemDelegate(this));
    }

    return static_cast<ZListViewItemDelegate*>(base::itemDelegate());
}

void ZListView::setItemDelegate(ZListViewItemDelegate *newdel)
{
    //if (dynamic_cast<ZListViewItemDelegate*>(base::itemDelegate()) == nullptr)
    base::itemDelegate()->deleteLater();

    newdel->setParent(this);
    base::setItemDelegate(newdel);

    saveColumnData();
    resetColumnData();
}

void ZListView::settingsChanged()
{
    Settings::updatePalette(this);

    viewport()->update();

    if (sizebase != ListSizeBase::Custom)
    {
        ListSizeBase tmp = sizebase;
        sizebase = ListSizeBase::Custom;
        setSizeBase(tmp);
    }
    autoResizeColumns(true);
}

//void ZListView::sectionResized(int ix, int oldsiz, int newsiz)
//{
//    if (ix != 0)
//        return;
//    if (oldsiz != newsiz)
//        oldsiz = newsiz;
//}

void ZListView::aboutToBeReset()
{
    saveColumnData();
}

void ZListView::reset()
{
    doubleclicktimer.stop();

    base::reset();

    int prevcur = currentrow;
    currentrow = -1;
    bool selchange = !selection->empty() || (selpivot != -1 && currentrow != -1);
    selection->clear();
    hover = false;
    pressed = false;
    mousecell = QModelIndex();

    selpivot = -1;

    resetColumnData();

    updateStatus();

    if (prevcur != currentrow)
        emit currentRowChanged(currentrow, prevcur);
    if (selchange)
        signalSelectionChanged();
}

void ZListView::selectAll()
{
    if (model() == nullptr || (seltype != ListSelectionType::Extended && seltype != ListSelectionType::Toggle))
        return;
    commitPivotSelection();
    int endrow = mapToSelection(model()->rowCount() - 1);
    if (selection->rangeSelected(0, endrow))
        return;

    selection->selectRange(0, endrow, true);

    viewport()->update();
    signalSelectionChanged();
}

void ZListView::clearSelection()
{
    if (model() == nullptr || (seltype != ListSelectionType::Extended && seltype != ListSelectionType::Toggle))
        return;
    updateSelection();
    commitPivotSelection();
    if (selection->size() == 0)
        return;

    selection->clear();

    updateSelection();
    signalSelectionChanged();
}

void ZListView::rowsRemoved(const smartvector<Range> &ranges)
{
    commitPivotSelection();

    int oldcurr = currentrow;
    currentrow = _removedPosition(ranges, currentrow);
    if (oldcurr != -1 && currentrow == -1 && model()->rowCount() != 0)
        currentrow = 0;
    selRemoved(ranges);
    //selection->remove(ranges);

    if (oldcurr != currentrow)
        emit currentRowChanged(currentrow, oldcurr);

    if (autosize)
        resetColumnData();

    viewport()->update();

    updateStatus();

    updateGeometries();
}

void ZListView::rowsInserted(const smartvector<Interval> &intervals)
{
    commitPivotSelection();

    int oldcurr = currentrow;
    currentrow = _insertedPosition(intervals, currentrow);
    selInserted(intervals);
    //selection->insert(intervals);

    if (oldcurr != currentrow)
        emit currentRowChanged(currentrow, oldcurr);

    if (autosize)
        resetColumnData();

    viewport()->update();

    updateStatus();

    updateGeometries();
}

void ZListView::rowsMoved(const smartvector<Range> &ranges, int pos)
{
    commitPivotSelection();

    int oldcurr = currentrow;
    currentrow = _movedPosition(ranges, pos, currentrow);
    selMoved(ranges, pos);
    //selection->move(ranges, pos);

    if (oldcurr != currentrow)
        emit currentRowChanged(currentrow, oldcurr);

    if (autosize)
        resetColumnData();

    viewport()->update();

    updateStatus();

    updateGeometries();
}

void ZListView::layoutAboutToBeChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/)
{
    commitPivotSelection();

    // Saving the selection into persistent indexes.

    perix.clear();
    ZAbstractTableModel *m = model();
    for (int ix = 0, siz = selection->rangeCount(); ix != siz; ++ix)
    {
        const Range &r = selection->ranges(ix);
        for (int iy = r.first; iy != r.last + 1; ++iy)
            perix << QPersistentModelIndex(m->index(iy, 0));
    }
    perix << QPersistentModelIndex(m->index(currentrow, 0));
    selection->clear();

    // After the layout change, the header view will reset the column data to the built in
    // nonsense defaults. The columns are saved to be restored after the change.
    saveColumnData();
}

void ZListView::layoutChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/)
{
    // Rebuilding the selection from persistent indexes.
    currentrow = perix.last().isValid() ? perix.last().row() : -1;
    perix.pop_back();

    std::sort(perix.begin(), perix.end(), [](const QPersistentModelIndex &a, const QPersistentModelIndex &b) {
        return a.isValid() && (!b.isValid() || a.row() < b.row());
    });

    for (int pos = 0, next = 1, siz = perix.size(), last = false; !last && pos != siz; ++next)
    {
        last = (next == siz || !perix.at(next).isValid());
        if (last || perix[next].row() - perix[next - 1].row() > 1)
        {
            selection->selectRange(perix[pos].row(), perix[next - 1].row(), true);
            pos = next;
        }
    }

    scrollToRow(currentrow);
    updateStatus();

    viewport()->update();

    // Restores column data from before the layout change.
    resetColumnData();
}

void ZListView::selRemoved(const smartvector<Range> &ranges)
{
    if (selection->remove(ranges))
        signalSelectionChanged();
}

void ZListView::selInserted(const smartvector<Interval> &intervals)
{
    if (selection->insert(intervals))
        signalSelectionChanged();
}

void ZListView::selMoved(const smartvector<Range> &ranges, int pos)
{
    if (selection->move(ranges, pos))
        signalSelectionChanged();
}

int ZListView::mapToSelection(int row) const
{
    return row;
}

int ZListView::mapFromSelection(int index, bool /*top*/) const
{
    return index;
}

int ZListView::dropRow(int ypos) const
{
    int row = ypos;

    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem)
    {

        row = row / verticalHeader()->defaultSectionSize() + verticalScrollBar()->value();
        if ((ypos % verticalHeader()->defaultSectionSize()) > verticalHeader()->defaultSectionSize() / 2)
            ++row;
    }
    else
    {
        row = (row + verticalScrollBar()->value()) / verticalHeader()->defaultSectionSize();
        if (((ypos + verticalScrollBar()->value()) % verticalHeader()->defaultSectionSize()) > verticalHeader()->defaultSectionSize() / 2)
            ++row;
    }
    row = std::min(row, model()->rowCount());

    return row;
}

//ZListViewItemDelegate* ZListView::createItemDelegate()
//{
//    return new ZListViewItemDelegate(this);
//}

//bool ZListView::isRowDragSelected(int row) const
//{
//    return rowSelected(row); //selectionModel()->isRowSelected(row, QModelIndex());
//}

QMimeData* ZListView::dragMimeData() const
{
    //auto rows = selectionModel()->selectedRows();
    //std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b) { return a.row() < b.row(); });

    QModelIndexList indexes;
    if (seltype != ListSelectionType::None)
        indexes = selectedIndexes();
    else if (currentrow != -1)
        indexes << model()->index(currentrow, 0);
    return model()->mimeData(indexes);
}

//void ZListView::toggleRowSelect(int row, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
//{
//    // It might be necessary to create our own selectRow function.
//    // ** KEEP THIS **
//    //selectionModel()->setCurrentIndex(mousecell, QItemSelectionModel::NoUpdate | QItemSelectionModel::Rows);
//    //selectionModel()->select(QItemSelection(model()->index(mousecell.row(), 0), model()->index(mousecell.row(), model()->columnCount() - 1)), QItemSelectionModel::SelectCurrent);
//
//    //QItemSelectionModel::SelectionFlags flags = model() != nullptr ? selectionCommand(model()->index(0, 0)) : 0;
//
//    if (seltype == NoSelection)
//        return;
//
//    selectRow(row);
//}

void ZListView::changeCurrent(int rowindex)
{
    int row = currentrow;
    if (row != rowindex)
    {
        if (currentrow != -1)
            updateRow(currentrow);
        currentrow = rowindex;
        if (currentrow != -1)
            updateRow(currentrow);

        updateStatus();

        emit currentRowChanged(currentrow, row);
    }
}

bool ZListView::requestingContextMenu(const QPoint &/*pos*/, const QPoint &/*globalpos*/, int /*selindex*/)
{
    return false;
}

void ZListView::scrollContentsBy(int dx, int dy)
{
    base::scrollContentsBy(dx, dy);

    if (state != State::Dragging)
        return;

    QDragMoveEvent e = QDragMoveEvent(viewport()->mapFromGlobal(QCursor::pos()), savedDropActions, savedMimeData, savedMouseButtons, savedKeyboardModifiers);
    dragMoveEvent(&e);
}

void ZListView::dataChanged(const QModelIndex &topleft, const QModelIndex &bottomright, const QVector<int> &roles)
{
    base::dataChanged(topleft, bottomright, roles);

    ZAbstractTableModel *m = model();
    if (m == nullptr)
        return;

    autoResizeColumns(true);
}

//bool tmptmp = false;

bool ZListView::event(QEvent *e)
{
    //if (e->type() == QEvent::DragEnter)
    //{
    //    setStyleSheet("background-color: yellow;");
    //    update();
    //    tmptmp = true;
    //    dragEnterEvent((QDragEnterEvent*)e);
    //    tmptmp = false;
    //    return true;
    //}
    //if (e->type() == ConstructedEvent::Type())
    //{
    //    // Only create our custom delegate if one hasn't been set previously.
    //    if (dynamic_cast<ZListViewItemDelegate*>(base::itemDelegate()) == nullptr)
    //    {
    //        base::itemDelegate()->deleteLater();
    //        setItemDelegate(createItemDelegate());
    //    }
    //    else
    //    {
    //        saveColumnData();
    //        resetColumnData();
    //    }

    //    return true;
    //}

    if (e->type() == QEvent::LanguageChange)
        updateStatus();

    return base::event(e);
}

void ZListView::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == doubleclicktimer.timerId())
    {
        doubleclicktimer.stop();
        
        selectionModel()->setCurrentIndex(editcell, QItemSelectionModel::SelectCurrent);
        edit(editcell);
        selectionModel()->select(editcell, QItemSelectionModel::Deselect);
        return;
    }
    base::timerEvent(e);
}

void ZListView::focusOutEvent(QFocusEvent *e)
{
    doubleclicktimer.stop();
    cancelActions();

    base::focusOutEvent(e);
}

void ZListView::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    autoResizeColumns(true);
}

void ZListView::verticalScrollbarValueChanged(int /*value*/)
{
    // Used to fetch more data and update some hover status we don't need. It uses persistent
    // indexes that must be prevented.
    // - base::verticalScrollbarValueChanged(value);

    autoResizeColumns();
}

void ZListView::horizontalScrollbarValueChanged(int /*value*/)
{
    // Used to fetch more data and update some hover status we don't need. It uses persistent
    // indexes that must be prevented.
    // - base::verticalScrollbarValueChanged(value);
}

void ZListView::showEvent(QShowEvent *e)
{
    doubleclicktimer.stop();

    base::showEvent(e);
    if (model() != nullptr)
        autoResizeColumns(true);
        //resetColumnData();
}

void ZListView::keyPressEvent(QKeyEvent *e)
{
    doubleclicktimer.stop();
    stopAutoScroll();

    if (model() == nullptr)
    {
        QFrame::keyPressEvent(e);
        return;
    }

    int rowcnt = model()->rowCount();
    //int colcnt = model()->columnCount();
    if (rowcnt == 0 && e->key() != Qt::Key_Left && e->key() != Qt::Key_Right)
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
        //: Deselect All key combination (Select All is Ctrl+A in English)
        || e == QKeySequence(tr("Ctrl+D", "Deselect")) || e == QKeySequence(tr("Ctrl+Shift+A", "Deselect"))
#endif
        )
    {
        clearSelection();
        QFrame::keyPressEvent(e);
        return;
    }

    int viewrows = lastrow - firstrow + 1;

    bool ctrl = (e->key() == Qt::Key_Space || (seltype != ListSelectionType::None && seltype != ListSelectionType::Single)) && (e->modifiers() & Qt::ControlModifier) != 0;
    bool shift = (seltype != ListSelectionType::None && seltype != ListSelectionType::Single) && (e->modifiers() & Qt::ShiftModifier) != 0;

    int row = currentrow;

    switch (e->key()) {
    case Qt::Key_Down:
        if (row == -1)
            row = 0;
        else
            row = std::min(rowcnt - 1, row + 1);
        break;
    case Qt::Key_Up:
        if (row == -1)
            row = 0;
        else
            row = std::max(0, row - 1);
        break;
    case Qt::Key_Left:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - 15);
        QFrame::keyPressEvent(e);
        return;
    case Qt::Key_Right:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 15);
        QFrame::keyPressEvent(e);
        return;
    case Qt::Key_Home:
        row = 0;
        break;
    case Qt::Key_End:
        row = rowcnt - 1;
        break;
    case Qt::Key_PageUp:
        row = std::max(0, row - viewrows);
        break;
    case Qt::Key_PageDown:
        row = std::min(rowcnt - 1, row + viewrows);
        break;
    case Qt::Key_Select:
        if (currentrow == -1)
        {
            QFrame::keyPressEvent(e);
            return;
        }
        ctrl = true;
    case Qt::Key_Space:
    {
        if (currentrow == -1 || seltype == ListSelectionType::None)
        {
            QFrame::keyPressEvent(e);
            return;
        }

        if (checkcol != -1 && !ctrl)
        {
            if (model()->flags(model()->index(currentrow, checkcol)).testFlag(Qt::ItemIsUserCheckable))
            {
                QModelIndex index = model()->index(currentrow, checkcol);
                clickCheckBox(index/*, model()->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked*/);
                //viewport()->update(itemDelegate()->checkBoxRect(index));
                return;
            }
        }

        if (seltype == ListSelectionType::Single)
            setCurrentRow(currentrow);
        else
        {
            commitPivotSelection();
            scrollToRow(currentrow);
            int selcurrent = mapToSelection(currentrow);
            bool sel = selection->selected(selcurrent);
            if (!ctrl && seltype != ListSelectionType::Toggle && sel)
            {
                QFrame::keyPressEvent(e);
                return;
            }
            if ((ctrl || seltype == ListSelectionType::Toggle) && sel)
                updateSelection();
            selection->setSelected(selcurrent, ctrl || seltype == ListSelectionType::Toggle ? !sel : true);
            updateRow(currentrow);
            if ((!ctrl && seltype != ListSelectionType::Toggle) || !sel)
                updateSelection();
            signalSelectionChanged();
        }
        break;
    }

    // COPIED (and modified) FROM QT SOURCE qabstractitemview.cpp
#ifdef Q_OS_MAC
    case Qt::Key_Enter:
    case Qt::Key_Return:
        // Propagate the enter if you couldn't edit the item and there are no
        // current editors (if there are editors, the event was most likely propagated from it).
        if (editcolumn == -1 || model()->columnCount() <= editcolumn || currentrow == -1 || currentrow >= model()->rowCount() || model() == nullptr || !model()->flags(model()->index(currentrow, editcolumn)).testFlag(Qt::ItemIsEditable) || (!edit(model()->index(currentrow, editcolumn), EditKeyPressed, e) && base::state() != EditingState))
            e->ignore();
            break;
#else
    case Qt::Key_F2:
    {
        if (editcolumn == -1 || model()->columnCount() <= editcolumn || currentrow == -1 || currentrow >= model()->rowCount() || model() == nullptr || !model()->flags(model()->index(currentrow, editcolumn)).testFlag(Qt::ItemIsEditable) || !edit(model()->index(currentrow, editcolumn), EditKeyPressed, e))
            e->ignore();
        break;
    }
    case Qt::Key_Enter:
    case Qt::Key_Return:
        // ### we can't open the editor on enter, becuse
        // some widgets will forward the enter event back
        // to the viewport, starting an endless loop
        if (base::state() != EditingState || hasFocus()) {
            //if (currentIndex().isValid())
            //    emit activated(currentIndex());
            e->ignore();
        }
        break;
#endif
    // END COPY


    default:
        QFrame::keyPressEvent(e);
        return;
    }

    if (row == currentrow)
    {
        QFrame::keyPressEvent(e);
        return;
    }

    if (!shift && ctrl)
    {
        //if (!ctrl)
        //    clearSelection();
        //else
            commitPivotSelection();
    }

    if (currentrow == -1 || (!shift && !ctrl))
    {
        setCurrentRow(row);
        QFrame::keyPressEvent(e);
        return;
    }

    if (shift)
    {
        int selcurrent = mapToSelection(currentrow);
        int selindex = mapToSelection(row);

        bool selchange = false;
        if (!ctrl && !selection->empty())
        {
            updateSelection();
            selection->clear();
            selchange = true;
        }

        // Temporary range selection.

        if (selpivot == -1)
        {
            selpivot = selcurrent;
            selchange = true;
        }
        else if (!selchange && selcurrent != selindex)
        {
            // Updating the old selection range.

            int first = selcurrent;
            int last = selpivot;
            if (first > last)
                std::swap(first, last);
            updateRows(mapFromSelection(first, true), mapFromSelection(last, false));
            selchange = true;
        }

        std::swap(currentrow, row);
        scrollToRow(currentrow);
        selchange = selchange || !selection->rangeSelected(std::min(selpivot, selindex), std::max(selpivot, selindex));

        if (selchange)
        {
            // Updating the new selection range.

            int first = selindex;
            int last = selpivot;
            if (first > last)
                std::swap(first, last);
            updateRows(mapFromSelection(first, true), mapFromSelection(last, false));

            signalSelectionChanged();
        }

        emit currentRowChanged(currentrow, row);

        QFrame::keyPressEvent(e);
        return;
    }

    if (ctrl)
    {
        // Move the cursor only.
        if (currentrow != -1)
            updateRow(currentrow);
        std::swap(currentrow, row);
        updateRow(currentrow);
        scrollToRow(currentrow);

        emit currentRowChanged(currentrow, row);
    }

    QFrame::keyPressEvent(e);
}

void ZListView::mouseMoveEvent(QMouseEvent *e)
{
    QFrame::mouseMoveEvent(e);
    e->accept();

    if (mousedown && (state == State::CanDrag || state == State::CanDragOrSelect))
    {
        if ((mousedownpos - e->pos()).manhattanLength() > QApplication::startDragDistance())
        {
            commitPivotSelection();

            QMimeData *mdat = dragMimeData();
            if (mdat == nullptr)
                return;

            mousedown = false;

            QDrag *drag = new QDrag(this);
            drag->setMimeData(mdat);

            state = State::None;

            if (drag->exec(model()->supportedDragActions(), Qt::CopyAction) == Qt::MoveAction)
                removeDragRows();

            stopAutoScroll();

            return;
        }
    }

    QModelIndex i = indexAt(e->pos());

    if (hover)
    {
        QRect chr = itemDelegate()->checkBoxHitRect(mousecell);
        if (i != mousecell || !chr.contains(e->pos()))
        {
            // Moving out from a checkbox.
            hover = false;

            viewport()->update(itemDelegate()->checkBoxRect(mousecell)); //base::update(mousecell);

            if (!pressed)
                mousecell = QModelIndex();
        }
    }

    if (hover || !i.isValid() || (pressed && i != mousecell) || (!pressed && e->buttons().testFlag(Qt::LeftButton)))
        return;

    bool chk = pressed || i.flags().testFlag(Qt::ItemIsUserCheckable);
    if (chk)
    {
        QRect chr = itemDelegate()->checkBoxHitRect(i);

        // Moving over a checkbox.

        if (chr.contains(e->pos()))
        {
            hover = true;
            mousecell = i;

            viewport()->update(itemDelegate()->checkBoxRect(i)); //base::update(mousecell);

        }
    }

    if (!pressed)
        mousecell = i;
}

void ZListView::mousePressEvent(QMouseEvent *e)
{
    QFrame::mousePressEvent(e);
    e->accept();

    doubleclicktimer.stop();

    if (doubleclick || mousedown || (e->button() != Qt::LeftButton && e->button() != Qt::RightButton))
    {
        doubleclick = false;
        return;
    }

    QModelIndex i = indexAt(e->pos());

    if (hover)
    {
        QRect chr = itemDelegate()->checkBoxHitRect(mousecell);
        if (i != mousecell || !chr.contains(e->pos()))
        {
            // Moving out from a checkbox.
            hover = false;

            viewport()->update(itemDelegate()->checkBoxRect(mousecell)); //base::update(mousecell);
        }
    }

    if (!hover && i.isValid())
    {
        bool chk = i.flags().testFlag(Qt::ItemIsUserCheckable);
        if (chk)
        {
            QRect chr = itemDelegate()->checkBoxHitRect(i);

            // Moving over a checkbox.

            if (chr.contains(e->pos()))
            {
                hover = true;
                viewport()->update(itemDelegate()->checkBoxRect(i)); //base::update(mousecell);
            }
        }
    }

    mousecell = i;
    if (e->button() == Qt::LeftButton)
        mousedown = true;

    mousedownpos = e->pos();

    canclickedit = false;
    if (!mousecell.isValid())
        return;

    if (hover && e->modifiers() == 0)
    {
        if (e->button() != Qt::LeftButton)
            return;
        pressed = true;
        viewport()->update(itemDelegate()->checkBoxRect(mousecell)); //base::update(mousecell);
        return;
    }

    //if (seltype == ListSelectionType::None)
    //{
    //    changeCurrent(mousecell.row());
    //    //selectionModel()->setCurrentIndex(mousecell, QItemSelectionModel::NoUpdate);
    //    return;
    //}

    bool allowmulti = seltype == ListSelectionType::Extended;
    bool allowtoggle = allowmulti || seltype == ListSelectionType::Toggle;
    bool multi = allowmulti && e->modifiers().testFlag(Qt::ShiftModifier);
    bool toggle = allowtoggle && (seltype == ListSelectionType::Toggle || e->modifiers().testFlag(Qt::ControlModifier));

    if (!multi && !toggle)
    {
        //selectionModel()->setCurrentIndex(model()->index(mousecell.row(), 0), QItemSelectionModel::NoUpdate);
        bool sel = rowSelected(mousecell.row()); //isRowDragSelected(mousecell.row());
        canclickedit = sel && !e->modifiers().testFlag(Qt::ShiftModifier) && !e->modifiers().testFlag(Qt::ControlModifier) && e->button() == Qt::LeftButton && selectionSize() == 1 && model() != nullptr && model()->flags(mousecell).testFlag(Qt::ItemIsEditable);
        if (!sel || (e->button() == Qt::LeftButton && (!dragEnabled() || e->modifiers().testFlag(Qt::ShiftModifier) || e->modifiers().testFlag(Qt::ControlModifier))))
            setCurrentRow(mousecell.row()); //toggleRowSelect(mousecell.row(), e->button(), e->modifiers());
        else
        {
            if (sel && e->button() == Qt::LeftButton)
                state = State::CanDragOrSelect;
            commitPivotSelection();
            changeCurrent(mousecell.row());
        }
    }
    else if (multi)
    {
        // Shift key is pressed. Ctrl might be pressed too if toggle is true.

        bool selchange = false;

        int selmouse = mapToSelection(mousecell.row());
        int selcurrent = currentrow == -1 ? -1 : mapToSelection(currentrow);

        if (!toggle && !selection->empty())
        {
            updateSelection();
            selection->clear();
            selchange = true;
        }

        if (selpivot == -1)
        {
            selpivot = currentrow != -1 ? selcurrent : selmouse;
            selchange = true;
        }
        else if (!selchange && selcurrent != selmouse)
        {
            // Updating old selection range.

            int first = selpivot;
            int last = selcurrent;
            if (first > last)
                std::swap(first, last);
            updateRows(mapFromSelection(first, true), mapFromSelection(last, false));

            selchange = true;
        }

        int first = selpivot;
        int last = selmouse;
        if (first > last)
            std::swap(first, last);

        selchange = selchange || selcurrent != selmouse || !selection->rangeSelected(std::min(selpivot, selmouse), std::max(selpivot, selmouse));
        changeCurrent(mousecell.row());

        if (selchange)
        {
            updateRows(mapFromSelection(first, true), mapFromSelection(last, false));
            signalSelectionChanged();
        }

        //QItemSelection sel = QItemSelection(model()->index(toprow, 0), model()->index(bottomrow, model()->columnCount() - 1));
        //ignorechange = true;
        //int oldrow = currentrow != -1 ? currentrow : -1;
        //int newrow = mousecell.row();
        //if (oldrow != -1 && oldrow != newrow)
        //    updateRow(oldrow);
        //selectionModel()->setCurrentIndex(model()->index(mousecell.row(), 0), QItemSelectionModel::NoUpdate | QItemSelectionModel::Rows);
        //if (oldrow != newrow)
        //    updateRow(newrow);

        //QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows;
        //if (!toggle)
        //    flags |= QItemSelectionModel::Clear;
        //selectionModel()->select(sel, flags);

        //ignorechange = false;

        //newrow = currentrow != -1 ? currentrow : -1;
        //if (newrow != oldrow)
        //    emit currentRowChanged(newrow, oldrow);
    }
    else /* if (toggle) */
    {
        // Only ctrl key pressed.

        commitPivotSelection();

        int selmouse = mapToSelection(mousecell.row());

        bool sel = selection->selected(selmouse);
        if (sel && e->button() != Qt::LeftButton)
        {
            changeCurrent(mousecell.row());
            return;
        }

        selection->setSelected(selmouse, !sel);

        changeCurrent(mousecell.row());
        updateRows(mapFromSelection(selmouse, true), mapFromSelection(selmouse, false));

        //toggleRowSelect(mousecell.row(), e->button(), e->modifiers());

        signalSelectionChanged();
    }

    if (e->button() == Qt::LeftButton && dragEnabled() && state == State::None && (rowSelected(mousecell.row()) || (seltype == ListSelectionType::None && currentrow == mousecell.row())))
        state = State::CanDrag;

}

void ZListView::mouseDoubleClickEvent(QMouseEvent *e)
{
    doubleclicktimer.stop();

    doubleclick = true;
    QFrame::mouseDoubleClickEvent(e);
    e->accept();

    if (!mousecell.isValid() || hover || e->button() != Qt::LeftButton)
        return;

    QPersistentModelIndex per;
    if (editcolumn != -1 && mousecell.column() == editcolumn && editTriggers().testFlag(DoubleClicked) && model() != nullptr && model()->flags(mousecell).testFlag(Qt::ItemIsEditable))
        per = mousecell;

    emit rowDoubleClicked(mousecell.row());

    if (per.isValid())
        edit(per, DoubleClicked, e);
}

void ZListView::mouseReleaseEvent(QMouseEvent *e)
{
    QFrame::mouseReleaseEvent(e);
    e->accept();

    if (!mousedown || e->button() != Qt::LeftButton)
        return;

    if (state == State::CanDragOrSelect && rowSelected(mousecell.row()))
        setCurrentRow(mousecell.row()); //toggleRowSelect(mousecell.row(), e->button(), e->modifiers());

    state = State::None;
    mousedown = false;

    if (editcolumn != -1 && editcolumn == mousecell.column() && canclickedit && editTriggers().testFlag(SelectedClicked))
    {
        if (editTriggers().testFlag(DoubleClicked))
        {
            // Start editing right away if double click would also start an editor event.
            // Otherwise edit only after the double click time is up.
            selectionModel()->setCurrentIndex(mousecell, QItemSelectionModel::SelectCurrent);
            edit(mousecell/*, SelectedClicked, e*/);
            selectionModel()->select(mousecell, QItemSelectionModel::Deselect);
        }
        else
        {
            editcell = mousecell;
            doubleclicktimer.start(qApp->doubleClickInterval() + 100, this);
        }
    }
    canclickedit = false;

    if (!pressed || !mousecell.isValid())
        return;

    pressed = false;

    bool chk = mousecell.flags().testFlag(Qt::ItemIsUserCheckable); //(model()->data(mousecell, (int)CellRoles::Flags).toInt() & (int)CellFlags::CheckBox) == (int)CellFlags::CheckBox;
    if (chk)
    {
        QRect chr = itemDelegate()->checkBoxHitRect(mousecell);
        hover = chr.contains(e->pos());
        if (hover)
            clickCheckBox(mousecell/*, model()->data(mousecell, Qt::CheckStateRole).toInt() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked*/);
            //model()->setData(mousecell, model()->data(mousecell, Qt::CheckStateRole).toInt() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        else
            viewport()->update(itemDelegate()->checkBoxRect(mousecell)); //base::update(mousecell);
    }

    mouseMoveEvent(e);
}

void ZListView::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    // Don't forget viewportEvent() when changing this.

    if (!mousecell.isValid())
        return;

    // Moving out of the cell.

    if (hover)
    {
        hover = false;
        viewport()->update(itemDelegate()->checkBoxRect(mousecell));

        if (!pressed)
            mousecell = QModelIndex();
    }
}

void ZListView::dragEnterEvent(QDragEnterEvent *e)
{
    //if (!tmptmp)
    //{
    //    setStyleSheet("background-color: red;");
    //    update();
    //    repaint();
    //}

    //if (e->source() != this)
    //    QMessageBox::information(parentWidget(), "zkanji", "Imhere", QMessageBox::Ok);

    e->ignore();

    if (model() == nullptr)
    {
        //if (e->source() != this)
        //    QMessageBox::information(parentWidget(), "zkanji", "nomodel :(", QMessageBox::Ok);
        return;
    }

    //bool supported = true;
    const QMimeData *mdat = e->mimeData();

    if (((int)e->possibleActions() & (int)model()->supportedDropActions(e->source() == this, mdat)) == 0)
    {
        //if (e->source() != this)
        //    QMessageBox::information(parentWidget(), "zkanji", QString("%1, %2").arg(e->possibleActions()).arg(model()->supportedDropActions(e->source() == this, mdat->formats())), QMessageBox::Ok);

        return;
    }

    //QStringList types = model()->mimeTypes();
    //for (const QString &t : types)
    //{
    //    if (mdat->hasFormat(t))
    //    {
    //        supported = true;
    //        break;
    //    }
    //}

    //if (!supported)
    //{
    //    //if (e->source() != this)
    //    //    QMessageBox::information(parentWidget(), "zkanji", "nosupport :(", QMessageBox::Ok);
    //    return;
    //}
    //if (e->source() != this)
    //    QMessageBox::information(parentWidget(), "zkanji", "accepted", QMessageBox::Ok);

    e->accept();
    state = State::Dragging;

    savedDropActions = e->dropAction();
    savedMimeData = e->mimeData();
    savedMouseButtons = e->mouseButtons();
    savedKeyboardModifiers = e->keyboardModifiers();

    setDragColorScheme(true);
    viewport()->update();
}

void ZListView::dragLeaveEvent(QDragLeaveEvent * /*e*/)
{
    if (state != State::Dragging)
        return;

    stopAutoScroll();

    setDragColorScheme(false);

    state = State::None;
    dragpos = -1;

    viewport()->update();
}

void ZListView::dragMoveEvent(QDragMoveEvent *e)
{
    e->ignore();

    if (state != State::Dragging)
    {
        //QMessageBox::information(parentWidget(), "zkanji", QString("notdragging"), QMessageBox::Ok);
        return;
    }

    savedDropActions = e->dropAction();
    savedMimeData = e->mimeData();
    savedMouseButtons = e->mouseButtons();
    savedKeyboardModifiers = e->keyboardModifiers();

    int row = dropRow(e->pos().y());

    Qt::DropAction action = e->source() == this ? Qt::MoveAction : e->dropAction();
    Qt::DropActions supaction = model()->supportedDropActions(e->source() == this, e->mimeData());
    if ((action & supaction) == 0)
    {
        action = (e->source() != this && (supaction & Qt::CopyAction) != 0) ? Qt::CopyAction : (supaction & Qt::MoveAction) != 0 ? Qt::MoveAction : Qt::IgnoreAction;
        if (action == Qt::IgnoreAction)
            return;
    }

    if (hasAutoScroll())
    {
        QRect vr = viewport()->rect();
        QPoint cp = e->pos();
        if (cp.y() < vr.top() + autoScrollMargin() || cp.y() > vr.bottom() - autoScrollMargin())
            startAutoScroll();
    }

    QModelIndex curitem = model()->index(indexAt(e->pos()).row(), 0);
    QRect currowrect = visualRect(curitem);
    bool onitem = !currowrect.isEmpty() && (e->pos().y() >= currowrect.top() + currowrect.height() / 4) && (e->pos().y() < currowrect.bottom() - currowrect.height() / 4);

    bool abortdrop = false;

    if (onitem)
    {
        if (curitem.isValid() && model()->canDropMimeData(e->mimeData(), action, -1, -1, curitem))
            row = curitem.row();
        else
        {
            onitem = false;

            if (!model()->canDropMimeData(e->mimeData(), action, row, 0, QModelIndex()))
                abortdrop = true;
        }
    }
    else if (!model()->canDropMimeData(e->mimeData(), action, row, 0, QModelIndex()))
    {
        if (curitem.isValid() && model()->canDropMimeData(e->mimeData(), action, -1, -1, curitem))
        {
            row = curitem.row();
            onitem = true;
        }
        else
            abortdrop = true;
    }

    if (abortdrop)
    {
        // Couldn't find good position on or above/below an item.

        if (dragpos != -1)
            updateDragIndicator();
        row = -1;
        dragpos = -1;
        dragon = false;

        //if (e->source() != this)
        //    QMessageBox::information(parentWidget(), "zkanji", "aborting", QMessageBox::Ok);

        // Checks whether drop is allowed on the view itself and returns if not.
        if (!model()->canDropMimeData(e->mimeData(), action, -1, -1, QModelIndex()))
            return;
    }

    //if (e->source() != this)
    //    QMessageBox::information(parentWidget(), "zkanji", "notaborted", QMessageBox::Ok);


    if (action != e->dropAction())
    {
        e->setDropAction(action);
        e->accept();
    }
    else
        e->acceptProposedAction();

    if (row != dragpos || onitem != dragon)
    {
        if (dragpos != -1)
            updateDragIndicator();
        dragon = onitem;
        dragpos = row;
        if (dragpos != -1)
            updateDragIndicator();
    }
}

void ZListView::dropEvent(QDropEvent *e)
{
    e->ignore();

    stopAutoScroll();

    //if (state == State::Dragging && dragpos != -1)
    //    updateDragIndicator();
    dragpos = -1;
    viewport()->update();

    if (state != State::Dragging || (e->source() == this && (model()->supportedDropActions(e->source() == this, e->mimeData()) & Qt::MoveAction) == 0))
        return;

    state = State::None;
    int row = dropRow(e->pos().y());

    Qt::DropAction action = this == e->source() ? Qt::MoveAction : e->dropAction();
    Qt::DropActions supaction = model()->supportedDropActions(e->source() == this, e->mimeData());
    if ((action & supaction) == 0)
    {
        action = (this != e->source() && (supaction & Qt::CopyAction) != 0) ? Qt::CopyAction : (supaction & Qt::MoveAction) != 0 ? Qt::MoveAction : Qt::IgnoreAction;
        if (action == Qt::IgnoreAction)
            return;
    }

    QModelIndex curitem = model()->index(indexAt(e->pos()).row(), 0);
    QRect currowrect = visualRect(curitem);
    bool onitem = !currowrect.isEmpty() && (e->pos().y() >= currowrect.top() + currowrect.height() / 4) && (e->pos().y() < currowrect.bottom() - currowrect.height() / 4);

    if (onitem)
    {
        // Trying to drop into an item first, then below/above a row, and last into the view
        // in general.

        if (!curitem.isValid() || !model()->dropMimeData(e->mimeData(), action, -1, -1, curitem))
        {
            if (!model()->dropMimeData(e->mimeData(), action, row, 0, QModelIndex()) &&
                !model()->dropMimeData(e->mimeData(), action, -1, -1, QModelIndex()))
                return;
        }
    }
    else if (!model()->dropMimeData(e->mimeData(), action, row, 0, QModelIndex()))
    {
        // After trying to drop below/above a row, then into an item, tries to drop into the
        // view in general too.
        if ((!curitem.isValid() || !model()->dropMimeData(e->mimeData(), action, -1, -1, curitem)) &&
            !model()->dropMimeData(e->mimeData(), action, -1, -1, QModelIndex()))
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

    setDragColorScheme(false);
}

void ZListView::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (state != State::Dragging || dragpos == -1 || !showDropIndicator())
        return;

    //QPalette::ColorGroup colorgrp = hasFocus() ? QPalette::Active : QPalette::Inactive;
    QColor col = Settings::textColor(this, ColorSettings::SelBg); ;//palette().color(colorgrp, QPalette::Highlight);

    if (!dragon)
    {
        int cnt = model()->rowCount();
        QRect vrect = visualRect(model()->index(dragpos < cnt ? dragpos : cnt - 1, 0));
        int vh = vrect.height() / 3;
        if (cnt <= dragpos)
            vrect.adjust(0, vrect.height(), 0, vrect.height());

        //viewport()->update(vrect.left(), vrect.top() - vh - 1, vh * 2 + 1, vh * 2 + 1);
        QPainter p(viewport());

        QPainterPath pp;
        pp.moveTo(0, vrect.top() - vh - 1);
        pp.lineTo(vh, vrect.top());
        pp.lineTo(0, vrect.top() + vh);

        p.fillPath(pp, QBrush(col));

        QSize s = viewport()->size();
        p.setPen(col);
        p.drawLine(vh, vrect.top() - 1, s.width() - 1, vrect.top() - 1);
        p.drawLine(vh, vrect.top(), s.width() - 1, vrect.top());
        return;
    }

    QRect vrect = rowRect(dragpos).adjusted(0, 0, -1, -1);
    QPainter p(viewport());

    QPen pen = QPen(col);
    p.setPen(pen);
    p.drawRect(vrect);
    vrect.adjust(1, 1, -1, -1);
    p.drawRect(vrect);
}

bool ZListView::viewportEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::HoverMove:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
        // Moving out of the cell.
        if (mousecell.isValid() && hover)
        {
            hover = false;
            viewport()->update(itemDelegate()->checkBoxRect(mousecell));

            if (!pressed)
                mousecell = QModelIndex();
        }

        // Skipping base class implementations that would save persistent indexes.
        return QAbstractScrollArea::viewportEvent(e);
    default:
        break;
    }
    return base::viewportEvent(e);
}

void ZListView::contextMenuEvent(QContextMenuEvent *e)
{
    doubleclicktimer.stop();

    QModelIndex index = indexAt(e->pos());
    int row = index.isValid() ? mapToSelection(index.row()) : -1;
    if (requestingContextMenu(e->pos(), e->globalPos(), row))
        e->accept();
}

void ZListView::statusDestroyed()
{
    if (status != nullptr)
        assignStatusBar(nullptr);
}

void ZListView::autoResizeColumns(bool forced)
{
    if (!isVisible() || dynamic_cast<ZListViewItemDelegate*>(base::itemDelegate()) == nullptr)
        return;

    ZAbstractTableModel *m = model();
    if (m == nullptr /*|| m->rowCount() == 0*/)
        return;

    bool norows = m->rowCount() == 0;

    int frow = norows ? -1 : rowAt(0);
    int lrow = norows ? -1 : rowAt(viewport()->height() - 1);
    if (!norows && lrow == -1 && viewport()->height() > 0)
        lrow = m->rowCount() - 1;

    if (!forced && firstrow == frow && lastrow == lrow)
        return;

    firstrow = frow;
    lastrow = lrow;
    //if (firstrow == -1 || lastrow == -1)
    //    return;

    doResizeColumns();
}

void ZListView::doResizeColumns()
{
    // Space left in the viewport of the table not filled by a column.
    int stretchw = viewport()->width();

    ZAbstractTableModel *m = model();

    int cc = m->columnCount();

    QStyleOptionViewItem measureoption;
    measureoption.init(this);

    // Width of all the stretchable columns before stretching.
    int stretchsiz = 0;

    std::vector<int> widths;
    widths.resize(cc);

    for (int ix = 0; ix != cc; ++ix)
    {
        //bool stretch = m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::Stretched).toBool();
        ColumnAutoSize autosiztype = (ColumnAutoSize)m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::AutoSized).toInt();

        int w = 48;

        if (autosize && (autosiztype == ColumnAutoSize::Auto || autosiztype == ColumnAutoSize::HeaderAuto || autosiztype == ColumnAutoSize::StretchedAuto))
        {
            if (autosiztype == ColumnAutoSize::HeaderAuto)
            {
                QFont f = horizontalHeader()->font();
                QFontMetrics fm = QFontMetrics(f);

                QString str = m->headerData(ix, Qt::Horizontal, Qt::DisplayRole).toString();
                if (!str.isEmpty())
                    w = std::max(w, fm.boundingRect(str).width() + 8);
            }

            if (firstrow != -1 && lastrow != -1)
            {
                for (int rix = firstrow; rix != lastrow + 1; ++rix)
                {
                    measureoption.rect = visualRect(m->index(rix, ix));
                    w = std::max(w, itemDelegate()->sizeHint(measureoption, m->index(rix, ix)).width() + (showGrid() ? 1 : 0));
                }
            }
        }
        else if (autosiztype == ColumnAutoSize::Stretched)
        {
            w = m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::DefaultWidth).toInt();
            if (w < 0)
                w = -1 * w * verticalHeader()->defaultSectionSize();
            else
                w = std::max(w, 1);

        }
        else
            w = columnWidth(ix);

        widths[ix] = w;
        if (autosiztype == ColumnAutoSize::Stretched || autosiztype == ColumnAutoSize::StretchedAuto)
            stretchsiz += w;
        stretchw -= w;
    }

    if (stretchsiz != 0 && stretchw > 0)
    {
        // Distributing the visible area among the stretched columns. Their initial size
        // determines the distribution of the available space.

        // Include the original width of the stretched columns in the available space.
        stretchw += stretchsiz;

        for (int ix = 0; ix != cc; ++ix)
        {
            ColumnAutoSize autosiztype = (ColumnAutoSize)m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::AutoSized).toInt();
            if (autosiztype != ColumnAutoSize::Stretched && autosiztype != ColumnAutoSize::StretchedAuto)
                continue;

            int w = widths[ix];
            if (w < stretchsiz)
                widths[ix] = stretchw * (double(w) / stretchsiz);
            else
            {
                // Last column found.

                widths[ix] = stretchw;
                break;
            }

            stretchw -= widths[ix];
            stretchsiz -= w;
        }
    }

    for (int ix = 0; ix != cc; ++ix)
        setColumnWidth(ix, widths[ix]);
}

void ZListView::saveColumnData(ZAbstractTableModel *columnmodel)
{
    //if (autosize)
    //    return;

    bool selfmodel = model() != nullptr;
    ZAbstractTableModel *m = selfmodel ? model() : columnmodel;
    if (m == nullptr)
        return;

    int cc = m->columnCount();
    if (cc == 0)
        return;

    tmpcolsize.resize(cc);
    for (int ix = 0; ix != cc; ++ix)
    {
        tmpcolsize[ix].autosize = (ColumnAutoSize)m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::AutoSized).toInt();
        tmpcolsize[ix].defwidth = m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::DefaultWidth).toInt();
        tmpcolsize[ix].freesized = m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool();
        tmpcolsize[ix].width = selfmodel ? columnWidth(ix) : -1;
    }
}

void ZListView::resetColumnData()
{
    ZAbstractTableModel *m = model();
    if (m == nullptr)
    {
        //tmpcolsize.clear();
        return;
    }

    int cc = m->columnCount();
    if (cc != tosigned(tmpcolsize.size()))
        tmpcolsize.clear();

    bool savedmatches = false;
    if (!tmpcolsize.empty())
    {
        savedmatches = true;
        for (int ix = 0; savedmatches && ix != cc; ++ix)
        {
            if (tmpcolsize[ix].autosize != (ColumnAutoSize)m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::AutoSized).toInt() ||
                tmpcolsize[ix].defwidth != m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::DefaultWidth).toInt() ||
                tmpcolsize[ix].freesized != m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
                savedmatches = false;
        }
    }

    //bool norows = m->rowCount() == 0;
    for (int ix = 0; ix != cc; ++ix)
    {
        if (m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
            horizontalHeader()->setSectionResizeMode(ix, QHeaderView::Interactive);
        else
            horizontalHeader()->setSectionResizeMode(ix, QHeaderView::Fixed);

        if (/*!norows &&*/ autosize && m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::AutoSized).toInt() == (int)ColumnAutoSize::Auto)
            continue;

        int w;
        if (savedmatches && tmpcolsize[ix].width != -1)
            w = tmpcolsize[ix].width;
        else if (m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::FreeSized).toBool())
            w = columnWidth(ix);
        else
        {
            w = m->headerData(ix, Qt::Horizontal, (int)ColumnRoles::DefaultWidth).toInt();
            if (w == 0)
            {
                QFont f = horizontalHeader()->font();
                QFontMetrics fm = QFontMetrics(f);

                QString str = m->headerData(ix, Qt::Horizontal, Qt::DisplayRole).toString();
                if (str.isEmpty())
                    w = -1;
                else
                    w = fm.boundingRect(str).width() + 8;
            }
            if (w < 0)
                w = -1 * w * verticalHeader()->defaultSectionSize();
        }
        setColumnWidth(ix, w);
    }

    tmpcolsize.clear();
    autoResizeColumns(true);
}

void ZListView::updateDragIndicator()
{
    if (dragpos == -1)
        return;
   
    if (!dragon)
    {
        int cnt = model()->rowCount();
        QRect vrect = visualRect(model()->index(dragpos < cnt ? dragpos : cnt - 1, 0));
        int vh = vrect.height() / 3;
        if (cnt <= dragpos)
            vrect.adjust(0, vrect.height(), 0, vrect.height());

        QSize s = viewport()->size();
        viewport()->update(0, vrect.top() - vh - 1, vh * 2 + 1, vh * 2 + 1);
        viewport()->update(0, vrect.top() - 1, s.width(), 2);
    }
    else
    {
        QRect vrect = visualRect(model()->index(dragpos, 0));
        vrect.setLeft(0);
        vrect.setWidth(viewport()->width());
        viewport()->update(vrect);
    }
}

void ZListView::setDragColorScheme(bool turnon)
{
    static bool schemeon = false;
    if (schemeon == turnon)
        return;

    schemeon = turnon;

    QPalette newpal = palette();

    if (schemeon)
    {
        // Change palette color of highlighted text and background to the inactive scheme.

        dragbgcol = palette().color(QPalette::Active, QPalette::Highlight);
        dragtextcol = palette().color(QPalette::Active, QPalette::HighlightedText);

        newpal.setColor(QPalette::Active, QPalette::Highlight, palette().color(QPalette::Inactive, QPalette::Highlight));
        newpal.setColor(QPalette::Active, QPalette::HighlightedText, palette().color(QPalette::Inactive, QPalette::HighlightedText));
    }
    else
    {
        // Restores original palette colors saved when scheme was turned on.
        newpal.setColor(QPalette::Active, QPalette::Highlight, dragbgcol);
        newpal.setColor(QPalette::Active, QPalette::HighlightedText, dragtextcol);
    }

    setPalette(newpal);
}

void ZListView::_invalidate()
{
    if (parentWidget()->layout() != nullptr)
        parentWidget()->layout()->invalidate();
}

void ZListView::updateStatus()
{
    if (status == nullptr || model() == nullptr)
    {
        if (status != nullptr)
            status->clear();
        return;
    }
    
    int scnt = model()->statusCount();
    if ((scnt == 0 && status->size() != 1) || (scnt != 0 && tosigned(status->size()) != scnt + 1))
    {
        status->clear();

        status->add(QString(), 0, "0 :", 9, "0", 10);

        if (scnt == 0)
            status->add(QString(), 0);
        else
        {
            for (int ix = 0, siz = scnt; ix != siz; ++ix)
            {
                switch (model()->statusType(ix))
                {
                case StatusTypes::TitleValue:
                    status->add(model()->statusText(ix, -1, -1), model()->statusSize(ix, -1), model()->statusText(ix, 0, currentrow), model()->statusSize(ix, 0), model()->statusAlignRight(ix));
                    break;
                case StatusTypes::TitleDouble:
                    status->add(model()->statusText(ix, -1, -1), model()->statusSize(ix, -1), model()->statusText(ix, 0, currentrow), model()->statusSize(ix, 0), model()->statusText(ix, 1, currentrow), model()->statusSize(ix, 1));
                    break;
                case StatusTypes::DoubleValue:
                    status->add("", 0, model()->statusText(ix, 0, currentrow), model()->statusSize(ix, 0), model()->statusText(ix, 1, currentrow), model()->statusSize(ix, 1));
                    break;
                case StatusTypes::SingleValue:
                    status->add(model()->statusText(ix, 0, currentrow), model()->statusSize(ix, 0));
                    break;
                }
            }
        }
    }

    status->setValues(0, QString::number(mapToSelection(model()->rowCount() - 1) + 1) + " /", QString::number(mapToSelection(currentrow) + 1));

    for (int ix = 0, siz = scnt; ix != siz; ++ix)
    {
        switch (model()->statusType(ix))
        {
        case StatusTypes::TitleValue:
            status->setValue(ix + 1, model()->statusText(ix, 0, currentrow));
            break;
        case StatusTypes::TitleDouble:
            status->setValues(ix + 1, model()->statusText(ix, 0, currentrow), model()->statusText(ix, 1, currentrow));
            break;
        case StatusTypes::DoubleValue:
            status->setValues(ix + 1, model()->statusText(ix, 0, currentrow), model()->statusText(ix, 1, currentrow));
            break;
        case StatusTypes::SingleValue:
            status->setValue(ix + 1, model()->statusText(ix, 0, currentrow));
            break;
        }
    }
}


//-------------------------------------------------------------


