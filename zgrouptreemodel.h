/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZGROUPTREEMODEL_H
#define ZGROUPTREEMODEL_H

#include <QSet>
#include "zabstracttreemodel.h"
#include "ranges.h"

class GroupCategoryBase;
class GroupBase;
class Dictionary;

enum class GroupTypes;

// Tree view model listing the groups of a top level word or kanji category. Categories with
// no sub-categories display an item with a message. When the user begins to add a category
// or group the message item is hidden, and a new empty item is shown. The item's name can be
// edited by the user to create the group or category. Call startAddCategory() or
// startAddGroup() to add the empty item. The tree view must start editing the item's name.
class GroupTreeModel : public ZAbstractTreeModel
{
    Q_OBJECT

public:
    GroupTreeModel(GroupCategoryBase *root, bool showplaceholder = true, bool onlycateg = false, QObject *parent = nullptr);
    GroupTreeModel(GroupCategoryBase *root, QString filterstr, QObject *parent = nullptr);
    virtual ~GroupTreeModel();

    virtual int rowCount(const TreeItem *parent = nullptr) const override;
    virtual QVariant data(const TreeItem *item, int role = Qt::DisplayRole) const override;
    virtual bool setData(TreeItem *item, const QVariant &value, int role = Qt::EditRole) override;
    virtual intptr_t treeRowData(int row, const TreeItem *parent = nullptr) const override;
    virtual Qt::ItemFlags flags(const TreeItem *item) const override;
    virtual bool hasChildren(const TreeItem *parent) const override;

    virtual Qt::DropActions supportedDropActions() const override;
    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
    virtual QStringList mimeTypes() const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, TreeItem *parent) override;

    //virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    //virtual QModelIndex parent(const QModelIndex &index) const;
    //virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    //void setGroups(GroupCategoryBase *group);

    Dictionary* dictionary() const;

    GroupTypes groupType() const;

    // The model only displays a root item and category items, no groups.
    bool onlyCategories() const;

    // The model lists a subset of all the groups that match the filterstr passed to the
    // constructor.
    bool isFiltered() const;

    // Returns the root category for every group represented by the model.
    GroupCategoryBase* rootCategory();
    // Returns the root category for every group represented by the model.
    const GroupCategoryBase* rootCategory() const;

    // Creates a fake sub-category that's name can be edited to create a category in the
    // model's group. Returns the item for the created category.
    TreeItem* startAddCategory(TreeItem *parent);

    // Creates a fake group that's name can be edited to create a category in the model's
    // group. Returns the item for the created group.
    TreeItem* startAddGroup(TreeItem *parent);

    // Returns the tree item representing the passed group. The implementation has to step
    // through every child item of the root TreeItem, until it's found.
    TreeItem* findTreeItem(const GroupBase *which);

    // Only use to remove the fake item created in categories.
    void remove(TreeItem *which);

    //void remove(const std::vector<TreeItem*> which);

    bool isCategory(const TreeItem *which) const;
protected:
    virtual bool event(QEvent *e) override;
private slots:
    virtual void categoryAdded(GroupCategoryBase *parent, int index);
    virtual void groupAdded(GroupCategoryBase *parent, int index);
    virtual void categoryAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupCategoryBase *cat);
    virtual void categoryDeleted(GroupCategoryBase *parent, int index, void *oldptr);
    virtual void groupAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupBase *grp);
    virtual void groupDeleted(GroupCategoryBase *parent, int index, void *oldptr);
    virtual void categoryMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex);
    virtual void groupMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex);
    virtual void categoriesMoved(GroupCategoryBase *srcparent, const Range &range, GroupCategoryBase *destparent, int destindex);
    virtual void groupsMoved(GroupCategoryBase *srcparent, const Range &range, GroupCategoryBase *destparent, int destindex);
    virtual void categoryRenamed(GroupCategoryBase *parent, int index);
    virtual void groupRenamed(GroupCategoryBase *parent, int index);
private:

    void init();

    // The root group category managing all the child groups.
    GroupCategoryBase *root;

    // Showing the fake items only to display the "add new category or groups..." text.
    bool placeholder;
    // Whether only categories (and a root) is listed.
    bool onlycateg;

    // When filtering items, lists the groups in alphabetic order to be included.
    std::vector<GroupBase*> filterlist;
    // Whether filtering items or not. The model might still be filtered even if filterlist is
    // empty.
    bool filtered;
    // The string used to filter group names.
    QString filterstr;

    enum AddMode { NoAdd, CategoryAdd, GroupAdd };
    // Set to a value other than NoAdd when a new category or group is being added in the view
    // showing the model. Calling startAddCategory() or startAddGroup() will set this value, while
    // a call to setData() or remove() resets it.
    AddMode addmode;
    // The parent category of the item being added with startAddCategory() or startAddGroup().
    TreeItem *editparent;

    // Set to true when a new category or group has been added or an old one removed by this
    // model. Only other models should handle them as the slots add and remove tree items.
    bool ignoresignals;

    typedef ZAbstractTreeModel   base;
};

class CheckedGroupTreeModel : public GroupTreeModel
{
    Q_OBJECT    
public:
    CheckedGroupTreeModel(GroupCategoryBase *root, bool showplaceholder = true, bool onlycateg = false, QObject *parent = nullptr);
    CheckedGroupTreeModel(GroupCategoryBase *root, QString filterstr, QObject *parent = nullptr);
    virtual ~CheckedGroupTreeModel();

    // Returns true if any groups is checked in the model.
    bool hasChecked() const;
    // Returns the checked groups in result, overwriting its contents.
    void checkedGroups(QSet<GroupBase*> &result) const;

    virtual QVariant data(const TreeItem *item, int role = Qt::DisplayRole) const override;
    virtual bool setData(TreeItem *item, const QVariant &value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const TreeItem *item) const override;
private slots:
    virtual void categoryAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupCategoryBase *cat) override;
    virtual void groupAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupBase *grp) override;
private:
    // Fills dest with every children of parent recursively.
    void collectCategoryChildren(TreeItem *parent, QSet<TreeItem*> &dest);

    // Which categories and groups are checked.
    QSet<GroupBase*> checked;
    // Whether categories are checked normally. When this is false, categories check all their
    // children and only show their checked/unchecked state depending on their children's
    // check state. (Including partially checked.)
    bool checkcateg;

    typedef GroupTreeModel  base;
};

#endif //  ZGROUPTREEMODEL_H

