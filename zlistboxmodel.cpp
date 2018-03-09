/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "zlistboxmodel.h"


//-------------------------------------------------------------


ZAbstractListBoxModel::ZAbstractListBoxModel(QObject *parent) : base(parent)
{
    ;
}

ZAbstractListBoxModel::~ZAbstractListBoxModel()
{
    ;
}

int ZAbstractListBoxModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() || rowCount() == 0)
        return 0;
    return 1;
}

QVariant ZAbstractListBoxModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != (int)ColumnRoles::AutoSized || section != 0 || orientation != Qt::Horizontal)
        return base::headerData(section, orientation, role);

    return (int)ColumnAutoSize::StretchedAuto;
}


//-------------------------------------------------------------
