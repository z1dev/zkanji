/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QScrollBar>
#include <QPainter>
#include <QLabel>
#include "ztooltip.h"
#include "fastarray.h"
#include "zstatview.h"
#include "zabstractstatmodel.h"
#include "colorsettings.h"

ZStatView::ZStatView(QWidget *parent) : base(parent), m(nullptr), lm(16), tm(lm), rm(lm), bm(lm), tickspacing(60), hwidth(16)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setMouseTracking(true);
}

ZStatView::~ZStatView()
{

}

ZAbstractStatModel* ZStatView::model() const
{
    return m;
}

void ZStatView::setModel(ZAbstractStatModel *model)
{
    if (m == model)
        return;

    m = model;
    colpos.clear();
    hlabel = QString();
    vlabel = QString();
    if (m != nullptr)
    {
        if (m->type() == ZStatType::BarScroll)
        {
            int p = 0;
            for (int ix = 0, siz = m->count(); ix != siz; ++ix)
            {
                p += m->barWidth(this, ix);
                colpos.push_back(p);
            }
        }

        hlabel = m->axisLabel(Qt::Horizontal);
        vlabel = m->axisLabel(Qt::Vertical);
    }

    updateView();
    if (m != nullptr && m->type() == ZStatType::BarScroll)
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    else
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    if (column >= colpos.size())
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
        return;
    }

    //QRect r = statRect();
    //int mid = (colpos[column] + (column == 0 ? 0 : colpos[column - 1])) / 2;
    //horizontalScrollBar()->setValue(std::max(0, std::min(mid - r.width() - 2, horizontalScrollBar()->maximum())));
    horizontalScrollBar()->setValue(column == 0 ? 0 : colpos[column - 1]);
}

int ZStatView::columnAt(int x) const
{
    QRect r = statRect();
    if (!r.contains(QPoint(x, r.top())))
        return -1;

    x += horizontalScrollBar()->value() - r.left();
    return std::upper_bound(colpos.begin(), colpos.end(), x) - colpos.begin();
}

void ZStatView::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);

    if (m == nullptr || (int)e->buttons() != 0)
    {
        ZToolTip::hideNow();
        return;
    }

    int col = columnAt(e->pos().x());
    if (col == -1)
    {
        ZToolTip::hideNow();
        return;
    }

    QString str = m->tooltip(col);
    if (str.isEmpty())
    {
        ZToolTip::hideNow();
        return;
    }

    QLabel *contents = new QLabel();
    contents->setText(str);
    ZToolTip::show(e->globalPos(), contents, viewport(), viewport()->rect(), INT_MAX, /*ZToolTip::isShown() ? 0 : -1*/ 0);
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

    ZRect r = statRect();
    p.setPen(Settings::uiColor(ColorSettings::Grid));
    p.drawLine(r.left() - 1, r.top(), r.left() - 1, r.bottom());
    
    // Draw vertical axis ticks.
    for (int ix = 0, siz = ticks.size(); ix != siz; ++ix)
    {
        int num = ticks[ix].first;
        int pos = ticks[ix].second;
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        if (ix != siz - 1 || ix == 0 || ticks[ix - 1].second != 0)
            p.drawLine(r.left() - 5, r.top() + pos, r.right(), r.top() + pos);

        // Draw tick text left to the grid lines.

        if (ix == siz - 1 && ix != 0 && ticks[ix - 1].second - fmh * 1.5 < 0)
            break;

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
    int left = m->type() == ZStatType::BarScroll ? horizontalScrollBar()->value() : 0;
    // Find the first bar to draw.
    int pos = m->type() == ZStatType::BarScroll ? std::upper_bound(colpos.begin(), colpos.end(), left) - colpos.begin() : 0;
    int prev = (pos == 0 ? 0 : colpos[pos - 1]);
    p.save();
    while ((m->type() == ZStatType::BarScroll && pos < colpos.size() && prev - left < r.width()) || (m->type() == ZStatType::BarStretch && pos < m->count()))
    {
        ZRect r2;
        if (m->type() == ZStatType::BarScroll)
        {
            r2 = ZRect(r.left() + prev - left, r.top(), colpos[pos] - prev, r.height());
            prev = colpos[pos];
        }
        else
        {
            int next = (r.width() - prev) / (m->count() - pos);
            r2 = ZRect(r.left() + prev, r.top(), next, r.height());
            prev += next;
        }

        p.setClipRect(r.intersected(r2), Qt::ClipOperation::ReplaceClip);
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        p.drawLine(r2.right() - 1, r.top(), r2.right() - 1, r.bottom());
        paintBar(p, pos, r2);

        r2.setTop(r2.bottom() + 6);
        r2.setBottom(height());
        p.setClipRect(QRect(QPoint(std::max(r2.left(), r.left()), r2.top()), QPoint(std::min(r2.right(), r.right()), r2.bottom())), Qt::ReplaceClip);
        if (m != nullptr)
        {
            p.setPen(Settings::textColor(ColorSettings::Text));
            p.drawText(r2, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, m->barLabel(pos));
        }

        ++pos;
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

void ZStatView::paintBar(QPainter &p, int col, ZRect r)
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


    // Start bar drawing below grid top, leaving out enough space for text above bar.
    if (r.height() > 0)
        maxval = maxval + maxval * (double)fmh * 1.5 / r.height();

    for (int ix = 0; ix != cnt; ++ix)
    {
        if (br.isNull())
        {
            br = QRect(r.left() + r.width() * 0.2, r.bottom() - (r.height() * ((double)sum / maxval)), r.width() * 0.6, 0);

            // Draw text above the bar
            p.setPen(Settings::textColor(ColorSettings::Text));
            QRect tr = QRect(br.left(), br.top() - fmh - 2, br.width(), fmh + 2);
            QString str = QString::number(sum);
            QRect br2 = p.boundingRect(tr, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, str);
            p.fillRect(br2.adjusted(-1, 0, 1, 0), Settings::textColor(ColorSettings::Bg));
            p.drawText(tr, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, str);
        }
        br.setBottom(r.bottom() - (r.height() * double(sum - stats[ix]) / maxval));
        sum -= stats[ix];
        p.fillRect(br, Settings::uiColor((ColorSettings::UIColorTypes)((int)ColorSettings::Stat1 + ix)));
        br.setTop(br.bottom());
    }
}

QRect ZStatView::statRect() const
{
    int fmh = fontMetrics().height();

    QRect r = viewport() != nullptr ? viewport()->rect() : rect();
    r.adjust(lm + hwidth + 6 + (vlabel.isEmpty() ? 0 : fmh), tm + fmh * 1.5, std::max( -(r.width() - (lm + hwidth + 6 + (vlabel.isEmpty() ? 0 : fmh))), -rm), std::max<int>(-(r.height() - tm), -bm - fmh * 1.5 - fmh - (hlabel.isEmpty() ? 0 : fmh)));
    if (r.height() < 0)
        r.setHeight(0);
    return r;
}

void ZStatView::updateView()
{
    ticks.clear();
    hwidth = 0;

    // Calculate the vertical tick positions.

    ZRect r = statRect();
    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();
    int maxval = m == nullptr ? 0 : m->maxValue();

    // When drawing the bar, fmh * 1.5 height will be excluded from top to draw the bar texts,
    // but the grid should be drawn above it. To compensate, another max val is used.
    if (r.height() > 0)
        maxval = maxval + maxval * (double)fmh * 1.5 / r.height();

    if (maxval != 0 && r.height() != 0)
    {

        // Calculating the tick steps.
        int steps = std::max<int>(maxval / ((double)r.height() / tickspacing) + 0.9, 1);

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
        int pos = r.height();
        // Saving horizontal tick lines and values.
        while (v <= maxval)
        {
            ticks.push_back(std::make_pair(v, pos));

            v += steps;
            pos = r.height() * ((double)(maxval - v) / maxval);
        }

        if (!ticks.empty() && ticks.back().second != 0)
            ticks.push_back(std::make_pair(maxval, 0));
    }
    if (!ticks.empty())
        hwidth = fm.width(QString::number(ticks.back().first));

    if (m != nullptr && m->type() == ZStatType::BarScroll)
    {
        horizontalScrollBar()->setRange(0, colpos.size() == 0 ? 0 : std::max(0, colpos.back() - r.width()));
        horizontalScrollBar()->setEnabled(colpos.back() > r.width());
        horizontalScrollBar()->setPageStep(std::max(1, r.width() - 24));
        horizontalScrollBar()->setSingleStep(std::max(1, std::min(r.width(), 24)));
    }
    viewport()->update();
}

