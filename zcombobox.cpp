/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QAbstractItemView>

#include "zcombobox.h"
#include "zlineedit.h"
#include "zkanalineedit.h"

//-------------------------------------------------------------


ZComboBox::ZComboBox(QWidget *parent) : base(parent), /*firstshow(true),*/ checkchange(true), shwidth(-1)
{
    connect(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ZComboBox::indexChanged);
}

ZComboBox::~ZComboBox()
{
}

void ZComboBox::setEditable(bool editable)
{
    base::setEditable(editable);
    if (editable)
        installEditor();
}

void ZComboBox::setValidator(const QValidator *val)
{
    if (!isEditable())
        return;

    if (dynamic_cast<ZLineEdit*>(lineEdit()) != nullptr)
        ((ZLineEdit*)lineEdit())->setValidator(val);
}

QString ZComboBox::text() const
{
    if (!isEditable())
        return QString();

    checkThrow();
    return ((ZLineEdit*)lineEdit())->text();
}

void ZComboBox::setText(const QString &str)
{
    if (!isEditable())
        return;

    checkThrow();
    ((ZLineEdit*)lineEdit())->setText(str);
}

void ZComboBox::setCharacterSize(int chsiz)
{
    if (chsiz <= 0)
    {
        shwidth = -1;
        return;
    }

    QFontMetrics fm(font());
    int w = fm.averageCharWidth();
    shwidth = w * (chsiz + 1);
}

QSize ZComboBox::sizeHint() const
{
    QSize sh = base::sizeHint();
    if (shwidth > 0)
        sh.setWidth(shwidth);
    return sh;
}

QSize ZComboBox::minimumSizeHint() const
{
    if (shwidth < 0)
        return base::minimumSizeHint();
    return sizeHint();
}

void ZComboBox::installEditor()
{
    if (dynamic_cast<ZLineEdit*>(lineEdit()) == nullptr)
        setLineEdit(new ZLineEdit(this));
}

//void ZComboBox::showEvent(QShowEvent *e)
//{
//    base::showEvent(e);
//    if (firstshow)
//    {
//        setText(QString());
//        view()->setCurrentIndex(QModelIndex());
//    }
//    firstshow = false;
//}

void ZComboBox::indexChanged(int /*index*/)
{
    if (checkchange)
    {
        checkchange = false;
        setCurrentIndex(-1);
    }
}

void ZComboBox::checkThrow() const
{
    if (dynamic_cast<ZLineEdit*>(lineEdit()) == nullptr)
        throw "Editor not set.";
}


//-------------------------------------------------------------


ZKanaComboBox::ZKanaComboBox(QWidget *parent) : base(parent)
{
}

ZKanaComboBox::~ZKanaComboBox()
{
}

Dictionary* ZKanaComboBox::dictionary() const
{
    if (dynamic_cast<ZKanaLineEdit*>(lineEdit()) != nullptr)
        return ((ZKanaLineEdit*)lineEdit())->dictionary();
    return nullptr;
}

void ZKanaComboBox::setDictionary(Dictionary *dict)
{
    if (dynamic_cast<ZKanaLineEdit*>(lineEdit()) != nullptr)
        ((ZKanaLineEdit*)lineEdit())->setDictionary(dict);
}

void ZKanaComboBox::installEditor()
{
    if (dynamic_cast<ZKanaLineEdit*>(lineEdit()) == nullptr)
        setLineEdit(new ZKanaLineEdit(this));
}


//-------------------------------------------------------------

