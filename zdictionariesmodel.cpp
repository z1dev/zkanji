/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QIcon>
#include <QApplication>

#include "zdictionariesmodel.h"
#include "words.h"
#include "globalui.h"
#include "zui.h"
#include "generalsettings.h"


//-------------------------------------------------------------


DictionariesModel::DictionariesModel(QObject *parent) : base(parent)
{
    connect(gUI, &GlobalUI::dictionaryAdded, this, &DictionariesModel::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &DictionariesModel::dictionaryToBeRemoved);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionariesModel::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryMoved, this, &DictionariesModel::dictionaryMoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &DictionariesModel::dictionaryRenamed);
    connect(gUI, &GlobalUI::dictionaryFlagChanged, this, &DictionariesModel::dictionaryFlagChanged);
}

int DictionariesModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    return 0;
}

QVariant DictionariesModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::DecorationRole) || !index.isValid() || index.row() < 0)
        return QVariant();

    int row = ZKanji::dictionaryPosition(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return  ZKanji::dictionary(row)->name();

    //if (row == 0)
    //    return QIcon(":/flagen.svg");
    return ZKanji::dictionaryMenuFlag(/*QSize(Settings::scaled(18), Settings::scaled(18)),*/ ZKanji::dictionary(row)->name()/*, Flags::Flag*/);  //QIcon(":/flaggen.svg");
}

QModelIndex DictionariesModel::index(int row, int column, const QModelIndex &/*parent*/) const
{
    return createIndex(row, column, nullptr);
}

QModelIndex DictionariesModel::parent(const QModelIndex &/*index*/) const
{
    return QModelIndex();
}

int DictionariesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return ZKanji::dictionaryCount();
    return 0;
}

void DictionariesModel::dictionaryAdded()
{
    beginInsertRows(QModelIndex(), ZKanji::dictionaryCount() - 1, ZKanji::dictionaryCount() - 1);
    endInsertRows();
}

void DictionariesModel::dictionaryToBeRemoved(int /*index*/, int order)
{
    beginRemoveRows(QModelIndex(), order, order);
}

void DictionariesModel::dictionaryRemoved(int /*index*/, int /*order*/)
{
    endRemoveRows();
}

void DictionariesModel::dictionaryMoved(int from, int to)
{
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    endMoveRows();
}

void DictionariesModel::dictionaryRenamed(const QString &/*oldname*/, int /*index*/, int order)
{
    emit dataChanged(createIndex(order, 0), createIndex(order, columnCount() - 1));
}

void DictionariesModel::dictionaryFlagChanged(int /*index*/, int order)
{
    emit dataChanged(createIndex(order, 0), createIndex(order, columnCount() - 1));
}


//-------------------------------------------------------------


DictionariesProxyModel::DictionariesProxyModel(QObject *parent) : base(parent), dict(nullptr)
{
    setSourceModel(ZKanji::dictionariesModel());
}

void DictionariesProxyModel::setDictionary(Dictionary *d)
{
    if (dict == d)
        return;

    dict = d;
    invalidateFilter();
}

Dictionary* DictionariesProxyModel::dictionaryAtRow(int row)
{
    if (sourceModel() == nullptr)
        return nullptr;

    return ZKanji::dictionary(dictionaryIndexAtRow(row));
}

int DictionariesProxyModel::dictionaryIndexAtRow(int row)
{
    if (sourceModel() == nullptr)
        return -1;

    row = mapToSource(index(row, 0)).row();
    return ZKanji::dictionaryPosition(row);
}

int DictionariesProxyModel::dictionaryOrderAtRow(int row)
{
    if (sourceModel() == nullptr)
        return -1;

    return mapToSource(index(row, 0)).row();
}

bool DictionariesProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid() || sourceModel() == nullptr)
        return true;

    return source_row != ZKanji::dictionaryOrderIndex(dict);
}


//-------------------------------------------------------------


namespace ZKanji
{
    static DictionariesModel *dictmodel = nullptr;
    DictionariesModel* dictionariesModel()
    {
        if (dictmodel == nullptr)
            dictmodel = new DictionariesModel(qApp);
        return dictmodel;
    }
}


//-------------------------------------------------------------

