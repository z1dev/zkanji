/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
#include "zui.h"

#include "checked_cast.h"

//-------------------------------------------------------------


ZStatView::ZStatView(QWidget *parent) : base(parent), m(nullptr), lm(32), tm(12), rm(lm), bm(tm), tickspacing(60), hwidth(16)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

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

    if (m != nullptr)
        disconnect(m, &ZAbstractStatModel::modelChanged, this, &ZStatView::onModelChanged);
    m = model;

    onModelChanged();

    if (m != nullptr)
        connect(m, &ZAbstractStatModel::modelChanged, this, &ZStatView::onModelChanged);
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
    if (column >= tosigned(colpos.size()))
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

bool ZStatView::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        onModelChanged();

    return base::event(e);
}

void ZStatView::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);

    if (m == nullptr || (int)e->buttons() != 0)
    {
        ZToolTip::hideNow();
        return;
    }

    if (m != nullptr && m->type() == ZStatType::Bar)
    {
        ZAbstractBarStatModel *sm = dynamic_cast<ZAbstractBarStatModel*>(m);

        int col = columnAt(e->pos().x());
        if (col == -1)
        {
            ZToolTip::hideNow();
            return;
        }

        QString str = sm->tooltip(col);
        if (str.isEmpty())
        {
            ZToolTip::hideNow();
            return;
        }

        QLabel *contents = new QLabel();
        contents->setText(str);
        ZToolTip::show(e->globalPos(), contents, viewport(), viewport()->rect(), INT_MAX, 0);
    }
    else if (m != nullptr && m->type() == ZStatType::Area)
    {
        ZAbstractAreaStatModel *am = dynamic_cast<ZAbstractAreaStatModel*>(m);
        
        ZRect r = statRect();
        if (!r.contains(e->pos()))
            return;

        qint64 from = am->firstDate();
        qint64 to = am->lastDate();

        qint64 ldatepos = from + (to - from) * ((double)(e->pos().x() - r.left() + 1) / r.width());
        int col = -1;
        if (ldatepos > to)
            col = am->count();
        else if (ldatepos >= from)
        {
            int lpos = 0;
            int rpos = am->count();
            int mid;
            qint64 middatepos;
            //qint64 mindatepos = from;
            //qint64 maxdatepos = to;
            while (lpos < rpos)
            {
                mid = (lpos + rpos) / 2;
                middatepos = am->valueDate(mid);
                if (middatepos <= ldatepos)
                {
                    lpos = mid + 1;
                    //mindatepos = am->valueDate(mid + 1)
                }
                else
                {
                    rpos = mid;
                    //maxdatepos = am->valueDate(mid);
                }
            }
            col = lpos - 1;
        }

        QString str = am->tooltip(col);
        if (str.isEmpty())
        {
            ZToolTip::hideNow();
            return;
        }

        QLabel *contents = new QLabel();
        contents->setText(str);
        ZToolTip::show(e->globalPos(), contents, viewport(), viewport()->rect(), INT_MAX, 0);
    }

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

void ZStatView::scrollContentsBy(int /*dx*/, int /*dy*/)
{
    viewport()->update();
}

void ZStatView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(viewport());
    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();

    p.fillRect(rect(), Settings::textColor(this, ColorSettings::Bg));

    ZRect r = statRect();
    p.setPen(Settings::uiColor(ColorSettings::Grid));
    p.drawLine(r.left() - 1, r.top(), r.left() - 1, r.bottom() + 5);

    int maxval = m == nullptr ? 0 : m->maxValue();

    // Draw vertical axis ticks.
    for (int ix = 0, siz = tosigned(ticks.size()); ix != siz; ++ix)
    {
        int num = ticks[ix].first;
        int pos = ticks[ix].second;
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        if (ix != siz - 1 || ix == 0 || ticks[ix - 1].second != 0)
            p.drawLine(r.left() - 5, r.top() + pos, r.right() - 1, r.top() + pos);

        // Draw tick text left to the grid lines.

        if (ix == siz - 1 && ix != 0 && ticks[ix - 1].second - fmh * 1.5 < 0)
            break;

        p.setPen(Settings::textColor(this, ColorSettings::Text));
        p.drawText(QRectF(0, r.top() + pos - fmh / 2, r.left() - 7, fmh), QString::number(num), Qt::AlignRight | Qt::AlignVCenter);
    }
    if (ticks.empty() || ticks.back().second != 0)
    {
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        p.drawLine(r.left() - 5, r.top(), r.right() - 1, r.top());

        if (ticks.empty())
            p.drawLine(r.left() - 5, r.bottom(), r.right() - 1, r.bottom());
    }

    if (!vlabel.isEmpty())
    {
        p.save();
        p.setPen(Settings::textColor(this, ColorSettings::Text));
        p.rotate(-90);
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(-r.top() - r.height(), 0, r.height(), r.left() - 6 - hwidth), Qt::AlignVCenter | Qt::AlignHCenter, vlabel);
        p.restore();
    }
    p.save();

    int lastlinepos = -1;

    if (m != nullptr && m->type() == ZStatType::Bar)
    {
        ZAbstractBarStatModel *sm = dynamic_cast<ZAbstractBarStatModel*>(m);

        // Draw bars and horizontal axis.
        int left = !stretched ? horizontalScrollBar()->value() : 0;
        // Find the first bar to draw.
        int pos = !stretched ? std::upper_bound(colpos.begin(), colpos.end(), left) - colpos.begin() : 0;
        int prev = (pos == 0 ? 0 : colpos[pos - 1]);

        while ((!stretched && pos < tosigned(colpos.size()) && prev - left < r.width()) || (stretched && pos < sm->count()))
        {
            ZRect r2;
            if (!stretched)
            {
                r2 = ZRect(r.left() + prev - left, r.top(), colpos[pos] - prev, r.height());
                prev = colpos[pos];
            }
            else
            {
                int next = (r.width() - prev) / (sm->count() - pos);
                r2 = ZRect(r.left() + prev, r.top(), next, r.height());
                prev += next;
            }

            p.setPen(Settings::uiColor(ColorSettings::Grid));
            lastlinepos = r2.right() - 1;
            p.setClipRect(r.intersected(r2).adjusted(0, 0, 0, 5), Qt::ClipOperation::ReplaceClip);
            p.drawLine(r2.right() - 1, r.top(), r2.right() - 1, r.bottom() + 5);
            paintBar(p, pos, r2);

            r2.setTop(r2.bottom() + 7);
            r2.setBottom(height());
            p.setClipRect(QRect(QPoint(std::max(r2.left(), r.left()), r2.top()), QPoint(std::min(r2.right(), r.right()), r2.bottom())), Qt::ReplaceClip);

            p.setPen(Settings::textColor(this, ColorSettings::Text));
            p.drawText(r2, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, sm->barLabel(pos));

            ++pos;
        }
    }
    else if (m != nullptr && m->type() == ZStatType::Area)
    {
        ZAbstractAreaStatModel *am = dynamic_cast<ZAbstractAreaStatModel*>(m);

        qint64 from = am->firstDate();
        qint64 to = am->lastDate();

        // Draw x-axis grid lines at equal distances but only for exact date positions. They
        // all should show the same time of the day as the first one.

        p.setPen(Settings::textColor(this, ColorSettings::Text));

        int barw = fm.width(QStringLiteral("9999:99:99")) + 16;
        // Pixel position in stat drawing rectangle.
        int left = 0;

        QDateTime date = QDateTime::fromMSecsSinceEpoch(from);
        QTime time = date.time();
        QString str;
        QRect dater;

        while (left < r.width())
        {
            str = DateTimeFunctions::formatDay(date.date());
            // Rectangle of string adjusted with a random margin to make it fit for sure.
            dater = fm.boundingRect(str).adjusted(-5, 0, 5, 5);
            dater.moveTo(left + r.left() - dater.width() / 2, r.bottom() + 7);

            p.setPen(Settings::textColor(this, ColorSettings::Text));
            p.drawText(dater, Qt::AlignHCenter | Qt::AlignTop, str);

            if (left != 0)
            {
                p.setPen(Settings::uiColor(ColorSettings::Grid));
                p.drawLine(left + r.left(), r.top(), left + r.left(), r.bottom() + 5);
            }

            // Adjust leftition for next bar and label.

            if (to == from || r.width() == 0)
                break;

            left += barw;
            qint64 datenum = from + (to - from) * ((double)left / r.width());
            QDateTime date2 = QDateTime::fromMSecsSinceEpoch(datenum);
            if (date2.time() > time)
                date = QDateTime(date2.date().addDays(1), time);
            else
                date = QDateTime(date2.date(), time);

            datenum = date.toMSecsSinceEpoch();
            left = ((long double)(datenum - from) / (to - from)) * r.width();
            //date = QDateTime::fromMSecsSinceEpoch(datenum);
        }

        // Draw the graph on top of the grid lines.

        left = 0;

        int pos = 0;
        int siz = am->count();

        // Date at the current pixel position.
        qint64 fdatepos = from;
        // Date at the next pixel position.
        qint64 ldatepos = from + (to - from) * ((double)(left + 1) / r.width());

        // Position in the model's values in front of the current pixel.
        int fpos = -1;
        // Last position in the model's values in the current pixel, or a position after that
        // if no position is within the pixel.
        int lpos = siz;
        qint64 fdate = 0;
        qint64 ldate = fdatepos;

        // Find the data positions to be drawn for the current pixel column.
        while (pos < siz)
        {
            qint64 datenum = am->valueDate(pos);
            if (datenum < fdatepos)
            {
                // Fpos should always refer to data before current pixel column.

                fpos = pos;
                fdate = datenum;
            }
            else if (lpos == siz || datenum < ldatepos)
            {
                // Lpos should always refer to data within current pixel column, or after only
                // when no suitable data position is within this column.
                lpos = pos;
                ldate = datenum;
            }
            
            if (datenum >= ldatepos)
                break;
            ++pos;
        }

        if (lpos == siz)
        {
            // There is no data position in or after the current pixel column. Setting it to a
            // lower value will make sure no drawing is done.
            lpos = fpos;
            ldate = fdate;
        }

        while (maxval != 0 && left < r.width())
        {
            if (fdatepos > ldate)
                break;

            // Draw the next column.
            if (fdatepos > fdate && (fpos != -1 || ldate < ldatepos))
            {
                double v0 = 0;
                double v1 = 0;
                double v2 = 0;
                if (ldatepos <= ldate)
                {
                    // Current pixel is between two values. Draw an average of the two sides.

                    double mid = ((long double)(fdatepos + ldatepos) / 2 - fdate) / (ldate - fdate);

                    int fv0 = fpos == -1 ? 0 : am->value(fpos, 0);
                    int fv1 = fpos == -1 ? 0 : am->value(fpos, 1);
                    int fv2 = fpos == -1 ? 0 : am->value(fpos, 2);

                    int lv0 = lpos == siz ? 0 : am->value(lpos, 0);
                    int lv1 = lpos == siz ? 0 : am->value(lpos, 1);
                    int lv2 = lpos == siz ? 0 : am->value(lpos, 2);

                    v0 = fv0 + double(lv0 - fv0) * mid;
                    v1 = fv1 + double(lv1 - fv1) * mid;
                    v2 = fv2 + double(lv2 - fv2) * mid;
                }
                else
                {
                    pos = fpos;

                    while (++pos <= std::min(lpos, siz - 1))
                    {
                        int fv0 = am->value(pos, 0);
                        int fv1 = am->value(pos, 1);
                        int fv2 = am->value(pos, 2);

                        fv0 += fv1 + fv2;
                        fv1 += fv2;

                        if (v0 < fv0)
                            v0 = fv0;
                        if (v1 < fv1)
                            v1 = fv1;
                        if (v2 < fv2)
                            v2 = fv2;

                        //if (fv0 + fv1 + fv2 > v0 + v1 + v2)
                        //{
                        //    v0 = fv0;
                        //    v1 = fv1;
                        //    v2 = fv2;
                        //}
                    }
                    if (v0 < std::max(v1, v2))
                        v0 = 0;
                    else
                        v0 -= std::max(v1, v2);
                    if (v1 < v2)
                        v1 = 0;
                    else
                        v1 -= v2;

                }


                if (v0 != 0)
                {
                    p.setPen(Settings::uiColor(ColorSettings::Stat1));
                    p.drawLine(left + r.left(), r.bottom() - double(v0 + v1 + v2) / maxval * r.height(), left + r.left(), r.bottom() - double(v1 + v2) / maxval * r.height());
                }
                if (v1 != 0)
                {
                    p.setPen(Settings::uiColor(ColorSettings::Stat2));
                    p.drawLine(left + r.left(), r.bottom() - double(v1 + v2) / maxval * r.height(), left + r.left(), r.bottom() - double(v2) / maxval * r.height());
                }

                if (v2 != 0)
                {
                    p.setPen(Settings::uiColor(ColorSettings::Stat3));
                    p.drawLine(left + r.left(), r.bottom() - double(v2) / maxval * r.height(), left + r.left(), r.bottom());
                }
            }

            ++left;
            fdatepos = ldatepos;
            ldatepos = from + (to - from) * ((double)(left + 1) / r.width());

            if (fdatepos > ldate)
            {
                fpos = lpos;
                fdate = ldate;

                pos = lpos;
                while (++pos < siz)
                {
                    qint64 datenum = am->valueDate(pos);
                    if (datenum < ldatepos || ldate < fdatepos)
                    {
                        lpos = pos;
                        ldate = datenum;
                    }
                    if (datenum >= ldatepos)
                        break;
                }

                if (pos >= siz)
                {
                    lpos = siz;
                    ldate = QDateTime(QDateTime::fromMSecsSinceEpoch(am->valueDate(siz - 1)).addDays(1).date(), time).toMSecsSinceEpoch();
                }
            }
        }


    }

    p.restore();

    if (lastlinepos != r.right() - 1)
    {
        p.setPen(Settings::uiColor(ColorSettings::Grid));
        p.drawLine(r.right() - 1, r.top(), r.right() - 1, r.bottom());
    }


    if (!hlabel.isEmpty())
    {
        p.save();
        p.setPen(Settings::textColor(this, ColorSettings::Text));
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(r.left(), r.bottom() + fmh + 12, r.width(), fmh), Qt::AlignTop | Qt::AlignHCenter, hlabel);
        p.restore();
    }
}

void ZStatView::onModelChanged()
{
    colpos.clear();
    hlabel = QString();
    vlabel = QString();

    stretched = true;
    if (m != nullptr)
    {
        ZAbstractBarStatModel *sm = dynamic_cast<ZAbstractBarStatModel*>(m);
        if (sm != nullptr)
        {
            if (sm->count() != 0 && sm->barWidth(this, 0) >= 0)
            {
                stretched = false;
                int p = 0;
                for (int ix = 0, siz = sm->count(); ix != siz; ++ix)
                {
                    p += sm->barWidth(this, ix);
                    colpos.push_back(p);
                }
            }
        }

        hlabel = m->axisLabel(Qt::Horizontal);
        vlabel = m->axisLabel(Qt::Vertical);
    }

    updateView();
    if (m != nullptr && !stretched)
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    else
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ZStatView::paintBar(QPainter &p, int col, ZRect r)
{
    ZAbstractBarStatModel *sm = dynamic_cast<ZAbstractBarStatModel*>(m);
    if (sm == nullptr)
        return;

    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();
    int maxval = m->maxValue();
    if (maxval == 0)
        return;

    int cnt = sm->valueCount();
    int sum = 0;

    fastarray<int> stats(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        int v = sm->value(col, ix);
        stats[ix] = v;
        sum += v;
    }

    if (sum == 0)
        return;

    // Drawing the bar rectangles.
    QRect br;

    // When drawing the bar, fmh * 1.5 height will be excluded from top to draw the bar texts,
    // but the grid should be drawn above it.
    int gap = std::min<int>(r.height() / 2, fmh * 1.5);

    for (int ix = 0; ix != cnt; ++ix)
    {
        if (br.isNull())
        {
            br = QRect(r.left() + r.width() * 0.2, r.bottom() - ((r.height() - gap) * ((double)sum / maxval)), r.width() * 0.6, 0);

            // Draw text above the bar
            p.setPen(Settings::textColor(this, ColorSettings::Text));
            QRect tr = QRect(br.left(), br.top() - fmh - 2, br.width(), fmh + 2);
            QString str = QString::number(sum);
            QRect br2 = p.boundingRect(tr, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, str);
            p.fillRect(br2.adjusted(-1, 0, 1, 0), Settings::textColor(this, ColorSettings::Bg));
            p.drawText(tr, Qt::AlignTop | Qt::AlignHCenter | Qt::TextSingleLine, str);
        }
        br.setBottom(r.bottom() - ((r.height() - gap) * double(sum - stats[ix]) / maxval));
        sum -= stats[ix];
        p.fillRect(br, Settings::uiColor((ColorSettings::UIColorTypes)((int)ColorSettings::Stat1 + ix)));
        br.setTop(br.bottom());
    }
}

QRect ZStatView::statRect() const
{
    int fmh = fontMetrics().height();

    QRect r = viewport() != nullptr ? viewport()->rect() : rect();
    r.adjust(lm + hwidth + 6 + (vlabel.isEmpty() ? 0 : fmh), tm + fmh * 1.5, -rm, -bm - fmh - 12 - (hlabel.isEmpty() ? 0 : fmh));
    if (r.height() < 0)
        r.setHeight(0);
    if (r.width() < 0)
        r.setWidth(0);
    return r;
}

void ZStatView::updateView()
{
    ticks.clear();
    hwidth = 0;

    // Calculate the vertical tick and grid positions.

    ZRect r = statRect();
    QFontMetrics fm = fontMetrics();
    int fmh = fm.height();
    int maxval = m == nullptr ? 0 : m->maxValue();

    // When drawing the bar, fmh * 1.5 height will be excluded from top to draw the bar texts,
    // but the grid should be drawn above it.
    int gap = m == nullptr || m->type() != ZStatType::Bar ? 0 : std::min<int>(r.height() / 2, fmh * 1.5);

    if (maxval != 0 && r.height() != 0)
    {
        // Calculating the tick steps.

        // Number of values between ticks.
        int steps = std::max<int>(maxval / ((double)(r.height() - gap) / tickspacing) + 0.9, 1);
        // Maximum value that could be shown in the grids.
        int gapmaxval = gap == 0 || r.height() - gap == 0 ? maxval : maxval + gap * (maxval / (double)(r.height() - gap));

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
        while (v <= gapmaxval)
        {
            ticks.push_back(std::make_pair(v, pos));

            v += steps;
            pos = (r.height() - gap) * ((double)(maxval - v) / maxval) + gap;
        }

        if (!ticks.empty() && ticks.back().second != 0 && gapmaxval != maxval)
            ticks.push_back(std::make_pair(gapmaxval, 0));
    }
    if (!ticks.empty())
    {
        hwidth = fm.width(QString::number(ticks.back().first));
        r.setLeft(r.left() + hwidth);
    }

    if (m != nullptr && !stretched)
    {
        horizontalScrollBar()->setRange(0, colpos.size() == 0 ? 0 : std::max(0, colpos.back() - r.width()));
        horizontalScrollBar()->setEnabled(colpos.back() > r.width());
        horizontalScrollBar()->setPageStep(std::max(1, r.width() - 24));
        horizontalScrollBar()->setSingleStep(std::max(1, std::min(r.width(), 24)));
    }
    viewport()->update();
}


//-------------------------------------------------------------
