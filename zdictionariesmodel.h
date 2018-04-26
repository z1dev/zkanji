/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZDICTIONARYLISTMODEL_H
#define ZDICTIONARYLISTMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>


class Dictionary;

// Model listing the dictionaries in their user defined order, with decoration showing the
// dictionary flag. The model can be used in combo boxes.
class DictionariesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    DictionariesModel(QObject *parent = nullptr);

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &index) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

public slots:
    void dictionaryAdded();
    void dictionaryToBeRemoved(int index, int order);
    void dictionaryRemoved(int index, int order);
    void dictionaryMoved(int from, int to);
    void dictionaryRenamed(const QString &oldname, int index, int order);
    void dictionaryFlagChanged(int index, int order);
private:
    typedef QAbstractItemModel  base;
};

// Proxy model for the DictionariesModel that lists the dictionaries the same way, except the
// dictionary set with setDictionary(), which is excluded.
class DictionariesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    DictionariesProxyModel(QObject *parent);
    void setDictionary(Dictionary *d);

    // Returns a pointer to the dictionary that would be displayed in a view at row with this
    // model.
    Dictionary* dictionaryAtRow(int row);
    // Returns the index to the dictionary that would be displayed in a view at row with this
    // model. For the user ordered index, use dictionaryOrderAtRow().
    int dictionaryIndexAtRow(int row);
    // Returns the user defined index to the dictionary that would be displayed in a view at
    // row with this model. For the real index use dictionaryIndexAtRow().
    int dictionaryOrderAtRow(int row);
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
private:
    Dictionary *dict;

    typedef QSortFilterProxyModel base;
};

namespace ZKanji
{
    DictionariesModel* dictionariesModel();
}


#endif // ZDICTIONARYLISTMODEL_H

