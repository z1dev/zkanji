/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "zdictionarycombobox.h"
#include "zdictionariesmodel.h"
#include "generalsettings.h"

//-------------------------------------------------------------


ZDictionaryComboBox::ZDictionaryComboBox(QWidget *parent) : base(parent)
{
    setModel(ZKanji::dictionariesModel());
    setIconSize(Settings::scaled(iconSize()));
}

ZDictionaryComboBox::~ZDictionaryComboBox()
{
}


//-------------------------------------------------------------
