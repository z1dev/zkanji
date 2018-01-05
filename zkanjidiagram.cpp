/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QWindow>

#include <cmath>

#include "zkanjidiagram.h"
#include "kanji.h"
#include "zui.h"
#include "kanjistrokes.h"
#include "fontsettings.h"
#include "colorsettings.h"
#include "globalui.h"
#include "zkanjimain.h"

//-------------------------------------------------------------


const double kanjiRectMul = 0.87;
const double partcountdiv = 75.0;

ZKanjiDiagram::ZKanjiDiagram(QWidget *parent) : base(parent), kindex(INT_MIN), showstrokes(false), showradical(false), showdiagram(true), showgrid(false),
        showshadow(false), shownumbers(false), showdir(false), drawspeed(3), strokepos(0), partcnt(0), partpos(0), delaycnt(0), drawnpos(0)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);

    //connect(qApp, &QGuiApplication::applicationStateChanged, this, &ZKanjiDiagram::appStateChanged);
    connect(qApp, &QGuiApplication::focusWindowChanged, this, &ZKanjiDiagram::appStateChanged);
    connect(gUI, &GlobalUI::settingsChanged, this, &ZKanjiDiagram::settingsChanged);
}

ZKanjiDiagram::~ZKanjiDiagram()
{

}

void ZKanjiDiagram::setKanjiIndex(int newindex)
{
    if (kindex == newindex)
        return;

    stop();

    kindex = newindex;
    numrects.clear();

    partpos = 0;
    partcnt = 0;
    delaycnt = 0;
    strokepos = 0;

    image.reset();
    if (kindex != INT_MIN && ZKanji::elements()->size() != 0)
        strokepos = ZKanji::elements()->strokeCount(newindex < 0 ? (-1 - newindex) : ZKanji::kanjis[kindex]->element, 0);

    update();
}

int ZKanjiDiagram::kanjiIndex() const
{
    return kindex;
}

bool ZKanjiDiagram::strokes() const
{
    return showstrokes;
}

void ZKanjiDiagram::setStrokes(bool setshow)
{
    if (ZKanji::elements()->size() == 0 || showstrokes == setshow)
        return;

    showstrokes = true;
    update();
}

bool ZKanjiDiagram::radical() const
{
    return showradical;
}

void ZKanjiDiagram::setRadical(bool setshow)
{
    if (showradical == setshow)
        return;
    showradical = setshow;
    update();
}

bool ZKanjiDiagram::diagram() const
{
    return showdiagram;
}

void ZKanjiDiagram::setDiagram(bool setshow)
{
    if (showdiagram == setshow)
        return;

    if (showdiagram)
        stop();
    showdiagram = setshow;
    update();
}

bool ZKanjiDiagram::grid() const
{
    return showgrid;
}

void ZKanjiDiagram::setGrid(bool setshow)
{
    if (showgrid == setshow)
        return;

    showgrid = setshow;
    update();
}

bool ZKanjiDiagram::shadow() const
{
    return showshadow;
}

void ZKanjiDiagram::setShadow(bool setshow)
{
    if (showshadow == setshow)
        return;

    showshadow = setshow;
    image.reset();
    update();
}

bool ZKanjiDiagram::numbers() const
{
    return shownumbers;
}

void ZKanjiDiagram::setNumbers(bool setshow)
{
    if (shownumbers == setshow)
        return;

    shownumbers = setshow;
    image.reset();
    update();
}

bool ZKanjiDiagram::direction() const
{
    return showdir;
}

void ZKanjiDiagram::setDirection(bool setshow)
{
    if (showdir == setshow)
        return;

    showdir = setshow;
    image.reset();
    update();
}

int ZKanjiDiagram::speed() const
{
    return drawspeed;
}

void ZKanjiDiagram::setSpeed(int val)
{
    val = clamp(val, 1, 4);
    if (drawspeed == val)
        return;
    drawspeed = val;
    if (playing())
    {
        pause();
        play();
    }
}

bool ZKanjiDiagram::playing() const
{
    if (kindex == INT_MIN)
        return false;
    return timer.isActive();
}

bool ZKanjiDiagram::paused() const
{
    if (kindex == INT_MIN)
        return false;
    return strokepos != ZKanji::elements()->strokeCount(kindex < 0 ? (-1 - kindex) : ZKanji::kanjis[kindex]->element, 0);
}

void ZKanjiDiagram::play()
{
    if (kindex == INT_MIN || (kindex >= 0 && !showdiagram))
        return;

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    bool start = false;
    if (strokepos == ZKanji::elements()->strokeCount(elem, 0))
    {
        start = true;

        // Restarting the kanji drawing.
        QRect r = rect();
        int siz = std::min(r.width(), r.height()) * kanjiRectMul;
        strokepos = 0;
        partpos = 0;
        delaycnt = 0;
        numrects.clear();
        partcnt = ZKanji::elements()->strokePartCount(elem, 0, strokepos, QRectF(0, 0, siz, siz), siz / partcountdiv, parts);

        if (!stroke || stroke->width() != siz)
            stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));

        stroke->fill(qRgba(0, 0, 0, 0));

        update();
    }

    timer.start(drawspeed == 1 ? 50 : drawspeed == 2 ? 30 : drawspeed == 3 ? 18 : 10, this);

    if (start)
        emit strokeChanged(strokepos, false);
}

void ZKanjiDiagram::pause()
{
    timer.stop();
}

void ZKanjiDiagram::stop()
{
    if (kindex == INT_MIN || ZKanji::elements()->size() == 0)
        return;

    timer.stop();

    int elem = kindex < 0 ? (-1 - kindex) : ZKanji::kanjis[kindex]->element;
    if (strokepos != ZKanji::elements()->strokeCount(elem, 0))
    {
        strokepos = ZKanji::elements()->strokeCount(elem, 0);

        stroke.reset();
        partpos = 0;
        partcnt = 0;
        delaycnt = 0;

        emit strokeChanged(strokepos, true);
        update();
    }
}

void ZKanjiDiagram::rewind()
{
    timer.stop();
    if (kindex == INT_MIN || (kindex >= 0 && !showdiagram))
        return;

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    strokepos = 0;
    partpos = 0;
    delaycnt = 0;
    numrects.clear();
    QRect r = rect();
    int siz = std::min(r.width(), r.height()) * kanjiRectMul;
    partcnt = ZKanji::elements()->strokePartCount(elem, 0, strokepos, QRectF(0, 0, siz, siz), siz / partcountdiv, parts);

    if (!stroke || stroke->width() != siz)
        stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));

    stroke->fill(qRgba(0, 0, 0, 0));

    emit strokeChanged(0, false);

    update();
}

void ZKanjiDiagram::back()
{
    timer.stop();
    if (kindex == INT_MIN || (kindex >= 0 && !showdiagram) || (strokepos == 0 && partpos == 0))
        return;

    bool sposchanged = false;
    if (partpos == 0)
    {
        strokepos = strokepos - 1;
        sposchanged = true;
    }
    partpos = 0;
    delaycnt = 0;

    if (!numrects.empty())
        numrects.pop_back();

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    QRect r = rect();
    int siz = std::min(r.width(), r.height()) * kanjiRectMul;

    if (!stroke || stroke->width() != siz)
        stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));

    stroke->fill(qRgba(0, 0, 0, 0));

    partcnt = ZKanji::elements()->strokePartCount(elem, 0, strokepos, QRectF(0, 0, siz, siz), siz / partcountdiv, parts);

    if (sposchanged)
        emit strokeChanged(strokepos, false);

    update();
}

void ZKanjiDiagram::next()
{
    timer.stop();
    if (kindex == INT_MIN || (kindex >= 0 && !showdiagram) || strokepos == ZKanji::elements()->strokeCount(kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element, 0))
        return;

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    if (strokepos == ZKanji::elements()->strokeCount(elem, 0) - 1)
    {
        stop();
        return;
    }

    strokepos = strokepos + 1;
    partpos = 0;
    delaycnt = 0;

    QRect r = rect();
    int siz = std::min(r.width(), r.height()) * kanjiRectMul;

    if (!stroke || stroke->width() != siz)
        stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));

    stroke->fill(qRgba(0, 0, 0, 0));

    partcnt = ZKanji::elements()->strokePartCount(elem, 0, strokepos, QRectF(0, 0, siz, siz), siz / partcountdiv, parts);

    emit strokeChanged(strokepos, strokepos == ZKanji::elements()->strokeCount(elem, 0));

    update();
}

bool ZKanjiDiagram::event(QEvent *e)
{
    if (e->type() == QEvent::Timer)
    {
        QTimerEvent *te = (QTimerEvent*)e;
        if (te->timerId() == timer.timerId())
        {
            if (delaycnt != 0)
            {
                --delaycnt;
                return true;
            }

            int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;

            bool resized = false;
            QRect r = rect();
            int siz = std::min(r.width(), r.height()) * kanjiRectMul;
            if (stroke && stroke->width() != siz)
            {
                stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));
                stroke->fill(qRgba(0, 0, 0, 0));
                resized = true;
            }

            ++partpos;
            QPainter p;
            p.begin(stroke.get());
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::transparent);
            p.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Text));

            if (resized)
                for (int ix = 0; ix != partpos - 1; ++ix)
                {
                    if (showdir)
                        ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, ix, Settings::uiColor(ColorSettings::StrokeDot), Settings::textColor(isActiveWindow(), ColorSettings::Text));
                    else
                        ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, ix);
                }

            if (showdir)
                ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, partpos - 1, Settings::uiColor(ColorSettings::StrokeDot), Settings::textColor(isActiveWindow(), ColorSettings::Text));
            else
                ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, partpos - 1);
            p.end();

            if (partpos == partcnt)
            {
                // Copy finished stroke to image.

                QPainter pt;
                pt.begin(image.get());
                pt.drawImage(0, 0, *stroke.get());
                pt.end();

                // Start the next stroke but don't paint it yet.

                partpos = 0;

                // TODO: make delay between strokes settable.
                delaycnt = 8;
                ++strokepos;
                ++drawnpos;
                stroke->fill(qRgba(0, 0, 0, 0));

                if (strokepos != ZKanji::elements()->strokeCount(elem, 0))
                {
                    int siz = std::min(stroke->width(), stroke->height());
                    partcnt = ZKanji::elements()->strokePartCount(elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), siz / partcountdiv, parts);
                    emit strokeChanged(strokepos, false);
                }
                else
                {
                    stroke.reset();
                    delaycnt = 0;
                    timer.stop();
                    emit strokeChanged(strokepos, true);
                }
            }

            update();

            return true;
        }
    }

    return base::event(e);
}

void ZKanjiDiagram::resizeEvent(QResizeEvent *e)
{
    numrects.clear();
    update();
    base::resizeEvent(e);
}

namespace ZKanji
{
    // Same as the radsymbols in ZRadicalGrid, without the added stroke count.
    extern QChar radsymbols[231];
}

void ZKanjiDiagram::paintEvent(QPaintEvent *e)
{
    // Size, the kanji should take up inside the drawing, between 0 and 1.
    const double kanjifontsize = kanjiRectMul;
    const int gridpadding = 6;

    QPainter p(this);
    QRect r = rect();
    p.fillRect(r, Settings::textColor(isActiveWindow(), ColorSettings::Bg));

    p.setRenderHint(QPainter::Antialiasing);

    if (showgrid)
    {
        //QColor gridColor;
        //if (Settings::colors.grid.isValid())
        //    gridColor = Settings::colors.grid;
        //else
        //{
        //    QStyleOptionViewItem opts;
        //    opts.initFrom(this);
        //    opts.showDecorationSelected = true;
        //    int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, this);
        //    gridColor = static_cast<QRgb>(gridHint);
        //}

        QColor gridColor = Settings::uiColor(ColorSettings::Grid);

        p.setPen(gridColor);
        p.setBrush(Qt::transparent);
        QPen pen = p.pen();
        pen.setWidth(2);
        p.setPen(pen);

        p.drawLine(QLineF(r.center().x() - 0.5, r.top() + gridpadding, r.center().x() - 0.5, r.bottom() - gridpadding));
        p.drawLine(QLineF(r.left() + gridpadding, r.center().y() - 0.5, r.right() - gridpadding, r.center().y() - 0.5));

        pen.setWidth(3);
        p.setPen(pen);

        int x = (r.width() * (1 - kanjifontsize * 1.05) * 0.5);
        int y = (r.height() * (1 - kanjifontsize * 1.05) * 0.5);
        p.drawRect(QRectF(r.left() + x - 0.5, r.top() + y - 0.5, r.width() - x * 2, r.height() - y * 2));

        pen.setStyle(Qt::DashLine);
        pen.setWidth(1);
        p.setPen(pen);

        x = r.left() + (r.width() * (1 - kanjifontsize * 1.05) * 0.5) - 0.5;
        p.drawLine(QLineF(r.left() + int((r.center().x() + x) * 0.5) - 0.5, r.top() + gridpadding, r.left() + int((r.center().x() + x) * 0.5) - 0.5, r.bottom() - gridpadding));
        p.drawLine(QLineF(r.center().x() + int((r.center().x() - x) * 0.5) - 0.5, r.top() + gridpadding, r.center().x() + int((r.center().x() - x) * 0.5) - 0.5, r.bottom() - gridpadding));
        y = r.top() + (r.height() * (1 - kanjifontsize * 1.05) * 0.5) - 0.5;
        p.drawLine(QLineF(r.left() + gridpadding, r.top() + int((r.center().y() + y) * 0.5) - 0.5, r.right() - gridpadding, r.top() + int((r.center().y() + y) * 0.5) - 0.5));
        p.drawLine(QLineF(r.left() + gridpadding, r.center().y() + int((r.center().y() - y) * 0.5) - 0.5, r.right() - gridpadding, r.center().y() + int((r.center().y() - y) * 0.5) - 0.5));

        pen.setStyle(Qt::SolidLine);
        p.setPen(pen);
    }

    if (kindex == INT_MIN)
        return;

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    QChar k = kindex < 0 ? QChar() : ZKanji::kanjis[kindex]->ch;

    if ((kindex >= 0 && !showdiagram) || elem == -1)
    {
        QFont f = Settings::kanjiFont();

        //adjustFontSize(f, r.height() * kanjifontsize/*, k*/);
        f.setPixelSize(r.height() * kanjifontsize * 1.05);

        //f.setOverline(true);
        //f.setUnderline(true);

        //QFontMetrics fm(f);

        p.setPen(Settings::textColor(isActiveWindow(), ColorSettings::Text));
        p.setFont(f);

        //p.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, QString(k));

        int y = r.top() + r.height() * (0.38 + kanjifontsize * 1.05 * 0.5);
        drawTextBaseline(&p, r.left(), y, true, r, k);

        //p.drawLine(r.left(), y, r.right(), y);
        //y = y - fm.ascent();
        //p.drawLine(r.left(), y, r.right(), y);
        //y = y + fm.ascent() + fm.descent();
        //p.drawLine(r.left(), y, r.right(), y);

    }
    else
    {
        updateImage();
        p.drawImage((r.width() - image->width()) / 2, (r.height() - image->height()) / 2, *image);

        if (stroke)
        {
            p.setPen(Qt::transparent);
            p.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Text));

            p.drawImage((r.width() - stroke->width()) / 2, (r.height() - stroke->height()) / 2, *stroke);
            if (strokepos != ZKanji::elements()->strokeCount(elem, 0) && partpos != 0)
            {
                if (showdir)
                    ZKanji::elements()->drawStrokePart(p, true, elem, 0, strokepos, QRectF((r.width() - stroke->width()) / 2, (r.height() - stroke->height()) / 2, stroke->width(), stroke->height()), parts, partpos - 1, Settings::uiColor(ColorSettings::StrokeDot), Settings::textColor(isActiveWindow(), ColorSettings::Text));
                else
                    ZKanji::elements()->drawStrokePart(p, true, elem, 0, strokepos, QRectF((r.width() - stroke->width()) / 2, (r.height() - stroke->height()) / 2, stroke->width(), stroke->height()), parts, partpos - 1);
            }
        }

        while (numrects.size() > strokepos + (partpos != 0 ? 1 : 0))
            numrects.pop_back();
        for (int ix = numrects.size(), siz = strokepos + (partpos != 0 ? 1 : 0); ix != siz; ++ix)
        {
            StrokeDirection dir;
            QPoint pt;
            ZKanji::elements()->strokeData(elem, 0, ix, image->rect(), dir, pt);
            addNumberRect(ix, dir, pt + QPoint((r.width() - image->width()) / 2, (r.height() - image->height()) / 2));
        }

        // TODO: (later) ADD BACK IF a dot indication is needed at the start of strokes for the stroke direction.
        //if (showdir)
        //{
        //    for (int ix = 0, siz = numrects.size(); ix != siz; ++ix)
        //    {
        //        QPointF pos = numrects[ix].first;
        //        p.setBrush(Qt::red);
        //        p.setPen(Qt::transparent);
        //        double rsiz = ZKanji::elements()->basePenWidth(std::min(image->width(), image->height())) * 0.45;
        //        p.drawEllipse(pos, rsiz, rsiz);
        //    }
        //}

        if (shownumbers)
        {
            for (int ix = 0, siz = numrects.size(); ix != siz; ++ix)
                drawNumberRect(p, ix, false);
            for (int ix = 0, siz = numrects.size(); ix != siz; ++ix)
                drawNumberRect(p, ix, true);
        }
    }

    double h = 0;
    double minw = 0;
    if (showstrokes)
    {
        p.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Bg));
        p.setPen(Settings::textColor(isActiveWindow(), ColorSettings::Text));

        //QFont f = Settings::mainFont();
        //f.setPointSize(10);
        //p.setFont(f);
        //QString str = tr("Strokes") + ": ";
        //drawTextBaseline(&p, 5, r.height() - 6, false, r, str);

        //QFontMetricsF fm(f);
        //int x = fm.width(str);

        //f.setPointSize(14);
        //p.setFont(f);
        //str = QString::number(ZKanji::kanjis[kindex]->strokes);
        //drawTextBaseline(&p, 5 + x, r.height() - 6, false, r, str);

        //QSizeF s = fm.boundingRect("09").size();
        //QRectF rf = QRectF(0.5, 0.5, std::max(s.width(), s.height()) + 2, std::max(s.width(), s.height()) + 2);
        ////rf.moveTo(0.5, r.height() - rf.height() - 0.5);
        //p.drawRect(rf);

        //p.setFont(f);
        //drawTextBaseline(&p, QPointF(rf.left(), rf.top() + rf.height() * 0.8), true, rf, QString::number(ZKanji::kanjis[kindex]->strokes));

        QFont f = Settings::mainFont();
        f.setPointSize(13);
        QFontMetricsF fm(f);

        QSizeF s = fm.boundingRect("09").size();
        QRectF rf = QRectF(0, 0, std::max(s.width(), s.height()) + 2, std::max(s.width(), s.height()) + 2);
        rf.moveTo(r.width() - rf.width() - 0.5, 0.5);
        h = rf.height();
        minw = rf.width();
        p.drawRect(rf);

        p.setFont(f);
        drawTextBaseline(&p, QPointF(rf.left(), rf.top() + rf.height() * 0.8), true, rf, QString::number(kindex < 0 ? ZKanji::elements()->strokeCount(-1 - kindex, 0) : ZKanji::kanjis[kindex]->strokes));
    }
    if (showradical)
    {
        p.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Bg));
        p.setPen(Settings::textColor(isActiveWindow(), ColorSettings::Text));

        QFont radf = Settings::radicalFont();
        radf.setPointSize(12);
        QFontMetricsF fm(radf);
        qreal rh = fm.height();

        QFont f = Settings::mainFont();
        f.setPointSize(8);
        fm = QFontMetricsF(f);
        QSizeF s = fm.boundingRect("909").size();

        QRectF rf = QRectF(0, 0, std::max(minw, std::max(s.width(), s.height()) + 4), std::max(s.width(), s.height()) + 2 + rh);
        rf.moveTo(r.width() - rf.width() - 0.5, 0.5 + h);
        p.drawRect(rf);

        p.setFont(radf);
        uchar rad = kindex < 0 ? 0 : ZKanji::kanjis[kindex]->rad;
        drawTextBaseline(&p, QPointF(0, rf.top() + rh + 1), true, rf, kindex < 0 ? QChar('-') : QString(ZKanji::radsymbols[rad - 1]));
        p.setFont(f);
        drawTextBaseline(&p, QPointF(0, rf.top() + rh + (rf.height() - rh) * 0.8), true, rf, kindex < 0 ? QChar('-') : QString::number((int)rad));
    }
}

void ZKanjiDiagram::updateImage()
{
    QRect r = rect();
    int siz = std::min(r.width(), r.height()) * kanjiRectMul;

    if (drawnpos == strokepos && image && image->width() == siz)
        return;

    if (!image || image->width() != siz)
    {
        image.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));

        if (stroke && partpos != 0)
        {
            stroke.reset(new QImage(siz, siz, QImage::Format_ARGB32_Premultiplied));
            stroke->fill(qRgba(0, 0, 0, 0));

            int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;

            QPainter p;
            p.begin(stroke.get());
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::transparent);
            p.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Text));
            for (int ix = 0; ix != partpos; ++ix)
            {
                if (showdir)
                    ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, ix, Settings::uiColor(ColorSettings::StrokeDot), Settings::textColor(isActiveWindow(), ColorSettings::Text));
                else
                    ZKanji::elements()->drawStrokePart(p, false, elem, 0, strokepos, QRectF(0, 0, stroke->width(), stroke->height()), parts, ix);
            }
            p.end();

            update();
        }

        // Force redrawing.
        drawnpos = strokepos + 1;
    }

    if (drawnpos > strokepos)
    {
        drawnpos = 0;
        image->fill(qRgba(0, 0, 0, 0));
    }

    if (strokepos == 0 && !showshadow)
        return;

    int elem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
    QPainter painter;
    painter.begin(image.get());
    painter.setRenderHint(QPainter::Antialiasing);

    if (drawnpos == 0 && showshadow)
    {
        QColor col = mixColors(Settings::textColor(isActiveWindow(), ColorSettings::Text), Settings::textColor(isActiveWindow(), ColorSettings::Bg), 0.2);
        painter.setPen(Qt::transparent);
        painter.setBrush(col);

        double pw = ZKanji::elements()->basePenWidth(siz);
        for (int ix = 0, cnt = ZKanji::elements()->strokeCount(elem, 0); ix != cnt; ++ix)
            ZKanji::elements()->drawStroke(painter, elem, 0, ix, QRectF(pw * 0.4, pw * 0.4, siz, siz), siz / partcountdiv);
    }
    if (strokepos == 0)
    {
        painter.end();
        return;
    }

    painter.setPen(Qt::transparent);
    painter.setBrush(Settings::textColor(isActiveWindow(), ColorSettings::Text));

    while (drawnpos < strokepos)
    {
        if (showdir)
            ZKanji::elements()->drawStroke(painter, elem, 0, drawnpos, QRectF(0, 0, siz, siz), siz / partcountdiv, Settings::uiColor(ColorSettings::StrokeDot), Settings::textColor(isActiveWindow(), ColorSettings::Text));
        else
            ZKanji::elements()->drawStroke(painter, elem, 0, drawnpos, QRectF(0, 0, siz, siz), siz / partcountdiv);
        ++drawnpos;
    }
    painter.end();
}

void ZKanjiDiagram::addNumberRect(int strokeix, StrokeDirection dir, QPoint pt)
{
    if (!image)
        return;

    double pw = ZKanji::elements()->basePenWidth(std::min(image->width(), image->height()));

    QFont f = Settings::mainFont();
    f.setBold(true);
    f.setPixelSize(std::max<int>(10, pw * 1.35));
    QFontInfo fi(f);
    if (!fi.exactMatch())
    {
        f.setFamily(fi.family());
        f.setPixelSize(fi.pixelSize());
    }

    QFontMetrics fm = QFontMetrics(f);

    QRect r = fm.boundingRect(QString::number(strokeix + 1));

    QRect pos[7];

    double pw1 = pw / 1.3; //1.6;
    double pw2 = pw / 1.1; //1.3;
    double pw2b = pw / 1.3;
    double pw3 = pw / 1; //1.1;
    double pw4 = pw / 2.5; //3.5;
    double pw4b = pw / 3.5;
    double pw5 = pw * 1.7;
    double pw6 = pw / 1.8;
    double pw6b = pw / 2.0;
    switch (dir)
    {
    case StrokeDirection::Down:
        pos[0] = QRect(pt.x() - ((double)r.width() * 1) - (pw1) - 1, pt.y() - ((double)r.height() * 0.6) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() - ((double)r.width() * 1) - (pw1) - 1, pt.y() - ((double)r.height() * 0.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() - ((double)r.width() * 1) - (pw1) - 1, pt.y() + ((double)r.height() * 0.1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() + ((double)r.width() * 0.3), pt.y() - ((double)r.height() * 0.16) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() - ((double)r.width() * 1) - (pw3) - 1, pt.y() - ((double)r.height() * 1.1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.width() * 0.3), pt.y() + ((double)r.height() * 0.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() + ((double)r.height() * 0.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    case StrokeDirection::Right:
        pos[0] = QRect(pt.x() + ((double)r.width() * 0) + (pw4b) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() + ((double)r.width() * 0) + 1 - 1, pt.y() - ((double)r.height() * 1.1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() - ((double)r.width() * 0) - (pw4) - 1, pt.y() - ((double)r.height() * 1.1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() - ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.height() * 0.1) + (pw4b) - 1, pt.y() - ((double)r.height() * 1) - 1 + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() - ((double)r.height() * 0) - (pw4) - 1, pt.y() - ((double)r.height() * 0.9) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    case StrokeDirection::DownLeft:
        pos[0] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() - ((double)r.height() * 1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() - ((double)r.width() * 1) - pw - 1, pt.y() - ((double)r.height() * 0.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() - ((double)r.width() * 1) - (pw * 1.5) - 1, pt.y() + ((double)r.height() * 0.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() + ((double)r.width() * 0) + (pw6b) - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() + ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    case StrokeDirection::DownRight:
        pos[0] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() - ((double)r.height() * 1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() - ((double)r.height() * 1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() + ((double)r.width() * 0) + 1 - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() + ((double)r.height() * 0.4) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() + ((double)r.width() * 0.5) + (pw2b) - 1, pt.y() + ((double)r.height() * 0) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    case StrokeDirection::UpRight:
        pos[0] = QRect(pt.x() - ((double)r.width() * 0) - (pw3) - 1, pt.y() - ((double)r.height() * 1.16) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() - ((double)r.height() * 1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() + ((double)r.width() * 0) + 1 - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() + ((double)r.height() * 0.4) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() + ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() + ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    default:
        pos[0] = QRect(pt.x() - ((double)r.width() * 1) - (pw2) - 1, pt.y() - ((double)r.height() * 1) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[1] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[2] = QRect(pt.x() + ((double)r.width() * 0) + 1 - 1, pt.y() - ((double)r.height() * 1.3) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[3] = QRect(pt.x() - ((double)r.width() * 1) - 1, pt.y() + ((double)r.height() * 0.4) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[4] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() - ((double)r.height() * 1.2) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[5] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() + ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        pos[6] = QRect(pt.x() + ((double)r.width() * 0.5) - 1, pt.y() + ((double)r.height() * 0.5) + 1, r.width() + pw / 2, r.height() - pw / 3 + 1);
        break;
    }

    bool nopos[7] = { false, false, false, false, false, false, false };
    bool moved[7] = { false, false, false, false, false, false, false };

    int d = std::ceil(pw * 0.1);
    for (int ix = 0, siz = numrects.size(); ix != siz; ++ix)
    {
        if (!numrects[ix].second.isValid())
            continue;
        for (int iy = 0; iy != 7; ++iy)
        {
            if (nopos[iy])
                continue;
            QRect numr = numrects[ix].second.adjusted(-d, -d, d, d);
            QRect posr = pos[iy].adjusted(-d, -d, d, d);
            if (numr.intersects(posr))
            {
                if (!moved[iy])
                {
                    // Nudge pos[iy] away from the existing number rectangles to make space
                    // for it. Only move it by the intersecting pixels and only if it's a
                    // small nudge.
                    int dx = (numr.right() < posr.left() || numr.left() > posr.right() ? 999999 : numr.left() <= posr.left() ? numr.right() - posr.left() : posr.right() - numr.left()) + 1;
                    int dy = (numr.bottom() < posr.top() || numr.top() > posr.bottom() ? 999999 : numr.top() <= posr.top() ? numr.bottom() - posr.top() : posr.bottom() - numr.top()) + 1;

                    if (dx <= dy && dx <= std::ceil(pw * 0.7))
                    {
                        pos[iy].adjust(numr.left() <= posr.left() ? dx : -dx, 0, numr.left() <= posr.left() ? dx : -dx, 0);
                        moved[iy] = true;
                    }
                    else if (dx > dy && dy <= std::ceil(pw * 0.7))
                    {
                        pos[iy].adjust(0, numr.top() <= posr.top() ? dy : -dy, 0, numr.top() <= posr.top() ? dy : -dy);
                        moved[iy] = true;
                    }
                    if (moved[iy])
                    {
                        // When the new number's rectangle is moved, we must recheck whether
                        // there are previous number rectangles intersecting with it.
                        posr = pos[iy].adjusted(-d, -d, d, d);
                        for (int iz = 0; iz != ix; ++iz)
                        {
                            numr = numrects[iz].second.adjusted(-d, -d, d, d);
                            if (numr.intersects(posr))
                            {
                                nopos[iy] = true;
                                break;
                            }
                        }
                    }
                    else
                        nopos[iy] = true;
                }
                else
                    nopos[iy] = true;
            }
        }
    }

    int goodpos = -1;
    for (int ix = 0; ix != 7; ++ix)
    {
        if (!nopos[ix])
        {
            goodpos = ix;
            numrects.push_back({ pt, pos[ix] });
            break;
        }
    }
    if (goodpos == -1)
        numrects.push_back({ pt, QRect() });
}

void ZKanjiDiagram::drawNumberRect(QPainter &p, int strokeix, bool numdraw)
{
    if (numrects.size() <= strokeix)
        return;

    // [stroke position of next color, current color]
    // TODO: make these colors available in the settings.
    const std::pair<int, QColor> colors[] = { { 7, QColor(0, 64, 180) }, { 10, QColor(0, 140, 100) }, { 14, QColor(20, 190, 0) }, { 16, QColor(140, 170, 0) }, { 20, QColor(220, 150, 0) }, { 25, QColor(160, 80, 0) }, { 30, QColor(160, 0, 0) }, { 100, QColor(120, 0, 150) } };

    double pw = ZKanji::elements()->basePenWidth(std::min(image->width(), image->height()));

    QRect r = numrects[strokeix].second;
    if (!r.isValid())
        return;

    QFont f;
    if (numdraw)
    {
        f = Settings::mainFont();
        f.setBold(true);
        f.setPixelSize(std::max<int>(10, pw * 1.35));
        QFontInfo fi(f);
        if (!fi.exactMatch())
        {
            f.setFamily(fi.family());
            f.setPixelSize(fi.pixelSize());
        }
    }

    if (!numdraw)
    {
        int colx = 0;
        while (strokeix > colors[colx].first)
            ++colx;

        int prev = (colx == 0 ? 0 : colors[colx - 1].first);
        QColor col = mixColors(colors[colx].second, colors[colx + 1].second, (double((colors[colx].first - prev) - (strokeix - prev)) / (colors[colx].first - prev)));

        p.setBrush(col);

        p.setPen(Qt::transparent);
        p.drawRoundedRect(r, pw / 2.8, pw / 2.8);

        // Draw small triangle connecting the filled number rectangle with the center of the
        // stroke's starting point.
        QPoint pt[3];
        pt[0] = numrects[strokeix].first;
        pt[1] = numrects[strokeix].second.center();
        double len = pw * 0.4;
        double dx = 0;
        double dy = 0;
        if (pt[1].x() == pt[0].x())
            dx = std::max<int>(len, 1), dy = 0.0;
        else if (pt[1].y() == pt[0].y())
            dx = 0.0, dy = std::max<int>(len, 1);
        else if (std::abs(pt[1].y() - pt[0].y()) > std::abs(pt[1].x() - pt[0].x()))
        {
            double d = (pt[1].y() - pt[0].y()) / double(pt[1].x() - pt[0].x());
            dy = len / std::sqrt(d * d + 1);
            dx = dy * d;
        }
        else
        {
            double d = (pt[1].x() - pt[0].x()) / double(pt[1].y() - pt[0].y());
            dx = len / std::sqrt(d * d + 1);
            dy = dx * d;
        }
        pt[2] = QPoint(pt[1].x() + dx, pt[1].y() - dy);
        pt[1] = QPoint(pt[1].x() - dx, pt[1].y() + dy);

        p.drawPolygon(pt, 3);
    }

    if (numdraw)
    {
        p.setPen(Qt::white);
        p.setFont(f);
        drawTextBaseline(&p, 0, r.top() + r.height() * 0.9, true, r, QString::number(strokeix + 1));// , Qt::AlignHCenter | Qt::AlignVCenter);
    }
}

void ZKanjiDiagram::settingsChanged()
{
    numrects.clear();
    image.reset();
    stroke.reset();

    update();
}

void ZKanjiDiagram::appStateChanged()
{
    image.reset();
    stroke.reset();

    update();
}


//-------------------------------------------------------------

