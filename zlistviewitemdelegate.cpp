/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPainter>
#include "zlistviewitemdelegate.h"
#include "zlistview.h"
#include "zabstracttablemodel.h"
#include "fontsettings.h"
#include "zui.h"


//-------------------------------------------------------------


ZListViewItemDelegate::ZListViewItemDelegate(ZListView *parent) : base(parent)
{

}

ZListViewItemDelegate::~ZListViewItemDelegate()
{

}

void ZListViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // WARNING: when this is updated, update sizeHint() calculations too.

    bool current = index.isValid() ? owner()->hasFocus() && owner()->currentRow() == index.row() : false;
    bool selected = /*owner()->selectionType() == ListSelectionType::Single ? current : */owner()->rowSelected(index.row());

    QPalette::ColorGroup colorgrp = (index.isValid() && !index.flags().testFlag(Qt::ItemIsEnabled)) ? QPalette::Disabled : !owner()->isActiveWindow()/*(option.state & QStyle::State_Active)*/ ? QPalette::Inactive : QPalette::Active;

    QColor cellcol = index.data((int)CellRoles::CellColor).value<QColor>();
    QColor basecol = option.palette.color(colorgrp, QPalette::Base);
    QColor selbasecol = option.palette.color(colorgrp, QPalette::Highlight);
    QColor bgcol = selected ? selbasecol : basecol;
    if (cellcol.isValid())
        bgcol = colorFromBase(basecol, bgcol, cellcol);

    if (cellcol.isValid() || selected)
        painter->fillRect(option.rect, bgcol);

    //int flag = index.data((int)CellRoles::Flags).toInt();

    //if ((flag & (int)CellFlags::Image) == (int)CellFlags::Image)
    //{
    //    QImage *img = (QImage*)index.data(Qt::DisplayRole).toInt();
    //    if (img == nullptr)
    //        return;

    //    painter->drawImage(QPoint(option.rect.left() + (option.rect.width() - img->width()) / 2, option.rect.top() + (option.rect.height() - img->height()) / 2), *img);
    //    return;
    //}

    QRect chkr;

    //if ((flag & (int)CellFlags::CheckBox) == (int)CellFlags::CheckBox)
    if (/*index.flags().testFlag(Qt::ItemIsUserCheckable)*/ index.data(Qt::CheckStateRole).isValid())
        chkr = drawCheckBox(painter, index);

    QString str = index.data(Qt::DisplayRole).toString();
    QFont f = Settings::mainFont();
    painter->save();

    painter->setFont(f);

    QColor textcol = index.data((int)CellRoles::TextColor).value<QColor>();
    QColor basetextcol = option.palette.color(colorgrp, QPalette::Text);
    QColor seltextcol = option.palette.color(colorgrp, QPalette::HighlightedText);
    QColor pencol = selected ? seltextcol : basetextcol;
    if (textcol.isValid())
        pencol = colorFromBase(basetextcol, pencol, textcol);

    painter->setPen(pencol);

    int left = option.rect.left() + 4;
    if (!chkr.isEmpty())
        left = chkr.right() + 4;

    QVariant v = index.data(Qt::DecorationRole);
    if (v.isValid())
    {
        if (v.type() == QVariant::Pixmap)
        {
            QPixmap pic = v.value<QPixmap>();
            painter->drawPixmap(left, option.rect.top() + (option.rect.height() - pic.height()) / 2, pic);
            left += pic.width() + 4;
        }
        else if (v.type() == QVariant::Icon)
        {
            QIcon pic = v.value<QIcon>();
            int h = option.rect.height() * 0.8;
            QSize siz = pic.actualSize(QSize(h, h));
            pic.paint(painter, left, option.rect.top() + (option.rect.height() - siz.height()), Qt::AlignLeft, QIcon::Normal);
            left += siz.width() + 4;
        }
    }
    painter->drawText(left, option.rect.top(), option.rect.width() - 8, option.rect.height(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, str);

    if (current)
    {
        // Draw the focus rectangle.
        QStyleOptionFocusRect fop;
        fop.backgroundColor = option.palette.color(colorgrp, QPalette::Highlight);
        fop.palette = option.palette;
        fop.direction = option.direction;
        fop.rect = owner()->rowRect(index.row());
        fop.rect.adjust(0, 0, -1, -1);
        fop.state = QStyle::State(QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange | (selected ? QStyle::State_Selected : QStyle::State_None));
        owner()->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &fop, painter, owner());
    }

    painter->restore();
}

QSize ZListViewItemDelegate::sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
{
    //int flag = index.data((int)CellRoles::Flags).toInt();
    //if ((flag & (int)CellFlags::Image) == (int)CellFlags::Image)
    //{
    //    QImage *img = (QImage*)index.data(Qt::DisplayRole).toInt();
    //    if (img == nullptr)
    //        return QSize(1, 1);

    //    return img->size();
    //}

    QRect chkr;
    //if ((flag & (int)CellFlags::CheckBox) == (int)CellFlags::CheckBox)
    if (/*index.flags().testFlag(Qt::ItemIsUserCheckable)*/ index.data(Qt::CheckStateRole).isValid())
        chkr = checkBoxRect(index);

    QString str = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm = QFontMetrics(Settings::mainFont());

    QSize siz = fm.boundingRect(str).size();
    siz.setWidth(siz.width() + 8);
    if (!chkr.isEmpty())
        siz.setWidth(siz.width() + 4 + chkr.width());
    return siz;
}

bool ZListViewItemDelegate::editorEvent(QEvent * /*event*/, QAbstractItemModel * /*model*/, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/)
{
    return false;
}

QRect ZListViewItemDelegate::checkBoxRect(const QModelIndex &index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);
    return ::checkBoxRect(owner(), 4, owner()->visualRect(index), &opb);
}

QRect ZListViewItemDelegate::checkBoxHitRect(const QModelIndex &index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);
    return ::checkBoxHitRect(owner(), 4, owner()->visualRect(index), &opb);
}

QRect ZListViewItemDelegate::drawCheckBox(QPainter *p, const QModelIndex &index) const
{
    QStyleOptionButton opb;
    initCheckBoxOption(index, &opb);
    ::drawCheckBox(p, owner(), 4, owner()->visualRect(index), &opb);

    return opb.rect;
}

ZListView* ZListViewItemDelegate::owner() const
{
    return (ZListView*)parent();
}

//QSize ZListViewItemDelegate::checkBoxRectHelper(QStyleOptionButton *opb) const
//{
//    return owner()->style()->sizeFromContents(QStyle::CT_CheckBox, opb, QSize(), owner());
//
//    //QPoint p;
//    //p.setX(opb->rect.left());
//    //p.setY(opb->rect.top() + (opb->rect.height() - chksiz.height()) / 2);
//
//    //return QRect(p.x(), p.y(), chksiz.width(), chksiz.height());
//}

void ZListViewItemDelegate::initCheckBoxOption(const QModelIndex &index, QStyleOptionButton *opt) const
{
    ::initCheckBoxOption((QWidget*)parent(), owner()->checkBoxMouseState(index), index.flags().testFlag(Qt::ItemIsUserCheckable), (Qt::CheckState)index.data(Qt::CheckStateRole).toInt(), opt);
    opt->state &= ~QStyle::State_HasFocus;
}


//-------------------------------------------------------------
