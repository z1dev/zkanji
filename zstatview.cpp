/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QScrollBar>
#include <QPainter>
#include "fastarray.h"
#include "zstatview.h"
#include "zabstractstatmodel.h"
#include "colorsettings.h"

ZStatView::ZStatView(QWidget *parent) : base(parent), m(nullptr), lm(16), tm(lm), rm(lm), bm(lm), tickspacing(50), hwidth(16)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

ZStatView::~ZStatView()
{

}

void ZStatView::setModel(ZAbstractStatModel *model)
{
    if (m == model)
        return;

    m = model;
    colpos.clear();
    int p = 0;
    hlabel = QString();
    vlabel = QString();
    if (m != nullptr)
    {
        for (int ix = 0, siz = m->count(); ix != siz; ++ix)
        {
            p += m->barWidth(this, ix);
            colpos.push_back(p);
        }
        hlabel = m->axisLabel(Qt::Horizontal);
        vlabel = m->axisLabel(Qt::Vertical);
    }

    updateView();
}

void ZStatView::setMargins(int leftmargin, int topmargin, int rightmargin, int bottommargin)
{
    if (lm == leftmargin && tm == topmargin && rm == rightmargin && bm == bottommargin)
        return;

    lm = leftmargin;
    tm = topmargin;
    rm = rightmargin;
    bm = bottommargin;
    updateView();
}

int ZStatView::tickSpacing() const
{
    return tickspacing;
}

void ZStatView::setTickSpacing(int val)
{
    val = std::max(val, fontMetrics().height());
    if (val == tickspacing)
        return;
    tickspacing = val;
    updateView();
}

void ZStatView::scrollTo(int column)
{
    QRect r = statRect();
    int mid = (colpos[column] - (column == 0 ? 0 : colpos[column - 1])) / 2;
    horizontalScrollBar()->setValue(mid - r.width() - 2);
}

void ZStatView::changeEvent(QEvent *e)
{
    base::changeEvent(e);
    if (e->type() == QEvent::FontChange)
    {
        tickspacing = std::max(tickspacing, fontMetrics().height());
        updateView();
    }
}

bool ZStatView::viewportEvent(QEvent *e)
{
    bool r = base::viewportEvent(e);
    if (e->type() == QEvent::Resize)
        updateView();
    return r;
}

void ZStatView::scrollContentsBy(int dx, int dy)
{
    viewport()->update();
}

void ZStatView::paintEvent(QPaintEvent *event)
{
    QPainter p(viewport());
    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();

    QRect r = statRect();
    p.setPen(Settings::uiColor(ColorSettings::Grid));
    p.drawLine(r.left() - 1, r.top(), r.left() - 1, r.bottom());
    
    // Draw vertical axis ticks.
    for (int ix = 0, siz = ticks.size(); ix != siz; ++ix)
    {
        int num = ticks[ix].first;
        int pos = ticks[ix].second;
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        p.drawLine(r.left() - 5, r.top() + pos, r.right(), r.top() + pos);

        p.setPen(Settings::textColor(ColorSettings::Text));
        p.drawText(QRectF(0, r.top() + pos - fmh / 2, r.left() - 7, fmh), QString::number(num), Qt::AlignRight | Qt::AlignVCenter);
    }

    if (!vlabel.isEmpty())
    {
        p.save();
        p.setPen(Settings::textColor(ColorSettings::Text));
        p.rotate(-90);
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(-r.top() - r.height(), 0, r.height(), r.left() - 6 - hwidth), Qt::AlignVCenter | Qt::AlignHCenter, vlabel);
        p.restore();
    }

    // Draw bars and horizontal axis.
    int left = horizontalScrollBar()->value();
    // Find the first bar to draw with binary search.
    int hpos = std::upper_bound(colpos.begin(), colpos.end(), left) - colpos.begin();
    int prev = (hpos == 0 ? 0 : colpos[hpos - 1]);
    p.save();
    while (hpos < colpos.size() && prev - left < r.width())
    {
        QRect r2 = QRect(r.left() + prev - left, r.top(), colpos[hpos] - prev, r.height());
        prev = colpos[hpos];

        p.setClipRect(r.intersected(r2), Qt::ClipOperation::ReplaceClip);
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        p.drawLine(r2.right() - 1, r.top(), r2.right() - 1, r.bottom());
        paintBar(p, hpos, r2);

        r2.setTop(r2.bottom() + 6);
        r2.setBottom(height());
        p.setClipRect(QRect(QPoint(std::max(r2.left(), r.left()), r2.top()), QPoint(std::min(r2.right(), r.right()), r2.bottom())), Qt::ReplaceClip);
        if (m != nullptr)
        {
            p.setPen(Settings::textColor(ColorSettings::Text));
            p.drawText(r2, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, m->barLabel(hpos));
        }

        ++hpos;
    }
    p.restore();

    if (!hlabel.isEmpty())
    {
        p.save();
        p.setPen(Settings::textColor(ColorSettings::Text));
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(r.left(), r.bottom() + 6 + fmh + 4, r.width(), fmh), Qt::AlignTop | Qt::AlignHCenter, hlabel);
        p.restore();
    }
}

void ZStatView::paintBar(QPainter &p, int col, QRect r)
{
    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();
    int maxval = m->maxValue();
    if (maxval == 0)
        return;

    int cnt = m->valueCount();
    int sum = 0;

    fastarray<int> stats(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        int v = m->value(col, ix);
        stats[ix] = v;
        sum += v;
    }

    if (sum == 0)
        return;

    // Drawing the bar rectangles.
    QRect br;

    // Start bar drawing below grid top.
    r.setTop(std::min<int>(r.bottom(), r.top() + fmh));

    for (int ix = 0; ix != cnt; ++ix)
    {
        if (br.isNull())
            br = QRect(r.left() + r.width() * 0.2, r.bottom() - (r.height() * ((double)sum / maxval)), r.width() * 0.6, 0);
        br.setBottom(r.bottom() - (r.height() * (sum - stats[ix]) / maxval));
        sum -= stats[ix];
        p.fillRect(br, Settings::uiColor((ColorSettings::UIColorTypes)((int)ColorSettings::Stat1 + ix)));
        br.setTop(br.bottom());
    }
}

QRect ZStatView::statRect() const
{
    int fmh = fontMetrics().height();

    QRect r = viewport() != nullptr ? viewport()->rect() : rect();
    r.adjust(lm + hwidth + 6 + (vlabel.isEmpty() ? 0 : fmh), tm, std::max( -(r.width() - (lm + hwidth + 6 + (vlabel.isEmpty() ? 0 : fmh))), -rm), std::max(-(r.height() - tm), -bm - fmh - (hlabel.isEmpty() ? 0 : fmh)));
    return r;
}

void ZStatView::updateView()
{
    ticks.clear();
    hwidth = 0;

    // Calculate the vertical tick positions.
    QRect r = statRect();
    QFontMetrics fm = fontMetrics();
    //p->setFont(parentWidget()->font());
    int fmh = fm.height();
    //r.setTop(std::min<int>(r.bottom(), r.top() + fmh));

    //p->setPen(Settings::uiColor(ColorSettings::Grid));
    //p->drawLine(r.topRight(), r.bottomRight());

    //QAbstractItemModel *m = model();
    int maxval = m == nullptr ? 0 : m->maxValue(); //m->index(0, 0).data((int)StatRoles::MaxRole).toInt();
    // When drawing the bar, fmh height will be excluded from top to draw the bar
    // texts, but the grid should be drawn above it. To compensate, another max val is used.
    int gridmaxval = maxval + maxval * (double)fmh / r.height();
    if (gridmaxval != 0 && r.height() != 0)
    {

        // Calculating the tick steps.
        int steps = std::max<int>(gridmaxval / ((double)r.height() / tickspacing) + 0.9, 1);

        int v = steps;
        int zeroes = 0;
        while (v > 0)
        {
            v /= 10;
            ++zeroes;
        }
        zeroes = (int)std::pow(10, zeroes / 2);
        // Final tick step value.
        if (zeroes > 2)
            steps = (int((steps - 1) / (zeroes / 2)) + 1) * (zeroes / 2);
        v = 0;
        int pos = r.bottom();
        // Drawing horizontal tick lines and values.
        while (v <= gridmaxval)
        {
            //p->setPen(Settings::uiColor(ColorSettings::Grid));
            //p->drawLine(r.right() - 4, pos, r.right(), pos);
            //p->setPen(Settings::textColor(ColorSettings::Text));
            //p->drawText(QRectF(r.left(), pos - fmh / 2, r.width() - 6, fmh), QString::number(v), Qt::AlignRight | Qt::AlignVCenter);
            ticks.push_back(std::make_pair(v, pos - r.top()));

            v += steps;
            pos = r.bottom() - (r.height() * ((double)v / gridmaxval));
        }
    }
    if (!ticks.empty())
        hwidth = fm.width(QString::number(ticks.back().first));


    r = statRect();
    horizontalScrollBar()->setRange(0, colpos.size() == 0 ? 0 : std::max(0, colpos.back() - r.width()));
    horizontalScrollBar()->setEnabled(colpos.back() > r.width());
    horizontalScrollBar()->setPageStep(std::max(1, r.width() - 24));
    horizontalScrollBar()->setSingleStep(std::max(1, std::min(r.width(), 24)));
    viewport()->update();
}

