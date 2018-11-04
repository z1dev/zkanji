/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "zabstracttablemodel.h"
#include "zlistview.h"
#include "zstatusbar.h"


//-------------------------------------------------------------


ZAbstractTableModel::ZAbstractTableModel(QObject *parent) : base(parent)
{
    ;
}

ZAbstractTableModel::~ZAbstractTableModel()
{
    ;
}

//QVariant ZAbstractTableModel::data(const QModelIndex &index, int role) const
//{
//    return QVariant();
//}

QVariant ZAbstractTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case (int)ColumnRoles::AutoSized:
        return (int)ColumnAutoSize::NoAuto;
    case (int)ColumnRoles::FreeSized:
        return true;
    case (int)ColumnRoles::DefaultWidth:
        return -1;
    }

    return base::headerData(section, orientation, role);
}

QMap<int, QVariant> ZAbstractTableModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles = base::itemData(index);
    QVariant variantData;

    //variantData = data(index, (int)CellRoles::Flags);
    //if (variantData.isValid())
    //    roles.insert((int)CellRoles::Flags, variantData);
    variantData = data(index, (int)CellRoles::CellColor);
    if (variantData.isValid())
        roles.insert((int)CellRoles::CellColor, variantData);
    variantData = data(index, (int)CellRoles::TextColor);
    if (variantData.isValid())
        roles.insert((int)CellRoles::TextColor, variantData);

    return roles;
}

QVariant ZAbstractTableModel::rowData(int row, int role) const
{
    return data(index(row, 0), role);
}

bool ZAbstractTableModel::canDropMimeData(const QMimeData * /*data*/, Qt::DropAction /*action*/, int row, int column, const QModelIndex &/*parent*/) const
{
    return (row != -1 && column != -1);
}

Qt::DropActions ZAbstractTableModel::supportedDragActions() const
{
    return 0;
}

Qt::DropActions ZAbstractTableModel::supportedDropActions(bool /*samesource*/, const QMimeData * /*mime*/) const
{
    return 0;
}

int ZAbstractTableModel::statusCount() const
{
    return 0;
}

StatusTypes ZAbstractTableModel::statusType(int /*statusindex*/) const
{
    return StatusTypes::TitleValue;
}

QString ZAbstractTableModel::statusText(int /*statusindex*/, int /*labelindex*/, int /*kanjipos*/) const
{
    return QString();
}

int ZAbstractTableModel::statusSize(int /*statusindex*/, int /*labelindex*/) const
{
    return 0;
}

bool ZAbstractTableModel::statusAlignRight(int /*statusindex*/) const
{
    return false;
}

void ZAbstractTableModel::signalRowsRemoved(const smartvector<Range> &ranges)
{
    int rcnt = rowCount();
    beginRemoveRows(QModelIndex(), rcnt, rcnt + _rangeSize(ranges) - 1);
    emit rowsWereRemoved(ranges);
    endRemoveRows();
}

void ZAbstractTableModel::signalRowsInserted(const smartvector<Interval> &intervals)
{
    int rcnt = rowCount();
    beginInsertRows(QModelIndex(), rcnt - _intervalSize(intervals), rcnt - 1);
    emit rowsWereInserted(intervals);
    endInsertRows();
}

void ZAbstractTableModel::signalRowsMoved(const smartvector<Range> &ranges, int pos)
{
    emit rowsWereMoved(ranges, pos);
}

Qt::DropActions ZAbstractTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


//-------------------------------------------------------------


ZAbstractProxyTableModel::ZAbstractProxyTableModel(QObject *parent) : base(parent), model(nullptr)
{

}

ZAbstractProxyTableModel::~ZAbstractProxyTableModel()
{

}

void ZAbstractProxyTableModel::setSourceModel(ZAbstractTableModel *newmodel)
{
    if (model == newmodel)
        return;

    if (model != nullptr)
        disconnect(model, &ZAbstractTableModel::destroyed, this, &ZAbstractProxyTableModel::sourceDestroyed);

    model = newmodel;

    if (model != nullptr)
        connect(model, &ZAbstractTableModel::destroyed, this, &ZAbstractProxyTableModel::sourceDestroyed);
}

ZAbstractTableModel* ZAbstractProxyTableModel::sourceModel() const
{
    return model;
}

void ZAbstractProxyTableModel::sourceDestroyed()
{
    model = nullptr;
}

bool ZAbstractProxyTableModel::submit()
{
    if (model != nullptr)
        return model->submit();
    return false;
}

void ZAbstractProxyTableModel::revert()
{
    if (model != nullptr)
        model->revert();
}

QVariant ZAbstractProxyTableModel::data(const QModelIndex &proxyIndex, int role) const
{
    if (model)
        return model->data(mapToSource(proxyIndex), role);
    return QVariant();
}


QVariant ZAbstractProxyTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (model)
        return model->headerData(orientation == Qt::Horizontal ? section : mapToSource(index(section, 0)).row(), orientation, role);
    return QVariant();
}

QMap<int, QVariant> ZAbstractProxyTableModel::itemData(const QModelIndex &proxyIndex) const
{
    if (model)
        return model->itemData(mapToSource(proxyIndex));
    return QMap<int, QVariant>();
}

Qt::ItemFlags ZAbstractProxyTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    if (model)
        return model->flags(mapToSource(index));
    return Qt::ItemFlags();
}

bool ZAbstractProxyTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (model)
        return model->setData(mapToSource(index), value, role);
    return false;
}

bool ZAbstractProxyTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (model)
        return model->setHeaderData(orientation == Qt::Horizontal ? mapToSource(index(0, section)).column() : mapToSource(index(section, 0)).row(), orientation, value, role);
    return false;
}

bool ZAbstractProxyTableModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if (model)
        return model->setItemData(mapToSource(index), roles);
    return false;
}

int ZAbstractProxyTableModel::statusCount() const
{
    return model->statusCount();
}

StatusTypes ZAbstractProxyTableModel::statusType(int statusindex) const
{
    return model->statusType(statusindex);
}

QString ZAbstractProxyTableModel::statusText(int statusindex, int labelindex, int rowpos) const
{
    return model->statusText(statusindex, labelindex, mapToSource(index(rowpos, 0)).row());
}

int ZAbstractProxyTableModel::statusSize(int statusindex, int labelindex) const
{
    return model->statusSize(statusindex, labelindex);
}

bool ZAbstractProxyTableModel::statusAlignRight(int statusindex) const
{
    return model->statusAlignRight(statusindex);
}

QModelIndex ZAbstractProxyTableModel::buddy(const QModelIndex &index) const
{
    if (model)
        return mapFromSource(model->buddy(mapToSource(index)));
    return QModelIndex();
}

bool ZAbstractProxyTableModel::canFetchMore(const QModelIndex &parent) const
{
    if (model)
        return model->canFetchMore(mapToSource(parent));
    return false;
}

void ZAbstractProxyTableModel::fetchMore(const QModelIndex &parent)
{
    if (model)
        model->fetchMore(mapToSource(parent));
}

void ZAbstractProxyTableModel::sort(int column, Qt::SortOrder order)
{
    if (model)
        model->sort(column, order);
}

QSize ZAbstractProxyTableModel::span(const QModelIndex &index) const
{
    if (model)
        return model->span(mapToSource(index));
    return QSize();
}

bool ZAbstractProxyTableModel::hasChildren(const QModelIndex &parent) const
{
    if (model)
    {
        // For some reason the hasChildren in QAbstractTableModel is private, so the original
        // behavior must be duplicated here.

        QModelIndex index = mapToSource(parent);

        if (index.model() == model || !index.isValid())
            return model->rowCount(index) > 0 && model->columnCount(index) > 0;
        return false;
    }
    return false;
}

QModelIndex ZAbstractProxyTableModel::sibling(int row, int column, const QModelIndex &ix) const
{
    return index(row, column, ix.parent());
}

//QStringList ZAbstractProxyTableModel::mimeTypes() const
//{
//    if (model)
//        return model->mimeTypes();
//    return QStringList();
//}

Qt::DropActions ZAbstractProxyTableModel::supportedDragActions() const
{
    if (model)
        return model->supportedDragActions();
    return Qt::DropActions();
}

Qt::DropActions ZAbstractProxyTableModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (model)
        return model->supportedDropActions(samesource, mime);
    return Qt::DropActions();
}

QMimeData* ZAbstractProxyTableModel::mimeData(const QModelIndexList &indexes) const
{
    if (model)
    {
        QModelIndexList srcindexes;
        for (auto it = indexes.cbegin(); it != indexes.cend(); ++it)
            srcindexes << mapToSource(*it);
        return model->mimeData(srcindexes);
    }

    return nullptr;
}

bool ZAbstractProxyTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (model)
    {
        QModelIndex srcparent = parent;
        if (row == -1 && column == -1)
            srcparent = mapToSource(parent);
        else if (row == rowCount(parent))
        {
            srcparent = mapToSource(parent);
            row = model->rowCount(srcparent);
            column = -1;
        }
        else
        {
            QModelIndex ix = mapToSource(index(row, column, parent));
            srcparent = mapToSource(parent);
            row = ix.row();
            column = ix.column();
        }

        return model->canDropMimeData(data, action, row, column, srcparent);
    }
    return false;
}

bool ZAbstractProxyTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (model)
    {
        QModelIndex srcparent = parent;
        if (row == -1 && column == -1)
            srcparent = mapToSource(parent);
        if (row == rowCount(parent))
        {
            srcparent = mapToSource(parent);
            row = model->rowCount(srcparent);
            column = -1;
        }
        else
        {
            QModelIndex ix = mapToSource(index(row, column, parent));
            srcparent = mapToSource(parent);
            row = ix.row();
            column = ix.column();
        }

        return model->dropMimeData(data, action, row, column, srcparent);
    }
    return false;
}


//-------------------------------------------------------------
