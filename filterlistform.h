/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
    FilterListForm(WordFilterConditions *conditions, const QRect &r, QWidget *parent = nullptr);
    ~FilterListForm();
signals:
    // Signaled when the inclusion of a filter has changed from exclude/include or ignore.
    void includeChanged(int index, Inclusion oldinclude);
    // Signaled when the "word in example" or "word in group" condition changes.
    void conditionChanged();
public slots:
    void on_editButton_clicked(bool checked);
    void on_delButton_clicked();
    void on_upButton_clicked();
    void on_downButton_clicked();

    void on_nameEdit_textEdited(const QString &text);

    void editInitiated(int row);
    void deleteInitiated(int row);
    void currentRowChanged();

    void discardClicked();
    void saveClicked();
    void filterChanged(int index);
protected:
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;
private:
    // Retranslates button texts.
    void translateTexts();

    // Shows or hides the filter editor while resizing the window.
    void toggleEditor(bool show);

    // Checks whether the name in the edit box has already been specified for a filter, and
    // returns the index of that filter if found. Returns -1 if the name is empty or doesn't
    // match any filters.
    // Set msgbox to true to show a message box when the name is empty or conflicts with
    // an unselected filter. In this case returns 0 if a message box is shown, otherwise 1.
    int nameIndex(bool msgbox = true);

    // Called on change to update the enabled state of the apply button.
    void allowApply();

    Ui::FilterListForm *ui;

    // Width of the editor widget used to calculate new window size when it's shown.
    int editwidth;
    // Index of filter currently being edited.
    int filterindex;

    WordFilterConditions *conditions;

    typedef DialogWindow    base;
};

#endif // FILTERLISTFORM_H
