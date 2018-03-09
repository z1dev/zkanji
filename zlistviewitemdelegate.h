/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZLISTVIEWITEMDELEGATE_H
#define ZLISTVIEWITEMDELEGATE_H


#include <QStyledItemDelegate>

class ZListView;
// Item delegate used in ZListViews. To change checkbox drawing, derive from this class and
// override checkBoxRect(), checkBoxHitRect() and drawCheckBox(). The model must return
// Qt::ItemIsUserCheckable in flags for the index where checkbox is to be drawn.
class ZListViewItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ZListViewItemDelegate(ZListView *parent = nullptr);
    virtual ~ZListViewItemDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    // Returns the rectangle of a checkbox drawn at index.
    virtual QRect checkBoxRect(const QModelIndex &index) const;
    // Returns the rectangle of a checkbox drawn at index that responds to mouse.
    virtual QRect checkBoxHitRect(const QModelIndex &index) const;
protected:
    // Paints a checkbox and returns the rectangle where it was drawn.
    virtual QRect drawCheckBox(QPainter *p, const QModelIndex &index) const;

    // Returns the parent() cast to ZListView.
    ZListView* owner() const;
private:
    //QSize checkBoxRectHelper(QStyleOptionButton *opt) const;

    void initCheckBoxOption(const QModelIndex &index, QStyleOptionButton *opt) const;

    typedef QStyledItemDelegate base;
};


#endif // ZLISTVIEWITEMDELEGATE_H

