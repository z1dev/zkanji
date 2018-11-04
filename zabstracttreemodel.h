/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZABSTRACTTREEMODEL_H
#define ZABSTRACTTREEMODEL_H

#include <QAbstractItemModel>

class TreeItem
{
public:
    typedef size_t  size_type;

    TreeItem(const TreeItem &other) = delete;
    TreeItem& operator=(const TreeItem &other) = delete;
    TreeItem();
    TreeItem(int newrow, TreeItem *itemparent, intptr_t itemdata);
    TreeItem(TreeItem &&other);
    TreeItem& operator=(TreeItem &&other);
    ~TreeItem();

    bool empty() const;
    size_type size() const;
    int row() const;
    int indexOf(const TreeItem* item) const;
    TreeItem* parent() const;
    // Returns whether the passed tree item is the direct or indirect parent of this one. If
    // the item and this are the same, returns false.
    bool hasParent(TreeItem *item) const;

    const TreeItem* items(int index) const;
    TreeItem* items(int index);

    intptr_t userData() const;
    void setUserData(intptr_t data);
private:
    // When returns true, the parent parameter is the root of the owner tree model.
    // Items in the root cannot be used in some operations. The parent() function for
    // example returns nullptr if the parent is the root.
    bool inRoot() const;

    // Returns the parent even if it's the root.
    TreeItem* realParent() const;

    void clear();
    void reserve(int cnt);
    int capacity();

    // Deletes cnt number of child tree items starting at first.
    void removeItems(int first, int cnt);
    // Removes cnt number of child tree items starting at first, and places them in dest. The
    // item parent value is unchanged until they are inserted with insertItems().
    void takeItems(int first, int cnt, std::vector<TreeItem*> &dest);
    // Inserts tree items as child items, with a new starting index of pos. The parent of the
    // items is updated if updateparent is true.
    void insertItems(int pos, const std::vector<TreeItem*> &items, bool updateparent);
    TreeItem* addChild(intptr_t childdata);

    // Used for caching row index.
    mutable int oldrow; 
    // Parent.
    TreeItem *pr;
    // User data.
    intptr_t dat;

    std::vector<TreeItem*> list;

    friend class ZAbstractTreeModel;
};

// Model used in ZTreeView widgets. The model is empty when first created. If its derived
// class holds data at the end of its constructor, it must reset itself as the last operation.
// The model holds TreeItem objects, starting at a root TreeItem. It's filled when reset() is
// called, and between beginInsertRows() and endInsertRows(). TreeItem objects are removed
// between the calls of beginRemoveRows() and endRemoveRows(). Derived classes implement the
// abstract treeRowData() function that returns a value they can use to identify their data.
// This data is stored in TreeItem objects and can be retrieved calling their userData().
class ZAbstractTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    typedef size_t  size_type;

    ZAbstractTreeModel(QObject * parent = nullptr);

    // Calls beginResetModel() and endResetModel().
    void reset();

    // Returns an item in the root.
    TreeItem* items(int index);
    // Returns an item in the root.
    const TreeItem* items(int index) const;
    // Returns an item in the given parent. Passing nullptr in parent will return the same as
    // items() with the passed index. It's not checked whether parent is part of this tree or
    // not.
    TreeItem* getItem(TreeItem *parent, int index);
    // Returns an item in the given parent. Passing nullptr in parent will return the same as
    // items() with the passed index. It's not checked whether parent is part of this tree or
    // not.
    const TreeItem* getItem(const TreeItem *parent, int index) const;
    // Number of items in the root. This is faster than calling rowCount() because
    // it always requests the count from the derived implementation, while this
    // uses the already built table.
    size_type size() const;
    // Number of items under a given parent item. Passing nullptr in parent will return the
    // same as size(). It's not checked whether the passed parent is part of this tree or not.
    size_type getSize(const TreeItem *parent) const;

    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, TreeItem *parent);

    // Return number of columns in the tree. By default returns 1.
    virtual int columnCount(const TreeItem *parent = nullptr) const;

    virtual Qt::ItemFlags flags(const TreeItem *item) const;

    // Return whether the given parent item has sub-items.
    // See QAbstractItemModel::hasChildren() for details.
    virtual bool hasChildren(const TreeItem *parent) const;

    virtual bool setData(TreeItem *item, const QVariant & value, int role = Qt::EditRole);

    /* Pure virtual functions that must be overriden. */

    // Return number of rows belonging to a parent tree item.
    virtual int rowCount(const TreeItem *parent = nullptr) const = 0;
    // Return item data requested by the model with the given role.
    // See QAbstractItemModel::data() for details.
    virtual QVariant data(const TreeItem *item, int role = Qt::DisplayRole) const = 0;

    // Returns custom user data that helps derived classes identify a given row in a parent.
    // The user data can be retrieved by calling TreeItem::userData() in the other functions.
    virtual intptr_t treeRowData(int row, const TreeItem *parent = nullptr) const = 0;

    // Returns the QAbstractItemIndex::index() equivalent for the tree item.
    virtual QModelIndex index(const TreeItem *item) const;
signals:
    // Notifies subscribers that data has changed in items between top and
    // bottom inclusive. The items must have the same parent. Don't pass null.
    // ZAbstractTreeModel does not emit this signal. It must be done by
    // derived classes in their setData implementation. ZAbstractTreeModel
    // will in turn emit the QAbstractItemModel::dataChanged(), so both
    // signals are valid to handle.
    void dataChanged(const TreeItem *top, const TreeItem *bottom, const QVector<int> &roles = QVector<int>());
protected:
    void beginResetModel();
    void endResetModel();
    void beginInsertRows(TreeItem *parent, int first, int last);
    void endInsertRows();
    void beginRemoveRows(TreeItem *parent, int first, int last);
    void endRemoveRows();
    void beginMoveRows(TreeItem *parent, int first, int last, TreeItem *destparent, int destindex);
    void endMoveRows();

    const TreeItem* itemFromIndex(const QModelIndex &index) const;
    TreeItem* itemFromIndex(const QModelIndex &index);
private:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
    virtual QModelIndex parent(const QModelIndex &index) const override final;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override final;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override final;
    virtual bool hasChildren(const QModelIndex &parent) const override final;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override final;

    using QAbstractItemModel::index;

    // Creates tree items between the given range in the given parent at the specified position
    // and fills them with child items if they exist.
    void fillItemRange(TreeItem *parent, int insertpos, int cnt);

    TreeItem root;

    // Range of rows for insertion or other operation in progress when the begin***/end*** functions are called.
    TreeItem *rowparent;
    int rowfirst;
    int rowlast;
    // Parent item used as destination in a beginMoveRows/endMoveRows.
    TreeItem *destrowparent;
    int destrowindex;

#ifdef _DEBUG
    // The current operation in progress when calling the begin***/end*** functions. It's a bug
    // if an operation is in progress and another is started, but it is only checked during
    // debug.
    enum Operation { NoOp, Insertion, Removal, Resetting, Moving };
    Operation op;
#endif

    typedef QAbstractItemModel  base;

    using base::dataChanged;
    using QAbstractItemModel::dropMimeData;

    friend class ZTreeView;

private slots:
    // The original implementation has QModelIndex items instead of TreeItem, so the dataChanged
    // signal must be forwarded to the base class. Emits the original data changed signal, with
    // indexes created from the passed TreeItem values.
    void forwardDataChanged(const TreeItem *top, const TreeItem *bottom, const QVector<int> &roles);
};

#endif // ZABSTRACTTREEMODEL_H
