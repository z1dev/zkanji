/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJITOOLTIPWIDGET_H
#define KANJITOOLTIPWIDGET_H

#include <QWidget>

struct KanjiEntry;
class Dictionary;
class KanjiToolTipWidget : public QWidget
{
    Q_OBJECT
public:
    KanjiToolTipWidget(QWidget *parent = nullptr);
    virtual ~KanjiToolTipWidget();

    void setKanji(Dictionary *d, int kindex);

    virtual QSize sizeHint() const override;
    virtual void paintEvent(QPaintEvent *e) override;
private:
    void recalculateSize();

    const int hpad;
    const int vpad;
    const int penw;

    Dictionary *dict;
    int index;

    QSize hint;

    typedef QWidget base;
};

#endif // KANJITOOLTIPWIDGET_H

