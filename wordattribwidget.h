/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDATTRIBWIDGET_H
#define WORDATTRIBWIDGET_H

#include <QtCore>
#include <QWidget>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include "zabstracttreemodel.h"

#include "words.h"

namespace Ui {
    class WordAttribWidget;
}

class WordAttribWidget;
class WordAttribModel : public ZAbstractTreeModel
{
    Q_OBJECT
signals:
    // Signaled when any checkbox has changed in the tree by user interaction. This signal
    // is only emitted once even if a whole subtree is changed when clicking on the parent.
    void checked(const TreeItem *index, bool checked);
public:
    WordAttribModel(WordAttribWidget *owner, bool showglobal);

    // Whether word information field or JLPT level items are listed in the model.
    bool showGlobal() const;
    // Set whether word information field or JLPT level items are listed in the model.
    void setShowGlobal(bool show);

    // Emits the data changed signal for every item in the model.
    void ownerDataChanged();

    //virtual QModelIndex index(int row, int column, const TreeItem *parent) const override;
    //virtual QModelIndex parent(const TreeItem *child) const override;

    virtual intptr_t treeRowData(int row, const TreeItem *parent = nullptr) const override;

    virtual int rowCount(const TreeItem *parent = nullptr) const override;
    virtual bool hasChildren(const TreeItem *parent = nullptr) const override;
    virtual QVariant data(const TreeItem *item, int role = Qt::DisplayRole) const override;
    virtual bool setData(TreeItem *item, const QVariant &value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const TreeItem *item) const override;
private:
    // Collects direct and indirect children of parent in result. The existing items in result
    // are kept.
    void collectChildren(TreeItem *parent, QSet<TreeItem*> &result);

    // Returns whether a given attribute is checked or not. The attribute is
    // specified as the item at attribMap[index].
    bool attribChecked(int index) const;

    // Returns the next sub-item's index of parentindex in attribMap[], or -1 for the last
    // item. Category items are skipped. Set index to -1 to get the first normal sub-item.
    // The items don't have to be direct children of parent.
    int nextSubItem(int parentindex, int index = -1) const;

    //// Emits the dataChanged signal for every item and sub-item under the passed parent index
    //// recursively.
    //void emitCheckedChanged(const TreeItem *parent);

    WordAttribWidget *owner;
    bool global;

    int attribSize;

    typedef ZAbstractTreeModel  base;
};

class WordAttribProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    WordAttribProxyModel(QObject *parent = nullptr);
    ~WordAttribProxyModel();

    void setSourceModel(WordAttribModel *m);
    void setFilterText(QString str);

    virtual QModelIndex mapToSource(const QModelIndex &index) const override;
    virtual QModelIndex mapFromSource(const QModelIndex &index) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
private slots:
    void sourceDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright, const QVector<int> &roles = QVector<int>());
private:
    // Items from the tree shown by the proxy model.
    std::vector<TreeItem*> list;

    typedef QAbstractProxyModel   base;
};

struct WordDefAttrib;
class WordAttribWidget : public QWidget
{
    Q_OBJECT
signals:
    // Signaled when the state of any checkbox in the word attrib tree changes.
    void changed();
public:
    WordAttribWidget(QWidget *parent = nullptr);
    ~WordAttribWidget();

    // Use to initialize the checked state of the attributes before the widget
    // is first shown.
    void setChecked(const WordDefAttrib &types, uchar inf = 0, uchar jlpt = 0);
    // Unchecks every attribute's checkbox.
    void clearChecked();

    const WordDefAttrib& checkedTypes() const;
    uchar checkedInfo() const;
    uchar checkedJLPT() const;

    bool showGlobal() const;
    void setShowGlobal(bool show);

    // Whether a given type is selected in the widget.
    bool typeSelected(uchar index) const;
    // Whether a given note is selected in the widget.
    bool noteSelected(uchar index) const;
    // Whether a given field is selected in the widget.
    bool fieldSelected(uchar index) const;
    // Whether a given dialect is selected in the widget.
    bool dialectSelected(uchar index) const;
    // Whether a given inf field is selected in the widget.
    bool infoSelected(uchar index) const;
    // Whether a given jlpt is selected in the widget.
    bool jlptSelected(uchar index) const;
    // Selects a type of the given index in the widget.
    void setTypeSelected(uchar index, bool select);
    // Selects a note of the given index in the widget.
    void setNoteSelected(uchar index, bool select);
    // Selects a field of the given index in the widget.
    void setFieldSelected(uchar index, bool select);
    // Selects a dialect of the given index in the widget.
    void setDialectSelected(uchar index, bool select);
    // Selects an inf field of the given index in the widget.
    void setInfoSelected(uchar index, bool select);
    // Selects a jlpt of the given index in the widget.
    void setJlptSelected(uchar index, bool select);
    // Selects or deselects all types matching the passed bits.
    void setTypeBits(uint bits, bool select);
    // Selects or deselects all notes matching the passed bits.
    void setNoteBits(uint bits, bool select);
    // Selects or deselects all fields matching the passed bits.
    void setFieldBits(uint bits, bool select);
    // Selects or deselects all dialects matching the passed bits.
    void setDialectBits(ushort bits, bool select);
    // Selects or deselects all info fields matching the passed bits.
    void setInfoBits(uchar bits, bool select);
    // Selects or deselects all jlpt matching the passed bits.
    void setJlptBits(uchar bits, bool select);
protected:
    virtual bool event(QEvent *e) override;
private:
    void textChanged(const QString &str);

    Ui::WordAttribWidget *ui;

    WordDefAttrib attrib;
    uchar inf;
    uchar jlpt;

    WordAttribModel *treemodel;
    WordAttribProxyModel *listmodel;

    typedef QWidget base;
};

#endif // WORDATTRIBWIDGET_H
