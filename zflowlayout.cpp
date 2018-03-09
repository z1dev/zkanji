/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QWidget>
#include <cmath>

#include "zflowlayout.h"


ZFlowLayout::ZFlowLayout(QWidget *parent) : base(parent), cached(-2, -2)/*, modeSpace(0, 0)*/, hspac(-1), vspac(-1), align(Qt::AlignTop)
{

}

ZFlowLayout::~ZFlowLayout()
{

}

//void ZFlowLayout::makeModeSpace(const QSize &size)
//{
//    modeSpace = size;
//}

void ZFlowLayout::addItem(QLayoutItem *item)
{
    list.push_back(item);
    cached = QSize(-2, -2);
}

QLayoutItem* ZFlowLayout::itemAt(int index) const
{
    if (index < 0 || index >= list.size())
        return nullptr;
    return const_cast<QLayoutItem*>(list[index]);
}

QLayoutItem* ZFlowLayout::takeAt(int index)
{
    if (index < 0 || index >= list.size())
        return nullptr;
    cached = QSize(-2, -2);

    QLayoutItem *result;
    list.removeAt(list.begin() + index, result);
    return result;
}

int ZFlowLayout::count() const
{
    return list.size();
}

Qt::Orientations ZFlowLayout::expandingDirections() const
{
    return 0;
}

QSize ZFlowLayout::sizeHint() const
{
    QSize s = minimumSize();
    s.setHeight(cached.height());
    return s;
}

QSize ZFlowLayout::minimumSize() const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    QSize size = QSize(0, 0);
    //bool first = true;
    for (const QLayoutItem *item : list)
    {
        //if (first)
        //{
        //    first = false;
        //    QSize s = item->minimumSize();
        //    s.setWidth(s.width() + modeSpace.width());
        //    s.setHeight(std::max(s.height(), modeSpace.height()));
        //    size.expandedTo(s);
        //    continue;
        //}
        if (!item->isEmpty())
        {
            size.expandedTo(item->minimumSize());
            QWidget *w = const_cast<QLayoutItem*>(item)->widget();
            if (w != nullptr && (w->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed || w->sizePolicy().horizontalPolicy() == QSizePolicy::Minimum || w->sizePolicy().horizontalPolicy() == QSizePolicy::MinimumExpanding))
                size.setWidth(std::max(size.width(), w->sizeHint().width()));
        }
    }

    size += QSize(left + right, top + bottom);

    size.setHeight(heightForWidth(geometry().width()));
    return size;
}

void ZFlowLayout::setGeometry(const QRect &r)
{
    recompute(r, parentWidget()->isVisibleTo(parentWidget()->window()));
    QRect r2 = QRect(r.x(), r.y(), cached.width(), cached.height());
    base::setGeometry(r2);
    
    QSize siz = minimumSize();
    parentWidget()->setMinimumHeight(siz.height());
    parentWidget()->setMaximumHeight(siz.height());
    parentWidget()->setMinimumWidth(siz.width());
}

bool ZFlowLayout::hasHeightForWidth() const
{
    return true;
}

int ZFlowLayout::heightForWidth(int w) const
{
    if (cached.width() != w)
    {
        ZFlowLayout *ptr = const_cast<ZFlowLayout*>(this);
        // Passing false for the update arg makes sure the layout doesn't change (apart from
        // the already mutable cached value).
        ptr->recompute(QRect(0, 0, w, 0), false);
    }

    return cached.height();
}

void ZFlowLayout::invalidate()
{
    cached = QSize(-2, -2);
    base::invalidate();
}

void ZFlowLayout::restrictWidth(QWidget *w, uint width)
{
    if (width != QWIDGETSIZE_MAX)
        restrictions[w] = width;
    else
    {
        auto it = restrictions.find(w);
        if (it != restrictions.end())
            restrictions.erase(it);
    }
}

uint ZFlowLayout::restrictedWidth(QWidget *w) const
{
    auto it = restrictions.find(w);
    if (it == restrictions.end())
        return QWIDGETSIZE_MAX;
    return it->second;
}


int ZFlowLayout::horizontalSpacing() const
{
    if (hspac >= 0)
        return hspac;
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

void ZFlowLayout::setHorizontalSpacing(int spac)
{
    if (spac == hspac)
        return;

    if (spac < 0)
        hspac = -1;
    else
        hspac = spac;
    cached = QSize(-2, -2);
}

int ZFlowLayout::verticalSpacing() const
{
    if (vspac >= 0)
        return vspac;
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

void ZFlowLayout::setVerticalSpacing(int spac)
{
    if (spac == vspac)
        return;

    if (spac < 0)
        vspac = -1;
    else
        vspac = spac;
    cached = QSize(-2, -2);
}

uint ZFlowLayout::spacingAfter(QWidget *w) const
{
    auto it = spacingafter.find(w);
    if (it == spacingafter.end())
        return QWIDGETSIZE_MAX;
    return it->second;
}

void ZFlowLayout::setSpacingAfter(QWidget *w, uint val)
{
    uint r = spacingAfter(w);
    if (r == val)
        return;
    if (val != QWIDGETSIZE_MAX)
        spacingafter[w] = val;
    else
        spacingafter.erase(w);

    cached = QSize(-2, -2);
}

Qt::Alignment ZFlowLayout::alignment() const
{
    return align;
}

void ZFlowLayout::setAlignment(Qt::Alignment val)
{
    val &= (Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter);
    if (val == align)
        return;
    val = align;
}


int ZFlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *p = parent();
    if (!p)
        return -1;

    if (p->isWidgetType())
    {
        QWidget *pw = (QWidget*)p;
        return pw->style()->pixelMetric(pm, 0, pw);
    }

    return ((QLayout *)p)->spacing();
}

void ZFlowLayout::recompute(const QRect &r, bool update)
{
    int w = r.width();

    cached.setWidth(w);

    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    QRect area = r.adjusted(left, top, -right, -bottom);

    // Height of the current row of items with the spacing between rows.
    int rowh = 0;
    // Height of spacing between the current row of items.
    int rows = 0;

    int x = area.left();
    int y = area.top();

    QStyle *style = nullptr;
    int hs = horizontalSpacing();
    int vs = verticalSpacing();
    if ((hs < 0 || vs < 0) && parentWidget() != nullptr)
        style = parentWidget()->style();

    int spacex = 0;
    int spacey = 0;

    // Stores the computed sizes of widgets in the current row. The sizes are calculated and
    // stored in this list according to policy and maximum sizes. Width of items that cannot
    // grow larger is stored as a negative value.
    std::vector<int> sizes;
    sizes.resize(list.size());

    for (auto lit = list.begin(); lit != list.end();)
    {
        // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
        if ((*lit)->isEmpty())
        {
            ++lit;
            continue;
        }

        auto endit = lit;

        // Difference between the width the items in the current row take up and the available
        // width.
        int wdiff = 0;

        // Number of items that don't have a fixed width in the current row.
        int expnum = 0;
        // Number of items with a horizontal policy of "Minimum". These items only grow if
        // there are no other items in the same row that can expand. If this is the last row
        // this value is ignored.
        int minexpnum = 0;
        // True when there are no items with expanding policies.
        bool expandminimum = false;

        int sizespos = 0;

        // First step: find the items that fit on the current line, and the number of items
        // that can be freely resized. The expanding items will grow wider in the second run
        // than the value used here.
        for (endit = lit; endit != list.end() /*&& (x <= area.right() || rowh == 0)*/; ++endit, ++sizespos)
        {
            // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
            if ((*endit)->isEmpty())
                continue;

            QLayoutItem *item = *endit;

            QSize hint = item->sizeHint();

            int itemw = hint.width();

            if (x + itemw > area.right() && rowh != 0)
            {
                wdiff = area.right() - x + spacex;
                break;
            }

            QWidget *widget = item->widget();
            int whs = hs;
            if (widget != nullptr)
            {
                auto sait = spacingafter.find(widget);
                if (sait != spacingafter.end())
                    whs = sait->second;
            }

            // Negative for items that cannot grow from their current size.
            int limitprefix = -1;

            if (widget != nullptr && widget->sizePolicy().horizontalPolicy() != QSizePolicy::Fixed)
            {
                // Making sure something is saved in sizes even if sizeHint() returned a bad
                // value. If sizes[sizespos] would be 0, this item wouldn't be expanded in the
                // next loop.
                if (itemw <= 0)
                    itemw = 1;

                if (widget->sizePolicy().horizontalPolicy() == QSizePolicy::Minimum)
                    ++minexpnum;
                else
                {
                    limitprefix = 1;
                    ++expnum;
                }
            }

            auto nextit = nextVisibleIt(endit, list.end());
            computeSpacing(item, nextit != list.end() ? *nextit : nullptr, spacex, spacey, whs, vs, style);

            int itemh = hint.height();
            if (rowh + rows < itemh + spacey)
            {
                rowh = itemh;
                rows = spacey;
            }

            x += itemw + spacex;

            sizes[sizespos] = limitprefix * itemw;
        }

        if (endit == list.end())
            wdiff = area.right() - x + spacex;

        if (update)
        {
            // Nothing would grow without the minimal policy items. Make those grow instead.
            if (expnum == 0 && minexpnum != 0 && endit != list.end())
            {
                expnum = minexpnum;
                expandminimum = true;

                sizespos = 0;
                for (auto it = lit; it != endit; ++it, ++sizespos)
                {
                    QLayoutItem *item = *it;

                    // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
                    if (item->isEmpty())
                        continue;

                    QWidget *widget = item->widget();

                    if (widget != nullptr && widget->sizePolicy().horizontalPolicy() == QSizePolicy::Minimum)
                        sizes[sizespos] = std::abs(sizes[sizespos]);
                }
            }

            // Second step: find the actual width of the items when placed in the layout.
            while (wdiff != 0 && expnum != 0)
            {
                sizespos = 0;

                // Width that can still be distributed among widgets after the next step.
                int leftover = 0;

                // Number of expanding widgets that can still grow after the next step.
                int expnum2 = 0;

                for (auto it = lit; it != endit && wdiff != 0; ++it, ++sizespos)
                {
                    QLayoutItem *item = *it;

                    // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
                    if (item->isEmpty() || sizes[sizespos] <= 0)
                        continue;

                    uint maxw = std::max<uint>(restrictedWidth(item->widget()), item->widget()->sizeHint().width());

                    int extraw = wdiff / expnum;

                    if (sizes[sizespos] + extraw >= maxw)
                    {
                        int tmpw = std::max(0, (int)maxw - sizes[sizespos]);
                        leftover += extraw - tmpw;
                        sizes[sizespos] = -sizes[sizespos] - tmpw;
                    }
                    else
                    {
                        ++expnum2;
                        sizes[sizespos] += extraw;
                    }

                    wdiff -= extraw;
                    --expnum;
                }

                expnum = expnum2;
                wdiff += leftover;
            }

            if (wdiff != 0 && endit != list.end())
            {
                // There is still space left to distribute. If there are items with restricted
                // widths, they are forcefully expanded to fill the space, unless this is the
                // last row.

                expnum = 0;
                sizespos = 0;
                for (auto it = lit; it != endit; ++it, ++sizespos)
                {
                    QLayoutItem *item = *it;

                    // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
                    if (item->isEmpty())
                        continue;

                    QWidget *widget = item->widget();
                    if (widget != nullptr && widget->sizePolicy().horizontalPolicy() != QSizePolicy::Fixed && (expandminimum || widget->sizePolicy().horizontalPolicy() != QSizePolicy::Minimum))
                        ++expnum;
                }

                sizespos = 0;
                for (auto it = lit; it != endit && expnum != 0; ++it, ++sizespos)
                {
                    QLayoutItem *item = *it;

                    // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
                    if (item->isEmpty())
                        continue;

                    QWidget *widget = item->widget();
                    if (widget == nullptr || widget->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed || (!expandminimum && widget->sizePolicy().horizontalPolicy() == QSizePolicy::Minimum))
                        continue;

                    int extraw = wdiff / expnum;
                    wdiff -= extraw;
                    --expnum;

                    sizes[sizespos] = -std::abs(sizes[sizespos]) - extraw;
                }
            }

            x = area.left();

            sizespos = 0;
            // Third step: computing the final sizes of widgets and placing them.
            for (auto it = lit; it != endit; ++it, ++sizespos)
            {
                QLayoutItem *item = *it;

                // QWidgetItem::isEmpty() returns true for hidden widgets. Skipping hidden items.
                if (item->isEmpty())
                    continue;

                int itemw = std::abs(sizes[sizespos]);

                QSize hint = item->sizeHint();
                hint.setWidth(itemw);

                auto nextit = nextVisibleIt(it, list.end());

                QWidget *widget = item->widget();
                int whs = hs;
                if (widget != nullptr)
                {
                    auto sait = spacingafter.find(widget);
                    if (sait != spacingafter.end())
                        whs = sait->second;
                }

                computeSpacing(item, nextit != list.end() ? *nextit : nullptr, spacex, spacey, whs, vs, style);

                int alignedy = y;
                if (hint.height() < rowh && align != Qt::AlignTop)
                {
                    if (align == Qt::AlignBottom)
                        alignedy = y + rowh - hint.height();
                    else if (align == Qt::AlignVCenter)
                        alignedy = y + (rowh - hint.height()) / 2;
                }
                item->setGeometry(QRect(QPoint(x, alignedy), hint));

                x += itemw + spacex;
            }
        }

        y += rowh;
        if (endit != list.end())
            y += rows;
        x = area.left();
        rowh = 0;
        rows = 0;

        lit = endit;
    }
    
    cached.setHeight(y + rowh - r.y() + bottom);
}

void ZFlowLayout::computeSpacing(QLayoutItem *first, QLayoutItem *next, int &spacex, int &spacey, int hs, int vs, const QStyle *style)
{
    QSizePolicy::ControlTypes c1 = QSizePolicy::DefaultType;
    QSizePolicy::ControlTypes c2 = QSizePolicy::DefaultType;

    spacex = hs;
    spacey = vs;

    QWidget *p = parentWidget();

    if (hs < 0)
    {
        c1 = first->controlTypes();

        if (next != nullptr)
            c2 = next->controlTypes();

        if (style != nullptr)
            spacex = style->combinedLayoutSpacing(c1, c2, Qt::Horizontal, 0, p);
        else
            spacex = 0;
    }

    if (vs < 0)
    {
        if (hs >= 0)
        {
            // Get only once;
            c1 = first->controlTypes();
            if (next != nullptr)
                c2 = next->controlTypes();
        }

        if (style != nullptr)
            spacey = style->combinedLayoutSpacing(c1, c2, Qt::Vertical, 0, p);
        else
            spacey = 0;
    }
}

smartvector_iterator<QLayoutItem> ZFlowLayout::nextVisibleIt(smartvector_iterator<QLayoutItem> it, smartvector_iterator<QLayoutItem> endit)
{
    while ((it = std::next(it)) != endit)
        if (!(*it)->isEmpty())
            break;

    return it;
}

