/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZLISTBOX_H
#define ZLISTBOX_H

#include "zlistview.h"

class ZAbstractListBoxModel;
// Same as QListView where the sizeHint() can be specified to make the view resizable to a
// smaller size than originally.
class ZListBox : public ZListView
{
    Q_OBJECT
public:
    ZListBox(QWidget *parent = nullptr);
    virtual ~ZListBox();

    void setModel(ZAbstractListBoxModel *newmodel);
    ZAbstractListBoxModel* model() const;
private:

    typedef ZListView  base;
};

#endif // ZLISTBOX_H
