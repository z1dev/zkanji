/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZABSTRACTTABLEMODEL_H
#define ZABSTRACTTABLEMODEL_H

#include <QAbstractItemModel>
#include <QSize>
#include "smartvector.h"
#include "ranges.h"

enum class StatusTypes;

// Custom roles for columns in a ZListView. Any model used in the list view can provide these
// values and they will be used.
// AutoSized: enum ColumnAutoSize. The type of auto sizing of a column. If this value is not
//      ColumnAutoSize::NoAuto, the column should be automatically sized when the view scrolls
//      or is resized. The column will fit the widest visible cell, whose size is returned by
//      the itemdelegate's sizeHint(). See comment for ColumnAutoSize. 
//      Default: false.
// FreeSized: bool. Whether the column can be freely resized by the user. If AutoSize is also
//      true for a column, the user made changes are lost each time the view changes.
//      Default: true.
// DefaultWidth: int. The default width of a column. Columns can be autosized, or the table
//      displaying the model's items might save the last set width. In that case this value is
//      just a hint. If this value is negative, the width will equal to the negative height
//      multiplied by this number. A value of 0 will make the default to fit the displayed
//      text in the header.
//      Default: -1
enum class ColumnRoles {
    AutoSized = Qt::UserRole + 1,
    FreeSized,
    DefaultWidth,

    Last
};

// Custom roles for cells in a ZListView.
// CellColor: QColor. The color of a cell's background. When the cell is selected, this color
//      is mixed with the highlight color.
// TextColor: QColor. The color of a cell's text. When the cell is selected, this color is
//      mixed with the color of highlighted text.
//// Flags: int. See the CellFlags enum for possible values.
// Custom: Custom data appropriate for the specific model and ZListView descendant. It can be
//      any type the view, model and possibly the delegate understand.
enum class CellRoles {
    CellColor = (int)ColumnRoles::Last,
    TextColor,
    //Flags,
    Custom,

    User,
};

// The type of auto sizing of a column.
// NoAuto: The column is not auto sized.
// Auto: The column's width is updated when the ZListView is shown, resized or its contents
//      scroll, to fit the contents of the widest cell currently on screen. The size is
//      returned by the item delegate sizeHint() function.
// HeaderAuto: The column is auto sized, just like with Auto, but the content of the header
//      is taken into account as well.
// Stretched: The column, with every other stretched columns, should be resized to take up the
//      rest of the available space when the view scrolls or is resized. Every column is first
//      resized based on default size, and if there's still space left, it's evenly
//      distributed among the stretched columns.
// StretchedAuto: A combination of stretched and auto. Sizes the column to at least the width
//      of the visible rows' contents. If there's still space left, the row is stretched.
enum class ColumnAutoSize {
    NoAuto,
    Auto,
    HeaderAuto,
    Stretched,
    StretchedAuto
};


// Implementing drag and drop in models used in ZListView.
//
// Drop:
// Call: setAcceptDrops(true) 
// Implement in models: supportedDropActions(), dropMimeData(), canDropMimeData()
// canDropMimeData() will be called more times depending on its return value:
// - when near the middle of a row, with row and column set to -1 and parent set to a valid
// row to test whether dropping onto that parent is possible.
// - with row set to a value of 0 or above, and parent set to invalid index, it tests whether
// dropping in front of another row or at the end of table is possible.
// - row and column set to -1 and parent set to an invalid index tests whether dropping on the
// table, but not above or into another row should work.
//
// dropMimeData() should either call canDropMimeData() before trying to drop, or check for the
// row and column and parent again.
// When the result for supportedDropActions() is MoveAction and dropping to the source of the
// drag, the model must handle the move. No remove will be called.
//
// Drag:
// Call: setDragEnabled(true)
// Implement in models: supportedDragActions(), mimeData()

// Base class for derived models used in a ZListView. Returns default values for ColumnRoles
// values in data and itemData. 
class ZAbstractTableModel : public QAbstractTableModel
{
    Q_OBJECT
signals:
    // Signal replacements for models. Similar to rowsRemoved, rowsInserted and rowsMoved.

    void rowsWereRemoved(const smartvector<Range> &ranges);
    void rowsWereInserted(const smartvector<Interval> &intervals);
    void rowsWereMoved(const smartvector<Range> &ranges, int pos);

    // Notifies the view that has an assigned status bar that it should update the status
    // label texts.
    void statusChanged();
public:
    ZAbstractTableModel(QObject *parent = nullptr);
    virtual ~ZAbstractTableModel();

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    //virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    // Same as data, but always returns it for column 0.
    QVariant rowData(int row, int role = Qt::DisplayRole) const;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const;

    // The number of status widgets needed on a status bar for this view. Excludes the number
    // of the row count widget, as it's always included.
    virtual int statusCount() const;
    // Type of status widget at statusindex position on a status bar.
    virtual StatusTypes statusType(int statusindex) const;
    // Text of status labels on widget at statusindex position on a status bar. Negative
    // value means the title text for this status widget. In that case the labelindex is
    // ignored. There can be one or two labels after the title text, denoted by label index,
    // depending on statusType(). Rowpos says which value the text should refer to.
    virtual QString statusText(int statusindex, int labelindex, int rowpos) const;
    // Size of status label on widget at statusindex position on a status bar. Labelindex can
    // be negative to mean the title label.
    virtual int statusSize(int statusindex, int labelindex) const;
    // Whether the first status label after the title label is text-aligned right or not. Only
    // called when the statusType() is TitleValue.
    virtual bool statusAlignRight(int statusindex) const;

    // Signal replacements:
    void signalRowsRemoved(const smartvector<Range> &ranges);
    void signalRowsInserted(const smartvector<Interval> &intervals);
    void signalRowsMoved(const smartvector<Range> &ranges, int pos);
private:
    virtual Qt::DropActions supportedDropActions() const override final;

    using QAbstractTableModel::beginInsertRows;
    using QAbstractTableModel::endInsertRows;
    using QAbstractTableModel::beginRemoveRows;
    using QAbstractTableModel::endRemoveRows;
    using QAbstractTableModel::beginMoveRows;
    using QAbstractTableModel::endMoveRows;

    using QAbstractTableModel::rowsAboutToBeInserted;
    using QAbstractTableModel::rowsInserted;
    using QAbstractTableModel::rowsAboutToBeRemoved;
    using QAbstractTableModel::rowsRemoved;
    using QAbstractTableModel::rowsAboutToBeMoved;
    using QAbstractTableModel::rowsMoved;


    typedef QAbstractTableModel base;
};


class ZAbstractProxyTableModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    ZAbstractProxyTableModel(QObject *parent = nullptr);
    ~ZAbstractProxyTableModel();

    virtual void setSourceModel(ZAbstractTableModel *newmodel);
    ZAbstractTableModel* sourceModel() const;

    virtual QModelIndex mapToSource(const QModelIndex &index) const = 0;
    virtual QModelIndex mapFromSource(const QModelIndex &index) const = 0;

    virtual bool submit() override;
    virtual void revert() override;

    virtual QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &proxyIndex) const override;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::DisplayRole) override;
    virtual bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;

    virtual int statusCount() const override;
    virtual StatusTypes statusType(int statusindex) const override;
    virtual QString statusText(int statusindex, int labelindex, int rowpos) const override;
    virtual int statusSize(int statusindex, int labelindex) const override;
    virtual bool statusAlignRight(int statusindex) const override;

    virtual QModelIndex buddy(const QModelIndex &index) const override;
    virtual bool canFetchMore(const QModelIndex &parent) const override;
    virtual void fetchMore(const QModelIndex &parent) override;
    virtual void sort(int column, Qt::SortOrder order) override;
    virtual QSize span(const QModelIndex &index) const override;
    virtual bool hasChildren(const QModelIndex &parent) const override;
    virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    //virtual QStringList mimeTypes() const override;
    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;

    virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) override;
private slots:
    void sourceDestroyed();
private:
    ZAbstractTableModel *model;

    typedef ZAbstractTableModel base;
};



#endif // ZABSTRACTTABLEMODEL_H

