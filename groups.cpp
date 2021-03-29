/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>
#include <QSet>
#include <QStringBuilder>
#include <stack>

#include "zkanjimain.h"
#include "groups.h"
#include "words.h"
#include "ranges.h"
#include "kanji.h"
#include "groupstudy.h"
#include "grammar_enums.h"
#include "zdictionarymodel.h"
#include "zkanjigridmodel.h"

#include "checked_cast.h"


//-------------------------------------------------------------


GroupBase::GroupBase(GroupCategoryBase *parent) : parent(parent)
{

}

GroupBase::GroupBase(GroupCategoryBase *parent, QString name) : parent(parent), _name(name)
{

}

GroupBase::GroupBase(GroupBase &&src) : parent(nullptr)
{
    *this = std::move(src);
}

GroupBase& GroupBase::operator=(GroupBase &&src)
{
    std::swap(parent, src.parent);
    std::swap(_name, src._name);
    return *this;
}

GroupBase::~GroupBase()
{

}

void GroupBase::load(QDataStream &stream)
{
    stream >> make_zstr(_name, ZStrFormat::Byte);
}

void GroupBase::save(QDataStream &stream) const
{
    // TODO: don't allow group names longer than 255 anywhere in the UI. (Saving is safe because make_zstr trims it.)
    stream << make_zstr(_name, ZStrFormat::Byte);
}

void GroupBase::copy(GroupBase *src)
{
    _name = src->_name;
}

bool GroupBase::isCategory() const
{
    return false;
}

//Dictionary* GroupBase::dictionary()
//{
//    return owner->dictionary();
//}
//
//const Dictionary* GroupBase::dictionary() const
//{
//    return owner->dictionary();
//}

GroupCategoryBase* GroupBase::parentCategory()
{
    return parent;
}

const GroupCategoryBase* GroupBase::parentCategory() const
{
    return parent;
}

const QString& GroupBase::name() const
{
    return _name;
}

void GroupBase::setName(QString newname)
{
    // TODO: Check the validity of the passed name everywhere where this is called. Replace
    // the TAB character with space. Duplicate names are not allowed either. Figure out
    // whether the / character or some other should be forbidden and used as path separator.

    _name = newname;
    dictionary()->setToUserModified();
}

const QString GroupBase::fullEncodedName() const
{
    QString str;
    if (parent)
    {
        str = parent->fullEncodedName();
        if (!str.isEmpty())
            str += QChar('/');
    }

    // Replace characters like TAB, / and ^ in the names with ^ as escape character.
    QString encname;
    int from = 0;
    int to = 0;
    int len = _name.size();

    QString enc("^ ");
    while (to != len)
    {
        ushort ch = _name.at(to).unicode();
        if (ch == '/' || ch == '^' || ch == '[')
        {
            // Add anything between from and to - 1 into encname, and the last character
            // escaped with ^.

            if (from < to)
                encname += _name.mid(from, to - from);
            enc[1] = QChar(ch);
            encname += enc;

            from = to + 1;
            to = from - 1;
        }
        ++to;
    }
    if (from < to)
        encname += _name.mid(from, to - from);

    return str + encname;
}

const QString GroupBase::fullName(GroupCategoryBase *root) const
{
    QString str;
    if (parent && parent != root)
    {
        str = parent->fullName(root);
        if (!str.isEmpty())
            str += QChar('/');
    }

    return str + _name;
}

//Groups* GroupBase::getOwner()
//{
//#ifdef _DEBUG
//    if (!parent)
//        throw "no parent.";
//#endif
//    return parent->getOwner();
//}
//
//const Groups* GroupBase::getOwner() const
//{
//#ifdef _DEBUG
//    if (!parent)
//        throw "no parent.";
//#endif
//    return parent->getOwner();
//}

//GroupCategoryBase* GroupBase::topParent()
//{
//    GroupBase *p = this;
//    while (p->parent)
//        p = p->parent;
//    return p->parent;
//}
//
//const GroupCategoryBase* GroupBase::topParent() const
//{
//    const GroupBase *p = this;
//    while (p->parent)
//        p = p->parent;
//    return p->parent;
//}


//-------------------------------------------------------------


//GroupCategoryBase::GroupCategoryBase(GroupCategoryBase *parent) : base(parent, QString()), owner(parent ? parent->owner : nullptr)
//{
//    if (owner == nullptr)
//        owner = this;
//}

GroupCategoryBase::GroupCategoryBase(GroupCategoryBase *parent, QString name) : base(parent, name), owner(parent ? parent->owner : nullptr)
{
    if (owner == nullptr)
        owner = this;
}

GroupCategoryBase::GroupCategoryBase(GroupCategoryBase &&src) : base(nullptr)
{
    *this = std::forward<GroupCategoryBase>(src);
}

GroupCategoryBase& GroupCategoryBase::operator=(GroupCategoryBase &&src)
{
    base::operator=(std::forward<GroupCategoryBase>(src));
    std::swap(list, src.list);
    std::swap(groups, src.groups);
    std::swap(owner, src.owner);
    return *this;
}

GroupCategoryBase::~GroupCategoryBase()
{
}

void GroupCategoryBase::load(QDataStream &stream)
{
    clear();

    base::load(stream);

    // Reading categories.
    qint32 i;
    stream >> i;
    list.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(createCategory(QString()));
        list.back()->load(stream);
    }

    // Reading groups.
    stream >> i;
    groups.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        groups.push_back(createGroup(QString()));
        groups[ix]->load(stream);
    }

}

void GroupCategoryBase::save(QDataStream &stream) const
{
    base::save(stream);

    // Saving categories.
    stream << (qint32)list.size();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        list[ix]->save(stream);

    // Saving groups;
    stream << (qint32)groups.size();
    for (int ix = 0, siz = tosigned(groups.size()); ix != siz; ++ix)
        groups[ix]->save(stream);
}

bool GroupCategoryBase::isEmpty() const
{
    return groups.empty() && list.empty();
}

bool GroupCategoryBase::isCategory() const
{
    return true;
}

Dictionary* GroupCategoryBase::dictionary()
{
    return owner->dictionary();
}

const Dictionary* GroupCategoryBase::dictionary() const
{
    return owner->dictionary();
}

void GroupCategoryBase::copy(GroupCategoryBase *src)
{
    clear();

    base::copy(src);

    list.reserve(src->list.size());
    groups.reserve(src->groups.size());

    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        list.push_back(createCategory(src->list[ix]->name()));
        list.back()->copy(src->list[ix]);
    }

    for (int ix = 0, siz = tosigned(src->groups.size()); ix != siz; ++ix)
    {
        groups.push_back(createGroup(src->groups[ix]->name()));
        groups.back()->copy(src->groups[ix]);
    }
}

void GroupCategoryBase::clear()
{
    list.clear();
    groups.clear();

    //while (!list.empty())
    //    deleteCategory(list.size() - 1);
    //while (!groups.empty())
    //    deleteGroup(groups.size() - 1);
}

int GroupCategoryBase::addCategory(QString name)
{
    list.push_back(createCategory(name));

    dictionary()->setToUserModified();
    emit owner->categoryAdded(this, tosigned(list.size()) - 1);
    return tosigned(list.size()) - 1;
}

int GroupCategoryBase::addGroup(QString name)
{
    groups.push_back(createGroup(name));

    dictionary()->setToUserModified();
    emit owner->groupAdded(this, tosigned(groups.size()) - 1);

    return tosigned(groups.size()) - 1;
}

void GroupCategoryBase::deleteCategory(int index)
{
#ifdef _DEBUG
    if (index < 0 || index >= tosigned(list.size()))
        throw "Invalid category index.";
#endif

    list[index]->emitDeleted();
    void *oldptr = list[index];

    emit owner->categoryAboutToBeDeleted(this, index, list[index]);

    list.erase(list.begin() + index);
    dictionary()->setToUserModified();

    emit owner->categoryDeleted(this, index, oldptr);
}

//void GroupCategoryBase::deleteCategory(GroupCategoryBase *child)
//{
//    deleteCategory(categoryIndex(child));
//}

void GroupCategoryBase::deleteGroup(int index)
{
#ifdef _DEBUG
    if (index < 0 || index >= tosigned(groups.size()))
        throw "Invalid category index.";
#endif

    void *oldptr = groups[index];

    emit owner->groupAboutToBeDeleted(this, index, groups[index]);

    groups.erase(groups.begin() + index);
    dictionary()->setToUserModified();

    owner->emitGroupDeleted(this, index, oldptr);
    //emit owner->groupDeleted(this, index, oldptr);
}

void GroupCategoryBase::remove(const std::vector<GroupBase*> &which)
{
    std::vector<GroupBase*> items;
    selectTopItems(which, items);

    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
    {
        GroupCategoryBase *p = items[ix]->parentCategory();
        if (items[ix]->isCategory())
        {
            ((GroupCategoryBase*)items[ix])->emitDeleted();
            int index = p->categoryIndex((GroupCategoryBase*)items[ix]);

            emit owner->categoryAboutToBeDeleted(p, index, p->list[index]);
            p->list.erase(p->list.begin() + index);
            emit owner->categoryDeleted(p, index, (GroupCategoryBase*)items[ix]);
        }
        else
        {
            int index = p->groupIndex(items[ix]);
            emit owner->groupAboutToBeDeleted(p, index, p->groups[index]);
            p->groups.erase(p->groups.begin() + index);

            owner->emitGroupDeleted(p, index, items[ix]);
            //emit owner->groupDeleted(p, index, items[ix]);
        }
    }

    dictionary()->setToUserModified();
}

bool GroupCategoryBase::moveCategory(GroupCategoryBase *what, GroupCategoryBase *destparent, int destindex)
{
#ifdef _DEBUG
    if (destindex < 0 || destindex > tosigned(destparent->list.size()))
        throw "Index out of range.";
#endif

    GroupCategoryBase *p = what->parentCategory();
    int ix = p->categoryIndex(what);

    if (p == destparent)
    {
        // Same as the current position. No move is necessary.
        if (ix == destindex || ix == destindex - 1)
            return false;

        GroupCategoryBase **ldata = destparent->list.data();

        // Moving the other groups up or down by one position.
        memmove(ldata + std::min(destindex + 1, ix), ldata + std::min(destindex, ix + 1), (ix < destindex ? (destindex - ix - 1) : (ix - destindex)) * sizeof(GroupCategoryBase*));

        // Inserting the item where it belongs. If moving up in the same parent, destindex is
        // the index where it'd be placed if it were not removed first. After it's removed
        // the real index will be one less.
        if (destindex < ix)
            *(ldata + destindex) = what;
        else
            *(ldata + destindex - 1) = what;

    }
    else
    {
        QString wname = what->name().toLower();
        for (const GroupCategoryBase* g : destparent->list)
        {
            if (g != what && g->name().toLower() == wname)
                return false;
        }

        p->list.removeAt(p->list.begin() + ix);
        what->parent = destparent;
        destparent->list.insert(destparent->list.begin() + destindex, what);
    }

    dictionary()->setToUserModified();
    emit categoryMoved(p, ix, destparent, destindex);

    return true;
}

bool GroupCategoryBase::moveGroup(GroupBase *what, GroupCategoryBase *destparent, int destindex)
{
#ifdef _DEBUG
    if (destindex < 0 || destindex > tosigned(destparent->groups.size()))
        throw "Index out of range.";
#endif

    GroupCategoryBase *p = what->parentCategory();
    int ix = p->groupIndex(what);

    if (p == destparent)
    {
        // Same as the current position. No move is necessary.
        if (ix == destindex || ix == destindex - 1)
            return false;

        GroupBase **ldata = destparent->groups.data();

        // Moving the other groups up or down by one position.
        memmove(ldata + std::min(destindex + 1, ix), ldata + std::min(destindex, ix + 1), (ix < destindex ? (destindex - ix - 1) : (ix - destindex)) * sizeof(GroupBase*));

        // Inserting the item where it belongs. If moving up in the same parent, destindex is
        // the index where it'd be placed if it were not removed first. After it's removed
        // the real index will be one less.
        if (destindex < ix)
            *(ldata + destindex) = what;
        else
            *(ldata + destindex - 1) = what;

    }
    else
    {
        QString wname = what->name().toLower();
        for (const GroupBase* g : destparent->groups)
        {
            if (g != what && g->name().toLower() == wname)
                return false;
        }

        p->groups.removeAt(p->groups.begin() + ix);
        what->parent = destparent;
        destparent->groups.insert(destparent->groups.begin() + destindex, what);
    }

    dictionary()->setToUserModified();
    emit groupMoved(p, ix, destparent, destindex);

    return true;
}

void GroupCategoryBase::moveCategories(const std::vector<GroupCategoryBase*> &moved, GroupCategoryBase *destparent, int destindex)
{
#ifdef _DEBUG
    if (destindex < 0 || destindex > tosigned(destparent->list.size()))
        throw "Index out of range.";
#endif
    if (moved.empty())
        return;

    GroupCategoryBase *p = moved.front()->parentCategory();

    //int ix = p->categoryIndex(what);

    // Source indexes.
    std::vector<int> indexes;
    indexes.reserve(moved.size());
    for (int ix = 0, siz = tosigned(moved.size()); ix != siz; ++ix)
        indexes.push_back(p->categoryIndex(moved[ix]));
    std::sort(indexes.begin(), indexes.end());

    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);

    if (p == destparent)
    {

        GroupCategoryBase **ldata = destparent->list.data();
        int lsiz = tosigned(destparent->list.size());

        _moveRanges(ranges, destindex, [this, &ldata, lsiz, p](const Range &range, int pos) {
            _moveRange(ldata,
#ifdef _DEBUG
                lsiz,
#endif
                range, pos);
            emit categoriesMoved(p, range, p, pos);
        });
    }
    else
    {
        // Erasing groups from source and placing them in dest going backwards, one range at a
        // time.

        for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
        {
            Range *r = ranges[ix];
            for (auto b = p->list.begin() + r->first, e = p->list.begin() + (r->last + 1); b != e; ++b)
                (*b)->parent = destparent;
            destparent->list.insert(destparent->list.begin() + destindex, p->list.mbegin() + r->first, p->list.mbegin() + (r->last + 1));
            p->list.erase(p->list.begin() + r->first, p->list.begin() + (r->last + 1));
            emit categoriesMoved(p, *r, destparent, destindex);
        }
    }

    dictionary()->setToUserModified();
}

void GroupCategoryBase::moveGroups(const std::vector<GroupBase*> &moved, GroupCategoryBase *destparent, int destindex)
{
#ifdef _DEBUG
    if (destindex < 0 || destindex > tosigned(destparent->groups.size()))
        throw "Index out of range.";
#endif
    if (moved.empty())
        return;

    GroupCategoryBase *p = moved.front()->parentCategory();

    //int ix = p->categoryIndex(what);

    // Source indexes.
    std::vector<int> indexes;
    indexes.reserve(moved.size());
    for (int ix = 0, siz = tosigned(moved.size()); ix != siz; ++ix)
        indexes.push_back(p->groupIndex(moved[ix]));
    std::sort(indexes.begin(), indexes.end());

    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);

    if (p == destparent)
    {

        GroupBase **ldata = destparent->groups.data();
        int lsiz = tosigned(destparent->groups.size());

        _moveRanges(ranges, destindex, [this, &ldata, lsiz, p](const Range &range, int pos) {
            _moveRange(ldata,
#ifdef _DEBUG
                lsiz,
#endif
                range, pos);
            emit groupsMoved(p, range, p, pos);
        });
    }
    else
    {
        // Erasing groups from source and placing them in dest going backwards, one range at a
        // time.

        for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
        {
            Range *r = ranges[ix];
            for (auto b = p->groups.begin() + r->first, e = p->groups.begin() + (r->last + 1); b != e; ++b)
                (*b)->parent = destparent;

            destparent->groups.insert(destparent->groups.begin() + destindex, p->groups.mbegin() + r->first, p->groups.mbegin() + (r->last + 1));
            p->groups.erase(p->groups.begin() + r->first, p->groups.begin() + (r->last + 1));
            emit groupsMoved(p, *r, destparent, destindex);
        }
    }

    dictionary()->setToUserModified();
}

int GroupCategoryBase::categoryIndex(GroupCategoryBase *child)
{
    return list.indexOf(child);
}

int GroupCategoryBase::categoryIndex(QString name)
{
    name = name.toLower();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (list[ix]->name().toLower() == name)
            return ix;
    return -1;
}

int GroupCategoryBase::categoryCount() const
{
    return tosigned(list.size());
}

GroupCategoryBase* GroupCategoryBase::categories(int index)
{
    return list[index];
}

const GroupCategoryBase* GroupCategoryBase::categories(int index) const
{
    return list[index];
}

int GroupCategoryBase::groupCount() const
{
    int cnt = 0;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        cnt += list[ix]->groupCount();
    return cnt + tosigned(groups.size());
}

size_t GroupCategoryBase::size() const
{
    return groups.size();
}

GroupBase* GroupCategoryBase::items(int ix)
{
    return groups[ix];
}

const GroupBase* GroupCategoryBase::items(int ix) const
{
    return groups[ix];
}

int GroupCategoryBase::groupIndex(QString name) const
{
    name = name.toLower();
    for (int ix = 0, siz = tosigned(groups.size()); ix != siz; ++ix)
        if (groups[ix]->name().toLower() == name)
            return ix;
    return -1;
}

int GroupCategoryBase::groupIndex(GroupBase *group) const
{
    for (int ix = 0, siz = tosigned(groups.size()); ix != siz; ++ix)
        if (groups[ix] == group)
            return ix;
    return -1;
}

bool GroupCategoryBase::categoryNameTaken(QString name) const
{
    if (name.isEmpty())
        return false;
    name = name.toLower();
    for (const GroupCategoryBase* g : list)
        if (g->name().toLower() == name)
            return true;

    return false;
}

bool GroupCategoryBase::groupNameTaken(QString name) const
{
    if (name.isEmpty())
        return false;
    name = name.toLower();

    for (const GroupBase* g : groups)
        if (g->name().toLower() == name)
            return true;

    return false;
}

void GroupCategoryBase::setName(QString newname)
{
    if (name() == newname)
        return;

    base::setName(newname);

    GroupCategoryBase *p = parentCategory();
    if (p == nullptr)
        return;
    emit owner->categoryRenamed(p, p->categoryIndex(this));
}

int GroupCategoryBase::addGroup(QString name, GroupCategoryBase *cat)
{
    return cat->addGroup(name);
}

int GroupCategoryBase::addCategory(QString name, GroupCategoryBase *cat)
{
    return cat->addCategory(name);
}

void GroupCategoryBase::walkGroups(const std::function<void(GroupBase*)> &callback)
{
    std::stack<GroupCategoryBase*> stack;
    stack.push(this);

    while (!stack.empty())
    {
        GroupCategoryBase *pos = stack.top();
        stack.pop();

        for (int ix = 0; ix != tosigned(pos->groups.size()); ++ix)
            callback(pos->groups[ix]);

        for (int ix = 0; ix != tosigned(pos->list.size()); ++ix)
            stack.push(pos->list[ix]);
    }
}

bool GroupCategoryBase::isChild(GroupBase *item, bool recursive) const
{
    for (int ix = 0, siz = tosigned(size()); ix != siz; ++ix)
        if (groups[ix] == item)
            return true;
    if (!recursive)
        return false;
    for (int ix = 0, siz = categoryCount(); ix != siz; ++ix)
        if (list[ix]->isChild(item, true))
            return true;
    return false;
}

void GroupCategoryBase::emitDeleted()
{
    for (GroupCategoryBase *cat : list)
        cat->emitDeleted();

    for (GroupBase *grp : groups)
    {
        emit owner->groupAboutToBeDeleted(nullptr, -1, grp);
        owner->emitGroupDeleted(nullptr, -1, (void*)grp);
        //emit owner->groupDeleted(nullptr, -1, (void*)grp);
    }
}

void GroupCategoryBase::emitGroupDeleted(GroupCategoryBase *p, int index, void *oldaddress)
{
    emit groupDeleted(p, index, oldaddress);
}

void GroupCategoryBase::selectTopItems(const std::vector<GroupBase*> &src, std::vector<GroupBase*> &items) const
{
    // Rebuild the list to only contain categories and groups not already found in which. 
    items = src;

    // Positions of items found in which.
    std::map<GroupBase*, int> pos;
    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
        pos.insert(std::make_pair(items[ix], ix));

    QSet<GroupBase*> found;
    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
    {
        GroupBase *g = items[ix];
        if (!g->isCategory())
            continue;

        GroupCategoryBase *cat = (GroupCategoryBase*)g;

        std::list<GroupCategoryBase*> stack;
        stack.push_back(cat);
        while (!stack.empty())
        {
            cat = stack.front();
            stack.pop_front();

            for (int iy = 0, ysiz = cat->categoryCount(); iy != ysiz; ++iy)
            {
                GroupCategoryBase *c = cat->categories(iy);
                found.insert(c);
                stack.push_back(c);
            }
            for (int iy = 0, ysiz = tosigned(cat->size()); iy != ysiz; ++iy)
                found.insert(cat->items(iy));
        }
    }

    for (GroupBase *g : found)
    {
        auto it = pos.find(g);
        if (it != pos.end())
            items[it->second] = nullptr;
    }
    items.resize(std::remove(items.begin(), items.end(), nullptr) - items.begin());

}


//-------------------------------------------------------------


//template<typename T, typename O>
//GroupCategory<T, O>::GroupCategory(O *owner) : base(nullptr), owner(owner)
//{
//    ;
//}
//
//template<typename T, typename O>
//GroupCategory<T, O>::GroupCategory(GroupCategory<T, O> *parent) : base(parent), owner(((self_type*)parent)->getOwner())
//{
//    ;
//}
//
//template<typename T, typename O>
//GroupCategory<T, O>::GroupCategory(GroupCategory<T, O> *parent, QString name) : base(parent, name), owner(((self_type*)parent)->getOwner())
//{
//    ;
//}
//
//template<typename T, typename O>
//GroupCategory<T, O>::GroupCategory(GroupCategory<T, O> &&src) : base(nullptr), owner(nullptr)
//{
//    *this = std::forward<GroupCategory<T, O>>(src);
//}
//
//template<typename T, typename O>
//GroupCategory<T, O>& GroupCategory<T, O>::operator=(GroupCategory<T, O> &&src)
//{
//    base::operator=(std::move(src));
//    std::swap(list, src.list);
//    std::swap(owner, src.owner);
//    return *this;
//}

//template<typename T, typename O>
//GroupCategory<T, O>::~GroupCategory()
//{
//    clear();
//}
//
//template<typename T, typename O>
//void GroupCategory<T, O>::load(QDataStream &stream)
//{
//    base::load(stream);
//
//    qint32 i;
//    stream >> i;
//    list.reserve(i);
//    for (int ix = 0; ix != i; ++ix)
//    {
//        list.push_back(new T(this));
//        list[ix]->load(stream);
//    }
//}
//
//template<typename T, typename O>
//void GroupCategory<T, O>::save(QDataStream &stream) const
//{
//    base::save(stream);
//
//    stream << (qint32)list.size();
//    for (int ix = 0; ix != list.size(); ++ix)
//        list[ix]->save(stream);
//}

//template<typename T, typename O>
//Dictionary* GroupCategory<T, O>::dictionary()
//{
//    return owner->dictionary();
//}
//
//template<typename T, typename O>
//const Dictionary* GroupCategory<T, O>::dictionary() const
//{
//    return owner->dictionary();
//}

//template<typename T, typename O>
//void GroupCategory<T, O>::clear()
//{
//    list.clear();
//    base::clear();
//}

//template<typename T, typename O>
//int GroupCategory<T, O>::addGroup(QString name)
//{
//    list.push_back(new T(this, name));
//    return list.size() - 1;
//}

//template<typename T, typename O>
//void GroupCategory<T, O>::deleteGroup(int ix)
//{
//    list.erase(list.begin() + ix);
//}

//template<typename T, typename O>
//int GroupCategory<T, O>::size() const
//{
//    return list.size();
//}
//
//template<typename T, typename O>
//T* GroupCategory<T, O>::items(int ix)
//{
//    return list[ix];
//}
//
//template<typename T, typename O>
//const T* GroupCategory<T, O>::items(int ix) const
//{
//    return list[ix];
//}

//template<typename T, typename O>
//typename GroupCategory<T, O>::self_type* GroupCategory<T, O>::categories(int index)
//{
//    return (self_type*)base::categories(index);
//}
//
//template<typename T, typename O>
//const typename GroupCategory<T, O>::self_type* GroupCategory<T, O>::categories(int index) const
//{
//    return (const self_type*)base::categories(index);
//}

//template<typename T, typename O>
//GroupCategoryBase* GroupCategory<T, O>::createCategory()
//{
//    return new GroupCategory<T, O>(this);
//}

//template<typename T, typename O>
//bool GroupCategory<T, O>::validItemName(QString name) const
//{
//    for (int ix = list.size() - 1; ix != -1; --ix)
//        if (list[ix]->name().toLower() == name)
//            return false;
//    return true;
//}
//
//template<typename T, typename O>
//O* GroupCategory<T, O>::getOwner()
//{
//    return owner;
//}


//-------------------------------------------------------------


//WordGroup::WordGroup(WordGroups *parent) : base(parent, QString()), study(this)
//{
//    ;
//}

WordGroup::WordGroup(WordGroupCategory *parent, QString name) : base(parent, name), study(this)
{
    ;
}

WordGroup::WordGroup(WordGroup &&src) : base(nullptr), study(this)
{
    *this = std::forward<WordGroup>(src);
}

WordGroup& WordGroup::operator=(WordGroup &&src)
{
    base::operator=(std::forward<WordGroup>(src));
    std::swap(study, src.study);
    std::swap(list, src.list);
    //std::swap(defs, src.defs);

    return *this;
}

WordGroup::~WordGroup()
{
    clear();
}

void WordGroup::load(QDataStream &stream)
{
    base::load(stream);

    stream >> make_zvec<qint32, qint32>(list);
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        owner().groupsOfWord(list[ix]).push_front(this);
        dictionary()->wordEntry(list[ix])->dat |= (1 << (int)WordRuntimeData::InGroup);
    }

    study.load(stream);
}

void WordGroup::save(QDataStream &stream) const
{
    base::save(stream);

    stream << make_zvec<qint32, qint32>(list);
    study.save(stream);
}

bool WordGroup::isEmpty() const
{
    return list.empty()/* && study.finished()*/;
}

GroupTypes WordGroup::groupType() const
{
    return GroupTypes::Words;
}

void WordGroup::copy(GroupBase *src)
{
#ifdef _DEBUG
    if (dynamic_cast<WordGroup*>(src) == nullptr)
        throw "invalid copy";
#endif

    base::copy(src);
    modelptr.reset();
    list = ((WordGroup*)src)->list;
    study.copy(&((WordGroup*)src)->study);
}

void WordGroup::applyChanges(const std::map<int, int> &changes)
{
    QSet<int> found;

    bool changed = false;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        auto it = changes.find(list[ix]);
        if (it == changes.end() || it->second == -1 || found.contains(it->second))
        {
            list.erase(list.begin() + ix);
            --siz;
            --ix;
            changed = true;
            continue;
        }
        list[ix] = it->second;
        changed = true;
        found.insert(it->second);
    }

    study.applyChanges(changes);

    if (changed)
        dictionary()->setToUserModified();
}

void WordGroup::processRemovedWord(int windex)
{
    study.processRemovedWord(windex);

    int pos = removeIndexFromList(windex, list);
    if (pos != -1)
        emit owner().itemsRemoved(this, { { pos, pos } });
}

void WordGroup::clear()
{
    for (int ix = 0, siz = tosigned(size()); ix != siz; ++ix)
    {
        int wix = list[ix];
        if (owner().wordHasGroups(wix))
        {
            auto g = owner().groupsOfWord(wix);

            // Opening a scope here, to make sure `it` and `itn` are freed before their
            // parent list gets deleted in checkGroupsOfWord below, to satisfy the
            // VS' dumb debug version of the STL.
            {
                auto it = g.before_begin();
                auto itn = std::next(it);
                while (itn != g.end())
                {
                    if (*itn == this)
                    {
                        g.erase_after(it);
                        break;
                    }
                    ++it;
                    ++itn;
                }
            }

            if (g.empty())
                owner().checkGroupsOfWord(wix);
        }
    }
    list.clear();
}

size_t WordGroup::size() const
{
    return list.size();
}

//bool WordGroup::empty() const
//{
//    return list.empty();
//}

Dictionary* WordGroup::dictionary()
{
    return parentCategory()->dictionary();
}

const Dictionary* WordGroup::dictionary() const
{
    return parentCategory()->dictionary();
}

DictionaryGroupItemModel* WordGroup::groupModel()
{
    if (modelptr == nullptr)
    {
        modelptr.reset(new DictionaryGroupItemModel());
        modelptr->setWordGroup(this);
    }
    return modelptr.get();
}

//void WordGroup::reserve(int cnt)
//{
//    indexes.reserve(cnt);
//    defs.reserve(cnt);
//}

WordEntry* WordGroup::items(int ix)
{
    return dictionary()->wordEntry(list[ix]);
}

const WordEntry* WordGroup::items(int ix) const
{
    return dictionary()->wordEntry(list[ix]);
}

//WordEntry* WordGroup::operator[](int ix)
//{
//    return dictionary()->wordEntry(list[ix]);
//}
//
//const WordEntry* WordGroup::operator[](int ix) const
//{
//    return dictionary()->wordEntry(list[ix]);
//}

const std::vector<int>& WordGroup::getIndexes() const
{
    return list;
}

int WordGroup::indexes(int pos) const
{
    return list[pos];
}

int WordGroup::indexOf(int windex) const
{
    auto it = std::find(list.begin(), list.end(), windex);
    if (it == list.end())
        return -1;

    return it - list.begin();
}

void WordGroup::indexOf(const std::vector<int> &windexes, std::vector<int> &positions) const
{
    if (windexes.empty())
        return;

    // An ordering of list by word index.
    std::vector<int> order;
    order.reserve(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        order.push_back(ix);
    std::sort(order.begin(), order.end(), [this](int a, int b) { return list[a] < list[b]; });

    std::vector<int> worder = windexes;
    std::sort(worder.begin(), worder.end());

    positions.clear();

    int opos = std::lower_bound(order.begin(), order.end(), worder.front(), [this](int ord, int wix) { return list[ord] < wix; }) - order.begin();
    for (int wpos = 0, wsiz = tosigned(worder.size()), osiz = tosigned(order.size()); wpos != wsiz && opos != osiz;)
    {
        while (wpos != wsiz && worder[wpos] < list[order[opos]])
            ++wpos;
        if (wpos == wsiz)
            break;
        while (opos != osiz && worder[wpos] > list[order[opos]])
            ++opos;
        if (opos == osiz)
            break;

        while (opos != osiz && wpos != wsiz && worder[wpos] == list[order[opos]])
        {
            positions.push_back(order[opos]);
            ++opos;
            ++wpos;
        }
    }

    std::sort(positions.begin(), positions.end());
}


//uchar WordGroup::defCount(int index)
//{
//    uchar cnt = 0;
//
//    int windex = indexes[index];
//    uint64_t def = defs[index];
//
//    for (int ix = dictionary()->wordEntry(windex)->defs.size() - 1; ix != -1; --ix)
//        if (((def >> ix) & 1) == 1)
//            ++cnt;
//
//    return cnt;
//}
//
//uchar WordGroup::definition(int index, uchar defix)
//{
//    int windex = indexes[index];
//    uint64_t def = defs[index];
//
//    int defsize = dictionary()->wordEntry(windex)->defs.size();
//    uchar ix = 0;
//
//    while (ix != defsize)
//    {
//        if (((def >> ix) & 1) == 1)
//        {
//            if (defix == 0)
//                return ix;
//            --defix;
//        }
//        ++ix;
//    }
//
//#ifdef _DEBUG
//    throw "Defix is somewhat out of bounds.";
//#else
//    return 0;
//#endif
//}
//
//uint64_t WordGroup::defValue(int index)
//{
//    return defs[index];
//}

//void WordGroup::setStudyMethod(WordStudy::Method method)
//{
//    study.setMethod(method);
//}

int WordGroup::add(int index)
{
    return insert(index, tosigned(list.size()));
}

int WordGroup::insert(int windex, int pos)
{
#ifdef _DEBUG
    if (pos < -1 || pos > tosigned(list.size()))
        throw "Index out of range at word insertion to group.";
#endif

    if (pos == -1)
        pos = tosigned(list.size());

    // Check whether the entry is already added to this group to not add it again.

    std::forward_list<WordGroup*> wg = owner().groupsOfWord(windex);
    auto git = std::find(wg.begin(), wg.end(), this);
    if (git != wg.end())
    {
        // Word entry already added. Just return its position.

        auto it = std::find(list.begin(), list.end(), windex);
        return it - list.begin();
    }

    // Word entry wasn't added. Set its dat to remember that it has been added here.

    dictionary()->wordEntry(windex)->dat |= (1 << (int)WordRuntimeData::InGroup);
    list.insert(list.begin() + pos, windex);

    // The wg list holds which groups have the word entry. It must be updated.
    wg.push_front(this);

    dictionary()->setToUserModified();
    emit owner().itemsInserted(this, { { pos, 1 } });

    return pos;
}

int WordGroup::add(const std::vector<int> &windexes, std::vector<int> *positions)
{
    return insert(windexes, tosigned(list.size()), positions);
}

int WordGroup::insert(const std::vector<int> &windexes, int pos, std::vector<int> *positions)
{
#ifdef _DEBUG
    if (pos < -1 || pos > tosigned(list.size()))
        throw "Index out of range at word insertion to group.";
#endif

    if (pos == -1)
        pos = tosigned(list.size());

    // To avoid adding duplicate words, the list of words and the windexes are both sorted
    // and checked for duplicates.

    if (windexes.empty())
        return 0;

    if (windexes.size() == 1)
    {
        // Special case.

        int s = tosigned(list.size());

        pos = insert(windexes.front(), pos);

        if (positions != nullptr)
            positions->push_back(pos);

        // list size is changed in the insert() above.
        return s == tosigned(list.size()) ? 0 : 1;
    }

    // Pairs of [index in list, word index].
    std::vector<std::pair<int, int>> listorder;
    listorder.reserve(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        listorder.push_back(std::make_pair(ix, list[ix]));
    std::sort(listorder.begin(), listorder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.second < b.second; });

    // Pairs of [index in windexes, word index].
    std::vector<std::pair<int, int>> worder;
    worder.reserve(windexes.size());
    for (int ix = 0, siz = tosigned(windexes.size()); ix != siz; ++ix)
        worder.push_back(std::make_pair(ix, windexes[ix]));
    std::sort(worder.begin(), worder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.second < b.second; });

    if (positions != nullptr)
        positions->reserve(windexes.size());

    int added = tosigned(windexes.size());

    for (int ix = 0, wix = 0, siz = tosigned(listorder.size()), wsiz = tosigned(worder.size()); ix != siz && wix != wsiz; )
    {
        if (listorder[ix].second < worder[wix].second)
        {
            // Word in group but not in windexes.
            ++ix;
            continue;
        }

        if (listorder[ix].second == worder[wix].second)
        {
            // To show that a word has already been added, its word index is changed to a
            // negative value. The value is set to [-1 - word's old position in list]. This
            // value will be used later to update the positions list.
            worder[wix].second = -1 - listorder[ix].first;

            ++ix;
            --added;
        }

        ++wix;
    }

    // Restore the original order of the entry indexes to be inserted.
    std::sort(worder.begin(), worder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return a.first < b.first;
    });

    if (positions == nullptr && added == 0)
        return 0;

    list.insert(list.begin() + pos, added, 0);
    for (int ix = 0, aix = 0, siz = tosigned(worder.size()); ix != siz; ++ix)
    {
        int val = worder[ix].second;
        if (val < 0)
        {
            // Word already in group.

            if (positions != nullptr)
            {
                // Recompute the old position of the word in the list. If that position is
                // greater or equal to the pos insertion position, words will be inserted
                // before it, so this position is increased accordingly.
                int oldpos = -1 - val;
                if (oldpos >= pos)
                    oldpos += added;
                positions->push_back(oldpos);
            }

            continue;
        }

        // Mark the word as present in a group.
        dictionary()->wordEntry(val)->dat |= (1 << (int)WordRuntimeData::InGroup);

        list[pos + aix] = val;

        // The list that holds which groups have the word entry must be updated.
        std::forward_list<WordGroup*> wg = owner().groupsOfWord(val);
        wg.push_front(this);

        if (positions != nullptr)
            positions->push_back(pos + aix);

        ++aix;
    }

    if (added != 0)
    {
        dictionary()->setToUserModified();
        emit owner().itemsInserted(this, { { pos, added } });
    }

    return added;
}

void WordGroup::remove(WordEntry *e)
{
    Dictionary *d = dictionary();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (d->wordEntry(list[ix]) == e)
        {
            removeAt(ix);
            break;
        }
}

void WordGroup::remove(const smartvector<Range> &ranges)
{
    if (ranges.empty())
        return;

    //std::vector<int> sorted = indexes;

    //std::sort(sorted.begin(), sorted.end());

    //int lastpos = sorted.size() - 1;
    //int firstpos;
    //int last;
    //int first;

    //while (lastpos != -1)
    //{
    //    last = first = sorted[lastpos];
    //    firstpos = lastpos - 1;

    //    while (firstpos != -1 && sorted[firstpos] == first - 1)
    //        --first, --firstpos;

    //    for (int ix = last; ix != first - 1; --ix)
    //        _removeAt(ix);

    //    list.erase(list.begin() + first, list.begin() + last + 1);

    //    //removeRange(first, last);

    //    lastpos = firstpos;
    //}

    for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
    {
        const Range *r = ranges[ix];
        for (int iy = r->first, last = r->last; iy != last + 1; ++iy)
            removeFromGroup(iy);

        list.erase(list.begin() + r->first, list.begin() + r->last + 1);

        //if (prev == -1 || sorted[prev + 1] - sorted[prev] != 1)
        //{
        //    list.erase(list.begin() + (prev + 1), list.begin() + (pos + 1));
        //    pos = prev;
        //}
    }

    dictionary()->setToUserModified();
    emit owner().itemsRemoved(this, ranges);
}

void WordGroup::move(const smartvector<Range> &ranges, int pos)
{
    if (ranges.empty())
        return;

    if (pos == -1)
        pos = tosigned(list.size());

    if (_moveRanges(ranges, pos, list))
        dictionary()->setToUserModified();
    emit owner().itemsMoved(this, ranges, pos);
}

void WordGroup::removeAt(int index)
{
    removeFromGroup(index);

    //emit owner().beginItemsRemove(this, index, index);

    list.erase(list.begin() + index);
    dictionary()->setToUserModified();

    emit owner().itemsRemoved(this, { { index, 1 } }/*, index, index*/);
}

void WordGroup::removeRange(int first, int last)
{
#ifdef _DEBUG
    if (last < first)
        throw "Invalid range.";
#endif

    //emit owner().beginItemsRemove(this, first, last);

    for (int ix = last; ix != first - 1; --ix)
        removeFromGroup(ix);

    list.erase(list.begin() + first, list.begin() + last + 1);
    dictionary()->setToUserModified();

    emit owner().itemsRemoved(this, { { first, last } }/*, first, last*/);
}

WordStudy& WordGroup::studyData()
{
    return study;
}

void WordGroup::setName(QString newname)
{
    if (name() == newname)
        return;

    base::setName(newname);

    GroupCategoryBase *p = parentCategory();
    emit owner().groupRenamed(p, p->groupIndex(this));
}

WordGroups& WordGroup::owner()
{
    return dictionary()->wordGroups(); //(WordGroups*)topParent();
}

const WordGroups& WordGroup::owner() const
{
    return dictionary()->wordGroups(); //(const WordGroups*)topParent();
}

//const WordStudySettings& WordGroup::studySettings() const
//{
//    return study.studySettings();
//}
//
//void WordGroup::resetStudy()
//{
//    study.reset();
//}
//
//bool WordGroup::studySuspended() const
//{
//    return study.suspended();
//}

void WordGroup::removeFromGroup(int index)
{
    int windex = list[index];

    // Check whether the word is added to this group first, and erase it from the list of
    // groups for this word.
    std::forward_list<WordGroup*> wg = owner().groupsOfWord(windex);
    bool found = false;

    if (!wg.empty())
    {
        auto preit = wg.before_begin();
        auto it = std::next(preit);
        while (it != wg.end())
        {
            if (*it == this)
            {
                wg.erase_after(preit);
                found = true;
                break;
            }
            preit = it;
            it = std::next(preit);
        }

    }

#ifdef _DEBUG
    if (!found)
        throw "Word at specific index wasn't in the global list of words.";
#endif

    if (wg.empty())
        owner().checkGroupsOfWord(windex);
}

//void WordGroup::_move(int first, int last, int pos)
//{
//#ifdef _DEBUG
//    if (first > last || first < 0 || std::max(first, last) >= list.size() || (pos >= first && pos <= last + 1))
//        throw "Invalid parameters.";
//#endif
//
//    //emit owner().beginItemsMove(this, first, last, pos);
//
//    // Change parameters if needed to move the least number of items.
//
//    if (pos < first && last - first + 1 > first - pos)
//    {
//        int tmp = first;
//        first = pos;
//        pos = last + 1;
//        last = tmp - 1;
//    }
//    else if (pos > last && last - first + 1 > pos - 1 - last)
//    {
//        int tmp = last;
//        last = pos - 1;
//        pos = first;
//        first = tmp + 1;
//    }
//
//    std::vector<int> movelist(list.begin() + first, list.begin() + (last + 1));
//
//    int *ldat = list.data();
//    int *mdat = movelist.data();
//
//    if (pos < first)
//    {
//        // Move items.
//        for (int ix = 0, siz = first - pos; ix != siz; ++ix)
//            ldat[last - ix] = ldat[first - 1 - ix];
//
//        // Copy movelist back.
//        for (int ix = 0, siz = movelist.size(); ix != siz; ++ix)
//            ldat[pos + ix] = mdat[ix];
//    }
//    else
//    {
//        // Move items.
//        for (int ix = 0, siz = pos - last - 1; ix != siz; ++ix)
//            ldat[first + ix] = ldat[last + 1 + ix];
//
//        // Copy movelist back.
//        for (int ix = 0, siz = movelist.size(); ix != siz; ++ix)
//            ldat[pos - siz + ix] = mdat[ix];
//    }
//
//    //emit owner().itemsMoved(this);
//}


//-------------------------------------------------------------


//WordGroupCategory::WordGroupCategory(Dictionary *dict) : base(dict, QString())
//{
//    ;
//}
//
//WordGroupCategory::WordGroupCategory(Dictionary *dict, QString name) : base(dict, name)
//{
//    ;
//}
//
//WordGroupCategory::WordGroupCategory(WordGroupCategory &&src) : base(nullptr)
//{
//    *this = std::forward<WordGroupCategory>(src);
//}
//
//WordGroupCategory& WordGroupCategory::operator=(WordGroupCategory &&src)
//{
//    base::operator=(std::move(src));
//    std::swap(list, src.list);
//    return *this;
//}
//
//WordGroupCategory::~WordGroupCategory()
//{
//    clear();
//}
//
//void WordGroupCategory::clear()
//{
//    list.clear();
//    base::clear();
//}
//
//int WordGroupCategory::addGroup(QString name)
//{
//    list.push_back(new WordGroup(dictionary(), name));
//    return list.size() - 1;
//}
//
//void WordGroupCategory::deleteGroup(int ix)
//{
//    list.erase(list.begin() + ix);
//}
//
//int WordGroupCategory::size()
//{
//    return list.size();
//}
//
//const WordGroup* WordGroupCategory::items(int ix) const
//{
//    return list[ix];
//}
//
//WordGroup* WordGroupCategory::items(int ix)
//{
//    return list[ix];
//}
//
//GroupCategoryBase* WordGroupCategory::createCategory(QString name)
//{
//    return new WordGroupCategory(dictionary(), name);
//}
//
//bool WordGroupCategory::validItemName(QString name) const
//{
//    for (int ix = list.size() - 1; ix != -1; --ix)
//        if (list[ix]->getName().toLower() == name)
//            return false;
//    return true;
//}

template<typename T>
GroupCategory<T>::GroupCategory(self_type *parent, QString name) : base(parent, name)
{
}

template<typename T>
GroupCategory<T>::GroupCategory(self_type &&src)
{
    *this = std::move(src);
}

template<typename T>
GroupCategory<T>& GroupCategory<T>::operator=(self_type &&src)
{
    base::operator=(std::move(src));
    return this;
}

//template<typename T>
//T* GroupCategory<T>::items(int index)
//{
//    return (T*)base::items(index);
//}

template<typename T>
GroupTypes GroupCategory<T>::groupType() const
{
    return _groupType();
}

template<typename T>
const T* GroupCategory<T>::items(int index) const
{
    return dynamic_cast<const T*>(base::items(index));
}

template<typename T>
typename GroupCategory<T>::self_type* GroupCategory<T>::categories(int index)
{
    return dynamic_cast<self_type*>(base::categories(index));
}

template<typename T>
const typename GroupCategory<T>::self_type* GroupCategory<T>::categories(int index) const
{
    return dynamic_cast<const self_type*>(base::categories(index));
}

template<typename T>
T* GroupCategory<T>::groupFromEncodedName(const QString &name, int pos, int len, bool create)
{
    if (name.isEmpty())
        return nullptr;

    if (len == -1)
        len = name.size() - pos;
#ifdef _DEBUG
    if (pos < 0 || len < 1 || len + pos > name.size())
        throw "Invalid position or size for substring.";
#endif

    // TODO: check the validity of the groups' name. Currently only the string's validity is
    // checked. Return nullptr (and check this where used) on invalid string.

    // Length of the longest name in the passed "path".
    int maxlen = 0;
    // Length of the current name in the passed "path".
    int namelen = 0;

    // Check string validity. The ^ and [ characters can only appear if escaped by ^. The /
    // character is used as separator, and cannot be followed by another /.

    for (int ix = 0; ix != len; ++ix, ++namelen)
    {
        if (name.at(ix + pos) == QChar('^'))
        {
            // Invalid name if escape character is not followed by escaped character or wrong
            // character is following it.
            if (ix == len - 1)
                return nullptr;
            QChar ch = name.at(ix + pos + 1);
            if (ch != QChar('^') && ch != QChar('[') && ch != QChar('/'))
                return nullptr;
            ++ix;
            continue;
        }
        // Cannot have [ without escaping it.
        if (name.at(ix + pos) == QChar('['))
            return nullptr;
        if (name.at(ix + pos) == QChar('/'))
        {
            // Separator not followed by anything or the first name has zero length.
            if (ix == len - 1 || namelen == 0)
                return nullptr;
            // Empty sections are not allowed.
            if (name.at(ix + pos + 1) == QChar('/'))
                return nullptr;
            maxlen = std::max(namelen, maxlen);
            namelen = -1;
        }
    }

    maxlen = std::max(namelen, maxlen);

    QCharString str;
    QString nam;
    str.setSize(maxlen);

    namelen = 0;

    // Current category on the path of creating new sub-categories and groups.
    self_type *catpos = this;

    // True while it's possible that the current sub-category or final group exists. Set to
    // false as soon as the first sub-category or group must be created.
    bool finding = true;

    // Starting over, looking for or creating the sub categories and the final group.
    for (int ix = 0; ix != len; ++ix, ++namelen)
    {
        if (name.at(ix + pos) == QChar('^'))
        {
            str[namelen] = name.at(ix + pos + 1);
            ++ix;
            continue;
        }
        if (name.at(ix + pos) == QChar('/'))
        {
            nam = str.toQString(0, namelen);

            // Get or create sub group.
            if (finding)
            {
                int cix = catpos->categoryIndex(nam);
                if (cix != -1)
                    catpos = catpos->categories(cix);
                else if (!create)
                    return nullptr;
                else
                    finding = false;
            }

            // Not "else"! Finding is set to false above if the sub-category with the name nam
            // does not exist yet, so it must be created here.
            if (!finding)
                catpos = (self_type*)categories(catpos->addCategory(nam));

            namelen = -1;
            continue;
        }
        str[namelen] = name.at(ix + pos);
    }

    nam = str.toQString(0, namelen);
    if (finding)
    {
        int gix = catpos->groupIndex(nam);
        if (gix != -1)
            return (T*)catpos->items(gix);
        if (!create)
            return nullptr;
        finding = false;
    }

    return (T*)items(catpos->addGroup(nam));
}

template<typename T>
WordGroups::GroupCategoryBase* GroupCategory<T>::createCategory(QString name)
{
    return new self_type(this, name);
}

template<typename T>
WordGroups::GroupBase* GroupCategory<T>::createGroup(QString name)
{
    return new T(this, name);
}


//-------------------------------------------------------------


WordGroups::WordGroups(Groups *owner) : base(nullptr, QString()), owner(owner), lastgroup(nullptr)
{
}

WordGroups::~WordGroups()
{
    //groups.clear();
}

void WordGroups::clear()
{
    base::clear();
    wordsgroups.clear();

    //if (qApp->eventDispatcher() != nullptr)
    emit groupsReseted();
}

void WordGroups::applyChanges(const std::map<int, int> &changes)
{
    // Fix the words' groups listing.
    std::map<int, std::forward_list<WordGroup*>> tmp;
    for (auto it = wordsgroups.begin(); it != wordsgroups.end(); ++it)
    {
        auto ch = changes.find(it->first);
        if (ch->second != -1)
        {
            std::forward_list<WordGroup*> &tmplist = tmp[ch->second];
            if (tmplist.empty())
            {
                tmplist = std::move(it->second);
                dictionary()->wordEntry(ch->second)->dat |= (1 << (int)WordRuntimeData::InGroup);
            }
            else
                tmplist.insert_after(tmplist.before_begin(), it->second.begin(), it->second.end());
        }
    }
    std::swap(tmp, wordsgroups);
    for (auto it = wordsgroups.begin(); it != wordsgroups.end(); ++it)
    {
        it->second.sort();
        it->second.unique();
    }

    // Apply changes in categories and their groups recursively.
    groupsApplyChanges(this, changes);
    //for (int ix = 0; ix != groups.size(); ++ix)
    //    groups.items(ix)->applyChanges(changes);

    //for (int ix = 0; ix != groups.categoryCount(); ++ix)
    //    ((WordGroupCategory*)groups.categories(ix))->applyChanges(changes);
}

void WordGroups::copy(WordGroups *src)
{
    base::copy(src);
    fixWordsGroups();
}

void WordGroups::processRemovedWord(int windex)
{
    // Rebuilding the wordsgroups mapping.

    std::map<int, std::forward_list<WordGroup*>> tmp;
    std::swap(tmp, wordsgroups);

    for (auto it = tmp.begin(); it != tmp.end(); ++it)
    {
        if (it->first < windex)
            wordsgroups[it->first] = std::move(it->second);
        else if (it->first > windex)
            wordsgroups[it->first - 1] = std::move(it->second);
    }
    tmp.clear();

    // Removing the word from each group.
    walkGroups([windex](GroupBase *g) { ((WordGroup*)g)->processRemovedWord(windex); });
}

Dictionary* WordGroups::dictionary()
{
    return owner->dictionary();
}

const Dictionary* WordGroups::dictionary() const
{
    return owner->dictionary();
}

std::forward_list<WordGroup*>& WordGroups::groupsOfWord(int windex)
{
    //auto it = wordsgroups.find(windex);

    //if (it != wordsgroups.end())
    //    return it->second;

    return wordsgroups[windex];
}

bool WordGroups::wordHasGroups(int windex) const
{
    return wordsgroups.find(windex) != wordsgroups.end();
}

int WordGroups::wordsInGroups() const
{
    return tosigned(wordsgroups.size());
}

void WordGroups::checkGroupsOfWord(int windex)
{
    auto it = wordsgroups.find(windex);
    if (it == wordsgroups.end() || it->second.empty())
    {
        dictionary()->wordEntry(windex)->dat &= ~(1 << (int)WordRuntimeData::InGroup);
        if (it != wordsgroups.end())
            wordsgroups.erase(it);
    }
}

WordGroup* WordGroups::groupFromEncodedName(const QString &fullname, int pos, int len, bool create)
{
    return base::groupFromEncodedName(fullname, pos, len, create);
}

WordGroup* WordGroups::lastSelected() const
{
    return lastgroup;
}

void WordGroups::setLastSelected(WordGroup *g)
{
    if (g != nullptr && !isChild(g, true))
        return;

    lastgroup = g;
}

void WordGroups::setLastSelected(const QString &name)
{
    if (name.isEmpty())
        lastgroup = nullptr;
    else
        lastgroup = groupFromEncodedName(name);
}

void WordGroups::emitGroupDeleted(GroupCategoryBase *p, int index, void *oldaddress)
{
    base::emitGroupDeleted(p, index, oldaddress);
    if (lastgroup == oldaddress)
        lastgroup = nullptr;
}

void WordGroups::groupsApplyChanges(GroupCategoryBase *cat, const std::map<int, int> &changes)
{
    for (int ix = 0, siz = tosigned(cat->size()); ix != siz; ++ix)
        ((WordGroup*)cat->items(ix))->applyChanges(changes);

    for (int ix = 0, siz = cat->categoryCount(); ix != siz; ++ix)
        groupsApplyChanges(cat->categories(ix), changes);
}

void WordGroups::fixWordsGroups()
{
    wordsgroups.clear();

    std::list<WordGroupCategory*> stack;

    stack.push_back(this);

    while (!stack.empty())
    {
        WordGroupCategory *cat = stack.front();
        stack.pop_front();

        for (int ix = 0, siz = cat->categoryCount(); ix != siz; ++ix)
            stack.push_back(cat->categories(ix));

        for (int ix = 0, siz = tosigned(cat->size()); ix != siz; ++ix)
        {
            WordGroup *g = cat->items(ix);
            for (int iy = 0, siy = tosigned(g->size()); iy != siy; ++iy)
            {
                auto wg = groupsOfWord(g->indexes(iy));
                wg.push_front(g);
            }
        }
    }
}



//-------------------------------------------------------------


//KanjiGroup::KanjiGroup(KanjiGroupCategory *parent) : base(parent, QString())
//{
//
//}
//
KanjiGroup::KanjiGroup(KanjiGroupCategory *parent, QString name) : base(parent, name)
{

}

KanjiGroup::KanjiGroup(KanjiGroup &&src) : base(std::forward<KanjiGroup>(src))
{
    *this = std::forward<KanjiGroup>(src);
}

KanjiGroup& KanjiGroup::operator=(KanjiGroup &&src)
{
    base::operator=(std::forward<KanjiGroup>(src));
    std::swap(list, src.list);
    return *this;
}

KanjiGroup::~KanjiGroup()
{

}

void KanjiGroup::load(QDataStream &stream)
{
    base::load(stream);
    stream >> make_zvec<qint32, qint32>(list);
}

void KanjiGroup::save(QDataStream &stream) const
{
    base::save(stream);
    stream << make_zvec<qint32, qint32>(list);
}

bool KanjiGroup::isEmpty() const
{
    return list.empty();
}

GroupTypes KanjiGroup::groupType() const
{
    return GroupTypes::Kanji;
}

void KanjiGroup::copy(GroupBase *src)
{
#ifdef _DEBUG
    if (dynamic_cast<KanjiGroup*>(src) == nullptr)
        throw "invalid copy";
#endif

    base::copy(src);
    modelptr.reset();
    list = ((KanjiGroup*)src)->list;
}

KanjiEntry* KanjiGroup::items(int index)
{
    return ZKanji::kanjis[list[index]];
}

const KanjiEntry* KanjiGroup::items(int index) const
{
    return ZKanji::kanjis[list[index]];
}

//KanjiEntry* KanjiGroup::operator[](int index)
//{
//    return items(index);
//}
//
//const KanjiEntry* KanjiGroup::operator[](int index) const
//{
//    return items(index);
//}

KanjiGroup::size_type KanjiGroup::size() const
{
    return list.size();
}

//bool KanjiGroup::empty()
//{
//    return list.empty();
//}

Dictionary* KanjiGroup::dictionary()
{
    return parentCategory()->dictionary();
}

const Dictionary* KanjiGroup::dictionary() const
{
    return parentCategory()->dictionary();
}

KanjiGroupModel* KanjiGroup::groupModel()
{
    if (modelptr == nullptr)
    {
        modelptr.reset(new KanjiGroupModel());
        modelptr->setKanjiGroup(this);
    }
    return modelptr.get();
}

const std::vector<ushort>& KanjiGroup::getIndexes() const
{
    return list;
}

int KanjiGroup::indexOf(ushort kindex) const
{
    auto it = std::find(list.begin(), list.end(), kindex);
    if (it == list.end())
        return -1;

    return it - list.begin();
}

void KanjiGroup::indexOf(const std::vector<ushort> &kindexes, std::vector<int> &positions) const
{
    // Pairs of [index in list, kanji index].
    std::vector<std::pair<int, ushort>> listorder;
    listorder.reserve(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        listorder.push_back(std::make_pair(ix, list[ix]));

    std::sort(listorder.begin(), listorder.end(), [](const std::pair<int, ushort> &a, const std::pair<int, ushort> &b) {
        return a.second < b.second;
    });

    std::vector<ushort> ordered = kindexes;
    std::sort(ordered.begin(), ordered.end());

    positions.clear();
    positions.reserve(ordered.size());

    for (int ix = 0, iy = 0, siz = tosigned(listorder.size()), ysiz = tosigned(ordered.size()); ix != siz && iy != ysiz; )
    {
        if (listorder[ix].second < ordered[iy])
        {
            ++ix;
            continue;
        }
        else if (listorder[ix].second == ordered[iy])
        {
            positions.push_back(listorder[ix].first);
            ++ix;
        }

        ++iy;
    }

    std::sort(positions.begin(), positions.end());
}

int KanjiGroup::add(ushort kindex)
{
    return insert(kindex, tosigned(list.size()));
}

int KanjiGroup::insert(ushort kindex, int pos)
{
    if (pos == -1)
        pos = tosigned(list.size());

    auto it = std::find(list.begin(), list.end(), kindex);
    if (it != list.end())
        return it - list.begin();

    list.insert(list.begin() + pos, kindex);
    dictionary()->setToUserModified();
    emit owner().itemsInserted(this, { { pos, 1 } });

    return pos;
}

int KanjiGroup::add(const std::vector<ushort> &indexes, std::vector<int> *positions)
{
    return insert(indexes, tosigned(list.size()), positions);
}

void KanjiGroup::remove(const smartvector<Range> &ranges)
{
    if (ranges.empty())
        return;
    //std::vector<int> sorted = indexes;

    //std::sort(sorted.begin(), sorted.end());

    //int lastpos = sorted.size() - 1;
    //int firstpos;
    //int last;
    //int first;

    //while (lastpos != -1)
    //{
    //    last = first = sorted[lastpos];
    //    firstpos = lastpos - 1;

    //    while (firstpos != -1 && sorted[firstpos] == first - 1)
    //        --first, --firstpos;
    //    removeAt(first, last);
    //    lastpos = firstpos;
    //}

    for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
    {
        const Range *r = ranges[ix];

        list.erase(list.begin() + r->first, list.begin() + r->last + 1);

        //for (int pos = sorted.size() - 1, prev = pos - 1; pos != -1; --prev)
        //{
        //    if (prev == -1 || sorted[prev + 1] - sorted[prev] != 1)
        //    {
        //        list.erase(list.begin() + (prev + 1), list.begin() + (pos + 1));
        //        pos = prev;
    }

    dictionary()->setToUserModified();
    emit owner().itemsRemoved(this, ranges);
}

int KanjiGroup::insert(const std::vector<ushort> &kindexes, int pos, std::vector<int> *positions)
{
#ifdef _DEBUG
    if (pos < -1 || pos > tosigned(list.size()))
        throw "Index out of range at word insertion to group.";
#endif

    if (pos == -1)
        pos = tosigned(list.size());

    // To avoid adding duplicate kanji, the list of kanji and the kindexes are both sorted
    // and checked for duplicates.

    if (kindexes.empty())
        return 0;

    if (kindexes.size() == 1)
    {
        // Special case.

        int s = tosigned(list.size());

        pos = insert(kindexes.front(), pos);

        if (positions != nullptr)
            positions->push_back(pos);

        // list size is changed in the insert() above.
        return s == tosigned(list.size()) ? 0 : 1;
    }

    // Pairs of [index in list, kanji index].
    std::vector<std::pair<int, int>> listorder;
    listorder.reserve(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        listorder.push_back(std::make_pair(ix, list[ix]));
    std::sort(listorder.begin(), listorder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.second < b.second; });

    // Pairs of [index in kindexes, kanji index].
    std::vector<std::pair<int, int>> korder;
    korder.reserve(kindexes.size());
    for (int ix = 0, siz = tosigned(kindexes.size()); ix != siz; ++ix)
        korder.push_back(std::make_pair(ix, kindexes[ix]));
    std::sort(korder.begin(), korder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.second < b.second; });

    if (positions != nullptr)
        positions->reserve(kindexes.size());

    int added = tosigned(kindexes.size());

    for (int ix = 0, wix = 0, siz = tosigned(listorder.size()), wsiz = tosigned(korder.size()); ix != siz && wix != wsiz; )
    {
        if (listorder[ix].second < korder[wix].second)
        {
            // Kanji in group but not in kindexes.
            ++ix;
            continue;
        }

        if (listorder[ix].second == korder[wix].second)
        {
            // To show that a kanji has already been added, its kanji index is changed to a
            // negative value. The value is set to [-1 - kanji's old position in list]. This
            // value will be used later to update the positions list.
            korder[wix].second = -1 - listorder[ix].first;

            ++ix;
            --added;
        }

        ++wix;
    }

    if (positions == nullptr && added == 0)
        return 0;

    // Restore the original order of the kanji indexes to be inserted.
    std::sort(korder.begin(), korder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return a.first < b.first;
    });

    list.insert(list.begin() + pos, added, 0);
    for (int ix = 0, aix = 0, siz = tosigned(korder.size()); ix != siz; ++ix)
    {
        int val = korder[ix].second;
        if (val < 0)
        {
            // Kanji already in group.

            if (positions != nullptr)
            {
                // Recompute the old position of the kanji in the list. If that position is
                // greater or equal to the pos insertion position, kanji will be inserted
                // before it, so this position is increased accordingly.
                int oldpos = -1 - val;
                if (oldpos >= pos)
                    oldpos += added;
                positions->push_back(oldpos);
            }

            continue;
        }

        list[pos + aix] = val;

        if (positions != nullptr)
            positions->push_back(pos + aix);

        ++aix;
    }

    dictionary()->setToUserModified();
    emit owner().itemsInserted(this, { { pos, added } });

    return added;
}

void KanjiGroup::move(const smartvector<Range> &ranges, int pos)
{
    if (ranges.empty())
        return;

    if (_moveRanges(ranges, pos, list))
        dictionary()->setToUserModified();

    emit owner().itemsMoved(this, ranges, pos);
}

void KanjiGroup::removeAt(int index)
{
#ifdef _DEBUG
    if (index < 0 || index >= tosigned(list.size()))
        throw "Kanji remove index out of range.";
#endif

    //emit owner().beginItemsRemove(this, index, index);
    list.erase(list.begin() + index);
    dictionary()->setToUserModified();

    emit owner().itemsRemoved(this, { { index, index } }/*, first, last*/);
}

void KanjiGroup::removeRange(int first, int last)
{
#ifdef _DEBUG
    if (first > last || first < 0 || first >= tosigned(list.size()) || last < 0 || last >= tosigned(list.size()))
        throw "Kanji remove index out of range.";
#endif

    //emit owner().beginItemsRemove(this, first, last);
    list.erase(list.begin() + first, list.begin() + last + 1);
    dictionary()->setToUserModified();

    emit owner().itemsRemoved(this, { { first, last } }/*, first, last*/);
}

void KanjiGroup::setName(QString newname)
{
    if (name() == newname)
        return;

    base::setName(newname);

    GroupCategoryBase *p = parentCategory();
    emit owner().groupRenamed(p, p->groupIndex(this));
}

KanjiGroups& KanjiGroup::owner()
{
    return dictionary()->kanjiGroups();
}

const KanjiGroups& KanjiGroup::owner() const
{
    return dictionary()->kanjiGroups();
}

//void KanjiGroup::_move(int first, int last, int pos)
//{
//#ifdef _DEBUG
//    if (first > last || first < 0 || std::max(first, last) >= list.size() || (pos >= first && pos <= last + 1))
//        throw "Invalid parameters.";
//#endif
//
//    //emit owner().beginItemsMove(this, first, last, pos);
//
//    if (pos < first && last - first + 1 > first - pos)
//    {
//        int tmp = first;
//        first = pos;
//        pos = last + 1;
//        last = tmp - 1;
//    }
//    else if (pos > last && last - first + 1 > pos - 1 - last)
//    {
//        int tmp = last;
//        last = pos - 1;
//        pos = first;
//        first = tmp + 1;
//    }
//
//    std::vector<ushort> movelist(list.begin() + first, list.begin() + (last + 1));
//
//    ushort *ldat = list.data();
//    ushort *mdat = movelist.data();
//
//    if (pos < first)
//    {
//        // Move items.
//        for (int ix = 0, siz = first - pos; ix != siz; ++ix)
//            ldat[last - ix] = ldat[first - 1 - ix];
//
//        // Copy movelist back.
//        for (int ix = 0, siz = movelist.size(); ix != siz; ++ix)
//            ldat[pos + ix] = mdat[ix];
//    }
//    else
//    {
//        // Move items.
//        for (int ix = 0, siz = pos - last - 1; ix != siz; ++ix)
//            ldat[first + ix] = ldat[last + 1 + ix];
//
//        // Copy movelist back.
//        for (int ix = 0, siz = movelist.size(); ix != siz; ++ix)
//            ldat[pos - siz + ix] = mdat[ix];
//    }
//
//    //emit owner().itemsMoved(this);
//}


//-------------------------------------------------------------


KanjiGroups::KanjiGroups(Groups *owner) : base(nullptr, QString()), owner(owner), lastgroup(nullptr)
{
}

KanjiGroups::~KanjiGroups()
{

}

void KanjiGroups::clear()
{
    base::clear();

    //if (qApp->eventDispatcher() != nullptr)
    emit groupsReseted();
}

Dictionary* KanjiGroups::dictionary()
{
    return owner->dictionary();
}

const Dictionary* KanjiGroups::dictionary() const
{
    return owner->dictionary();
}

KanjiGroup* KanjiGroups::groupFromEncodedName(const QString &fullname, int pos, int len, bool create)
{
    return base::groupFromEncodedName(fullname, pos, len, create);
}

KanjiGroup* KanjiGroups::lastSelected() const
{
    return lastgroup;
}

void KanjiGroups::setLastSelected(KanjiGroup *g)
{
    if (g != nullptr && !isChild(g, true))
        return;

    lastgroup = g;
}

void KanjiGroups::setLastSelected(const QString &name)
{
    if (name.isEmpty())
        lastgroup = nullptr;
    else
        lastgroup = groupFromEncodedName(name);
}

void KanjiGroups::emitGroupDeleted(GroupCategoryBase *p, int index, void *oldaddress)
{
    base::emitGroupDeleted(p, index, oldaddress);
    if (lastgroup == oldaddress)
        lastgroup = nullptr;
}



//-------------------------------------------------------------


Groups::Groups(Dictionary *dict) : dict(dict), wordgroups(this), kanjigroups(this)/*, version(0)*/
{

}

Groups::~Groups()
{
    clear();
}

//int Groups::fileVersion()
//{
//    return version;
//}

void Groups::load(QDataStream &stream)
{
    wordgroups.load(stream);
    kanjigroups.load(stream);
}

void Groups::save(QDataStream &stream) const
{
    wordgroups.save(stream);
    kanjigroups.save(stream);
}

void Groups::clear()
{
    wordgroups.clear();
    kanjigroups.clear();
}

WordGroups& Groups::wordGroups()
{
    return wordgroups;
}

KanjiGroups& Groups::kanjiGroups()
{
    return kanjigroups;
}

const WordGroups& Groups::wordGroups() const
{
    return wordgroups;
}

const KanjiGroups& Groups::kanjiGroups() const
{
    return kanjigroups;
}

Dictionary* Groups::dictionary()
{
    return dict;
}

const Dictionary* Groups::dictionary() const
{
    return dict;
}

void Groups::applyChanges(std::map<int, int> &changes)
{
    wordgroups.applyChanges(changes);
}

void Groups::copy(Groups *src)
{
    wordgroups.copy(&src->wordgroups);
    kanjigroups.copy(&src->kanjigroups);
}

void Groups::processRemovedWord(int windex)
{
    wordgroups.processRemovedWord(windex);
}


//-------------------------------------------------------------

