/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "zstackedwidget.h"


//-------------------------------------------------------------


ZStackedWidget::ZStackedWidget(QWidget *parent) : base(parent)
{
    connect(this, &base::currentChanged, this, &ZStackedWidget::handleCurrentChanged);
}

ZStackedWidget::~ZStackedWidget()
{

}

QSize ZStackedWidget::sizeHint() const
{
    QWidget *w = currentWidget();
    if (w == nullptr)
        return base::sizeHint();

    return w->sizeHint();
}

QSize ZStackedWidget::minimumSizeHint() const
{
    QWidget *w = currentWidget();
    if (w == nullptr)
        return base::minimumSizeHint();

    return w->minimumSizeHint();
}

bool ZStackedWidget::hasHeightForWidth() const
{
    QWidget *w = currentWidget();
    if (w == nullptr)
        return base::hasHeightForWidth();

    return w->hasHeightForWidth();
}

int ZStackedWidget::heightForWidth(int width) const
{
    QWidget *w = currentWidget();
    if (w == nullptr)
        return base::heightForWidth(width);

    return w->heightForWidth(width);
}

void ZStackedWidget::handleCurrentChanged(int index)
{
    if (index != -1)
    {
        QWidget *w = currentWidget();
        if (w != nullptr)
            setMinimumSize(w->minimumSize());
        else
            setMinimumSize(QSize(0, 0));
    }
    else
        setMinimumSize(QSize(0, 0));

    updateGeometry();

    emit currentChanged(index);
}


//-------------------------------------------------------------

