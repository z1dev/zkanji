/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZTREEVIEW_H
#define ZTREEVIEW_H

#include <QTreeView>
#include <QBasicTimer>

class TreeItem;
class ZAbstractTreeModel;

// Tree view, that works on TreeItem objects instead of QModelIndex values. It can display
// models derived from ZAbstractTreeModel. The selection model is automatically deleted when
// the model is replaced.
class ZTreeView : public QTreeView
{
    Q_OBJECT
signals:
    void currentItemChanged(const TreeItem *current, const TreeItem *previous);
    void itemSelectionChanged();
    void collapsed(const TreeItem *item);
    void expanded(const TreeItem *item);
public:
    ZTreeView(QWidget *parent = nullptr);
    ~ZTreeView();

    ZAbstractTreeModel* model() const;
    virtual void setModel(ZAbstractTreeModel *model);

    // Returns the item currently selected.
    TreeItem* currentItem() const;
    // Returns the parent item of the currently selected item. Returns null if there's no
    // current item, or the current item is in the root.
    TreeItem* currentParent() const;

    // Returns the item at the given local coordinates.
    TreeItem* itemAt(const QPoint &pos) const;
    QRect visualRect(TreeItem *item) const;

    bool isExpanded(const TreeItem *item) const;
    void setExpanded(TreeItem *item, bool expanded);

    // Returns whether the passed item is selected. It should be an item in the tree or the
    // result is undefined.
    bool isSelected(TreeItem *item) const;

    // Returns an unordered list of every selected item in the tree.
    void selection(std::vector<TreeItem*> &result) const;
    // Returns a set of every selected item in the tree.
    void selection(QSet<TreeItem*> &result) const;
public slots:
    void edit(TreeItem *item);
    // Selects the item and deselects everything else.
    void setCurrentItem(TreeItem *item);
    // Called when system settings were changed by the user or on startup.
    void settingsChanged();
protected slots:
    //void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index, const QEvent *e) const override;

    // Forwards the expanded signal.
    void indexExpanded(const QModelIndex &index);
    // Forwards the collapsed signal.
    void indexCollapsed(const QModelIndex &index);
protected:
    virtual bool event(QEvent *e) override;
    virtual void dropEvent(QDropEvent *event) override;
    using QTreeView::edit;
private:
    using QTreeView::collapsed;
    using QTreeView::expanded;

    TreeItem* indexToItem(const QModelIndex &index);

    // Allow deselecting every item in single selection mode. Does not have any effect in
    // multiple selection. False by default.
    bool alldeselect;

    typedef QTreeView  base;
};

class GroupTreeModel;
class GroupBase;
class GroupTreeView : public ZTreeView
{
    Q_OBJECT
signals:
    // Signalled when a new item has been named and accepted.
    void itemCreated();
public:
    GroupTreeView(QWidget *parent = nullptr);
    ~GroupTreeView();

    GroupTreeModel* model();
    const GroupTreeModel* model() const;
    virtual void setModel(GroupTreeModel *model);

    // Changes the current selection to select the passed group or category.
    void select(GroupBase *group);
    // Changes the current selection to contain the passed groups.
    void selectGroups(const std::vector<GroupBase*> &groups);
    // Changes the current selection to have the first item selected in the gree if any is
    // present.
    void selectTop();


    //// Returns whether the current selection corresponds to a category.
    //bool currentIsCategory() const;
    //// Returns whether the current selection corresponds to a group.
    //bool currentIsGroup() const;

    // Returns whether the current selection corresponds to a category.
    bool isCategory(TreeItem *item) const;
    // Returns whether the current selection corresponds to a group.
    bool isGroup(TreeItem *item) const;
    // Returns whether the group or category of the passed item is empty. That is, it does not
    // contain other groups or items.
    //bool itemEmpty(TreeItem *item) const;

    // Returns whether there's a group among the selected items.
    bool hasGroupSelected() const;

    // Returns a list of the selected groups and categories in the tree.
    void selectedGroups(std::vector<GroupBase*> &result) const;

    // Creates a new sub-category item in the passed parent, opening the editor to enter a
    // name for it. If no name is entered when the editor closes, the item is removed and no
    // category is created. If a valid name is entered, the category is added to the parent
    // category.
    void createCategory(TreeItem *parent);
    // Creates a new group item in the passed parent, opening the editor to enter a name for
    // it. If no name is entered when the editor closes, the item is removed and no group is
    // created. If a valid name is entered, the group is added to the parent category.
    void createGroup(TreeItem *parent);
    // Deletes the category or group at the passed tree item.
    //void remove(TreeItem *item);
    // Deletes the passed categories and groups.
    //void remove(const std::vector<TreeItem*> items);

    // Returns whether the currently selected item in the tree is a fully constructed group.
    // If the current item is a group being created and has its editor open, false is
    // returned. If currentmustexist is true, and there is no current item selected or it's
    // the fake group, returns false, otherwise returns true.
    bool currentCompleted(bool currentmustexist);
protected:

    // Makes sure the drag drop indicator is updated while draggong.
    virtual void scrollContentsBy(int dx, int dy) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual bool event(QEvent *event) override;
    virtual void timerEvent(QTimerEvent *event) override;

    virtual void startDrag(Qt::DropActions supportedActions) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent * event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
protected slots:
    virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;
    virtual bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *e) override;
private:
    // Returns the rectangle where the group insertion indicator can be painted during drag
    // and drop.
    QRect _indicatorRect(TreeItem *item, bool after) const;

    // Idle: Not some of the other states.
    // Creation: There is an open editor for setting the name of a new group or category.
    // Dragging: Started a drag-drop operation, and it's possible to hover over an item.
    enum class State { Idle, Creation, Dragging };

    // Before: dragged item should be inserted before dragdest.
    // After: dragged item should be inserted after dragdest.
    // Inside: dragged item should be moved inside dragdest.
    // Undefined: no drag and drop operation is taking place over the tree view.
    enum class DragPos { Before, After, Inside, Undefined };

    State state;
    // Item saved before an action started to be able to restore a state or for checking
    // purposes. It should be set to null after use.
    TreeItem *cachepos;
    // Item being edited with a text editor open above it.
    TreeItem *edititem;
    // Timer used when hovering over a category during a drag and drop, and the category might
    // open if hovering over it long enough.
    QBasicTimer expandtimer;

    // Rectangle to be filled with the insertion indicator color while dragging a group. This
    // is an empty rectangle, if the group is to be added into a hovered category.
    QRect indrect;
    // The tree item used as destination during drag and drop. The dragged item might be
    // dropped into it, or placed before/after it.
    TreeItem *dragdest;
    DragPos dragpos;

    // Saved parameters while drag dropping, to be able to handle auto scrolling that updates
    // the drag and drop indicator position.

    Qt::DropActions savedDropActions;
    const QMimeData *savedMimeData;
    Qt::MouseButtons savedMouseButtons;
    Qt::KeyboardModifiers savedKeyboardModifiers;

    typedef ZTreeView  base;
};


#endif // ZTREEVIEW_H
