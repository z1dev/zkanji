/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FILTERLISTFORM_H
#define FILTERLISTFORM_H

#include <QtCore>
#include <QMainWindow>
#include <QDrag>

#include "zabstracttablemodel.h"
#include "zlistviewitemdelegate.h"
#include "dialogwindow.h"

namespace Ui {
    class FilterListForm;
}

enum class Inclusion : uchar;

struct WordFilterConditions;
class ZListView;

// Model for listing word dictionary filters in a ZListView. Displays the name of the filter
// in column 0, included/excluded in column 1 and 2, edit button in column 3 and delete button
// in column 4. When creating the model, the parent must be a ZListView descendant and cannot
// be null.
class FilterListModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    FilterListModel(WordFilterConditions *conditions, ZListView *parent);
    ~FilterListModel();

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &val, int role = Qt::EditRole) override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
signals:
    // Signaled when the inclusion of a filter has changed from exclude/include or ignore.
    void includeChanged(int row, Inclusion oldinc);
    // Signaled when the "word in example" or "word in group" condition changes.
    void conditionChanged();

    void editInitiated(int row);
    void deleteInitiated(int row);
public slots:
    void filterErased(int row);
    void filterCreated();
    void filterRenamed(int row);
    void filterMoved(int index, int to);
private:
    WordFilterConditions *conditions;

    typedef ZAbstractTableModel base;
};

//// Item delegate used with FilterListModel. Overrides the checkbox painting, replacing the
//// boxes with images generated from SVG.
//class FilterListItemDelegate : public ZListViewItemDelegate
//{
//    Q_OBJECT
//public:
//    FilterListItemDelegate(ZListView *parent = nullptr);
//    ~FilterListItemDelegate();
//
//    virtual QRect checkBoxRect(const QModelIndex &index) const override;
//    virtual QRect checkBoxHitRect(const QModelIndex &index) const override;
//protected:
//    virtual QRect drawCheckBox(QPainter *p, const QModelIndex &index) const override;
//private:
//    typedef ZListViewItemDelegate    base;
//};


class FilterListForm : public DialogWindow
{
    Q_OBJECT
public:
    FilterListForm(WordFilterConditions *conditions, QWidget *parent = nullptr);
    ~FilterListForm();

    void showAt(const QRect &rect);

    void updatePosition(const QRect &r);
signals:
    // Signaled when the inclusion of a filter has changed from exclude/include or ignore.
    void includeChanged(int index, Inclusion oldinclude);
    // Signaled when the "word in example" or "word in group" condition changes.
    void conditionChanged();
public slots:
    void on_addButton_clicked();
    void on_delButton_clicked();
    void on_editButton_clicked();

    void editInitiated(int row);
    void deleteInitiated(int row);
    void currentRowChanged();
protected:
    virtual void changeEvent(QEvent *e) override;
private:
    Ui::FilterListForm *ui;

    WordFilterConditions *conditions;

    typedef DialogWindow    base;
};

#endif // FILTERLISTFORM_H
