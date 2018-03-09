/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

//#include <QLayout>
//#include <QFontMetrics>
#include <QHeaderView>
#include <QApplication>
#include "zlistbox.h"
#include "zlistboxmodel.h"


//-------------------------------------------------------------


ZListBox::ZListBox(QWidget *parent) : base(parent)
{
    setShowGrid(false);
    horizontalHeader()->hide();
    verticalHeader()->hide();
    setFont(QApplication::font("QListViewFont"));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ZListBox::~ZListBox()
{
    ;
}

void ZListBox::setModel(ZAbstractListBoxModel *newmodel)
{
    base::setModel(newmodel);
}

ZAbstractListBoxModel* ZListBox::model() const
{
    return (ZAbstractListBoxModel*)base::model();
}


//-------------------------------------------------------------
