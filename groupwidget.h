/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPSELECTOR_H
#define GROUPSELECTOR_H

#include <QWidget>

namespace Ui {
    class GroupWidget;
}

class GroupCategoryBase;
class GroupTreeModel;
class Dictionary;
class TreeItem;
class GroupBase;
class QToolButton;
class QAbstractButton;

class GroupWidget : public QWidget
{
    Q_OBJECT
signals:
    // Signalled when a new item is selected in the groups tree view and that item is a fully
    // created group or category. Also signalled when a new item is created and named
    // correctly.
    void currentItemChanged(GroupBase *current);
    // Emited when the selection in the groups tree changes.
    void selectionChanged();
public:
    enum Modes { Words, Kanji };

    explicit GroupWidget(QWidget *parent = nullptr);
    ~GroupWidget();

    // Whether multiple items can be selected in the group tree. Even if this is true, when
    // filter searching groups, multi selection is disabled.
    bool multiSelect() const;
    // Set whether multiple items can be selected in the group tree. Even if this is set to
    // true, when filter searching groups, multi selection is disabled.
    void setMultiSelect(bool mul);

    // Lists categories in the group widget. Creating groups will be disabled.
    void showOnlyCategories();
    // Whether only categories are listed in the group widget.
    bool onlyCategories() const;

    //void makeModeSpace(const QSize &size);

    void setDictionary(Dictionary *dict);
    Dictionary* dictionary();

    void setMode(Modes newmode);
    // Returns the current item in the group tree if any item is current.
    GroupBase* current();

    // Returns the selected group, if only a single group is selected in the tree. If multiple
    // groups are selected, or allowcateg is false and a category is selected, returns null.
    GroupBase* singleSelected(bool allowcateg = false);

    // Selects the passed group in the groups tree. Only this group will be selected even in
    // multiple selection mode.
    void selectGroup(GroupBase *group);
    // Selects every group in the tree passed in the list. The groups' parents will be
    // expanded automatically. The previous selection is replaced.
    void selectGroups(const std::vector<GroupBase*> &groups);

    // Fills result with the list of selected groups. The order of the groups is the same as
    // their order in the tree view, with groups in categories coming first, if the category
    // is listed above the other selected groups. Groups in closed tree branches are ignored.
    void selectedGroups(std::vector<GroupBase*> &result);

    // Adds a button above the groups tree next to the default buttons, and returns it. The
    // button will be parented to this widget. The value of label will either be used as a
    // string for the button to show, or as an icon from a resource, if it starts with ":/".
    //QToolButton* addButton(const QString &label);

    // Adds a button above the groups tree, next to the default buttons, and before any
    // control widget added with addControlWidget(). The buttons will be placed next to each
    // other without padding. The button should be similar in size to the other buttons to
    // keep the layout.
    void addButtonWidget(QAbstractButton *button);

    // Adds a widget above the groups tree next to the buttons and any custom button added
    // with addButtonWidget(). The widget should be similar in height to the buttons to keep
    // the layout. Set rightAlign to true, to add the widget sticking to the right side of the
    // group widget. 
    void addControlWidget(QWidget *widget, bool rightAlign = false);
protected:
    virtual bool event(QEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;
private:
    // Resets the groups tree view after a dictionary or display mode change, to show the
    // groups for the appropriate mode.
    void updateGroups();

    Ui::GroupWidget *ui;

    Dictionary *dict;
    Modes mode;
    GroupTreeModel *groupmodel;
    bool multisel;
    bool onlycateg;

    typedef QWidget base;
private slots:
    void on_groupsTree_currentItemChanged(const TreeItem *, const TreeItem *);
    void on_groupsTree_itemSelectionChanged();
    void on_groupsTree_itemCreated();
    //void on_groupsTree_collapsed(const TreeItem *item);
    void on_btnAddCateg_clicked();
    void on_btnAddGroup_clicked();
    void on_btnDelGroup_clicked();
    void on_filterEdit_textChanged(const QString &text);
};

#endif // GROUPSELECTOR_H

