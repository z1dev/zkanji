/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QMimeData>
#include <QSet>
#include <stack>

#include "zgrouptreemodel.h"
#include "groups.h"
#include "zevents.h"
#include "zui.h"
#include "words.h"
#include "ztreeview.h"


//-------------------------------------------------------------


GroupTreeModel::GroupTreeModel(GroupCategoryBase *root, bool showplaceholder, bool onlycateg, QObject *parent) : base(parent), root(root), placeholder(showplaceholder), onlycateg(onlycateg), filtered(false), filterstr(QString()), addmode(NoAdd), editparent(nullptr), ignoresignals(false)
{
    init();
}

GroupTreeModel::GroupTreeModel(GroupCategoryBase *root, QString filterstr, QObject *parent) : base(parent), root(root), placeholder(true), onlycateg(false), filtered(!filterstr.isEmpty()), filterstr(filterstr), addmode(NoAdd), editparent(nullptr), ignoresignals(false)
{
    init();
}

GroupTreeModel::~GroupTreeModel()
{

}

void GroupTreeModel::init()
{
    connect(root, &GroupCategoryBase::categoryAdded, this, &GroupTreeModel::categoryAdded);
    connect(root, &GroupCategoryBase::groupAdded, this, &GroupTreeModel::groupAdded);
    connect(root, &GroupCategoryBase::categoryAboutToBeDeleted, this, &GroupTreeModel::categoryAboutToBeDeleted);
    connect(root, &GroupCategoryBase::categoryDeleted, this, &GroupTreeModel::categoryDeleted);
    connect(root, &GroupCategoryBase::groupAboutToBeDeleted, this, &GroupTreeModel::groupAboutToBeDeleted);
    connect(root, &GroupCategoryBase::groupDeleted, this, &GroupTreeModel::groupDeleted);
    connect(root, &GroupCategoryBase::groupsReseted, this, &GroupTreeModel::beginResetModel);
    connect(root, &GroupCategoryBase::groupsReseted, this, &GroupTreeModel::endResetModel);
    connect(root, &GroupCategoryBase::categoryMoved, this, &GroupTreeModel::categoryMoved);
    connect(root, &GroupCategoryBase::groupMoved, this, &GroupTreeModel::groupMoved);
    connect(root, &GroupCategoryBase::categoriesMoved, this, &GroupTreeModel::categoriesMoved);
    connect(root, &GroupCategoryBase::groupsMoved, this, &GroupTreeModel::groupsMoved);
    connect(root, &GroupCategoryBase::categoryRenamed, this, &GroupTreeModel::categoryRenamed);
    connect(root, &GroupCategoryBase::groupRenamed, this, &GroupTreeModel::groupRenamed);

    connect(root->dictionary(), &Dictionary::dictionaryReset, this, &GroupTreeModel::beginResetModel);
    connect(root->dictionary(), &Dictionary::dictionaryReset, this, &GroupTreeModel::endResetModel);

    if (!onlycateg && !filterstr.isEmpty())
    {
        // Go through every group and add them to filterlist if they match the filter string.

        // Holds the indexes of the currently checked categories in their parent.
        std::list<int> stack;

        GroupCategoryBase *pos = root;
        int cnt = root->categoryCount();
        for (int ix = 0; ix != cnt + 1; ++ix)
        {
            if (ix != cnt)
            {
                stack.push_front(ix);
                pos = pos->categories(ix);
                cnt = pos->categoryCount();
                ix = -1;
            }
            else
            {
                // At the end of the child categories, check the group names for match.
                int siz = pos->size();
                for (int iy = 0; iy != siz; ++iy)
                {
                    GroupBase *g = pos->items(iy);
                    if (!g->name().toLower().contains(filterstr))
                        continue;
                    filterlist.push_back(g);
                }

                if (stack.empty())
                    break;

                ix = stack.front();
                stack.pop_front();

                pos = pos->parentCategory();
                cnt = pos->categoryCount();
            }
        }

        std::sort(filterlist.begin(), filterlist.end(), [](const GroupBase *a, const GroupBase *b) {
            QString astr = a->name().toLower();
            QString bstr = b->name().toLower();
            if (astr == bstr)
            {
                astr = a->fullEncodedName().toLower();
                bstr = b->fullEncodedName().toLower();
            }
            return astr < bstr;
        });
    }

    reset();
}

int GroupTreeModel::rowCount(const TreeItem *parent) const
{
    // Size when group model is filtered depends on the filterlist only.
    if (filtered)
        return parent == nullptr ? filterlist.size() : 0;

    // parent has empty userData when it's a message item only for expanding empty categories,
    // or an empty item for editing the name of a new group or category.
    if (parent != nullptr && parent->userData() == 0)
        return 0;

    GroupCategoryBase* dat;
    if (parent == nullptr)
    {
        if (onlycateg)
            return 1;
        dat = root;
    }
    else if (isCategory(parent))
        dat = (GroupCategoryBase*)parent->userData();
    else
        return 0;

    // Empty categories return a message item. Categories with an item to be added return an
    // item to allow editing its name.
    return std::max(placeholder ? 1 : 0, dat->categoryCount() + (!onlycateg ? dat->size() : 0) + (addmode != NoAdd && editparent == parent ? 1 : 0));
}

QVariant GroupTreeModel::data(const TreeItem *item, int role) const
{
    if (item == nullptr)
        return QVariant();

    //if (role == Qt::DecorationRole)
    //{
    //    GroupBase *grp = (GroupBase*)item->userData();
    //    if ((grp != nullptr && grp->isCategory()) || (!grp && editparent == item->parent() && addmode == CategoryAdd))
    //        return imageFromSvg(":/box.svg", 14, 14, 0);
    //}

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        GroupBase *grp = (GroupBase*)item->userData();

        // Message items of empty categories have no user data.
        if (grp == nullptr && role != Qt::EditRole /*&& (addmode == NoAdd || editparent != item->parent())*/)
        {
            if (!onlycateg)
                return tr("Add group or category...");
            else
                return tr("Add new category...");
        }

        QString str = grp == root ? "root" : grp ? grp->name() : QString();
        if (role == Qt::DisplayRole && grp && grp->isCategory())
            str = QString("[%1]").arg(str);
        return str; ///*(role == Qt::DisplayRole && ((grp && grp->isCategory()) || (!grp && editparent == item->parent() && addmode == CategoryAdd)) ? "Cat: " : "") +*/ (grp ? grp->name() : QString());
    }

    return QVariant();
}

bool GroupTreeModel::setData(TreeItem *item, const QVariant &value, int role)
{
    if (filtered || role != Qt::EditRole)
        return false;

    QString str = value.toString().trimmed();
    if (str.isEmpty())
        return false;

    GroupBase *base = (GroupBase*)item->userData();
    if (onlycateg && base == root)
        return false;

    // Parent category of the edited item.
    GroupCategoryBase *cat = root;
    if (item->parent() != nullptr && item->parent()->userData() != 0)
        cat = (GroupCategoryBase*)item->parent()->userData();

    if ((base == nullptr || base->name().toLower() != str.toLower()) &&
        ((((base != nullptr && base->isCategory()) || (base == nullptr && addmode == CategoryAdd)) && cat->categoryNameTaken(str)) ||
        (((base != nullptr && !base->isCategory()) || (base == nullptr && addmode == GroupAdd)) && cat->groupNameTaken(str)))
        )
        return false;

#ifdef _DEBUG
    if (base == nullptr && addmode == NoAdd)
        throw "Only existing or added items can be edited.";
#endif

    // Name is only empty when a new group or category is being created.
    // If an appropriate name is set, remove the fake "add new ..." item.
    //if (item->parent() != nullptr && base->getName().isEmpty() && item->parent()->size() == 2 && item->parent()->items(1)->userData() == nullptr)
    //    qApp->postEvent(this, new TreeRemoveFakeItemEvent(item->parent()));

    if (base != nullptr)
        base->setName(str);
    else if (addmode == CategoryAdd)
    {
        // The user entered a name to a new category. Only a placeholder existed before, so
        // the category must be created here.

        addmode = NoAdd;
        editparent = nullptr;

        ignoresignals = true;
        GroupCategoryBase *pcat = cat->categories(cat->addCategory(str));
        ignoresignals = false;

        item->setUserData((intptr_t)pcat);
        // Add message item "add new..."
        if (pcat->categoryCount() + (onlycateg ? 0 : pcat->size()) == 0)
            qApp->postEvent(this, new TreeAddFakeItemEvent(item));
    }
    else
    {
        // The user entered a name to a new group. Only a placeholder existed before, so
        // the group must be created here.

        addmode = NoAdd;
        editparent = nullptr;

        ignoresignals = true;
        item->setUserData((intptr_t)cat->items(cat->addGroup(str)));
        ignoresignals = false;
    }

    emit dataChanged(item, item);
    return true;
}

intptr_t GroupTreeModel::treeRowData(int row, const TreeItem *parent) const
{
    if (filtered)
        return parent == nullptr ? (intptr_t)filterlist[row] : 0;

    GroupCategoryBase *cat;
    if (parent == nullptr)
    {
        if (onlycateg)
            return (intptr_t)root;
        cat = root;
    }
    else
        cat = (GroupCategoryBase*)parent->userData();

    if (row < cat->categoryCount())
        return (intptr_t)cat->categories(row);

    int groupcnt = (onlycateg ? 0 : cat->size());
    bool editrow = editparent == parent && ((addmode == CategoryAdd && row == cat->categoryCount()) || (addmode == GroupAdd && row == cat->categoryCount() + groupcnt));
    if (editrow || cat->categoryCount() + groupcnt == 0)
        return 0;

    return (intptr_t)cat->items(row - cat->categoryCount() - ((editrow && addmode == CategoryAdd) ? 1 : 0));
}

Qt::ItemFlags GroupTreeModel::flags(const TreeItem *item) const
{
    if (item == nullptr)
    {
        if (onlycateg)
            return 0;
        return Qt::ItemIsDropEnabled;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | (!filtered && ((item->userData() != 0 && item->userData() != (intptr_t)root) || (addmode != NoAdd && item->parent() == editparent)) ? Qt::ItemIsEditable : Qt::NoItemFlags) |
        (item->userData() != 0 ? Qt::ItemIsDropEnabled : Qt::NoItemFlags) | ((item->userData() != 0 && item->userData() != (intptr_t)root) ? Qt::ItemIsDragEnabled : Qt::NoItemFlags) |
        ((item->userData() == 0 || !((GroupBase*)item->userData())->isCategory()) ? Qt::ItemNeverHasChildren : Qt::NoItemFlags);
}

bool GroupTreeModel::hasChildren(const TreeItem *parent) const
{
    return parent == nullptr || (parent->userData() != 0 && ((GroupBase*)parent->userData())->isCategory() && (placeholder || 
        ((!onlycateg && !((GroupCategoryBase*)parent->userData())->isEmpty()) || ((GroupCategoryBase*)parent->userData())->categoryCount() != 0)
        ));
}

Qt::DropActions GroupTreeModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

QMimeData* GroupTreeModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;

    QMimeData *dat = new QMimeData();
    QByteArray arr;
    arr.resize(sizeof(intptr_t*) * indexes.size());
    char *arrdat = arr.data();

    int pos = 0;
    for (const QModelIndex &index : indexes)
    {
        const TreeItem *i = itemFromIndex(index);
        if (onlycateg && (GroupBase*)i->userData() == root)
        {
            arr.resize(arr.size() - sizeof(intptr_t*));
            continue;
        }
        *(intptr_t*)(arrdat + pos) = (intptr_t)(GroupBase*)i->userData();
        
        pos += sizeof(intptr_t*);
    }

    dat->setData("zkanji/groups", arr);
    return dat;
}

QStringList GroupTreeModel::mimeTypes() const
{
    QStringList types;
    types << "zkanji/groups";
    return types;
}

bool GroupTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, TreeItem *parent)
{
    if (data->hasFormat("zkanji/groups"))
    {
        // Dropping a kanji or word group. Only moves are accepted and only into categories.
        // Parent can be null when dropping in the root.
        if (action != Qt::MoveAction || (parent && !((GroupBase*)parent->userData())->isCategory()) || (onlycateg && parent == nullptr))
            return false;

        std::vector<GroupBase*> groups;
        std::vector<GroupCategoryBase*> cats;
        // The groups being dropped.
        int cnt = data->data("zkanji/groups").size() / sizeof(intptr_t);
        groups.reserve(cnt);
        cats.reserve(cnt);

        for (int ix = 0; ix != cnt; ++ix)
        {
            GroupBase *g = (GroupBase*)*(((intptr_t*)data->data("zkanji/groups").constData()) + ix);
            if (dynamic_cast<GroupCategoryBase*>(g))
                cats.push_back((GroupCategoryBase*)g);
            else
                groups.push_back(g);
        }

        if (groups.empty() && cats.empty())
            return false;

        // Where the group will be dropped.
        GroupCategoryBase *dest = parent == nullptr ? root : (GroupCategoryBase*)parent->userData();

        // Trying to drop into different dictionary.
        if (dest == nullptr || (!groups.empty() && groups.front()->dictionary() != dest->dictionary()) || (!cats.empty() && cats.front()->dictionary() != dest->dictionary()))
            return false;

        // A category is being dropped onto itself or into its child.
        for (int ix = 0, siz = cats.size(); ix != siz; ++ix)
        {
            // Check if dropping a parent into a child category.
            GroupBase *pos = dest;
            while (pos)
            {
                if (cats[ix] == pos)
                    return false;
                pos = pos->parentCategory();
            }
        }

        GroupCategoryBase *src = !groups.empty() ? groups.front()->parentCategory() : cats.front()->parentCategory();
        if (dest != src)
        {
            for (int ix = 0, siz = cats.size(); ix != siz; ++ix)
                if (dest->categoryNameTaken(cats[ix]->name()))
                    return false;
            for (int ix = 0, siz = groups.size(); ix != siz; ++ix)
                if (dest->groupNameTaken(groups[ix]->name()))
                    return false;
        }

        if (!cats.empty())
        {
            int pos = row;
            if (pos == -1)
                pos = dest->categoryCount();
            root->moveCategories(cats, dest, pos);
        }

        if (!groups.empty())
        {
            int pos = row;
            if (pos == -1)
                pos = dest->size();
            else
                pos = row - dest->categoryCount();
            root->moveGroups(groups, dest, pos);
        }

        return true;
    }
    else if (!onlycateg && data->hasFormat("zkanji/kanji"))
    {
        // Dropping dragged kanji into a group. Only valid to drop on kanji groups, not
        // categories or word groups. Copy and move are both acceptable. 
        if ((action != Qt::MoveAction && action != Qt::CopyAction) || groupType() != GroupTypes::Kanji || !parent || ((GroupBase*)parent->userData())->isCategory())
            return false;

        std::vector<ushort> indexes;
        QByteArray arr = data->data("zkanji/kanji");

        // Dropping between different dictionaries is not valid. - It is, for now.
        //if (group->dictionary() != (void*)(*(intptr_t*)arr.constData()))
        //    return false;

        // Dropping on the same group as the source, which is the second pointer sized value
        // in arr.
        if (parent->userData() == (*((intptr_t*)arr.constData() + 1)))
            return false;

        int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(ushort);
        if (((arr.size() - sizeof(intptr_t) * 2) % sizeof(ushort)) != 0)
            cnt = 0;
        indexes.reserve(cnt);

        const ushort *dat = (const ushort*)(arr.constData() + sizeof(intptr_t) * 2);
        while (cnt)
        {
            indexes.push_back(*dat);
            ++dat;
            --cnt;
        }

        if (!indexes.empty())
            ((KanjiGroup*)parent->userData())->add(indexes);
        else
            return false;
    }
    else if (!onlycateg && data->hasFormat("zkanji/words"))
    {
        // Dropping dragged words into a group. Only valid to drop on word groups, not
        // categories or kanji groups. Copy and move are both acceptable. 
        if ((action != Qt::MoveAction && action != Qt::CopyAction) || groupType() != GroupTypes::Words || !parent || ((GroupBase*)parent->userData())->isCategory())
            return false;

        std::vector<int> indexes;
        QByteArray arr = data->data("zkanji/words");

        // Dropping between different dictionaries is not valid.
        if (root->dictionary() != (void*)(*(intptr_t*)arr.constData()))
            return false;
        // Dropping on the same group as the source, which is the second pointer sized value in
        // arr.
        if (parent->userData() == (*((intptr_t*)arr.constData() + 1)))
            return false;

        int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(int);
        if (((arr.size() - sizeof(intptr_t) * 2) % sizeof(int)) != 0)
            cnt = 0;
        indexes.reserve(cnt);

        const int *dat = (const int*)(arr.constData() + sizeof(intptr_t) * 2);
        while (cnt)
        {
            indexes.push_back(*dat);
            ++dat;
            --cnt;
        }

        if (!indexes.empty())
            ((WordGroup*)parent->userData())->add(indexes);
        else
            return false;
    }

    return false;
}

Dictionary* GroupTreeModel::dictionary() const
{
    return root->dictionary();
}

GroupTypes GroupTreeModel::groupType() const
{
    return root->groupType();
}

bool GroupTreeModel::onlyCategories() const
{
    return onlycateg;
}

bool GroupTreeModel::isFiltered() const
{
    return filtered;
}

GroupCategoryBase* GroupTreeModel::rootCategory()
{
    return root;
}

const GroupCategoryBase* GroupTreeModel::rootCategory() const
{
    return root;
}

TreeItem* GroupTreeModel::startAddCategory(TreeItem *parent)
{
#ifdef _DEBUG
    if (filtered)
        throw "Cannot add category in a filtered model.";
#endif

    GroupCategoryBase *pcat;
    if (parent == nullptr)
        pcat = root;
    else
        pcat = (GroupCategoryBase*)parent->userData();

    beginInsertRows(parent, pcat->categoryCount(), pcat->categoryCount());
    editparent = parent;
    addmode = CategoryAdd;
    endInsertRows();

    // Remove the "add new..." message item.
    if (placeholder && pcat->categoryCount() + (onlycateg ? 0 : pcat->size()) == 0)
    {
        beginRemoveRows(parent, 1, 1);
        endRemoveRows();
    }

    return getItem(parent, pcat->categoryCount());
}

TreeItem* GroupTreeModel::startAddGroup(TreeItem *parent)
{
#ifdef _DEBUG
    if (filtered)
        throw "Cannot add group in a filtered model.";
#endif

    if (onlycateg)
        return nullptr;

    GroupCategoryBase *pcat;
    if (parent == nullptr)
        pcat = root;
    else
        pcat = (GroupCategoryBase*)parent->userData();

    beginInsertRows(parent, pcat->categoryCount() + pcat->size(), pcat->categoryCount() + pcat->size());
    editparent = parent;
    addmode = GroupAdd;
    endInsertRows();

    // Remove the "add new..." message item.
    if (placeholder && pcat->categoryCount() + pcat->size() == 0)
    {
        beginRemoveRows(parent, 1, 1);
        endRemoveRows();
    }

    return getItem(parent, pcat->categoryCount() + pcat->size());
}

TreeItem* GroupTreeModel::findTreeItem(const GroupBase *which)
{
    if (filtered)
    {
        auto it = std::find(filterlist.begin(), filterlist.end(), which);
        if (it == filterlist.end())
            return nullptr;
        return items(it - filterlist.begin());
    }

    std::stack<const GroupBase*> stack;

    while ((!onlycateg && which != root) || (onlycateg && which != nullptr))
    {
        stack.push(which);
        which = which->parentCategory();
    }

    TreeItem *top = nullptr;
    while (!stack.empty())
    {
        which = stack.top();
        stack.pop();

        int siz = getSize(top);
#ifdef _DEBUG
        bool found = false;
#endif
        for (int ix = 0; ix != siz; ++ix)
        {
            TreeItem *item = getItem(top, ix);
            if (item->userData() == (intptr_t)which)
            {
#ifdef _DEBUG
                found = true;
#endif
                top = item;
                break;
            }
        }
#ifdef _DEBUG
        if (!found)
            throw "Looking for item not in this tree.";
#endif
    }

    return top;
}

void GroupTreeModel::remove(TreeItem *which)
{
    if (filtered)
    {
#ifdef _DEBUG
        throw "Don't call this in filter mode";
#endif

        //GroupCategoryBase *pcat;

        //if (which->parent() == nullptr)
        //    pcat = root;
        //else
        //    pcat = (GroupCategoryBase*)which->parent()->userData();

        //ignoresignals = true;
        //beginRemoveRows(which->parent(), which->row(), which->row());
        //pcat->deleteGroup(pcat->groupIndex((GroupBase*)which->userData()));
        //filterlist.erase(filterlist.begin() + which->row());
        //endRemoveRows();
        //ignoresignals = false;
        return;
    }

    if (which && which->userData() == 0 && addmode != NoAdd && editparent == which->parent())
    {
        beginRemoveRows(editparent, which->row(), which->row());

        GroupCategoryBase *pcat = editparent != nullptr ? (GroupCategoryBase*)editparent->userData() : root;

        // The edited item was to be created as category or group, but the user cancelled. Add
        // the "add new..." message item.
        if (placeholder && pcat->categoryCount() + (onlycateg ? 0 : pcat->size()) == 0)
            qApp->postEvent(this, new TreeAddFakeItemEvent(editparent));

        editparent = nullptr;
        addmode = NoAdd;
        endRemoveRows();
        return;
    }

    if (which == nullptr || which->userData() == 0)
        return;

#ifdef _DEBUG
    throw "Don't call this to remove groups or categories, only for the fake item.";
#endif

    //GroupCategoryBase *pcat;

    //if (which->parent() == nullptr)
    //    pcat = root;
    //else
    //    pcat = (GroupCategoryBase*)which->parent()->userData();

    //ignoresignals = true;
    //beginRemoveRows(which->parent(), which->row(), which->row());
    //if (isCategory(which))
    //    pcat->deleteCategory(which->row());
    //else
    //    pcat->deleteGroup(which->row() - pcat->categoryCount());

    //// The 'which' variable becomes invalid after endRemoveRows, and we will need its parent.
    //which = which->parent();
    //endRemoveRows();
    //ignoresignals = false;

    //// The last item is removed from a parent category. Add the "add new..." message item.
    //if (pcat->categoryCount() + pcat->size() == 0)
    //    qApp->postEvent(this, new TreeAddFakeItemEvent(which));
}

//void GroupTreeModel::remove(const std::vector<TreeItem*> which)
//{
//    //if (filtered)
//    //{
//    //    GroupCategoryBase *pcat;
//
//    //    if (which->parent() == nullptr)
//    //        pcat = root;
//    //    else
//    //        pcat = (GroupCategoryBase*)which->parent()->userData();
//
//    //    ignoresignals = true;
//    //    beginRemoveRows(which->parent(), which->row(), which->row());
//    //    pcat->deleteGroup(pcat->groupIndex((GroupBase*)which->userData()));
//    //    filterlist.erase(filterlist.begin() + which->row());
//    //    endRemoveRows();
//    //    ignoresignals = false;
//    //    return;
//    //}
//
//    //if (which && which->userData() == nullptr && addmode != NoAdd && editparent == which->parent())
//    //{
//    //    beginRemoveRows(editparent, which->row(), which->row());
//
//    //    GroupCategoryBase *pcat = editparent != nullptr ? (GroupCategoryBase*)editparent->userData() : root;
//
//    //    // The edited item was to be created as category or group, but the user cancelled. Add
//    //    // the "add new..." message item.
//    //    if (pcat->categoryCount() + pcat->size() == 0)
//    //        qApp->postEvent(this, new TreeAddFakeItemEvent(editparent));
//
//    //    editparent = nullptr;
//    //    addmode = NoAdd;
//    //    endRemoveRows();
//    //    return;
//    //}
//
//    //if (which == nullptr || which->userData() == nullptr)
//    //    return;
//
//    //GroupCategoryBase *pcat;
//
//    //if (which->parent() == nullptr)
//    //    pcat = root;
//    //else
//    //    pcat = (GroupCategoryBase*)which->parent()->userData();
//
//    //ignoresignals = true;
//    //beginRemoveRows(which->parent(), which->row(), which->row());
//    //if (isCategory(which))
//    //    pcat->deleteCategory(which->row());
//    //else
//    //    pcat->deleteGroup(which->row() - pcat->categoryCount());
//
//    //// The 'which' variable becomes invalid after endRemoveRows, and we will need its parent.
//    //which = which->parent();
//    //endRemoveRows();
//    //ignoresignals = false;
//
//    //// The last item is removed from a parent category. Add the "add new..." message item.
//    //if (pcat->categoryCount() + pcat->size() == 0)
//    //    qApp->postEvent(this, new TreeAddFakeItemEvent(which));
//}

bool GroupTreeModel::isCategory(const TreeItem *which) const
{
    GroupBase *base = (GroupBase*)which->userData();
    return base->isCategory();
}

bool GroupTreeModel::event(QEvent *e)
{
    if (e->type() == TreeRemoveFakeItemEvent::Type())
    {
        // Currently not used.
        throw "";
        //TreeRemoveFakeItemEvent *event = (TreeRemoveFakeItemEvent*)e;
        //beginRemoveRows(event->itemParent(), 1, 1);
        //endRemoveRows();
        //return true;
    }
    else if (placeholder && e->type() == TreeAddFakeItemEvent::Type())
    {
        TreeAddFakeItemEvent *event = (TreeAddFakeItemEvent*)e;
        beginInsertRows(event->itemParent(), 0, 0);
        endInsertRows();
        return true;
    }

    return base::event(e);
}

void GroupTreeModel::categoryAdded(GroupCategoryBase *parent, int index)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *parentitem = findTreeItem(parent);

    beginInsertRows(parentitem, index, index);
    endInsertRows();

    // Remove the "add new..." message item if it was present in parent. The sum of the item
    // sizes is now 1 after an item was added.
    if (placeholder && parent->categoryCount() + (onlycateg ? 0 : parent->size()) == 1)
    {
        beginRemoveRows(parentitem, 1, 1);
        endRemoveRows();
    }
}

void GroupTreeModel::groupAdded(GroupCategoryBase *parent, int index)
{
    if (ignoresignals)
        return;

    if (filtered)
    {
        GroupBase *grp = parent->items(index);
        if (!grp->name().toLower().contains(filterstr))
            return;

        // Find the insert position of this group in the list.
        auto it = std::upper_bound(filterlist.begin(), filterlist.end(), grp, [](const GroupBase *a, const GroupBase *b) {
            QString astr = a->name().toLower();
            QString bstr = b->name().toLower();
            if (astr == bstr)
            {
                astr = a->fullEncodedName().toLower();
                bstr = b->fullEncodedName().toLower();
            }
            return astr < bstr;
        });

        index = it - filterlist.end();
        beginInsertRows(nullptr, index, index);
        filterlist.insert(filterlist.begin() + index, grp);
        endInsertRows();
        return;
    }

    TreeItem *parentitem = findTreeItem(parent);

    beginInsertRows(parentitem, parent->categoryCount() + index, parent->categoryCount() + index);
    endInsertRows();

    // Remove the "add new..." message item if it was present in parent. The sum of the item
    // sizes is now 1 after an item was added.
    if (placeholder && parent->categoryCount() + (onlycateg ? 0 : parent->size()) == 1)
    {
        beginRemoveRows(parentitem, 1, 1);
        endRemoveRows();
    }
}

void GroupTreeModel::categoryAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupCategoryBase *cat)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *parentitem = findTreeItem(parent);
    beginRemoveRows(parentitem, index, index);
}

void GroupTreeModel::categoryDeleted(GroupCategoryBase *parent, int index, void *oldptr)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *parentitem = findTreeItem(parent);

    endRemoveRows();

    // Message item for "add new..."
    if (placeholder && parent->categoryCount() + (onlycateg ? 0 : parent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(parentitem));
}

void GroupTreeModel::groupAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupBase *grp)
{
    if (ignoresignals)
        return;

    if (filtered)
    {
        auto it = std::find(filterlist.begin(), filterlist.end(), grp);
        if (it == filterlist.end())
            return;

        index = it - filterlist.begin();

        beginRemoveRows(nullptr, index, index);
        return;
    }

    if (parent == nullptr)
        return;

    TreeItem *parentitem = findTreeItem(parent);

    beginRemoveRows(parentitem, parent->categoryCount() + index, parent->categoryCount() + index);
}

void GroupTreeModel::groupDeleted(GroupCategoryBase *parent, int index, void *oldptr)
{
    if (ignoresignals)
        return;

    if (filtered)
    {
        auto it = std::find(filterlist.begin(), filterlist.end(), (GroupBase*)oldptr);
        if (it == filterlist.end())
            return;

        index = it - filterlist.begin();

        filterlist.erase(filterlist.begin() + index);
        endRemoveRows();
        return;
    }

    if (parent == nullptr)
        return;

    TreeItem *parentitem = findTreeItem(parent);

    endRemoveRows();

    // Message item for "add new..."
    if (placeholder && parent->categoryCount() + (onlycateg ? 0 : parent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(parentitem));
}

void GroupTreeModel::categoryMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *src = findTreeItem(srcparent);
    TreeItem *dest = findTreeItem(destparent);

    beginMoveRows(src, srcindex, srcindex, dest, destindex);
    endMoveRows();

    // Remove the "add new..." message item if it was present in destparent. The sum of the
    // item sizes is now 1 after an item was moved.
    if (placeholder && destparent != srcparent && destparent->categoryCount() + (onlycateg ? 0 : destparent->size()) == 1)
    {
        beginRemoveRows(dest, 1, 1);
        endRemoveRows();
    }

    // Message item for "add new..."
    if (placeholder && srcparent->categoryCount() + (onlycateg ? 0 : srcparent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(src));
}

void GroupTreeModel::groupMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *src = findTreeItem(srcparent);
    TreeItem *dest = findTreeItem(destparent);

    beginMoveRows(src, srcparent->categoryCount() + srcindex, srcparent->categoryCount() + srcindex, dest, destparent->categoryCount() + destindex);
    endMoveRows();

    // Remove the "add new..." message item if it was present in destparent. The sum of the
    // item sizes is now 1 after an item was moved.
    if (placeholder && destparent != srcparent && destparent->categoryCount() + (onlycateg ? 0 : destparent->size()) == 1)
    {
        beginRemoveRows(dest, 1, 1);
        endRemoveRows();
    }

    // Message item for "add new..."
    if (placeholder && srcparent->categoryCount() + (onlycateg ? 0 : srcparent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(src));
}

void GroupTreeModel::categoriesMoved(GroupCategoryBase *srcparent, const Range &r, GroupCategoryBase *destparent, int destindex)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *src = findTreeItem(srcparent);
    TreeItem *dest = findTreeItem(destparent);

    beginMoveRows(src, r.first, r.last, dest, destindex);
    endMoveRows();

    // Remove the "add new..." message item if it was present in destparent. The sum of the
    // item sizes is now (r.last - r.first + 1) after an item was moved.
    int cnt = (r.last - r.first + 1);
    if (placeholder && destparent != srcparent && destparent->categoryCount() + (onlycateg ? 0 : destparent->size()) == cnt)
    {
        beginRemoveRows(dest, cnt, cnt);
        endRemoveRows();
    }

    // Message item for "add new..."
    if (placeholder && srcparent->categoryCount() + (onlycateg ? 0 : srcparent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(src));
}

void GroupTreeModel::groupsMoved(GroupCategoryBase *srcparent, const Range &r, GroupCategoryBase *destparent, int destindex)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *src = findTreeItem(srcparent);
    TreeItem *dest = findTreeItem(destparent);

    beginMoveRows(src, srcparent->categoryCount() + r.first, srcparent->categoryCount() + r.last, dest, destparent->categoryCount() + destindex);
    endMoveRows();

    // Remove the "add new..." message item if it was present in destparent. The sum of the
    // item sizes is now (r.last - r.first + 1) after an item was moved.
    int cnt = (r.last - r.first + 1);
    if (placeholder && destparent != srcparent && destparent->categoryCount() + (onlycateg ? 0 : destparent->size()) == cnt)
    {
        beginRemoveRows(dest, cnt, cnt);
        endRemoveRows();
    }

    // Message item for "add new..."
    if (placeholder && srcparent->categoryCount() + (onlycateg ? 0 : srcparent->size()) == 0)
        qApp->postEvent(this, new TreeAddFakeItemEvent(src));
}

void GroupTreeModel::categoryRenamed(GroupCategoryBase *parent, int index)
{
    if (filtered || ignoresignals)
        return;

    TreeItem *item = findTreeItem(parent->categories(index));
    emit dataChanged(item, item, { Qt::DisplayRole });
}

void GroupTreeModel::groupRenamed(GroupCategoryBase *parent, int index)
{
    if (ignoresignals)
        return;

    if (filtered)
    {
        GroupBase *grp = parent->items(index);
        auto it = std::find(filterlist.begin(), filterlist.end(), grp);
        if (it == filterlist.end())
        {
            // The group is not listed. If its name matches the filter, it should be included.
            // Otherwise ignore it.

            if (!grp->name().toLower().contains(filterstr))
                return;

            // Find the insert position of this group in the list.
            auto it = std::upper_bound(filterlist.begin(), filterlist.end(), grp, [](const GroupBase *a, const GroupBase *b) {
                QString astr = a->name().toLower();
                QString bstr = b->name().toLower();
                if (astr == bstr)
                {
                    astr = a->fullEncodedName().toLower();
                    bstr = b->fullEncodedName().toLower();
                }
                return astr < bstr;
            });

            index = it - filterlist.begin();
            beginInsertRows(nullptr, index, index);
            filterlist.insert(filterlist.begin() + index, grp);
            endInsertRows();
            return;
        }

        if (!grp->name().toLower().contains(filterstr))
        {
            // The group has been renamed and doesn't match the filter string. It shouldn't be
            // listed any more.

            index = it - filterlist.begin();

            beginRemoveRows(nullptr, index, index);
            filterlist.erase(filterlist.begin() + index);
            endRemoveRows();

            return;
        }

        // Old index of item.
        index = it - filterlist.begin();

        // The item has been renamed but still matches the filter string. It might have to be
        // moved to a different index.

        filterlist.erase(filterlist.begin() + index);
        it = std::upper_bound(filterlist.begin(), filterlist.end(), grp, [](const GroupBase *a, const GroupBase *b) {
            QString astr = a->name().toLower();
            QString bstr = b->name().toLower();
            if (astr == bstr)
            {
                astr = a->fullEncodedName().toLower();
                bstr = b->fullEncodedName().toLower();
            }
            return astr < bstr;
        });

        // The index where the item would be inserted to.
        int newindex = it - filterlist.begin();

        filterlist.insert(filterlist.begin() + index, grp);

        if (newindex == index)
        {
            // There is no need to move the item. Notify the views about the name change.
            TreeItem *item = items(index);
            emit dataChanged(item, item, { Qt::DisplayRole });
            return;
        }

        beginMoveRows(nullptr, index, index, nullptr, newindex + (newindex >= index ? 1 : 0));
        filterlist.erase(filterlist.begin() + index);
        filterlist.insert(filterlist.begin() + newindex, grp);
        endMoveRows();

        return;
    }

    TreeItem *item = findTreeItem(parent->items(index));

    emit dataChanged(item, item, { Qt::DisplayRole });
}


//-------------------------------------------------------------


CheckedGroupTreeModel::CheckedGroupTreeModel(GroupCategoryBase *root, bool showplaceholder, bool onlycateg, QObject *parent) : base(root, showplaceholder, onlycateg, parent), checkcateg(false)
{
}

CheckedGroupTreeModel::CheckedGroupTreeModel(GroupCategoryBase *root, QString filterstr, QObject *parent) : base(root, filterstr, parent), checkcateg(false)
{

}

CheckedGroupTreeModel::~CheckedGroupTreeModel()
{

}

bool CheckedGroupTreeModel::hasChecked() const
{
    return !checked.empty();
}

void CheckedGroupTreeModel::checkedGroups(QSet<GroupBase*> &result) const
{
    result = checked;
}

QVariant CheckedGroupTreeModel::data(const TreeItem *item, int role) const
{
    if (role == Qt::CheckStateRole)
    {
        GroupBase *grp = (GroupBase*)item->userData();
        if (grp == nullptr)
            return QVariant();
        if (!grp->isCategory() || checkcateg)
            return checked.contains(grp) ? Qt::Checked : Qt::Unchecked;

        // See if any sub group is checked in the given category.
        GroupCategoryBase *cat = (GroupCategoryBase*)grp;
        bool hascheck = false;
        bool hasuncheck = false;
        for (int ix = 0, siz = cat->size(); ix != siz && (!hascheck || !hasuncheck); ++ix)
        {
            if (checked.contains(cat->items(ix)))
                hascheck = true;
            else
                hasuncheck = true;
        }

        if (hascheck && hasuncheck)
            return Qt::PartiallyChecked;

        for (int ix = 0, siz = cat->categoryCount(); ix != siz && (!hascheck || !hasuncheck); ++ix)
        {
            Qt::CheckState cs = data(item->items(ix), Qt::CheckStateRole).value<Qt::CheckState>();
            if (cs == Qt::PartiallyChecked)
                return cs;
            if (cs == Qt::Checked)
                hascheck = true;
            if (cs == Qt::Unchecked)
                hasuncheck = true;
        }

        if (hascheck && hasuncheck)
            return Qt::PartiallyChecked;
        return hascheck ? Qt::Checked : Qt::Unchecked;
    }

    return base::data(item, role);
}

bool CheckedGroupTreeModel::setData(TreeItem *item, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole)
    {
        ZTreeView *tree = dynamic_cast<ZTreeView*>(qApp->focusWidget());

        GroupBase *grp = (GroupBase*)item->userData();
        if (grp == nullptr)
            return false;

        QSet<TreeItem*> list;
        if (tree != nullptr && tree->isSelected(item))
            tree->selection(list);
        else
            list.insert(item);

        // Collecting every item in list that should be checked/unchecked. Those are added
        // that are in a checked category.

        QSet<TreeItem*> tmp = list;
        for (TreeItem *i : tmp)
        {
            if (i->userData() == 0)
                continue;
            if (((GroupBase*)i->userData())->isCategory())
                collectCategoryChildren(i, list);
            list.insert(i);
        }
        QSet<TreeItem*> parents;

        for (TreeItem *i : list)
        {
            GroupBase *grp = (GroupBase*)i->userData();
            if (grp == nullptr || (!checkcateg && grp->isCategory()))
                continue;

            bool check = checked.contains(grp);
            if ((value.toInt() == Qt::Checked) == check)
                continue;

            if (check)
                checked.remove(grp);
            else
                checked.insert(grp);
            emit dataChanged(i, i);

            while (i->parent() != nullptr)
            {
                i = i->parent();
                parents.insert(i);
            }
        }

        for (TreeItem *p : parents)
            emit dataChanged(p, p);

        return true;
    }

    return base::setData(item, value, role);
}

Qt::ItemFlags CheckedGroupTreeModel::flags(const TreeItem *item) const
{
    return base::flags(item) | (item->userData() != 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags);
}

void CheckedGroupTreeModel::categoryAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupCategoryBase *cat)
{
    if (checkcateg)
        checked.remove(cat);
}

void CheckedGroupTreeModel::groupAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupBase *grp)
{
    checked.remove(grp);
}

void CheckedGroupTreeModel::collectCategoryChildren(TreeItem *parent, QSet<TreeItem*> &dest)
{
    if (parent->empty())
        return;
    
    for (int ix = 0, siz = parent->size(); ix != siz; ++ix)
    {
        TreeItem *item = parent->items(ix);
        dest.insert(item);
        GroupBase *grp = (GroupBase*)item->userData();
        if (grp != nullptr && grp->isCategory())
            collectCategoryChildren(item, dest);
    }
}


//-------------------------------------------------------------

