/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZLISTBOXMODEL_H
#define ZLISTBOXMODEL_H

#include "zabstracttablemodel.h"

class ZAbstractListBoxModel : public ZAbstractTableModel
{
    Q_OBJECT
public:
    ZAbstractListBoxModel(QObject *parent = nullptr);
    virtual ~ZAbstractListBoxModel();

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
private:
    typedef ZAbstractTableModel base;
};

#endif // ZLISTBOXMODEL_H
