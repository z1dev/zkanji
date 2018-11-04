/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QHeaderView>
#include <QPainter>
#include <QMimeData>

#include "zkanjimain.h"
#include "ztreeview.h"
#include "zabstracttreemodel.h"
#include "groups.h"
#include "zgrouptreemodel.h"
#include "zevents.h"
#include "groups.h"
#include "zkanjigridview.h"
#include "zkanjigridmodel.h"
#include "zdictionarylistview.h"
#include "globalui.h"
#include "settings.h"
#include "colorsettings.h"

//-------------------------------------------------------------


ZTreeView::ZTreeView(QWidget *parent) : base(parent), alldeselect(false)
{
    connect(this, &base::collapsed, this, &ZTreeView::indexCollapsed);
    connect(this, &base::expanded, this, &ZTreeView::indexExpanded);
    connect(gUI, &GlobalUI::settingsChanged, this, &ZTreeView::settingsChanged);

    Settings::updatePalette(this);

    header()->setVisible(false);
    setUniformRowHeights(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
}

ZTreeView::~ZTreeView()
{

}

ZAbstractTreeModel* ZTreeView::model() const
{
    return (ZAbstractTreeModel*)base::model();
}

void ZTreeView::setModel(ZAbstractTreeModel *model)
{
    auto oldselmodel = selectionModel();

    base::setModel(model);

    if (oldselmodel && oldselmodel != selectionModel())
        oldselmodel->deleteLater();
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

TreeItem* ZTreeView::currentItem() const
{
    QModelIndex index = base::currentIndex();
    if (!index.isValid())
        return nullptr;
    return (TreeItem*)index.internalPointer();
}

TreeItem* ZTreeView::currentParent() const
{
    TreeItem *item = currentItem();
    if (item == nullptr)
        return nullptr;

    return item->parent();
}

TreeItem* ZTreeView::itemAt(const QPoint &pos) const
{
    QModelIndex index = base::indexAt(pos);
    if (!index.isValid())
        return nullptr;
    return (TreeItem*)index.internalPointer();
}

QRect ZTreeView::visualRect(TreeItem *item) const
{
    if (item == nullptr)
        return QRect();
    return base::visualRect(model()->index(item));
}

bool ZTreeView::isExpanded(const TreeItem *item) const
{
    return base::isExpanded(model()->index(item));
}

void ZTreeView::setExpanded(TreeItem *item, bool expanded)
{
    base::setExpanded(model()->index(item), expanded);
}

bool ZTreeView::isSelected(TreeItem *item) const
{
    QModelIndex ind = model()->index(item);
    return selectionModel()->isSelected(ind);
}

void ZTreeView::selection(std::vector<TreeItem*> &result) const
{
    std::vector<TreeItem*>().swap(result);
    QModelIndexList indexes = selectedIndexes();
    result.reserve(indexes.size());
    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        QModelIndex index = indexes.at(ix);
        if (!index.isValid())
            continue;
        result.push_back((TreeItem*)index.internalPointer());
    }
}

void ZTreeView::selection(QSet<TreeItem*> &result) const
{
    result.clear();
    QModelIndexList indexes = selectedIndexes();
    result.reserve(indexes.size());
    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        QModelIndex index = indexes.at(ix);
        if (!index.isValid())
            continue;
        result.insert((TreeItem*)index.internalPointer());
    }
}

void ZTreeView::edit(TreeItem *item)
{
    if (item == nullptr)
        return;

    base::edit(model()->index(item));
}

void ZTreeView::setCurrentItem(TreeItem *item)
{
    setCurrentIndex(model()->index(item));
}

void ZTreeView::settingsChanged()
{
    Settings::updatePalette(this);
}

void ZTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    base::currentChanged(current, previous);

    TreeItem *curr = indexToItem(current);
    TreeItem *prev = indexToItem(previous);

    emit currentItemChanged(curr, prev);

}

void ZTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    base::selectionChanged(selected, deselected);

    emit itemSelectionChanged();
}

QItemSelectionModel::SelectionFlags ZTreeView::selectionCommand(const QModelIndex &index, const QEvent *e) const
{
    if (selectionMode() == QAbstractItemView::SingleSelection)
    {
        if (selectionModel()->isSelected(index))
            return QItemSelectionModel::NoUpdate;
    }

    QItemSelectionModel::SelectionFlags flags = base::selectionCommand(index, e);

    if (/*!selectionModel()->isSelected(index) ||*/ (e != nullptr && e->type() == QEvent::MouseMove))
        return flags;


    if (e != nullptr && selectionMode() == QAbstractItemView::ExtendedSelection && (e->type() == QEvent::MouseButtonDblClick || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease))
    {
        QVariant value = index.data(Qt::CheckStateRole);
        if (value.isValid())
        {
            // Prevent changing the current selection when clicking on a checkbox.
            // The model must handle selecting multiple when checking inside a selection.

            QMouseEvent *me = (QMouseEvent*)e;

            QStyleOptionViewItem opt;
            opt.initFrom(this);


            opt.rect = QTreeView::visualRect(index);

            value = index.data(Qt::FontRole);
            if (value.isValid())
            {
                opt.font = qvariant_cast<QFont>(value).resolve(opt.font);
                opt.fontMetrics = QFontMetrics(opt.font);
            }
            value = index.data(Qt::TextAlignmentRole);
            if (value.isValid())
                opt.displayAlignment = Qt::Alignment(value.toInt());

            opt.index = index;
            value = index.data(Qt::CheckStateRole);
            if (value.isValid() && !value.isNull()) {
                opt.features |= QStyleOptionViewItem::HasCheckIndicator;
                opt.checkState = static_cast<Qt::CheckState>(value.toInt());
            }
            value = index.data(Qt::DisplayRole);
            if (value.isValid() && !value.isNull()) {
                opt.features |= QStyleOptionViewItem::HasDisplay;
                opt.text = value.toString(); //displayText(value, opt.locale);
            }
            opt.styleObject = nullptr;

            QRect r = style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, this);

            // We return QItemSelectionModel::Current to prevent the selection anchor to be
            // changed in the original implementation of QAbstractItemView::mousePressEvent().
            // This might stop working in a future version of Qt.
            if (r.contains(me->pos()))
                return QItemSelectionModel::Current;
        }
    }

    return flags;
}

TreeItem* ZTreeView::indexToItem(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;
    else
        return (TreeItem*)index.internalPointer();
}

void ZTreeView::indexExpanded(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    emit expanded((TreeItem*)index.internalPointer());
}

void ZTreeView::indexCollapsed(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QItemSelectionModel *sel = selectionModel();
    QModelIndexList slist = sel->selectedIndexes();

    QItemSelection items;
    for (QModelIndex &i : slist)
    {
        QModelIndex cur = i.isValid() ? i.parent() : QModelIndex();
        while (cur.isValid())
        {
            if (cur == index)
            {
                items.select(i, i);
                break;
            }
            cur = cur.parent();
        }
    }
    if (!items.empty())
    {
        sel->select(items, QItemSelectionModel::Deselect);
        sel->select(items, QItemSelectionModel::Deselect | QItemSelectionModel::Current);
    }

    QModelIndex cur = currentIndex();
    QModelIndex pos = cur;

    if (!cur.isValid())
    {
        pos = index;
        sel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    }
    while (cur.isValid())
    {
        cur = cur.parent();
        if (cur == index)
        {
            pos = cur;
            sel->setCurrentIndex(index, QItemSelectionModel::NoUpdate /*| (sel->hasSelection() ? QItemSelectionModel::NoUpdate : QItemSelectionModel::Select)*/);
            break;
        }
    }

    if (!sel->hasSelection() && pos.isValid())
        sel->select(pos, QItemSelectionModel::Select);

    emit collapsed((TreeItem*)index.internalPointer());
}

bool ZTreeView::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        // Update first column width on translation change. Since the column is stretched,
        // setting the size to 1 forces a recalculation of the contents.
        header()->resizeSection(0, 1);
    }

    return base::event(e);
}

void ZTreeView::dropEvent(QDropEvent *e)
{
    base::dropEvent(e);

    QItemSelectionModel *sel = selectionModel();
    QModelIndexList slist = sel->selectedIndexes();

    QItemSelection items;
    for (QModelIndex &i : slist)
    {
        QModelIndex cur = i.isValid() ? i.parent() : QModelIndex();
        while (cur.isValid())
        {
            if (!base::isExpanded(cur))
            {
                items.select(i, i);
                break;
            }
            cur = cur.parent();
        }
    }
    if (!items.empty())
    {
        sel->select(items, QItemSelectionModel::Deselect);
        sel->select(items, QItemSelectionModel::Deselect | QItemSelectionModel::Current);
    }

    QModelIndex cur = currentIndex();
    QModelIndex pos = cur;
    while (cur.isValid())
    {
        if (!base::isExpanded(cur))
            pos = cur;
        cur = cur.parent();
    }

    if (pos.isValid() && pos != cur)
        sel->setCurrentIndex(pos, QItemSelectionModel::NoUpdate /*| (sel->hasSelection() ? QItemSelectionModel::NoUpdate : QItemSelectionModel::Select)*/);

    if (!sel->hasSelection() && pos.isValid())
        sel->select(pos, QItemSelectionModel::Select);
}


//-------------------------------------------------------------


GroupTreeView::GroupTreeView(QWidget *parent) : base(parent), state(State::Idle), cachepos(nullptr), edititem(nullptr), dragdest(nullptr), dragpos(DragPos::Undefined)
{
    setDropIndicatorShown(false);
}

GroupTreeView::~GroupTreeView()
{

}

GroupTreeModel* GroupTreeView::model()
{
    return (GroupTreeModel*)base::model();
}

const GroupTreeModel* GroupTreeView::model() const
{
    return (const GroupTreeModel*)base::model();
}

void GroupTreeView::setModel(GroupTreeModel *model)
{
    base::setModel(model);
}

void GroupTreeView::select(GroupBase *group)
{
    TreeItem *item = model()->findTreeItem(group);
    setCurrentItem(item);
}

void GroupTreeView::selectGroups(const std::vector<GroupBase*> &groups)
{
    QItemSelectionModel *m = selectionModel();
    QItemSelection items;
    m->select(items, QItemSelectionModel::Clear);
    m->select(items, QItemSelectionModel::Clear | QItemSelectionModel::Current);

    for (const GroupBase *g : groups)
    {
        TreeItem *item = model()->findTreeItem(g);
        if (item == nullptr)
            continue;
        TreeItem *p = item->parent();
        while (p)
        {
            setExpanded(p, true);
            p = p->parent();
        }
        QModelIndex ind = model()->index(item);
        items.select(ind, ind);
    }
    m->select(items, QItemSelectionModel::Select);
}

void GroupTreeView::selectTop()
{
    if (model() == nullptr || model()->size() == 0)
        return;
    setCurrentItem(model()->items(0));
}

//bool GroupTreeView::currentIsCategory() const
//{
//    QModelIndex index = base::currentIndex();
//    return index.isValid() && ((GroupBase*)((TreeItem*)index.internalPointer())->userData())->isCategory();
//}
//
//bool GroupTreeView::currentIsGroup() const
//{
//    QModelIndex index = base::currentIndex();
//    return index.isValid() && !((GroupBase*)((TreeItem*)index.internalPointer())->userData())->isCategory();
//}

bool GroupTreeView::isCategory(TreeItem *item) const
{
    return item != nullptr && ((GroupBase*)item->userData())->isCategory();
}

bool GroupTreeView::isGroup(TreeItem *item) const
{
    return item != nullptr && !((GroupBase*)item->userData())->isCategory();
}

//bool GroupTreeView::itemEmpty(TreeItem *item) const
//{
//    return item != nullptr && ((GroupBase*)item->userData())->isEmpty();
//}

bool GroupTreeView::hasGroupSelected() const
{
    QModelIndexList indexes = selectedIndexes();
    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        QModelIndex index = indexes.at(ix);
        if (index.isValid() && ((TreeItem*)index.internalPointer())->userData() != 0 && !((GroupBase*)((TreeItem*)index.internalPointer())->userData())->isCategory())
            return true;
    }
    return false;
}

void GroupTreeView::selectedGroups(std::vector<GroupBase*> &result) const
{
    std::vector<GroupBase*>().swap(result);
    QModelIndexList indexes = selectedIndexes();
    result.reserve(indexes.size());
    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        QModelIndex index = indexes.at(ix);
        if (!index.isValid())
            continue;
        GroupBase *g = ((GroupBase*)((TreeItem*)index.internalPointer())->userData());
        if (g != nullptr)
            result.push_back(g);
    }
}

void GroupTreeView::createCategory(TreeItem *parent)
{
    if (state != State::Idle)
        return;

    cachepos = currentItem();
    if (cachepos && cachepos->userData() == 0)
        cachepos = cachepos->parent();

    state = State::Creation;
    edititem = model()->startAddCategory(parent);

    setCurrentItem(edititem);
    base::edit(edititem);
}

void GroupTreeView::createGroup(TreeItem *parent)
{
    if (state != State::Idle)
        return;

    cachepos = currentItem();
    if (cachepos && cachepos->userData() == 0)
        cachepos = cachepos->parent();

    state = State::Creation;
    edititem = model()->startAddGroup(parent);

    setCurrentItem(edititem);
    base::edit(edititem);
}

//void GroupTreeView::remove(TreeItem *item)
//{
//    if (state != State::Idle)
//        return;
//
//    model()->remove(item);
//}
//
//void GroupTreeView::remove(const std::vector<TreeItem*> items)
//{
//    if (state != State::Idle)
//        return;
//
//    model()->remove(items);
//}

bool GroupTreeView::currentCompleted(bool currentmustexist)
{
    if (state == State::Creation)
        return false;
    TreeItem *i = currentItem();
    if (currentmustexist && (!i || i->userData() == 0))
        return false;
    return true;
}

void GroupTreeView::scrollContentsBy(int dx, int dy)
{
    base::scrollContentsBy(dx, dy);

    if (state != State::Dragging)
        return;

    QDragMoveEvent e = QDragMoveEvent(viewport()->mapFromGlobal(QCursor::pos()), savedDropActions, savedMimeData, savedMouseButtons, savedKeyboardModifiers);
    dragMoveEvent(&e);
}

void GroupTreeView::mousePressEvent(QMouseEvent *e)
{
    if (state == State::Creation)
    {
        TreeItem *item = itemAt(e->pos());
        if (item != nullptr && item->userData() != 0)
        {
            cachepos = item;
            if (cachepos && cachepos->userData() == 0)
                cachepos = cachepos->parent();
        }

        base::mousePressEvent(e);
        return;
    }

    base::mousePressEvent(e);
}

void GroupTreeView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (hint == QAbstractItemDelegate::EditNextItem || hint == QAbstractItemDelegate::EditPreviousItem)
        hint = QAbstractItemDelegate::SubmitModelCache;

    base::closeEditor(editor, hint);

    if (state == State::Creation)
        qApp->postEvent(this, new EditorClosedEvent);
}

bool GroupTreeView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *e)
{
    if (trigger != QAbstractItemView::CurrentChanged && trigger != QAbstractItemView::NoEditTriggers)
        edititem = (TreeItem*)index.internalPointer();
    return base::edit(index, trigger, e);
}

QRect GroupTreeView::_indicatorRect(TreeItem *item, bool after) const
{
    // TODO: (later) scale the insert indicator depending on GUI size.
    const int INDICATOR_LEFT = 8;
    const int INDICATOR_RIGHT = 32;

    QRect r = visualRect(item);
    if (after)
    {
        r.setTop(r.bottom());
        r.setBottom(r.bottom() + 1);
    }
    else
    {
        r.setBottom(r.top());
        r.setTop(r.top() - 1);
    }

    r.setRight(r.left() + INDICATOR_RIGHT);
    r.setLeft(r.left() - INDICATOR_LEFT);

    // There's a special case when only categories are present in the root and the last one
    // is expanded. Dragging a category into the empty space below it would show the indicator
    // rectangle above the expanded item's children, and not after them.
    if (!model()->onlyCategories() && after && model()->rootCategory()->size() == 0 && model()->items(model()->rootCategory()->categoryCount() - 1) == item && isExpanded(item))
    {
        // Find the last visible item.

        while (isExpanded(item->items(tosigned(item->size()) - 1)))
            item = item->items(tosigned(item->size()) - 1);
        QRect r2 = visualRect(item->items(tosigned(item->size()) - 1));

        r.setTop(r2.bottom());
        r.setBottom(r2.bottom() + 1);
    }

    return r;
}

bool GroupTreeView::event(QEvent *e)
{
    if (e->type() == EditorClosedEvent::Type())
    {
        if (state != State::Creation)
            return true;

        state = State::Idle;

        if (edititem->userData() == 0 /*((GroupBase*)editem->userData())->name().isEmpty()*/)
        {
            setCurrentItem(cachepos);
            model()->remove(edititem);
            emit itemSelectionChanged();
        }
        else
        {
            emit itemCreated();

            // Expand and select the first sub item if the created item was a category.
            setExpanded(edititem, true);
            if (!model()->onlyCategories() && isExpanded(edititem) && edititem->size() != 0)
            {
                edititem = edititem->items(0);
                setCurrentItem(edititem);
            }
            else
            {
                if (currentItem() != edititem)
                    setCurrentItem(edititem);
                else
                {
                    emit currentItemChanged(edititem, edititem);
                    emit itemSelectionChanged();
                }
            }
        }

        edititem = nullptr;
        cachepos = nullptr;

        return true;
    }

    return base::event(e);
}

void GroupTreeView::timerEvent(QTimerEvent *e)
{
    if (expandtimer.timerId() == e->timerId())
    {
        QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
        if (state == State::Dragging && viewport()->rect().contains(pos) && itemAt(pos) == cachepos)
            setExpanded(cachepos, !isExpanded(cachepos));
        expandtimer.stop();
        return;
    }

    base::timerEvent(e);
}

void GroupTreeView::startDrag(Qt::DropActions supportedActions)
{
    QModelIndex curr = currentIndex();
    QModelIndexList list = selectionModel()->selectedIndexes();
    QItemSelection unsel;
    for (const QModelIndex ind : list)
    {
        if (ind.parent() != curr.parent())
            unsel.select(ind, ind);
    }
    selectionModel()->select(unsel, QItemSelectionModel::Deselect);

    base::startDrag(supportedActions);
}

void GroupTreeView::dragEnterEvent(QDragEnterEvent *e)
{
    enum DragFormats { Groups, Kanji, Words, Unknown };

    DragFormats format = e->mimeData()->hasFormat("zkanji/groups") ? Groups : e->mimeData()->hasFormat("zkanji/kanji") ? Kanji : e->mimeData()->hasFormat("zkanji/words") ? Words : Unknown;

    e->ignore();
    if (model()->isFiltered() && format == Groups && ((GroupBase*)*(intptr_t*)e->mimeData()->data("zkanji/groups").constData())->dictionary() != model()->dictionary())
        return;

    if (format == Groups && e->source() == this && (e->possibleActions() & Qt::MoveAction) == Qt::MoveAction)
    {
        e->setDropAction(Qt::MoveAction);
        e->accept();

        // Preventing hovered items to repaint with a default hover selection background.
        setStyleSheet(QString("QTreeView::item:!selected:hover { background: transparent; } QTreeView::item:active:selected { background: transparent; color: %1; } QTreeView::item:!active:selected { background: transparent; color: %2; }").arg(Settings::textColor(true, ColorSettings::Text).name()).arg(Settings::textColor(false, ColorSettings::Text).name()));
        Settings::updatePalette(this);
    }
    else if (((format == Kanji && model()->groupType() == GroupTypes::Kanji) || (format == Words && model()->groupType() == GroupTypes::Words)) &&
        ((e->possibleActions() & Qt::MoveAction) == Qt::MoveAction || (e->possibleActions() & Qt::CopyAction) == Qt::CopyAction))
    {
        // Only allow drag into the same dictionary for word groups.
        if (model()->groupType() == GroupTypes::Kanji || model()->dictionary() == (void*)(*(intptr_t*)e->mimeData()->data(format == Words ? "zkanji/words" : "zkanji/kanji").constData()))
            e->accept();
    }

    if (e->isAccepted())
    {
        savedDropActions = e->dropAction();
        savedMimeData = e->mimeData();
        savedMouseButtons = e->mouseButtons();
        savedKeyboardModifiers = e->keyboardModifiers();

        state = State::Dragging;
    }
}

void GroupTreeView::dragLeaveEvent(QDragLeaveEvent *e)
{
    if (state == State::Dragging)
        expandtimer.stop();

    state = State::Idle;
    cachepos = nullptr;

    if (dragdest != nullptr)
    {
        if (dragpos != DragPos::Inside && !indrect.isEmpty())
            viewport()->update(indrect);
        else if (dragpos == DragPos::Inside)
            viewport()->update(visualRect(dragdest));
    }
    dragdest = nullptr;
    dragpos = DragPos::Undefined;
    indrect = QRect();

    // Restoring style sheet to hover items again, that was disabled in dragEnterEvent.
    setStyleSheet("");
    Settings::updatePalette(this);

    base::dragLeaveEvent(e);
}

void GroupTreeView::dragMoveEvent(QDragMoveEvent *e)
{
    enum DragFormats { Groups, Kanji, Words, Unknown };

    DragFormats format = e->mimeData()->hasFormat("zkanji/groups") ? Groups : e->mimeData()->hasFormat("zkanji/kanji") ? Kanji : e->mimeData()->hasFormat("zkanji/words") ? Words : Unknown;

    if (format == Unknown ||
        (format == Groups && ((e->possibleActions() & Qt::MoveAction) == 0 || e->source() != this)) ||
        ((format == Kanji || format == Words) && (e->possibleActions() & (Qt::MoveAction | Qt::CopyAction)) == 0))
    {
        e->ignore();
        return;
    }

    base::dragMoveEvent(e);

    if (format == Groups && e->proposedAction() != Qt::MoveAction)
    {
        e->setDropAction(Qt::MoveAction);
        e->accept();
    }
    else
        e->acceptProposedAction();

    savedDropActions = e->dropAction();
    savedMimeData = e->mimeData();
    savedMouseButtons = e->mouseButtons();
    savedKeyboardModifiers = e->keyboardModifiers();

    if (state != State::Dragging)
    {
        state = State::Dragging;

        // The cachepos is used to identify the item we hover over. It must be set to null
        // just in case it was already set to an item in some other operation.
        cachepos = nullptr;
    }

    TreeItem *olddragdest = dragdest;
    DragPos olddragpos = dragpos;

    dragdest = nullptr;
    dragpos = DragPos::Undefined;

    // Remember the hover position to be able to draw a drop indicator where the group will be
    // inserted.

    // The group being dragged over the view.
    GroupBase *grp = format == Groups ? (GroupBase*)*(intptr_t*)e->mimeData()->data("zkanji/groups").constData() : nullptr;

    TreeItem *i = itemAt(e->pos());
    GroupBase *hovered = i == nullptr ? nullptr : (GroupBase*)i->userData();

    if (format == Groups)
    {
        if (grp->isCategory())
        {
            // Dragging a category inside its own tree is invalid.

            bool insidemove = false;
            if (i != nullptr)
            {
                TreeItem *pos = i->parent();
                while (pos && pos->userData() != (intptr_t)grp)
                    pos = pos->parent();

                // Invalid move confirmed. 
                if (pos != nullptr)
                    insidemove = true;
            }

            // Find the drag position.

            if (i == nullptr)
            {
                // Mouse is not on a tree item. The drop destination is after the last root
                // category.
                dragdest = model()->items(model()->rootCategory()->categoryCount() - 1);
                dragpos = DragPos::After;
            }
            else if (insidemove)
            {
                // Dragging inside the tree of the item being dragged is invalid.
                e->ignore();
                i = nullptr;
                dragdest = nullptr;
                dragpos = DragPos::Undefined;
            }
            else if (hovered == nullptr)
            {
                // Mouse is over the placeholder "add new item" tree item. Indicate that the
                // category will be moved above it.
                dragdest = i;
                dragpos = DragPos::Before;
            }
            else if (!hovered->isCategory())
            {
                // Find the rectangle after the last category in the parent of the hovered 
                // item. As the last category tree item might be opened, the indicator
                // rectangle is for the next item after the category.

                TreeItem *catitem;
                if (i->parent() != nullptr)
                {
                    GroupCategoryBase *hparent = (GroupCategoryBase*)i->parent()->userData();
                    catitem = i->parent()->items(hparent->categoryCount());
                }
                else
                    catitem = model()->items(model()->rootCategory()->categoryCount());

                dragdest = catitem;
                dragpos = DragPos::Before;
            }
            else
            {
                // Hovering over a category.

                QRect r = visualRect(i);

                if (hovered == grp)
                {
                    // When hovering over the category being dragged, either insertion before that
                    // category, after that category (when closed), or ignoring the drop action
                    // are valid.

                    if (e->pos().y() < r.top() + r.height() / 2)
                    {
                        dragdest = i;
                        dragpos = DragPos::Before;
                    }
                    else if (!isExpanded(i))
                    {
                        dragdest = i;
                        dragpos = DragPos::After;
                    }
                    else
                    {
                        e->ignore();
                        i = nullptr;
                        dragdest = nullptr;
                        dragpos = DragPos::Undefined;
                    }
                }
                else
                {
                    // We have to figure out whether inserting before/after that category, or moving
                    // into that category. If the category is open, only inserting before it or moving
                    // into it are possible.

                    if (e->pos().y() < r.top() + r.height() / 4)
                    {
                        dragdest = i;
                        dragpos = DragPos::Before;
                    }
                    else if (!isExpanded(i) && e->pos().y() >= r.bottom() - r.height() / 4) // Inserting after.
                    {
                        dragdest = i;
                        dragpos = DragPos::After;
                    }
                    else
                    {
                        dragdest = i;
                        dragpos = DragPos::Inside;
                    }
                }
            }
        }
        else
        {
            // Dragging a group.

            if (hovered == nullptr)
            {
                if (i != nullptr)
                {
                    // Mouse is over the placeholder "add new item" tree item.
                    dragdest = i;
                    dragpos = DragPos::Before;
                }
                else
                {
                    // Mouse is over the empty space after the last item. The dragged item should
                    // be placed as the new last item.
                    dragdest = model()->items(model()->rootCategory()->categoryCount() + tosigned(model()->rootCategory()->size()) - 1);
                    dragpos = DragPos::After;
                }
            }
            else if (!hovered->isCategory())
            {
                // Moving before or after the hovered group.
                QRect r = visualRect(i);
                if (e->pos().y() < r.top() + r.height() / 2)
                {
                    dragdest = i;
                    dragpos = DragPos::Before;
                }
                else
                {
                    dragdest = i;
                    dragpos = DragPos::After;
                }
            }
            else
            {
                // Moving either after the last category, or into the hovered category.

                QRect r = visualRect(i);

                if (!isExpanded(i) && hovered == hovered->parentCategory()->categories(hovered->parentCategory()->categoryCount() - 1) &&
                    e->pos().y() >= r.bottom() - r.height() / 4)
                {
                    // If it's the last category and it's not expanded, moving after it is possible.
                    dragdest = i;
                    dragpos = DragPos::After;
                }
                else
                {
                    dragdest = i;
                    dragpos = DragPos::Inside;
                }
            }
        }

        if (dragdest != olddragdest || dragpos != olddragpos)
        {
            if (!indrect.isEmpty())
                viewport()->update(indrect);

            if (dragdest != nullptr && dragpos != DragPos::Inside)
            {
                indrect = _indicatorRect(dragdest, dragpos == DragPos::After);
                viewport()->update(indrect);
            }
            else
                indrect = QRect();

            if (dragpos == DragPos::Inside)
            {
                // Hovering over a category item which will be the destination of the drop action.
                setStyleSheet(QString("QTreeView::item:active:selected { background: transparent; color: %1; } QTreeView::item:!active:selected { background: transparent; color: %2; }").arg(Settings::textColor(true, ColorSettings::Text).name()).arg(Settings::textColor(false, ColorSettings::Text).name()));
                //setStyleSheet("QTreeView::item:selected { background: transparent; color: palette(text); }");
                Settings::updatePalette(this);
                viewport()->update(visualRect(dragdest));
            }
            else if (olddragpos == DragPos::Inside)
            {
                setStyleSheet(QString("QTreeView::item:!selected:hover { background: transparent; } QTreeView::item:active:selected { background: transparent; color: %1; } QTreeView::item:!active:selected { background: transparent; color: %2; }").arg(Settings::textColor(true, ColorSettings::Text).name()).arg(Settings::textColor(false, ColorSettings::Text).name()));
                //setStyleSheet("QTreeView::item:!selected:hover { background: transparent; } QTreeView::item:selected { background: transparent; color: palette(text); }");
                Settings::updatePalette(this);
                viewport()->update(visualRect(olddragdest));
            }
        }
    }
    else 
    {
        // Words or Kanji format.

        dragpos = DragPos::Inside;
        dragdest = i;

        if (olddragdest != nullptr)
            viewport()->update(visualRect(olddragdest));

        if (hovered == nullptr)
        {
            // Dragging kanji or words over the empty area is invalid.
            cachepos = nullptr;
            expandtimer.stop();
            e->ignore();
            return;
        }

        // Can't drop kanji and words into a category, only into groups.
        if (hovered->isCategory())
            e->ignore();

        // No use dropping back into the source group.

        const QMimeData *mdat = e->mimeData();
        const auto arr = mdat->data(format == Words ? "zkanji/words" : "zkanji/kanji").constData();
        if (hovered == (void*)(*((intptr_t*)arr + 1)))
        {
            cachepos = nullptr;
            expandtimer.stop();
            e->ignore();
            return;
        }
        viewport()->update(visualRect(dragdest));

        indrect = QRect();
    }

    // Not hovering over any item means no need for the expand category timer.
    if (i == nullptr)
    {
        cachepos = nullptr;
        expandtimer.stop();
        return;
    }

    // If the item is not an empty category, start a timer and open/close it if the cursor
    // stays on top longer than a set duration.
    if (!i->empty() && cachepos != i && hovered != grp && indrect.isEmpty())
    {
        cachepos = i;
        // Timeout is handled in timerEvent().
        expandtimer.start(1200, this);
    }
    else if (i->empty() || !indrect.isEmpty() || hovered == grp)
    {
        cachepos = nullptr;
        expandtimer.stop();
    }

    // It's possible to move over an item with the cursor which can't open. In that case
    // the value of cachepos is not updated, and when moved back to the original item, the
    // expandtimer won't start. Clear the cachepos for that case.
    if (cachepos != i)
        cachepos = nullptr;
}

void GroupTreeView::dropEvent(QDropEvent *e)
{
    enum DragFormats { Groups, Kanji, Words, Unknown };

    DragFormats format = e->mimeData()->hasFormat("zkanji/groups") ? Groups : e->mimeData()->hasFormat("zkanji/kanji") ? Kanji : e->mimeData()->hasFormat("zkanji/words") ? Words : Unknown;

    if (state == State::Dragging)
        expandtimer.stop();

    state = State::Idle;
    cachepos = nullptr;

    if (format == Groups)
    {
        if ((e->possibleActions() & Qt::MoveAction) == Qt::MoveAction && dragdest != nullptr && dragpos != DragPos::Undefined)
        {

            // Restoring style sheet to hover items again, that was disabled in dragEnterEvent.
            setStyleSheet("");
            Settings::updatePalette(this);

            int row = dragpos == DragPos::Inside ? -1 : dragpos == DragPos::Before ? dragdest->row() : dragdest->row() + 1;

            if (!model()->dropMimeData(e->mimeData(), Qt::MoveAction, row, dragpos == DragPos::Inside ? -1 : 0, dragpos == DragPos::Inside ? dragdest : dragdest->parent()))
                dragdest = nullptr;
        }

        // By accepting the e before calling the base implementation, the base won't try
        // to handle the drop action.
        if (e->dropAction() != Qt::MoveAction)
        {
            e->setDropAction(Qt::MoveAction);
            e->accept();
        }
        else
            e->acceptProposedAction();
        if (e->isAccepted() && dragdest != nullptr && dragpos == DragPos::Inside)
            setExpanded(dragdest, true);
    }
    else if (format == Kanji || format == Words)
    {
        dragdest = itemAt(e->pos());
        GroupBase *hovered = dragdest == nullptr ? nullptr : (GroupBase*)dragdest->userData();

        // Drop is not supported when the source is the same as the destination group.
        const QMimeData *mdat = e->mimeData();
        const auto arr = mdat->data(format == Words ? "zkanji/words" : "zkanji/kanji").constData();
        if (hovered == (void*)(*((intptr_t*)arr + 1)))
            hovered = nullptr;

        //if (hovered != nullptr &&
        //    ((format == Kanji && dynamic_cast<ZKanjiGridView*>(e->source()) != nullptr &&
        //    ((ZKanjiGridView*)e->source())->model()->kanjiGroup() == hovered) ||
        //    (format == Words && dynamic_cast<ZDictionaryListView*>(e->source()) != nullptr &&
        //    ((ZDictionaryListView*)e->source())->wordGroup() == hovered)))
        //    hovered = nullptr;

        if (hovered != nullptr && !hovered->isCategory())
        {
            setCurrentItem(dragdest);
            if (!model()->dropMimeData(e->mimeData(), e->dropAction(), -1, -1, dragdest))
                dragdest = nullptr;
        }


        if (hovered == nullptr || hovered->isCategory() || ((e->possibleActions() & Qt::MoveAction) != Qt::MoveAction && (e->possibleActions() & Qt::CopyAction) != Qt::CopyAction))
        {
            // By accepting the e before calling the base implementation, the base won't
            // try to handle the drop action.
            e->accept();
            dragdest = nullptr;
        }
    }

    // The base e is only used for stopping the built-in auto scrolling.
    base::dropEvent(e);

    if (dragdest == nullptr)
        e->ignore();

    dragdest = nullptr;
    dragpos = DragPos::Undefined;
}

void GroupTreeView::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (state != State::Dragging || indrect.isEmpty())
        return;

    // Painting over the default implementation while dragging, to show the drop position.
    QPainter p(viewport());
    QColor hicol = Settings::textColor(this, ColorSettings::SelBg);
    p.fillRect(indrect, hicol);
}


//-------------------------------------------------------------

