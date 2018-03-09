/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZDICTIONARYCOMBOBOX_H
#define ZDICTIONARYCOMBOBOX_H


#include <QComboBox>

class ZDictionaryComboBox : public QComboBox
{
    Q_OBJECT
public:
    ZDictionaryComboBox(QWidget *parent = nullptr);
    virtual ~ZDictionaryComboBox();
private:
    typedef QComboBox   base;
};


#endif

