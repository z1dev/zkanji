/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QPainter>
#include <QStylePainter>
#include <QMimeData>
#include <QDrag>
#include <QMessageBox>
#include <QStringBuilder>
#include <QDesktopServices>

#include "zdictionarylistview.h"
#include "zdictionarymodel.h"
#include "zproxytablemodel.h"
#include "zkanjimain.h"
#include "words.h"
#include "zui.h"
#include "zstrings.h"
#include "kanji.h"
#include "groups.h"
#include "ranges.h"
#include "dictionarysettings.h"
#include "colorsettings.h"
#include "fontsettings.h"
#include "kanjisettings.h"
#include "grammar_enums.h"
#include "ztooltip.h"
#include "globalui.h"
#include "dialogs.h"
#include "zkanjiform.h"
#include "zkanjiwidget.h"
#include "wordeditorform.h"
#include "worddeck.h"
#include "sites.h"

//-------------------------------------------------------------


const double kanjiRowSize = 0.78;
const double defRowSize = 0.78;
const double notesRowSize = 0.55;

ZDictionaryListView::ZDictionaryListView(QWidget *parent) : base(parent), multi(false), /*selection(new RangeSelection), */studydefs(false), ignorechange(false), textselectable(true), selecting(false)
{
    setItemDelegate(new DictionaryListDelegate(this));

    setShowGrid(false);
    setSizeBase(ListSizeBase::Main);
    setAutoSizeColumns(Settings::dictionary.autosize);

    setMouseTracking(true);

    popup = new QMenu(this);

    //savedselmode = base::selectionMode();
    //if (savedselmode == QAbstractItemView::ExtendedSelection)
    //    base::setSelectionMode(QAbstractItemView::SingleSelection);

    //auto olddlg = itemDelegate();
    //setItemDelegate(new DictionaryListDelegate(this));
    //delete olddlg;
}

ZDictionaryListView::~ZDictionaryListView()
{
}

DictionaryListDelegate* ZDictionaryListView::itemDelegate()
{
    return dynamic_cast<DictionaryListDelegate*>(base::itemDelegate());
}

void ZDictionaryListView::setItemDelegate(DictionaryListDelegate *newdel)
{
    base::setItemDelegate(newdel);
}

WordGroup* ZDictionaryListView::wordGroup() const
{
    QAbstractItemModel *m = model();

    while (m != nullptr)
    {
        if (dynamic_cast<DictionaryGroupItemModel*>(m) != nullptr)
            return ((DictionaryGroupItemModel*)m)->wordGroup();

        if (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
            m = ((DictionarySearchFilterProxyModel*)m)->sourceModel();
        else
            m = nullptr;
    }

    return nullptr;
}

Dictionary* ZDictionaryListView::dictionary() const
{
    ZAbstractTableModel *m = model();

    while (m != nullptr)
    {
        if (dynamic_cast<DictionaryItemModel*>(m) != nullptr)
            return ((DictionaryItemModel*)m)->dictionary();

        if (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
            m = ((DictionarySearchFilterProxyModel*)m)->sourceModel();
        else
            m = nullptr;
    }

    return nullptr;
}

int ZDictionaryListView::indexes(int row) const
{
    ZAbstractTableModel *m = model();
    while (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
    {
        row = ((DictionarySearchFilterProxyModel*)m)->mapToSource(m->index(row, 0)).row();
        m = ((DictionarySearchFilterProxyModel*)m)->sourceModel();
    }

    if (dynamic_cast<DictionaryItemModel*>(m) != nullptr)
        return ((DictionaryItemModel*)m)->indexes(row);

    return -1;
}

void ZDictionaryListView::indexes(const std::vector<int> &rows, std::vector<int> &result) const
{
    result = rows;

    ZAbstractTableModel *m = model();
    while (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
    {
        DictionarySearchFilterProxyModel *proxy = (DictionarySearchFilterProxyModel*)m;
        for (int ix = 0, siz = rows.size(); ix != siz; ++ix)
            result[ix] = proxy->mapToSource(proxy->index(result[ix], 0)).row();

        m = proxy->sourceModel();
    }

    if (dynamic_cast<DictionaryItemModel*>(m) == nullptr)
    {
        result.clear();
        return;
    }

    for (int &val : result)
        val = ((DictionaryItemModel*)m)->indexes(val);
}

void ZDictionaryListView::executeCommand(int command)
{
    if (command < (int)CommandLimits::WordsBegin || command >= (int)CommandLimits::WordsEnd)
        return;

    switch (command)
    {
    case (int)Commands::WordsToGroup:
        selectionToGroup();
        break;
    case (int)Commands::EditWord:
        editWord();
        break;
    //case (int)Commands::CreateNewWord:
    //    break;
    case (int)Commands::DeleteWord:
        deleteWord();
        break;
    case (int)Commands::RevertWord:
        revertWord();
        break;
    case (int)Commands::StudyWord:
        wordsToDeck();
        break;
    case (int)Commands::CreateNewWord:
        createNewWord();
        break;
    case (int)Commands::CopyWordDef:
        copyWordDef();
        break;
    case (int)Commands::CopyWordKanji:
        copyWordKanji();
        break;
    case (int)Commands::CopyWordKana:
        copyWordKana();
        break;
    case (int)Commands::CopyWord:
        copyWord();
        break;
    case (int)Commands::AppendWordKanji:
        appendWordKanji();
        break;
    case (int)Commands::AppendWordKana:
        appendWordKana();
        break;
    }
}

void ZDictionaryListView::commandState(int command, bool &enabled, bool &checked, bool &visible) const
{
    if (command < (int)CommandLimits::WordsBegin || command >= (int)CommandLimits::WordsEnd)
        return;

    enabled = true;
    checked = false;
    visible = true;

    switch (command)
    {
    case (int)Commands::WordsToGroup:
        enabled = ZKanji::dictionaryCount() != 0 && hasSelection();
        break;
    case (int)Commands::EditWord:
        enabled = ZKanji::dictionaryCount() != 0 && selectionSize() == 1;
        break;
    //case (int)Commands::CreateNewWord:
    //    break;
    case (int)Commands::DeleteWord:
        enabled = ZKanji::dictionaryCount() != 0 && selectionSize() == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0))));
        visible = ZKanji::dictionaryCount() != 0 && selectionSize() == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0))));
        break;
    case (int)Commands::RevertWord:
        enabled = ZKanji::dictionaryCount() != 0 && dictionary() == ZKanji::dictionary(0) && selectionSize() == 1 && dictionary()->isModifiedEntry(indexes(selectedRow(0)));
        visible = ZKanji::dictionaryCount() != 0 && (selectionSize() != 1 || (dictionary() == ZKanji::dictionary(0) && !dictionary()->isUserEntry(indexes(selectedRow(0)))));

//        int windex = indexes(selectedRow(0)); // model()->data(model()->index(selectedRow(0), 0), (int)DictRowRoles::WordIndex).toInt();
//        enabled = windex != -1 && ZKanji::originals.find(windex) != nullptr;
        break;
    case (int)Commands::StudyWord:
        enabled = ZKanji::dictionaryCount() != 0 && hasSelection();
        visible = ZKanji::dictionaryCount() != 0;
        break;
    case (int)Commands::CreateNewWord:
        visible = ZKanji::dictionaryCount() != 0;
        break;
    case (int)Commands::CopyWordDef:
        enabled = hasSelection();
        break;
    case (int)Commands::CopyWordKanji:
        enabled = hasSelection();
        break;
    case (int)Commands::CopyWordKana:
        enabled = hasSelection();
        break;
    case (int)Commands::CopyWord:
        enabled = hasSelection();
        break;
    case (int)Commands::AppendWordKanji:
        enabled = hasSelection();
        break;
    case (int)Commands::AppendWordKana:
        enabled = hasSelection();
        break;
    }
}


void ZDictionaryListView::setStudyDefinitionUsed(bool study)
{
    studydefs = study;
    update();
}

bool ZDictionaryListView::isStudyDefinitionUsed() const
{
    return studydefs;
}

//bool ZDictionaryListView::rowSelected(int row) const
//{
//    if (savedselmode == QAbstractItemView::NoSelection || model() == nullptr)
//        return false;
//
//    if (multi && savedselmode == QAbstractItemView::SingleSelection)
//        return currentRow() != -1  && row == currentRow();
//
//    if (!multi || savedselmode != QAbstractItemView::ExtendedSelection)
//        return base::rowSelected(row); // selectionModel()->isSelected((multi ? multiModel() : model())->index(row, 0));
//
//    return selection->selected(multiModel()->mapToSourceRow(row));
//}
//
//int ZDictionaryListView::selectionSize() const
//{
//    if (savedselmode == QAbstractItemView::NoSelection || model() == nullptr)
//        return 0;
//
//    //if (savedselmode == QAbstractItemView::SingleSelection)
//    //    return selectionModel()->selection().size();
//
//    if (multi)
//        return selection->size();
//
//    return base::selectionSize();
//
//    //int cnt = 0;
//    //auto &sel = selectionModel()->selection();
//    //int siz = sel.size();
//    //for (int ix = 0; ix != siz; ++ix)
//    //    cnt += sel[ix].height();
//
//    //return cnt;
//}

//bool ZDictionaryListView::isSelectedWord(const QModelIndex &index) const
//{
//    if (selectionMode() == QAbstractItemView::NoSelection)
//        return false;
//
//    if (!multi || model() == nullptr || selection->size() != 1)
//        return false;
//    
//    return selection->selected(multiModel()->mapToSource(index).row());
//}

//bool ZDictionaryListView::isCurrentRow(int row) const
//{
//    return row == currentRow();
//}

bool ZDictionaryListView::isMultiLine() const
{
    return multi;
}

void ZDictionaryListView::setMultiLine(bool ismulti)
{
    if (ismulti == multi)
        return;

    //selection->clear();

    ZAbstractTableModel *m = multi && multiModel() != nullptr ? multiModel()->sourceModel() : base::model();

    if (multi && multiModel() != nullptr)
    {
        auto mm = multiModel();
        disconnect(mm, &MultiLineDictionaryItemModel::selectionWasInserted, this, &ZDictionaryListView::multiSelInserted);
        disconnect(mm, &MultiLineDictionaryItemModel::selectionWasRemoved, this, &ZDictionaryListView::multiSelRemoved);
        disconnect(mm, &MultiLineDictionaryItemModel::selectionWasMoved, this, &ZDictionaryListView::multiSelMoved);
    }

    if (ismulti && m != nullptr)
    {
        MultiLineDictionaryItemModel *mm = new MultiLineDictionaryItemModel;
        mm->setSourceModel(m);

        connect(mm, &MultiLineDictionaryItemModel::selectionWasInserted, this, &ZDictionaryListView::multiSelInserted);
        connect(mm, &MultiLineDictionaryItemModel::selectionWasRemoved, this, &ZDictionaryListView::multiSelRemoved);
        connect(mm, &MultiLineDictionaryItemModel::selectionWasMoved, this, &ZDictionaryListView::multiSelMoved);

        //connect(m, &ZAbstractTableModel::rowsInserted, this, &ZDictionaryListView::baseRowsInserted);
        //connect(m, &ZAbstractTableModel::rowsRemoved, this, &ZDictionaryListView::baseRowsRemoved);
        //connect(m, &ZAbstractTableModel::rowsMoved, this, &ZDictionaryListView::baseRowsMoved);
        //connect(m, &ZAbstractTableModel::modelReset, this, &ZDictionaryListView::baseModelReset);
        //connect(m, &ZAbstractTableModel::layoutAboutToBeChanged, this, &ZDictionaryListView::baseLayoutWillChange);
        //connect(m, &ZAbstractTableModel::layoutChanged, this, &ZDictionaryListView::baseLayoutChanged);

        m = mm;
    }

    if (!ismulti && m != nullptr)
    {
        //disconnect(m, &ZAbstractTableModel::rowsInserted, this, &ZDictionaryListView::baseRowsInserted);
        //disconnect(m, &ZAbstractTableModel::rowsRemoved, this, &ZDictionaryListView::baseRowsRemoved);
        //disconnect(m, &ZAbstractTableModel::rowsMoved, this, &ZDictionaryListView::baseRowsMoved);
        //disconnect(m, &ZAbstractTableModel::modelReset, this, &ZDictionaryListView::baseModelReset);
        //disconnect(m, &ZAbstractTableModel::layoutAboutToBeChanged, this, &ZDictionaryListView::baseLayoutWillChange);
        //disconnect(m, &ZAbstractTableModel::layoutChanged, this, &ZDictionaryListView::baseLayoutChanged);

        multiModel()->deleteLater();
    }

    multi = ismulti;
    base::setModel(m);

    //if (ismulti)
    //{
    //    savedselmode = base::selectionMode();
    //    if (savedselmode == QAbstractItemView::ExtendedSelection)
    //        base::setSelectionMode(QAbstractItemView::SingleSelection);
    //}
    //else
    //    base::setSelectionMode(savedselmode);

    //emit rowSelectionChanged();
}

bool ZDictionaryListView::groupDisplay()
{
    return ((DictionaryListDelegate*)itemDelegate())->groupDisplay();
}

void ZDictionaryListView::setGroupDisplay(bool val)
{
    ((DictionaryListDelegate*)itemDelegate())->setGroupDisplay(val);
}

//void ZDictionaryListView::setSelectionMode(QAbstractItemView::SelectionMode mode)
//{
//    if (savedselmode == mode)
//        return;
//
//    if (!multi)
//    {
//        savedselmode = mode;
//        base::setSelectionMode(mode);
//        return;
//    }
//
//    //if (savedselmode == QAbstractItemView::NoSelection)
//    //    base::setSelectionMode(QAbstractItemView::SingleSelection);
//
//    savedselmode = mode;
//
//    selection->clear();
//
//    if (savedselmode == QAbstractItemView::SingleSelection || savedselmode == QAbstractItemView::ExtendedSelection)
//    {
//        base::setSelectionMode(QAbstractItemView::SingleSelection);
//
//        if (currentRow() != -1)
//        {
//            selection->setSelected(currentRow(), true);
//            scrollToRow(currentRow()); // (currentIndex());
//        }
//    }
//    else
//        base::setSelectionMode(savedselmode);
//
//    viewport()->update();
//
//    emit rowSelectionChanged();
//}
//
//QAbstractItemView::SelectionMode ZDictionaryListView::selectionMode() const
//{
//    if (!multi)
//        return base::selectionMode();
//
//    return savedselmode;
//}

//void ZDictionaryListView::selectedRows(std::vector<int> &result) const
//{
//    if (!multi || savedselmode != QAbstractItemView::ExtendedSelection || model() == nullptr)
//    {
//        base::selectedRows(result);
//        return;
//    }
//
//    selection->getSelection(result);
//}

void ZDictionaryListView::setModel(ZAbstractTableModel *newmodel)
{
    ZAbstractTableModel* oldmodel = base::model();
    if (multi && oldmodel != nullptr)
        oldmodel = multiModel()->sourceModel();


    if (newmodel == oldmodel)
        return;

    if (!multi)
        base::setModel(newmodel);
    else
    {

        MultiLineDictionaryItemModel *oldmulti = multiModel();

        if (newmodel != nullptr)
        {
            if (base::model() == nullptr)
            {
                MultiLineDictionaryItemModel *mm = new MultiLineDictionaryItemModel(this);
                mm->setSourceModel(newmodel);

                base::setModel(mm);

                connect(mm, &MultiLineDictionaryItemModel::selectionWasInserted, this, &ZDictionaryListView::multiSelInserted);
                connect(mm, &MultiLineDictionaryItemModel::selectionWasRemoved, this, &ZDictionaryListView::multiSelRemoved);
                connect(mm, &MultiLineDictionaryItemModel::selectionWasMoved, this, &ZDictionaryListView::multiSelMoved);
            }
            else
                multiModel()->setSourceModel(newmodel);
        }
        else if (base::model() != nullptr)
        {
            base::model()->deleteLater();
            base::setModel(nullptr);
        }
    }

    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg && ZKanji::dictionaryCount() != 0)
    {
        int selcnt = selectionSize();

        ((ZKanjiForm*)window())->showCommand(makeCommand(Commands::DeleteWord, categ), selcnt == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0)))));
        ((ZKanjiForm*)window())->showCommand(makeCommand(Commands::RevertWord, categ), selcnt != 1 || (dictionary() == ZKanji::dictionary(0) && !dictionary()->isUserEntry(indexes(selectedRow(0)))));

        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::DeleteWord, categ), selcnt == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0)))));
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::RevertWord, categ), dictionary() == ZKanji::dictionary(0) && selcnt == 1 && dictionary()->isModifiedEntry(indexes(selectedRow(0))));
    }
}

ZAbstractTableModel* ZDictionaryListView::model() const
{
    if (!multi || base::model() == nullptr)
        return base::model();

    return ((MultiLineDictionaryItemModel*)base::model())->sourceModel();
}

DictionaryItemModel* ZDictionaryListView::baseModel() const
{
    ZAbstractTableModel *m = model();
    while (m != nullptr)
    {
        if (dynamic_cast<DictionaryItemModel*>(m) != nullptr)
            return (DictionaryItemModel*)m;

        if (dynamic_cast<ZAbstractProxyTableModel*>(m) != nullptr)
            m = ((ZAbstractProxyTableModel*)m)->sourceModel();
        else
            break;
    }

    return nullptr;
}

MultiLineDictionaryItemModel* ZDictionaryListView::multiModel() const
{
    if (!multi)
        return nullptr;
    return (MultiLineDictionaryItemModel*)base::model();
}

bool ZDictionaryListView::isTextSelecting() const
{
    return selecting;
}

QModelIndex ZDictionaryListView::textSelectionIndex() const
{
    return selcell;
}

int ZDictionaryListView::textSelectionPosition() const
{
    return selpos;
}

int ZDictionaryListView::textSelectionLength() const 
{
    return sellen;
}

bool ZDictionaryListView::cancelActions()
{
    bool r = base::cancelActions();

    if (selecting)
    {
        // Cancel selection.
        QRect rect = visualRect(selcell);
        selecting = false;
        viewport()->update(rect);
        return true;
    }

    return r;
}

//QModelIndex ZDictionaryListView::baseIndexAt(const QPoint &p) const
//{
//    QModelIndex ind = indexAt(p);
//
//    if (multi && ind.isValid() && model() != nullptr)
//        return model()->index(multiModel()->mapToSourceRow(ind.row()), ind.column());
//
//    return ind;
//}

void ZDictionaryListView::signalSelectionChanged()
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
    {
        int selcnt = selectionSize();
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::WordsToGroup, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::EditWord, categ), selcnt == 1);

        ((ZKanjiForm*)window())->showCommand(makeCommand(Commands::DeleteWord, categ), ZKanji::dictionaryCount() != 0 && selcnt == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0)))));
        ((ZKanjiForm*)window())->showCommand(makeCommand(Commands::RevertWord, categ), ZKanji::dictionaryCount() != 0 && (selcnt != 1 || (dictionary() == ZKanji::dictionary(0) && !dictionary()->isUserEntry(indexes(selectedRow(0))))));

        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::DeleteWord, categ), ZKanji::dictionaryCount() != 0 && selcnt == 1 && dictionary() != nullptr && (dictionary() != ZKanji::dictionary(0) || dictionary()->isUserEntry(indexes(selectedRow(0)))));
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::RevertWord, categ), ZKanji::dictionaryCount() != 0 && dictionary() == ZKanji::dictionary(0) && selcnt == 1 && dictionary()->isModifiedEntry(indexes(selectedRow(0))));

        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::StudyWord, categ), ZKanji::dictionaryCount() != 0 && selcnt != 0);

        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CopyWordDef, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CopyWordKanji, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CopyWordKana, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::CopyWord, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::AppendWordKanji, categ), selcnt != 0);
        ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::AppendWordKana, categ), selcnt != 0);
    }

    base::signalSelectionChanged();
}

void ZDictionaryListView::settingsChanged()
{
    base::settingsChanged();
    setAutoSizeColumns(Settings::dictionary.autosize);
}

void ZDictionaryListView::multiSelRemoved(const smartvector<Range> &ranges)
{
    base::selRemoved(ranges);
}

void ZDictionaryListView::multiSelInserted(const smartvector<Interval> &intervals)
{
    base::selInserted(intervals);
}

void ZDictionaryListView::multiSelMoved(const smartvector<Range> &ranges, int pos)
{
    base::selMoved(ranges, pos);
}

void ZDictionaryListView::selRemoved(const smartvector<Range> &ranges)
{
    if (!multi || base::model() == nullptr)
        base::selRemoved(ranges);
}

void ZDictionaryListView::selInserted(const smartvector<Interval> &intervals)
{
    if (!multi || base::model() == nullptr)
        base::selInserted(intervals);
}

void ZDictionaryListView::selMoved(const smartvector<Range> &ranges, int pos)
{
    if (!multi || base::model() == nullptr)
        base::selMoved(ranges, pos);
}

int ZDictionaryListView::mapToSelection(int row) const
{
    if (!multi || base::model() == nullptr)
        return base::mapToSelection(row);
    return multiModel()->mapToSourceRow(row);
}

int ZDictionaryListView::mapFromSelection(int index, bool top) const
{
    if (!multi || base::model() == nullptr)
        return base::mapFromSelection(index, top);

    if (!top)
    {
        ++index;
        if (index == model()->rowCount())
            return multiModel()->rowCount() - 1;
        return multiModel()->mapFromSourceRow(index) - 1;
    }

    return multiModel()->mapFromSourceRow(index);
}

//QModelIndexList ZDictionaryListView::baseIndexes(const QModelIndexList &indexes) const
//{
//#ifdef _DEBUG
//    if (multi)
//        throw "This function does not work in multi line mode. Use multiBaseRows() instead.";
//#endif
//
//    QAbstractItemModel *m = base::model();
//
//    QModelIndexList result = indexes;
//
//    while (m != nullptr)
//    {
//        if (dynamic_cast<DictionaryItemModel*>(m) != nullptr)
//            return result;
//
//        if (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
//        {
//            result = ((DictionarySearchFilterProxyModel*)m)->mapListToSource(result);
//            m = ((DictionarySearchFilterProxyModel*)m)->sourceModel();
//        }
//        //else if (dynamic_cast<MultiLineDictionaryItemModel*>(m) != nullptr)
//        //{
//        //    result = ((MultiLineDictionaryItemModel*)m)->uniqueSourceIndexes(result);
//        //    m = ((MultiLineDictionaryItemModel*)m)->sourceModel();
//        //}
//#ifdef _DEBUG
//        else
//            throw "Not holding a DictionaryItemModel as the base";
//#endif
//    }
//
//    return QModelIndexList();
//}

void ZDictionaryListView::baseRows(const std::vector<int> &src, std::vector<int> &result) const
{
    ZAbstractTableModel *m = model();

    // Result and src might match, so any operation in result should be non destructive.
    result = src;

    while (m != nullptr)
    {
        if (dynamic_cast<DictionaryItemModel*>(m) != nullptr)
            return;

        if (dynamic_cast<DictionarySearchFilterProxyModel*>(m) != nullptr)
        {
            DictionarySearchFilterProxyModel *proxy = (DictionarySearchFilterProxyModel*)m;
            for (int ix = 0; ix != result.size(); ++ix)
            {
                // Result and src might match, but setting the corresponding value of result
                // directly from src makes sure changed items are not touched twice.
                result[ix] = proxy->mapToSource(proxy->index(result[ix], 0)).row();
            }
            m = proxy->sourceModel();
        }
    }
}

void ZDictionaryListView::removeDragRows()
{
    WordGroup *grp = wordGroup();

    if (grp == nullptr)
        return;

    std::vector<int> rows;
    selectedRows(rows);
    baseRows(rows, rows);
    std::sort(rows.begin(), rows.end());

    smartvector<Range> ranges;
    _rangeFromIndexes(rows, ranges);

    grp->remove(ranges);
}

//ZListViewItemDelegate* ZDictionaryListView::createItemDelegate()
//{
//    return new DictionaryListDelegate(this);
//}

//bool ZDictionaryListView::isRowDragSelected(int row) const
//{
//    if (!multi || multiModel() == nullptr || savedselmode != QAbstractItemView::ExtendedSelection)
//        return base::isRowDragSelected(row);
//
//    return selection->selected(multiModel()->mapToSourceRow(row));
//}

QMimeData* ZDictionaryListView::dragMimeData() const
{
    if (!multi || multiModel() == nullptr)
        return base::dragMimeData();

    // Fills an index list with indexes of words in the base model, then returns the generated
    // mime data from those.
    ZAbstractTableModel *m = model();

    std::vector<int> rows;
    selectedRows(rows);

    QModelIndexList indexes;
    indexes.reserve(rows.size());

    for (int ix = 0, siz = rows.size(); ix != siz; ++ix)
        indexes.push_back(m->index(rows[ix], 0));

    return m->mimeData(indexes);
}

int ZDictionaryListView::dropRow(int ypos) const
{
    if (!multi)
    {
        // Filtered models might resize the items, so only dropping at an unspecified
        // location is permitted.
        if (dynamic_cast<DictionarySearchFilterProxyModel*>(model()) != nullptr)
            return -1;

        return base::dropRow(ypos);
    }

    // Multi rows only permit dropping between word boundaries, which can span several rows.

    // The real row near ypos will be "rounded" to the nearest word boundary.
    return multiModel()->roundRow(base::dropRow(ypos));
}

//QItemSelectionModel::SelectionFlags ZDictionaryListView::selectionCommand(const QModelIndex &index, const QEvent *e) const
//{
//    if (!multi || savedselmode != QAbstractItemView::ExtendedSelection || e == nullptr || e->type() != QEvent::KeyPress || (((QKeyEvent*)e)->modifiers() & Qt::ControlModifier) == 0)
//        return base::selectionCommand(index, e);
//
//    return QItemSelectionModel::NoUpdate;
//}

//void ZDictionaryListView::toggleRowSelect(int row, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
//{
//    if (!multi || model() == nullptr || savedselmode != QAbstractItemView::ExtendedSelection || button != Qt::LeftButton)
//    {
//        base::toggleRowSelect(row, button, modifiers);
//        return;
//    }
//
//    int realrow = row;
//    row = multiModel()->mapToSourceRow(row);
//
//    bool toggle = modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::ShiftModifier);
//
//    ignorechange = true;
//
//    bool oldsel = selection->selected(row);
//
//
//    if (selection->size() > 1 || !oldsel)
//    {
//        if (!toggle)
//        {
//            updateSelected();
//            selection->clear();
//            selection->setSelected(row, true);
//            updateWordRows(row);
//        }
//        else
//        {
//            selection->setSelected(row, !oldsel || !toggle);
//            updateWordRows(row);
//        }
//    }
//
//    ignorechange = false;
//
//    emit rowSelectionChanged();
//}

//void ZDictionaryListView::keyPressEvent(QKeyEvent *e)
//{
//    if (selecting)
//    {
//
//        return;
//    }
//
//    base::keyPressEvent(e);
//}

bool ZDictionaryListView::viewportEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::HoverLeave:
    case QEvent::Leave:
        ZToolTip::startHide();
        kanjitippos = -1;
        kanjitipcell = QModelIndex();
        unsetCursor();
        break;
    }

    return base::viewportEvent(e);
}

void ZDictionaryListView::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);

    if (model() == nullptr)
        return;

    QModelIndex index = indexAt(e->pos());

    if (!index.isValid())
        return;

    if (ZKanji::dictionaryCount() != 0)
    {

        //ZAbstractTableModel *m = multi ? multiModel() : model();

        DictionaryListDelegate *delegate = (DictionaryListDelegate*)itemDelegate();
        int k = delegate->kanjiAt(e->pos(), visualRect(index), index);
        if (k != -1)
        {
            gUI->showKanjiInfo(dictionary(), k);
            return;
        }
    }

    emit wordDoubleClicked(index.data((int)DictRowRoles::WordIndex).toInt(), index.data((int)DictRowRoles::DefIndex).toInt());
}

void ZDictionaryListView::mouseMoveEvent(QMouseEvent *e)
{
    if (selecting)
    {
        DictionaryListDelegate *del = (DictionaryListDelegate*)itemDelegate();
        QRect rect = visualRect(selcell);

        sellen = del->characterIndexAt(e->pos(), rect, selcell, true) - selpos;

        viewport()->update(rect);
        return;
    }

    base::mouseMoveEvent(e);

    if (model() == nullptr)
        return;

    QModelIndex index = indexAt(e->pos());

    if (!index.isValid())
        return;

    DictionaryListDelegate *del = (DictionaryListDelegate*)itemDelegate();
    QRect rect = visualRect(index);
    if (Settings::kanji.tooltip)
    {
        QRect krect;
        int kpos;
        int k = del->kanjiAt(e->pos(), rect, index, &krect, &kpos);
        if (k != -1 && (kpos != kanjitippos || index != kanjitipcell))
        {
            QRect g = viewport()->geometry();
            krect.adjust(g.left(), g.top(), g.left(), g.top());
            ZToolTip::show(e->globalPos(), ZKanji::kanjiTooltipWidget(dictionary(), k), this, krect, Settings::kanji.hidetooltip ? Settings::kanji.tooltipdelay * 1000 : 2000000000);
            kanjitippos = kpos;
            kanjitipcell = index;
        }

        if (k == -1)
        {
            ZToolTip::startHide();
            kanjitippos = -1;
            kanjitipcell = QModelIndex();
        }
    }
    if (textselectable)
    {
        if (del->characterIndexAt(e->pos(), rect, index) != -1)
            setCursor(Qt::IBeamCursor);
        else
            unsetCursor();
    }
}

void ZDictionaryListView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
    {
        if (selecting)
        {
            // Cancel selection.
            QRect rect = visualRect(selcell);
            selecting = false;
            viewport()->update(rect);
        }
    }
    else if (model() != nullptr)
    {
        DictionaryListDelegate *del = (DictionaryListDelegate*)itemDelegate();

        QModelIndex index = indexAt(e->pos());
        QRect rect = visualRect(index);
        selcell = index;
        selpos = del->characterIndexAt(e->pos(), rect, index);
        if (selpos != -1)
        {
            selecting = true;
            sellen = 0;
        }
    }

    base::mousePressEvent(e);
}

void ZDictionaryListView::mouseReleaseEvent(QMouseEvent *e)
{
    if (selecting && e->button() == Qt::LeftButton)
    {
        if (sellen != 0)
        {
            viewport()->update(visualRect(selcell));

            //DictColumnTypes coltype = (DictColumnTypes)selcell.data((int)DictColumnRoles::Type).toInt();
            QString str = selcell.data(Qt::DisplayRole).toString().mid(std::min(selpos, selpos + sellen), std::abs(sellen));

            showPopup(e->globalPos(), selcell, str, -1);
        }
        selecting = false;
    }

    base::mouseReleaseEvent(e);
}

void ZDictionaryListView::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    // Don't forget viewportEvent() when changing this.
    ZToolTip::startHide();
    kanjitippos = -1;
    kanjitipcell = QModelIndex();
}

void ZDictionaryListView::contextMenuEvent(QContextMenuEvent *e)
{
    //base::contextMenuEvent(e);

    QModelIndex index = indexAt(e->pos());
    if (index.isValid())
    {
        //DictColumnTypes coltype = (DictColumnTypes)index.data((int)DictColumnRoles::Type).toInt();
        DictionaryListDelegate *del = (DictionaryListDelegate*)itemDelegate();
        
        showPopup(e->globalPos(), index, QString(), del->kanjiAt(e->pos(), visualRect(index), index));
        e->accept();
    }
}

void ZDictionaryListView::selectionToGroup() const
{
    std::vector<int> sel;
    selectedRows(sel);

    if (sel.empty())
        return;

    indexes(sel, sel);

    wordToGroupSelect(dictionary(), sel/*, gUI->activeMainForm()*/);
}

void ZDictionaryListView::editWord() const
{
    ::editWord(dictionary(), indexes(selectedRow(0)), 0, gUI->activeMainForm());
}

void ZDictionaryListView::deleteWord() const
{
    deleteWord(indexes(selectedRow(0)));
}

void ZDictionaryListView::deleteWord(int windex) const
{
    if (QMessageBox::warning(gUI->activeMainForm(), "zkanji", tr("The selected word will be erased from every word group and study statistics, and deleted from the dictionary.\n\nDo you want to delete it?"), QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    dictionary()->removeEntry(windex);
}

void ZDictionaryListView::revertWord() const
{
    dictionary()->revertEntry(indexes(selectedRow(0)));
}

void ZDictionaryListView::createNewWord() const
{
    editNewWord(dictionary());
}

void ZDictionaryListView::wordsToDeck() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    wordsToDeck(sel);
}

void ZDictionaryListView::wordsToDeck(const std::vector<int> &windexes) const
{
    addWordsToDeck(dictionary(), dictionary()->wordDecks()->lastSelected(), windexes);
}

void ZDictionaryListView::copyWordDef() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    copyWordDef(sel);
}

void ZDictionaryListView::copyWordKanji() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    copyWordKanji(sel);
}

void ZDictionaryListView::copyWordKana() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    copyWordKana(sel);
}

void ZDictionaryListView::copyWord() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    copyWord(sel);
}

void ZDictionaryListView::appendWordKanji() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    appendWordKanji(sel);
}

void ZDictionaryListView::appendWordKana() const
{
    std::vector<int> sel;
    selectedRows(sel);

    indexes(sel, sel);

    appendWordKana(sel);
}

void ZDictionaryListView::copyWordDef(const std::vector<int> &windexes) const
{
    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += "\n";
        result += dictionary()->wordDefinitionString(i, true);
    }

    gUI->clipCopy(result);
}

void ZDictionaryListView::copyWordKanji(const std::vector<int> &windexes) const
{
    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += " ";
        result += dictionary()->wordEntry(i)->kanji.toQString();
    }

    gUI->clipCopy(result);
}

void ZDictionaryListView::copyWordKana(const std::vector<int> &windexes) const
{

    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += " ";
        result += dictionary()->wordEntry(i)->kana.toQString();
    }

    gUI->clipCopy(result);
}

void ZDictionaryListView::copyWord(const std::vector<int> &windexes) const
{
    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += "\n";

        WordEntry *e = dictionary()->wordEntry(i);
        if (e->kanji != e->kana)
            result += e->kanji.toQString() % "(" % e->kana.toQString() % ")" % " " % dictionary()->wordDefinitionString(i, true);
        else
            result += e->kanji.toQString() % " " % dictionary()->wordDefinitionString(i, true);
    }

    gUI->clipCopy(result);
}

void ZDictionaryListView::appendWordKanji(const std::vector<int> &windexes) const
{
    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += " ";
        result += dictionary()->wordEntry(i)->kanji.toQString();
    }

    gUI->clipAppend(result);
}

void ZDictionaryListView::appendWordKana(const std::vector<int> &windexes) const
{
    QString result;
    for (int i : windexes)
    {
        if (!result.isEmpty())
            result += " ";
        result += dictionary()->wordEntry(i)->kana.toQString();
    }

    gUI->clipAppend(result);
}


CommandCategories ZDictionaryListView::activeCategory() const
{
    if (!isVisibleTo(window()) || dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return CommandCategories::NoCateg;

    const QWidget *w = parentWidget();
    while (w != nullptr && dynamic_cast<const ZKanjiWidget*>(w) == nullptr)
        w = w->parentWidget();

    if (w == nullptr)
        return CommandCategories::NoCateg;

    ZKanjiWidget *zw = ((ZKanjiWidget*)w);
    if (zw->isActiveWidget())
        return zw->mode() == ViewModes::WordSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg;

    QList<ZKanjiWidget*> wlist = ((ZKanjiForm*)window())->findChildren<ZKanjiWidget*>();
    for (ZKanjiWidget *w : wlist)
    {
        if (w == zw || w->window() != window())
            continue;
        else if (w->mode() == zw->mode())
            return CommandCategories::NoCateg;
    }

    return zw->mode() == ViewModes::WordSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg;
}

void ZDictionaryListView::showPopup(const QPoint &pos, QModelIndex index, QString selstr, int kindex)
{
    popup->clear();

    //if (selstr.isEmpty())
    //    selstr = index.data(Qt::DisplayRole).toString();

    // Every kanji found in selstr.
    QSet<QChar> found;
    QString kstring;
    std::vector<ushort> kindexes;

    Dictionary *dict = dictionary();
    bool hasdict = ZKanji::dictionaryCount() != 0;
    if (dict == nullptr && ZKanji::dictionaryCount() != 0)
        dict = ZKanji::dictionary(0);

    std::vector<int> windexes;
    selectedRows(windexes);
    indexes(windexes, windexes);

    if (!selstr.isEmpty())
    {
        for (int ix = 0, siz = selstr.size(); ix != siz; ++ix)
            if (KANJI(selstr.at(ix).unicode()))
            {
                if (found.contains(selstr.at(ix)))
                    continue;
                found.insert(selstr.at(ix));
                kstring.push_back(selstr.at(ix));
                kindexes.push_back(ZKanji::kanjiIndex(selstr.at(ix)));
            }
    }
    else if (dict != nullptr && !windexes.empty())
    {
        for (int wix : windexes)
        {
            QCharString &str = dict->wordEntry(wix)->kanji;
            for (int ix = 0, siz = str.size(); ix != siz; ++ix)
            {
                if (!KANJI(str[ix].unicode()) || found.contains(str[ix]))
                    continue;
                found.insert(str[ix]);
                kstring.push_back(str[ix]);
                kindexes.push_back(ZKanji::kanjiIndex(str[ix]));
            }
        }
    }

    int insertpos = 0;


    if (selstr.size() == 1 && !found.isEmpty())
    {
        kindex = ZKanji::kanjiIndex(*found.begin());
        kstring.clear();
        kindexes.clear();
    }

    if (dict != nullptr && hasdict && kindex != -1 && selstr.size() < 2)
    {
        QAction *akinfo = popup->addAction(tr("Kanji information..."));
        connect(akinfo, &QAction::triggered, [dict, kindex]() {
            gUI->showKanjiInfo(dict, kindex);
        });
        popup->addSeparator();
        insertpos += 2;
    }

    DictColumnTypes coltype = (DictColumnTypes)index.data((int)DictColumnRoles::Type).toInt();

    // When changed: fix similar code in wordstudyform.cpp constructor.
    if (ZKanji::lookup_sites.size() != 0 && hasdict && dict != nullptr && (!selstr.isEmpty() || windexes.size() == 1) && coltype != DictColumnTypes::Definition)
    {
        QMenu *sub = popup->addMenu(tr("Online lookup"));
        for (int ix = 0, siz = ZKanji::lookup_sites.size(); ix != siz; ++ix)
        {
            QAction *a = sub->addAction(ZKanji::lookup_sites[ix].name);
            connect(a, &QAction::triggered, [dict, coltype, selstr, &windexes, ix]() {
                SiteItem &site = ZKanji::lookup_sites[ix];
                QString str = !selstr.isEmpty() ? selstr : (coltype == DictColumnTypes::Kanji ? dict->wordEntry(windexes[0])->kanji : dict->wordEntry(windexes[0])->kana).toQString();
                QString url = site.url.left(site.insertpos) % str % site.url.mid(site.insertpos);
                QDesktopServices::openUrl(QUrl(url));
            });
        }
    }
    popup->addSeparator();

    if (kindex == -1 && !selstr.isEmpty())
    {
        QAction *awkcopy = popup->addAction(tr("Copy selection"));
        QAction *awkapp = popup->addAction(tr("Append selection"));

        connect(awkcopy, &QAction::triggered, [/*dict, &windexes,*/ &selstr]() {
            gUI->clipCopy(selstr);
        });
        connect(awkapp, &QAction::triggered, [/*dict, &windexes,*/ &selstr]() {
            gUI->clipAppend(selstr);
        });
        popup->addSeparator();
    }

    QAction *menusep = popup->addSeparator();

    if (hasdict && dict != nullptr && selstr.isEmpty())
    {
        QAction *aadd = popup->addAction(tr("Add word to group..."));
        connect(aadd, &QAction::triggered, [dict, &windexes]() {
            wordToGroupSelect(dict, windexes/*, gUI->activeMainForm()*/);
        });
        QAction *astudy = popup->addAction(tr("Add word to study deck..."));
        connect(astudy, &QAction::triggered, [this, &windexes]() {
            wordsToDeck(windexes);
        });
        popup->addSeparator();
    }

    if (hasdict && dict != nullptr && (kindex != -1 || !kindexes.empty()))
    {
        QMenu *ksub = popup->addMenu(tr("Kanji"));
        QAction *akadd = ksub->addAction(tr("Add kanji to group..."));
        QAction *akedit = ksub->addAction(tr("Edit kanji definitions..."));

        connect(akadd, &QAction::triggered, [dict, kindex, &kindexes]() {
            if (kindex != -1)
                kanjiToGroupSelect(dict, kindex);
            else
                kanjiToGroupSelect(dict, kindexes);
        });
        connect(akedit, &QAction::triggered, [dict, kindex, &kindexes](){
            if (kindex != -1)
                editKanjiDefinition(dict, kindex);
            else
                editKanjiDefinition(dict, kindexes);
        });

        popup->addSeparator();
    }

    QMenu *csub = popup->addMenu(tr("Clipboard"));

    QAction *awcopy = csub->addAction(tr("Copy word"));
    connect(awcopy, &QAction::triggered, [this, &windexes]() {
        copyWord(windexes);
    });

    csub->addSeparator();

    QAction *awwcopy = csub->addAction(tr("Copy written form"));
    QAction *awwapp = csub->addAction(tr("Append written form"));

    connect(awwcopy, &QAction::triggered, [this, &windexes]() {
        copyWordKanji(windexes);
    });
    connect(awwapp, &QAction::triggered, [this, &windexes]() {
        appendWordKanji(windexes);
    });

    csub->addSeparator();

    QAction *awkcopy = csub->addAction(tr("Copy kana form"));
    QAction *awkapp = csub->addAction(tr("Append kana form"));

    connect(awkcopy, &QAction::triggered, [this, &windexes]() {
        copyWordKana(windexes);
    });
    connect(awkapp, &QAction::triggered, [this, &windexes]() {
        appendWordKana(windexes);
    });

    QAction *akcopy = nullptr;
    QAction *akapp = nullptr;

    if (kindex != -1 || !kstring.isEmpty())
    {
        csub->addSeparator();

        akcopy = csub->addAction(tr("Copy kanji"));
        akapp = csub->addAction(tr("Append kanji"));

        connect(akcopy, &QAction::triggered, [this, &kstring]() {
           gUI->clipCopy(kstring);
        });
        connect(akapp, &QAction::triggered, [this, &kstring]() {
            gUI->clipAppend(kstring);
        });
    }

    csub->addSeparator();

    QAction *awdcopy = csub->addAction(tr("Copy definition"));
    connect(awdcopy, &QAction::triggered, [this, &windexes]() {
        copyWordDef(windexes);
    });

    popup->addSeparator();

    if (hasdict && dict != nullptr && windexes.size() == 1 && selstr.isEmpty())
    {
        QAction *aedit = popup->addAction(tr("Edit word..."));
        connect(aedit, &QAction::triggered, [dict, &windexes]() {
            ::editWord(dict, windexes[0], 0, gUI->activeMainForm());
        });

        if (dict != ZKanji::dictionary(0) || dict->isUserEntry(windexes[0]))
        {
            QAction *adel = popup->addAction(dict == ZKanji::dictionary(0) ? tr("Revert to original") : tr("Delete word"));
            connect(adel, &QAction::triggered, [this, dict, &windexes]() {
                if (dict == ZKanji::dictionary(0))
                    dict->revertEntry(windexes[0]);
                else
                    deleteWord(windexes[0]);
            });
        }

    }

    if (kindex != -1)
    {
        csub->removeAction(akcopy);
        csub->removeAction(akapp);

        popup->insertAction(menusep, akcopy);
        popup->insertAction(menusep, akapp);
    }
    else if (coltype == DictColumnTypes::Kanji && selstr.isEmpty())
    {
        csub->removeAction(awwcopy);
        csub->removeAction(awwapp);

        popup->insertAction(menusep, awwcopy);
        popup->insertAction(menusep, awwapp);
    }
    else if (coltype == DictColumnTypes::Kana && selstr.isEmpty())
    {
        csub->removeAction(awkcopy);
        csub->removeAction(awkapp);

        popup->insertAction(menusep, awkcopy);
        popup->insertAction(menusep, awkapp);
    }
    else if (coltype == DictColumnTypes::Definition)
    {
        csub->removeAction(awdcopy);

        popup->insertAction(menusep, awdcopy);
    }

    emit contextMenuCreated(popup, popup->actions().at(popup->actions().size() > insertpos ? insertpos : 0), dict, coltype, selstr, windexes, kindexes);

    popup->exec(pos);
}

//void ZDictionaryListView::dragEnterEvent(QDragEnterEvent *e)
//{
//    e->ignore();
//
//    if (baseModel() == nullptr || !baseModel()->canDragEnter(e, e->source() == this))
//        return;
//
//    base::dragEnterEvent(e);
//}

//void ZDictionaryListView::selectionChanged(const QItemSelection &index, const QItemSelection &old)
//{
//    base::selectionChanged(index, old);
//
//    if (!multi || multiModel() == nullptr || ignorechange || savedselmode != QAbstractItemView::ExtendedSelection)
//    {
//        if (!ignorechange)
//            emit rowSelectionChanged();
//        return;
//    }
//
//    int row = -1;
//    int oldrow = -1;
//
//    // Row and oldrow are only set if there was a single selected row or item in the base
//    // view, that means a single word (with probably multiple rows) in this view.
//    if (index.size() == 1 && index.at(0).top() == index.at(0).bottom())
//        row = index.at(0).top();
//    if (old.size() == 1 && old.at(0).top() == old.at(0).bottom())
//        oldrow = old.at(0).top();
//
//    //if (savedselmode == QAbstractItemView::SingleSelection)
//    //{
//    //    if (oldrow != row && oldrow != -1)
//    //        updateRow(oldrow);
//    //    if (oldrow != row && row != -1)
//    //    {
//    //        selection->clear();
//    //        selection->setSelected(multiModel()->mapToSourceRow(row), true);
//    //        updateRow(row);
//    //    }
//    //    return;
//    //}
//
//    // The word's index at row.
//    int wordrow = row != -1 ? multiModel()->mapToSourceRow(row) : -1;
//    // The word's index at oldrow.
//    //int oldwordrow = oldrow != -1 ? multiModel()->mapToSourceRow(oldrow) : -1;
//
//    if (row != oldrow)
//    {
//        updateSelected();
//        selection->clear();
//        if (wordrow != -1)
//        {
//            selection->setSelected(wordrow, true);
//
//            updateWordRows(wordrow);
//        }
//        emit rowSelectionChanged();
//    }
//}

//void ZDictionaryListView::baseRowsInserted(const QModelIndex &parent, int first, int last)
//{
//    selection->insert(first, last - first + 1);
//    emit rowSelectionChanged();
//}
//
//void ZDictionaryListView::baseRowsRemoved(const QModelIndex &parent, int first, int last)
//{
//    selection->remove(first, last);
//    emit rowSelectionChanged();
//}
//
//void ZDictionaryListView::baseRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
//{
//    selection->move(start, end, row);
//    emit rowSelectionChanged();
//}
//
//void ZDictionaryListView::baseModelReset()
//{
//    selection->clear();
//    emit rowSelectionChanged();
//}
//
//void ZDictionaryListView::baseLayoutWillChange(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
//{
//    selection->beginLayouting(model());
//}
//
//void ZDictionaryListView::baseLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
//{
//    selection->endLayouting(model());
//}

//void ZDictionaryListView::updateWordRows(int wpos)
//{
//#ifdef _DEBUG
//    if (!multi || model() == nullptr)
//        throw "Cannot update rows when not in multi line mode or no model set.";
//#endif
//
//    if (wpos < 0)
//        return;
//
//    int toprow = multiModel()->mapFromSourceRow(wpos);
//    int siz = multiModel()->mappedRowSize(wpos);
//
//    if (toprow == -1 || siz == 0)
//        return;
//
//    // Top visual coordinate of first row to update.
//    int t = rowViewportPosition(toprow);
//    // Bottom visual coordinate of last row to update.
//    int b = rowViewportPosition(toprow + siz - 1) + rowHeight(toprow + siz - 1);
//
//    viewport()->update(0, t, viewport()->width(), b - t);
//}
//
//void ZDictionaryListView::updateSelected()
//{
//#ifdef _DEBUG
//    if (!multi || model() == nullptr)
//        throw "Cannot update rows when not in multi line mode or no model set.";
//#endif
//    if (savedselmode == QAbstractItemView::SingleSelection)
//    {
//        if (currentRow() != -1)
//            updateRow(currentRow());
//        return;
//    }
//
//    int cnt = selection->rangeCount();
//
//    for (int ix = 0; ix != cnt; ++ix)
//    {
//        auto &range = selection->ranges(ix);
//
//        int toprow = multiModel()->mapFromSourceRow(range.from);
//        int bottomrow = multiModel()->mapFromSourceRow(range.to);
//        int bottomsiz = multiModel()->mappedRowSize(range.to);
//
//        // Top visual coordinate of first row to update.
//        int t = rowViewportPosition(toprow);
//        // Bottom visual coordinate of last row to update.
//        int b = rowViewportPosition(bottomrow + bottomsiz - 1) + rowHeight(bottomrow + bottomsiz - 1);
//
//        viewport()->update(0, t, viewport()->width(), b - t);
//    }
//}


//-------------------------------------------------------------

//QImage* DictionaryListDelegate::pop = nullptr;
//QImage* DictionaryListDelegate::midpop = nullptr;
//QImage* DictionaryListDelegate::unpop = nullptr;

DictionaryListDelegate::DictionaryListDelegate(ZDictionaryListView *parent) : base(parent), showgroup(false)
{

}

void DictionaryListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // NOT-TO-DO: improve text draw performance by using QStaticText. (won't help because the text is hardly static during scrolling...)

    painter->setClipRect(option.rect);

    QPalette::ColorGroup colorgrp = (index.isValid() && !index.flags().testFlag(Qt::ItemIsEnabled)) ? QPalette::Disabled : (option.state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive;
    //QPalette::ColorGroup colorgrp = (option.state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive;

    int coltype = index.data((int)DictColumnRoles::Type).toInt();
    int defix = index.data((int)DictRowRoles::DefIndex).toInt();
    WordEntry *e = index.data((int)DictRowRoles::WordEntry).value<WordEntry*>();

    std::vector<InfTypes> *inf = (std::vector<InfTypes>*)index.data((int)DictRowRoles::Inflection).value<intptr_t>();

    // The painted item is in the current row of the owner view.
    bool current = index.isValid() ? owner()->currentRow() == index.row() : false;
    // This row is selected in the view.
    bool selected = /*owner()->selectionType() == ListSelectionType::Single ? current : */owner()->rowSelected(index.row());

    QColor gridColor;
    if (Settings::colors.grid.isValid())
        gridColor = Settings::colors.grid;
    else
    {
        int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &option, (QWidget*)option.styleObject);
        gridColor = static_cast<QRgb>(gridHint);
    }

    QColor celltextcol = index.data((int)CellRoles::TextColor).value<QColor>();
    QColor basetextcol = option.palette.color(colorgrp, QPalette::Text);
    QColor seltextcol = option.palette.color(colorgrp, QPalette::HighlightedText);
    QColor textcol = selected ? seltextcol : basetextcol;
    if (celltextcol.isValid())
        textcol = colorFromBase(basetextcol, textcol, celltextcol);


    QColor cellcol = index.data((int)CellRoles::CellColor).value<QColor>();
    QColor basecol = option.palette.color(colorgrp, QPalette::Base);
    QColor selbasecol = option.palette.color(colorgrp, QPalette::Highlight);
    QColor bgcol = selected ? selbasecol : basecol;
    if (cellcol.isValid())
        bgcol = colorFromBase(basecol, bgcol, cellcol);
    else if (inf != 0)
        bgcol = colorFromBase(basecol, bgcol, Settings::uiColor(ColorSettings::Inf));
    painter->setBrush(bgcol);

    if (bgcol != basecol)
        painter->fillRect(option.rect, bgcol);


    QPen p = painter->pen();
    painter->setPen(gridColor);
    if (defix == -1 || defix == e->defs.size() - 1)
        painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());

    painter->drawLine(option.rect.right(), option.rect.top(), option.rect.right(), option.rect.bottom());
    painter->setPen(p);

    // The position in the cell's rectangle where text can go. Skips the space of a checkbox
    // if present.
    int checkboxright = option.rect.left();

    if (/*index.flags().testFlag(Qt::ItemIsUserCheckable)*/ index.data(Qt::CheckStateRole).isValid())
    {
        QRect r = drawCheckBox(painter, index);
        checkboxright = r.right() + 1;
    }

    QRect r = option.rect.adjusted(0, 0, -1, -1);

    switch (coltype)
    {
    case (int)DictColumnTypes::Frequency:
    {
        if (defix > 0)
            break;

        ushort val = index.data(Qt::DisplayRole).value<ushort>();

        r.setLeft(checkboxright);

        // Looking for JLPT data in the commons tree.
        if (Settings::dictionary.showjlpt && (Settings::dictionary.jlptcolumn == DictionarySettings::Frequency || Settings::dictionary.jlptcolumn == DictionarySettings::Both))
        {
            WordCommons *c = ZKanji::commons.findWord(e->kanji.data(), e->kana.data(), e->romaji.data());
            if (c != nullptr && c->jlptn != 0)
            {
                QFont f = Settings::notesFont();
                f.setPointSize(r.height() / 2 - 2);
                painter->save();
                painter->setFont(f);

                if (!selected)
                {
                    switch (c->jlptn)
                    {
                    case 1:
                        painter->setPen(Settings::uiColor(ColorSettings::N1));
                        break;
                    case 2:
                        painter->setPen(Settings::uiColor(ColorSettings::N2));
                        break;
                    case 3:
                        painter->setPen(Settings::uiColor(ColorSettings::N3));
                        break;
                    case 4:
                        painter->setPen(Settings::uiColor(ColorSettings::N4));
                        break;
                    case 5:
                        painter->setPen(Settings::uiColor(ColorSettings::N5));
                        break;
                    }
                }
                else
                    painter->setPen(textcol);

                QString str = QString("N%1").arg((int)c->jlptn);
                //drawTextBaseline(painter, 0, r.top() + r.height() * 0.8, true, r, str);
                painter->drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, str);
                painter->restore();
                break;
            }
        }

        QImage img;
        QRect imgrect = r.adjusted(3, 3, -3, -3);
        if (imgrect.width() != imgrect.height())
        {
            int dif = imgrect.height() - imgrect.width();
            imgrect.setWidth(imgrect.height());
            imgrect.moveLeft(r.left() + (r.width() - imgrect.width()) / 2);
        }
        if (val > ZKanji::popularFreqLimit)
            img = imageFromSvg(":/pop.svg", imgrect.width(), imgrect.height(), owner()->sizeBase() == ListSizeBase::Popup ? 1 : 0);
        else if (val > ZKanji::mediumFreqLimit)
            img = imageFromSvg(":/midpop.svg", imgrect.width(), imgrect.height(), owner()->sizeBase() == ListSizeBase::Popup ? 1 : 0);
        else
            img = imageFromSvg(":/unpop.svg", imgrect.width(), imgrect.height(), owner()->sizeBase() == ListSizeBase::Popup ? 1 : 0);

        if (!img.isNull())
            painter->drawImage(imgrect, img);
        break;
    }
    case (int)DictColumnTypes::Kanji:
    {
        QFont f = Settings::kanaFont(); //{ kanaFontName(), 9 };
        f.setPixelSize(r.height() * kanjiRowSize /*/ 2 + 1*/);
        painter->save();

        if (defix > 0)
        {
            // Aiming for light gray text color when drawing rows of other definitions.
            painter->setPen(mixColors(bgcol, textcol, 0.75));
        }
        else if (!selected && !e->defs.empty() && (e->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0)
        {
            if (Settings::colors.kanaonly.isValid())
                painter->setPen(Settings::colors.kanaonly);
            else
                painter->setPen(Settings::uiColor(ColorSettings::KanaOnly));
        }
        else
            painter->setPen(textcol);

        painter->setFont(f);

        paintKanji(painter, index, checkboxright + 4, r.top(), r.top() + r.height() * 0.8, r);

        painter->restore();
        break;
    }
    case (int)DictColumnTypes::Kana:
    {
        QString str = index.data(Qt::DisplayRole).toString();
        QFont f = Settings::kanaFont(); // { kanaFontName(), 9 };
        f.setPixelSize(r.height() * kanjiRowSize /*/ 2 + 1*/);
        painter->save();

        if (defix > 0)
        {
            // Aiming for light gray text color when drawing rows of other definitions.
            painter->setPen(mixColors(bgcol, textcol, 0.75));
        }
        else
            painter->setPen(textcol);

        painter->setFont(f);
        if (owner()->isTextSelecting() && owner()->textSelectionIndex() == index)
            drawSelectionText(painter, checkboxright + 4, r.top() + r.height() * 0.8, r, str);
        else
            drawTextBaseline(painter, checkboxright + 4, r.top() + r.height() * 0.8, false, r, str);
            //painter->drawText(checkboxright + 4, option.rect.top(), option.rect.width() - (checkboxright - option.rect.left()) - 8, option.rect.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, str);

        painter->restore();
        break;
    }
    case (int)DictColumnTypes::Definition:
    {

        painter->save();

        if (e == nullptr)
            throw "No word to paint.";

        r.setLeft(checkboxright);

        // Baseline position.
        int y = r.top() + r.height() * 0.8;

        if (Settings::dictionary.showingroup && showgroup && (e->dat & (1 << (int)WordRuntimeData::InGroup)))
        {
            QRect imgrect = QRect(r.left() + 3, r.top() + 3, r.height() - 6, r.height() - 6);
            QImage img = imageFromSvg(":/box.svg", imgrect.width(), imgrect.height(), owner()->sizeBase() == ListSizeBase::Popup ? 1 : 0);
            painter->drawImage(imgrect, img);
            r.setLeft(r.left() + 6 + img.width());
        }
        r.setLeft(r.left() + 4);

        painter->setPen(textcol);

        if (!owner()->isStudyDefinitionUsed())
            paintDefinition(painter, textcol, r, y, e, (current && Settings::dictionary.inflection == DictionarySettings::CurrentRow) || (Settings::dictionary.inflection == DictionarySettings::Everywhere) ? inf : nullptr, defix, selected);
        else
        {
            QFont f = Settings::mainFont(); //{ kanaFontName(), 9 };
            f.setPixelSize(r.height() * defRowSize /*/ 2 + 1*/);
            painter->setFont(f);
            drawTextBaseline(painter, r.left(), y, false, r, owner()->dictionary()->displayedStudyDefinition(index.data((int)DictRowRoles::WordIndex).toInt()));
            //painter->drawText(r.left(), y, owner()->dictionary()->displayedStudyDefinition(index.data((int)DictRowRoles::WordIndex).toInt()));
        }

        painter->restore();
        break;
    }
    default:
        if (defix > 0)
            break;
        QStyleOptionViewItem opt = option;
        opt.rect.adjust(0, 0, -1, -1);
        base::paint(painter, opt, index);
    }

    if (current && owner()->hasFocus())
    {
        // Draw the focus rectangle.
        QStyleOptionFocusRect fop;
        fop.backgroundColor = option.palette.color(colorgrp, QPalette::Highlight);
        fop.palette = option.palette;
        fop.direction = option.direction;
        fop.rect = owner()->rowRect(index.row());
        fop.rect.adjust(0, 0, -1, -1);
        fop.state = QStyle::State(QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange | (selected ? QStyle::State_Selected : QStyle::State_None));
        owner()->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &fop, painter, owner());
    }
}

QSize DictionaryListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int checkboxwidth = 0;
    if (/*index.flags().testFlag(Qt::ItemIsUserCheckable)*/ index.data(Qt::CheckStateRole).isValid())
    {
        QRect r = checkBoxRect(index);
        checkboxwidth = r.width() + 4;
    }

    int coltype = index.data((int)DictColumnRoles::Type).toInt();
    if (coltype == (int)DictColumnTypes::Kanji || coltype == (int)DictColumnTypes::Kana)
    {
        QFont f = Settings::kanaFont();
        f.setPixelSize(option.rect.height() * kanjiRowSize /* / 2 + 1*/);
        QFontMetrics fm = QFontMetrics(f);

        QString str = index.data(Qt::DisplayRole).toString();
        QSize siz = fm.boundingRect(str).size();
        siz.setWidth(siz.width() + 8 + checkboxwidth);
        return siz;
    }

    QSize siz = base::sizeHint(option, index);
    siz.setWidth(siz.width() + checkboxwidth);
    return siz;
}

bool DictionaryListDelegate::groupDisplay()
{
    return showgroup;
}

void DictionaryListDelegate::setGroupDisplay(bool val)
{
    showgroup = val;
}

void DictionaryListDelegate::paintDefinition(QPainter *painter, QColor textcolor, QRect r, int y, WordEntry *e, std::vector<InfTypes> *inf, int defix, bool selected) const
{
    // Painting word definition is done in several steps.
    // First the word's global information is painted with the small font, followed by the
    // definitions. Each definition starts with the definition number in small font (but
    // different size), and the info text, also with the small font. The main definition
    // with meaning font comes in the middle, ending in note and dialect definitions also
    // with small font. This is repeated for each definition.
    // In case only a single definition is drawn on the line, the number is omitted.
    // (Unless using multiple table lines.)

    bool multidef = e->defs.size() > 1;

    if (selected)
        painter->setPen(textcolor);

    // Main definition font.
    QFont f = Settings::mainFont();
    f.setPixelSize(r.height() * defRowSize/* / 2 + 1*/);
    //f.setPointSize(r.height() / 2 + 1);
    // Small font for word notes text.
    QFont fs = Settings::notesFont();
    fs.setPixelSize(r.height() * notesRowSize /*/ 2 - 2*/);
    //fs.setPointSize(r.height() / 2 - 2);
    // Font used for drawing def number.
    QFont fsf = Settings::mainFont();
    fsf.setPixelSize(f.pixelSize());
    QFont fse = Settings::extraFont();
    fse.setPixelSize(f.pixelSize());

    QFontMetrics fmet{ f };
    QFontMetrics fsmet{ fs };
    QFontMetrics fsfmet{ fsf };
    QFontMetrics fsemet{ fse };

    QString str;

    int spacewidth = fmet.averageCharWidth();

    QColor textcol = painter->pen().color();

    if (inf != nullptr)
    {
        painter->setFont(fse);
        str = Strings::wordInflectionText(*inf);
        drawTextBaseline(painter, r.left(), y, false, r, str);
        //painter->drawText(r.left(), y, str);
        r.setLeft(r.left() + fsemet.boundingRect(str).width() + spacewidth);
    }

    // Looking for JLPT data in the commons tree.
    if (Settings::dictionary.showjlpt && (Settings::dictionary.jlptcolumn == DictionarySettings::Definition || Settings::dictionary.jlptcolumn == DictionarySettings::Both))
    {
        WordCommons *c = ZKanji::commons.findWord(e->kanji.data(), e->kana.data(), e->romaji.data());
        if (c != nullptr && c->jlptn != 0)
        {
            painter->setFont(fs);

            if (!selected)
            {
                switch (c->jlptn)
                {
                case 1:
                    painter->setPen(Settings::uiColor(ColorSettings::N1));
                    break;
                case 2:
                    painter->setPen(Settings::uiColor(ColorSettings::N2));
                    break;
                case 3:
                    painter->setPen(Settings::uiColor(ColorSettings::N3));
                    break;
                case 4:
                    painter->setPen(Settings::uiColor(ColorSettings::N4));
                    break;
                case 5:
                    painter->setPen(Settings::uiColor(ColorSettings::N5));
                    break;
                }
            }
            else
                painter->setPen(textcol);

            str = QString("N%1").arg((int)c->jlptn);
            //drawTextBaseline(painter, r.left(), y, false, r, str);
            painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth);
        }
    }

    // Drawing the global word info field.

    str = Strings::wordInfoText(e->inf);

    if (!str.isEmpty())
    {
        painter->setFont(fs);
        if (!selected)
            painter->setPen(Settings::uiColor(ColorSettings::Attrib));

        drawTextBaseline(painter, r.left(), y, false, r, str);
        //painter->drawText(r.left(), y, str);
        r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth / 2);
    }

    QString separator = qApp->translate("Dictionary", ", ");

    // Definitions.

    for (int ix = 0; ix < e->defs.size(); ++ix)
    {
        if (defix != -1)
            ix = defix;

        painter->setFont(fsf);
        if (multidef)
        {
            str = QStringLiteral("%1.").arg(ix + 1);
            if (!selected)
                painter->setPen(textcolor);
            drawTextBaseline(painter, r.left(), y, false, r, str);
            //painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsfmet.boundingRect(str).width() + spacewidth);
        }
        if (r.left() > r.right())
            break;

        painter->setFont(fs);

        WordDefinition &def = e->defs[ix];
        if (def.attrib.types != 0)
        {
            str = Strings::wordTypesText(def.attrib.types) + " ";
            if (!selected)
                painter->setPen(Settings::uiColor(ColorSettings::Types));

            drawTextBaseline(painter, r.left(), y, false, r, str);
            //painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth / 2);
        }
        if (r.left() > r.right())
            break;

        if (def.attrib.notes != 0)
        {
            str = Strings::wordNotesText(def.attrib.notes) + " ";

            if (!selected)
                painter->setPen(Settings::uiColor(ColorSettings::Notes));

            drawTextBaseline(painter, r.left(), y, false, r, str);
            //painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth / 2);
        }
        if (r.left() > r.right())
            break;


        painter->setFont(f);
        str = def.def.toQStringRaw();
        str.replace(GLOSS_SEP_CHAR, separator);
        if (!selected)
            painter->setPen(textcolor);
        drawTextBaseline(painter, r.left(), y, false, r, str);
        //painter->drawText(r.left(), y, str);
        r.setLeft(r.left() + fmet.boundingRect(str).width() + spacewidth / 2);
        if (r.left() > r.right())
            break;

        painter->setFont(fs);


        if (def.attrib.fields != 0)
        {
            str = Strings::wordFieldsText(def.attrib.fields) + " ";

            if (!selected)
                painter->setPen(Settings::uiColor(ColorSettings::Fields));

            drawTextBaseline(painter, r.left(), y, false, r, str);
            //painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth / 2);
        }

        if (def.attrib.dialects != 0)
        {
            str += " " + Strings::wordDialectsText(def.attrib.dialects);

            if (!selected)
                painter->setPen(Settings::uiColor(ColorSettings::Dialects));

            drawTextBaseline(painter, r.left(), y, false, r, str);
            //painter->drawText(r.left(), y, str);
            r.setLeft(r.left() + fsmet.boundingRect(str).width() + spacewidth / 2);
        }

        r.setLeft(r.left() + spacewidth);

        if (r.left() > r.right())
            break;

        if (defix != -1)
            break;
    }
}

void DictionaryListDelegate::paintKanji(QPainter *painter, const QModelIndex &index, int left, int top, int basey, QRect r) const
{
    QString str = index.data(Qt::DisplayRole).toString();
    if (owner()->isTextSelecting() && owner()->textSelectionIndex() == index)
        drawSelectionText(painter, left, basey, r, str);
    else
        drawTextBaseline(painter, left, basey, false, r, str);
    //painter->drawText(checkboxright + 4, option.rect.top(), option.rect.width() - (checkboxright - option.rect.left()) - 8, option.rect.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, str);
}

int DictionaryListDelegate::kanjiAt(QPoint pos, QRect itemrect, const QModelIndex &index, QRect *kanjirect, int *charpos)
{
    int coltype = index.data((int)DictColumnRoles::Type).toInt();
    if (coltype != (int)DictColumnTypes::Kanji && coltype != (int)DictColumnTypes::Kana)
        return -1;

    QFont f = Settings::kanaFont();
    f.setPixelSize(itemrect.height() * kanjiRowSize /*/ 2 + 1*/);
    QFontMetrics fm(f);

    int chheight = fm.height();
    int y = pos.y() - itemrect.top() - (itemrect.height() - chheight) / 2;

    if (y < 0 || y >= chheight)
        return -1;

    int left = (/*(index.flags() & Qt::ItemIsUserCheckable)*/ index.data(Qt::CheckStateRole).isValid() ? checkBoxRect(index).right() : itemrect.left()) + 4;
    int x = pos.x() - left;
    if (x < 0)
        return -1;

    QString str = index.data(Qt::DisplayRole).toString();

    for (int ix = 0; ix != str.size(); ++ix)
    {
        QChar ch = str.at(ix);
        int chwidth = fm.width(ch);
        if (x < chwidth)
        {
            int r = ZKanji::kanjiIndex(ch);
            if (kanjirect != nullptr && r != -1)
            {
                int h = fm.height();
                int top = itemrect.top() + (itemrect.height() - h) / 2;
                *kanjirect = QRect(left, top, chwidth, h);
            }
            if (charpos != nullptr && r != -1)
                *charpos = ix;
            return r;
        }
        x -= chwidth;
        left += chwidth;
    }

    return -1;
}

int DictionaryListDelegate::characterIndexAt(QPoint pos, QRect itemrect, const QModelIndex &index, bool forced)
{
    int coltype = index.data((int)DictColumnRoles::Type).toInt();
    if (coltype != (int)DictColumnTypes::Kanji && coltype != (int)DictColumnTypes::Kana)
        return -1;

    QFont f = Settings::kanaFont();
    f.setPixelSize(itemrect.height() * kanjiRowSize /*/ 2 + 1*/);
    QFontMetrics fm(f);

    if (!forced)
    {
        int chheight = fm.height();
        int y = pos.y() - itemrect.top() - (itemrect.height() - chheight) / 2;
        if (y < 0 || y >= chheight)
            return -1;
    }

    int left = (index.data(Qt::CheckStateRole).isValid() ? checkBoxRect(index).right() : itemrect.left()) + 4;
    int x = pos.x() - left;
    if (x < 0)
        return x >= -4 || forced ? 0 : -1;

    QString str = index.data(Qt::DisplayRole).toString();

    for (int ix = 0; ix != str.size(); ++ix)
    {
        QChar ch = str.at(ix);
        int chwidth = fm.width(ch);
        if (x < chwidth)
        {
            if (x >= chwidth - (chwidth / 2))
                return std::min(str.size(), ix + 1);
            return ix;
        }
        x -= chwidth;
        left += chwidth;
    }

    return x < 4 || forced ? str.size() : -1;
}

ZDictionaryListView* DictionaryListDelegate::owner()
{
    return (ZDictionaryListView*)parent();
}

void DictionaryListDelegate::drawSelectionText(QPainter *p, int x, int basey, const QRect &clip, QString str) const
{
    int sellen = owner()->textSelectionLength();
    int selpos = owner()->textSelectionPosition();
    if (sellen < 0)
    {
        selpos += sellen;
        sellen = -sellen;
    }

    QFontMetrics fm(p->font());

    int left = 0;
    if (selpos > 0)
        drawTextBaseline(p, x, basey, false, clip, str.left(selpos));
    while (left != selpos)
    {
        x += fm.width(str.at(left));
        ++left;
    }

    if (sellen > 0)
    {
        QColor pc = p->pen().color();
        QColor bc = p->brush().color();
        p->setPen(bc);
        p->setBrush(pc);

        QRect r = clip;
        r.setLeft(x);
        int x2 = x;
        while (left != selpos + sellen)
        {
            x2 += fm.width(str.at(left));
            ++left;
        }
        r.setRight(x2);

        p->fillRect(r, pc);

        drawTextBaseline(p, x, basey, false, clip, str.mid(selpos, sellen));
        p->setPen(pc);
        p->setBrush(bc);

        x = x2;
    }

    if (selpos + sellen < str.size())
        drawTextBaseline(p, x, basey, false, clip, str.mid(selpos + sellen));
}

const ZDictionaryListView* DictionaryListDelegate::owner() const
{
    return (ZDictionaryListView*)parent();
}


//-------------------------------------------------------------
