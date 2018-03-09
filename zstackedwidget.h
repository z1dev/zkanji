/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZSTACKEDWIDGET_H
#define ZSTACKEDWIDGET_H

#include <QStackedWidget>

// Modification of QStackedWidget to allow the minimum/maximum and hint sizes to match the
// currently visible widget only.
class ZStackedWidget : public QStackedWidget
{
    Q_OBJECT
signals:
    void currentChanged(int index);
public:
    ZStackedWidget(QWidget *parent = nullptr);
    ~ZStackedWidget();

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;
private:
    using QStackedWidget::currentChanged;

    void handleCurrentChanged(int index);

    typedef QStackedWidget  base;
};

#endif // ZSTACKEDWIDGET_H
