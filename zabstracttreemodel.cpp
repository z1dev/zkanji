/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <stack>
#include "zabstracttreemodel.h"

#include "checked_cast.h"

//-------------------------------------------------------------


TreeItem::TreeItem() : oldrow(-1), pr(nullptr), dat(0)
{

}

TreeItem::TreeItem(int newrow, TreeItem *itemparent, intptr_t itemdata) : oldrow(newrow), pr(itemparent), dat(itemdata)
{

}

TreeItem::TreeItem(TreeItem &&other) : oldrow(-1), pr(nullptr), dat(0)
{
    *this = std::move(other);
}

TreeItem& TreeItem::operator=(TreeItem &&other)
{
    std::swap(oldrow, other.oldrow);
    std::swap(pr, other.pr);
    std::swap(dat, other.dat);
    std::swap(list, other.list);
    return *this;
}

TreeItem::~TreeItem()
{
    clear();
}

bool TreeItem::empty() const
{
    return list.empty();
}

TreeItem::size_type TreeItem::size() const
{
    return list.size();
}

int TreeItem::row() const
{
    if (pr == nullptr)
        return 0;

    if (oldrow < tosigned(pr->size()) && pr->list[oldrow] == this)
        return oldrow;

    return (oldrow = pr->indexOf(this));
}

int TreeItem::indexOf(const TreeItem* item) const
{
    auto it = std::find(list.begin(), list.end(), item);
    if (it == list.end())
        return -1;
    return it - list.begin();
}

TreeItem* TreeItem::parent() const
{
    if (inRoot())
        return nullptr;
    return pr;
}

bool TreeItem::hasParent(TreeItem *item) const
{
    if (item == this)
        return false;

    const TreeItem *i = this;
    while (i->pr && i->pr != item)
        i = i->pr;

    return i->pr == item;
}

TreeItem* TreeItem::realParent() const
{
    return pr;
}

const TreeItem* TreeItem::items(int index) const
{
#ifdef _DEBUG
    if (index < 0 || index >= tosigned(list.size()))
        throw "Item index out of bounds.";
#endif
    return list[index];
}

TreeItem* TreeItem::items(int index)
{
#ifdef _DEBUG
    if (index < 0 || index >= tosigned(list.size()))
        throw "Item index out of bounds.";
#endif
    return list[index];
}

intptr_t TreeItem::userData() const
{
    return dat;
}

void TreeItem::setUserData(intptr_t data)
{
    dat = data;
}

bool TreeItem::inRoot() const
{
    return pr == nullptr || pr->pr == nullptr;
}

void TreeItem::clear()
{
    for (TreeItem *item : list)
        delete item;

    // Clears list by swapping it with a temporary that gets destroyed.
    std::vector<TreeItem*>().swap(list);
}

void TreeItem::reserve(int cnt)
{
    list.reserve(cnt);
}

int TreeItem::capacity()
{
    return tosigned(list.capacity());
}

void TreeItem::removeItems(int first, int cnt)
{
#ifdef _DEBUG
    if (first < 0 || cnt < 0 || first >= tosigned(list.size()) || first + cnt > tosigned(list.size()))
        throw "Passed range is invalid.";
#endif

    for (int ix = 0; ix != cnt; ++ix)
        delete list[first + ix];

    list.erase(list.begin() + first, list.begin() + first + cnt);
}

void TreeItem::takeItems(int first, int cnt, std::vector<TreeItem*> &dest)
{
    dest.insert(dest.begin(), list.begin() + first, list.begin() + first + cnt);
    list.erase(list.begin() + first, list.begin() + first + cnt);
}

TreeItem* TreeItem::addChild(intptr_t childdata)
{
    TreeItem *i = new TreeItem(tosigned(list.size()), this, childdata);
    list.push_back(i);
    return i;
}

void TreeItem::insertItems(int pos, const std::vector<TreeItem*> &items, bool updateparent)
{
    if (updateparent)
        for (TreeItem *item : items)
            item->pr = this;
    list.insert(list.begin() + pos, items.begin(), items.end());
}


//-------------------------------------------------------------


ZAbstractTreeModel::ZAbstractTreeModel(QObject * parent) : base(parent), rowparent(nullptr), rowfirst(-1), rowlast(-1)
#ifdef _DEBUG
    , op(NoOp)
#endif
{
    connect(this, SIGNAL(dataChanged(const TreeItem*, const TreeItem*, const QVector<int>&)), this, SLOT(forwardDataChanged(const TreeItem*, const TreeItem*, const QVector<int>&)));
}

void ZAbstractTreeModel::reset()
{
    beginResetModel();
    endResetModel();
}

TreeItem* ZAbstractTreeModel::items(int index)
{
    return root.items(index);
}

const TreeItem* ZAbstractTreeModel::items(int index) const
{
    return root.items(index);
}

TreeItem* ZAbstractTreeModel::getItem(TreeItem *parent, int index)
{
    if (parent == nullptr)
        return items(index);
    return parent->items(index);
}

const TreeItem* ZAbstractTreeModel::getItem(const TreeItem *parent, int index) const
{
    if (parent == nullptr)
        return items(index);
    return parent->items(index);
}

ZAbstractTreeModel::size_type ZAbstractTreeModel::size() const
{
    return root.size();
}

ZAbstractTreeModel::size_type ZAbstractTreeModel::getSize(const TreeItem *parent) const
{
    if (parent == nullptr)
        return size();
    return parent->size();
}

bool ZAbstractTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, TreeItem* parent)
{
    return base::dropMimeData(data, action, row, column, index(parent));
}

int ZAbstractTreeModel::columnCount(const TreeItem * /*parent*/) const
{
    return 1;
}

Qt::ItemFlags ZAbstractTreeModel::flags(const TreeItem *item) const
{
    if (item == nullptr)
        return 0;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool ZAbstractTreeModel::hasChildren(const TreeItem *parent) const
{
    return base::hasChildren(index(parent));
}

bool ZAbstractTreeModel::setData(TreeItem * /*item*/, const QVariant &/*value*/, int /*role*/)
{
    return false;
}

void ZAbstractTreeModel::beginResetModel()
{
#ifdef _DEBUG
    if (op != NoOp)
        throw "An operation is already in progress.";
    op = Resetting;
#endif
    base::beginResetModel();
    root.clear();
}

void ZAbstractTreeModel::fillItemRange(TreeItem *parent, int insertpos, int cnt)
{
    if (!cnt)
        return;

    std::vector<TreeItem*> list;

    std::stack<std::pair<int, TreeItem*>> stack;

    int itemcnt;

    // Item being iterated.
    TreeItem *parentitem = parent;
    bool isroot = parent == &root;
    // Item created at each step when iterating.
    TreeItem *item = nullptr;

    int pos = insertpos;
    list.reserve(cnt);
    cnt += insertpos;
    while (pos != cnt)
    {
        while (pos != cnt)
        {
            item = new TreeItem(pos, parentitem, treeRowData(pos, isroot ? nullptr : parentitem));

            list.push_back(item);
            itemcnt = rowCount(item);

            if (itemcnt != 0)
            {
                item->reserve(itemcnt);
                stack.emplace(itemcnt, item);
            }

            ++pos;
        }
        parentitem->insertItems(insertpos, list, false);

        if (stack.empty())
            break;

        auto tpl = stack.top();
        cnt = tpl.first;
        parentitem = tpl.second;
        isroot = false;

        pos = 0;
        insertpos = 0;
        stack.pop();
        list.clear();
        list.reserve(cnt);
    }
}

void ZAbstractTreeModel::endResetModel()
{
#ifdef _DEBUG
    if (op != Resetting)
        throw "Resetting not in progress.";
#endif

    fillItemRange(&root, 0, rowCount(nullptr));
    base::endResetModel();

#ifdef _DEBUG
    op = NoOp;
#endif
}

void ZAbstractTreeModel::beginInsertRows(TreeItem *parent, int first, int last)
{
#ifdef _DEBUG
    if (op != NoOp)
        throw "An operation is already in progress.";
    op = Insertion;
#endif
    rowparent = parent;
    if (parent == nullptr)
        rowparent = &root;

    QModelIndex parentindex;
    if (parent != nullptr)
        parentindex = createIndex(parent->row(), 0, (void*)parent);
    base::beginInsertRows(parentindex, first, last);

    rowfirst = first;
    rowlast = last;
}

void ZAbstractTreeModel::endInsertRows()
{
#ifdef _DEBUG
    if (op != Insertion)
        throw "Insertion not in progress.";
#endif

    fillItemRange(rowparent, rowfirst, rowlast - rowfirst + 1);
    base::endInsertRows();

#ifdef _DEBUG
    op = NoOp;
#endif
}

void ZAbstractTreeModel::beginRemoveRows(TreeItem *parent, int first, int last)
{
#ifdef _DEBUG
    if (op != NoOp)
        throw "An operation is already in progress.";
    op = Removal;
#endif
    QModelIndex parentindex;
    if (parent != nullptr)
        parentindex = createIndex(parent->row(), 0, (void*)parent);
    base::beginRemoveRows(parentindex, first, last);
    if (parent == nullptr)
        parent = &root;

    rowfirst = first;
    rowlast = last;
    rowparent = parent;
}

void ZAbstractTreeModel::endRemoveRows()
{
#ifdef _DEBUG
    if (op != Removal)
        throw "Removal not in progress.";
#endif
    rowparent->removeItems(rowfirst, rowlast - rowfirst + 1);
    base::endRemoveRows();
#ifdef _DEBUG
    op = NoOp;
#endif
}

void ZAbstractTreeModel::beginMoveRows(TreeItem *parent, int first, int last, TreeItem *destparent, int destindex)
{
#ifdef _DEBUG
    if (op != NoOp)
        throw "An operation is already in progress.";
    op = Moving;

#endif
    if (parent == destparent && destindex >= first && destindex <= last + 1)
    {
        destrowindex = -1;
        return;
    }

    QModelIndex parentindex;
    if (parent != nullptr)
        parentindex = createIndex(parent->row(), 0, (void*)parent);
    QModelIndex destparentindex;
    if (destparent != nullptr)
        destparentindex = createIndex(destparent->row(), 0, (void*)destparent);

    base::beginMoveRows(parentindex, first, last, destparentindex, destindex);
    if (parent == nullptr)
        parent = &root;
    if (destparent == nullptr)
        destparent = &root;

    rowparent = parent;
    rowfirst = first;
    rowlast = last;
    destrowparent = destparent;
    destrowindex = destindex;
}

void ZAbstractTreeModel::endMoveRows()
{
#ifdef _DEBUG
    if (op != Moving)
        throw "Move not in progress.";
#endif
    if (destrowindex == -1)
        return;

    std::vector<TreeItem*> items;
    rowparent->takeItems(rowfirst, rowlast - rowfirst + 1, items);
    if (destrowparent == rowparent && destrowindex > rowlast)
        destrowindex -= rowlast - rowfirst + 1;
    destrowparent->insertItems(destrowindex, items, true);
    base::endMoveRows();
#ifdef _DEBUG
    op = NoOp;
#endif
}

QModelIndex ZAbstractTreeModel::index(const TreeItem *item) const
{
    if (item == nullptr)
        return QModelIndex();

    return createIndex(item->row(), 0, (quintptr)item);
}

const TreeItem* ZAbstractTreeModel::itemFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return &root;
    return ((TreeItem*)index.internalPointer());
}

TreeItem* ZAbstractTreeModel::itemFromIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return &root;
    return ((TreeItem*)index.internalPointer());
}

QModelIndex ZAbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (root.empty() || row < 0 || row >= tosigned(itemFromIndex(parent)->size()))
        return QModelIndex();

    return createIndex(row, column, (void*)(itemFromIndex(parent)->items(row)));
}

QModelIndex ZAbstractTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();
    TreeItem *p = ((TreeItem*)index.internalPointer())->parent();
    if (p == nullptr)
        return QModelIndex();
    return createIndex(p->row(), 0, p);
}

int ZAbstractTreeModel::rowCount(const QModelIndex &parent) const
{
    return tosigned(itemFromIndex(parent)->size());
}

int ZAbstractTreeModel::columnCount(const QModelIndex &parent) const
{
    const TreeItem *p = itemFromIndex(parent);
    if (p == &root)
        p = nullptr;
    return columnCount(p);
}

QVariant ZAbstractTreeModel::data(const QModelIndex &index, int role) const
{
    const TreeItem *p = itemFromIndex(index);
    return data(p, role);
}

Qt::ItemFlags ZAbstractTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return flags(itemFromIndex(index));
}

bool ZAbstractTreeModel::hasChildren(const QModelIndex &parent) const
{
    const TreeItem *p = itemFromIndex(parent);
    if (p == &root)
        p = nullptr;
    return hasChildren(p);
}

bool ZAbstractTreeModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid())
        return false;

    return setData(itemFromIndex(index), value, role);
}

void ZAbstractTreeModel::forwardDataChanged(const TreeItem *top, const TreeItem *bottom, const QVector<int> &roles)
{
    emit dataChanged(createIndex(top->row(), 0, (void*)top), createIndex(bottom->row(), 0, (void*)bottom), roles);
}


//-------------------------------------------------------------


