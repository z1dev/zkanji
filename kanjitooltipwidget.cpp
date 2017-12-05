/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QToolTip>
#include <QStringBuilder>
#include <QPainter>

#include "kanjitooltipwidget.h"
#include "fontsettings.h"
#include "kanjisettings.h"
#include "colorsettings.h"
#include "kanji.h"
#include "words.h"
#include "zkanjimain.h"
#include "zui.h"


//-------------------------------------------------------------


namespace ZKanji
{
    // Same as the radsymbols in ZRadicalGrid, without the added stroke count.
    extern QChar radsymbols[231];
}

KanjiToolTipWidget::KanjiToolTipWidget(QWidget *parent) : base(parent), hpad(4), vpad(3), penw(1), dict(nullptr), index(-1)
{
    setObjectName(QStringLiteral("KanjiToolTipWidget"));

    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
}

KanjiToolTipWidget::~KanjiToolTipWidget()
{
    ;
}

void KanjiToolTipWidget::setKanji(Dictionary *d, int kindex)
{
    if (dict == d && index == kindex)
        return;

    dict = d;
    index = kindex;
    
    recalculateSize();
}

QSize KanjiToolTipWidget::sizeHint() const
{
    return hint;
}

void KanjiToolTipWidget::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (index == -1)
        return;
    if (hint.isEmpty())
        recalculateSize();

    QPainter p(this);

    KanjiEntry *k = ZKanji::kanjis[index];

    // Default tooltip font used for meaning and some values.
    QFont f = QApplication::font("QTipLabel");
    QFontMetrics fm(f);

    // Bold version of the tooltip font for the reference names.
    QFont fb = f;
    fb.setBold(true);
    QFontMetrics fbm(fb);

    // Large font for the stroke count.
    QFont fxl = f;
    fxl.setPointSize(f.pointSize() * 2.5);
    QFontMetrics fxlm(fxl);

    // Font for the radical.
    QFont rf = Settings::radicalFont();
    rf.setPointSize(f.pointSize() * 2);
    QFontMetrics rfm(rf);

    // Font for kanji.
    QFont kf = Settings::kanjiFont();
    kf.setPointSize(f.pointSize() * 2.5);
    QFontMetrics kfm(kf);

    // Font for readings.
    QFont kkf = Settings::kanaFont();
    kkf.setPointSize(f.pointSize() * 0.9);
    QFontMetrics kkfm(kkf);

    int maxwidth = hint.width() + 1;
    int spacew = fm.width(QChar(' '));


    // First line. Kanji | Strokes: num | Radical: rad

    int rowy = (kfm.overlinePos() + kfm.underlinePos()) * 0.9;
    int rowx = 0;
    p.setFont(kf);
    drawTextBaseline(&p, 0, rowy, false, QRect(), k->ch);

    rowx += kfm.width(k->ch)/*.width()*/ + hpad;

    QPen pen = p.pen();
    pen.setWidth(penw);
    p.setPen(pen);

    p.drawLine(rowx, 0, rowx, rowy + vpad);
    rowx += hpad + penw;

    p.setFont(fb);
    QString str = tr("Strokes:");
    int w = fbm.boundingRect(str).width();
    p.drawText(rowx, 0, hint.width() - rowx, rowy, Qt::AlignTop | Qt::AlignLeft, str);
    rowx += w + hpad * 3;

    p.setFont(fxl);
    w = fxlm.boundingRect(QString::number((int)k->strokes)).width();
    drawTextBaseline(&p, rowx, rowy, false, QRect(), QString::number((int)k->strokes));
    rowx += w + hpad;

    p.drawLine(rowx, 0, rowx, rowy + vpad);
    rowx += hpad + penw;

    p.setFont(fb);
    str = tr("Radical:");
    w = fbm.boundingRect(str).width();
    p.drawText(rowx, 0, hint.width() - rowx, rowy, Qt::AlignTop | Qt::AlignLeft, str);

    p.setFont(f);
    int numw = fm.boundingRect(QString::number((int)k->rad)).width();
    drawTextBaseline(&p, rowx + w - numw, rowy, false, QRect(), QString::number((int)k->rad));
    rowx += w + hpad * 3;

    p.setFont(rf);
    drawTextBaseline(&p, rowx, rowy, false, QRect(), ZKanji::radsymbols[k->rad - 1]);

    rowy += (kfm.overlinePos() + kfm.underlinePos()) * 0.1 + vpad;
    p.drawLine(0, rowy, hint.width(), rowy);

    rowy += penw + vpad;

    // Draw the kanji meaning.
    p.setFont(f);

    QString meaning = dict->kanjiMeaning(index);
    QCharTokenizer tokens(meaning.constData(), -1, [](QChar ch) { if (ch == ' ') return QCharKind::Delimiter; return QCharKind::Normal; });
    rowx = 0;
    while (tokens.next())
    {
        str = QString(tokens.token(), tokens.tokenSize());
        w = fm.boundingRect(str).width();

        if (rowx == 0 || rowx + spacew + w < maxwidth)
        {
            if (rowx != 0)
                rowx += spacew;
            p.drawText(rowx, rowy, hint.width() - rowx, hint.height(), Qt::AlignLeft | Qt::AlignTop, str);
            rowx += w;
        }
        else
        {
            rowy += fm.lineSpacing();
            p.drawText(0, rowy, hint.width(), hint.height(), Qt::AlignLeft | Qt::AlignTop, str);
            rowx = w;
        }
    }
    rowy += fm.lineSpacing() + vpad;

    p.drawLine(0, rowy, hint.width(), rowy);

    rowy += penw + vpad;

    // Draw ON and Kun readings.
    rowy += std::max(kkfm.ascent(), fbm.ascent());

    int rleft = std::max(fbm.boundingRect(tr("ON:")).width(), fbm.boundingRect(tr("Kun:")).width()) + hpad + penw + hpad;

    p.setFont(fb);
    drawTextBaseline(&p, 0, rowy, false, QRect(), tr("ON:"));

    p.setFont(kkf);
    rowx = rleft;
    for (int ix = 0, siz = k->on.size(); ix != siz; ++ix)
    {
        str = k->on[ix].toQString();
        if (ix != siz - 1)
            str += QChar(',');
        w = kkfm.width(str);
        if (rowx == rleft || rowx + spacew + w < maxwidth)
        {
            if (rowx != rleft)
                rowx += spacew;
            drawTextBaseline(&p, rowx, rowy, false, QRect(), str);
            rowx += w;
        }
        else
        {
            rowy += kkfm.lineSpacing();
            drawTextBaseline(&p, rleft, rowy, false, QRect(), str);
            rowx = rleft + w;
        }
    }
    rowy += std::max(kkfm.lineSpacing(), fbm.lineSpacing()) + 1/* + vpad*/;

    p.setFont(fb);
    drawTextBaseline(&p, 0, rowy, false, QRect(), tr("Kun:"));

    p.setFont(kkf);
    rowx = rleft;
    for (int ix = 0, siz = k->kun.size(); ix != siz; ++ix)
    {
        str = k->kun[ix].toQString();
        if (ix != siz - 1)
            str += QChar(',');
        w = kkfm.width(str);

        if (rowx == rleft || rowx + spacew + w < maxwidth)
        {
            if (rowx != rleft)
                rowx += spacew;
        }
        else
        {
            rowy += kkfm.lineSpacing();
            rowx = rleft;
        }

        int pos = str.indexOf('.');
        if (!Settings::colors.coloroku || pos == -1)
        {
            drawTextBaseline(&p, rowx, rowy, false, QRect(), str);
            rowx += w;
        }
        else
        {
            QString part = str.left(pos + 1);
            w = kkfm.width(part);
            drawTextBaseline(&p, rowx, rowy, false, QRect(), part);
            rowx += w;

            p.setPen(Settings::uiColor(ColorSettings::Oku));
            part = str.mid(pos + 1, str.size() - pos - 1 - (ix == siz - 1 ? 0 : 1));
            w = kkfm.width(part);
            drawTextBaseline(&p, rowx, rowy, false, QRect(), part);
            rowx += w;

            p.setPen(pen);

            if (ix != siz - 1)
            {
                part = QChar(',');
                w = kkfm.width(part);
                drawTextBaseline(&p, rowx, rowy, false, QRect(), part);
                rowx += w;
            }
        }
    }
    rowy += std::max(kkfm.descent(), fbm.descent()) + vpad;
    //rowy += kkfm.descent() + vpad;

    if (Settings::kanji.mainref1 == 0 && Settings::kanji.mainref2 == 0 && Settings::kanji.mainref3 == 0 && Settings::kanji.mainref4 == 0)
        return;

    std::vector<int> refs = { Settings::kanji.mainref1, Settings::kanji.mainref2, Settings::kanji.mainref3, Settings::kanji.mainref4 };
    refs.resize(std::remove(refs.begin(), refs.end(), 0) - refs.begin());

    for (int ix = 0, siz = refs.size(); ix != siz; ++ix)
    {
        int ref = refs[ix];

        if (ix == 2)
            rowy += vpad + std::max(fm.descent(), fbm.descent());

        if ((ix % 2) == 0)
        {
            p.drawLine(0, rowy, hint.width(), rowy);
            p.drawLine(hint.width() / 2, rowy, hint.width() / 2, rowy + vpad + std::max(fm.height(), fbm.height()));

            rowy += penw + vpad + std::max(fm.ascent(), fbm.ascent());
            rowx = 0;
        }

        p.setFont(fb);
        str = ZKanji::kanjiReferenceTitle(ref) + ":";
        drawTextBaseline(&p, rowx, rowy, false, QRect(), str);
        int w = fbm.width(str);
        //rowx += w + hpad + penw + hpad;

        p.setFont(f);
        str = ZKanji::kanjiReference(k, ref);
        w = fm.width(str);
        drawTextBaseline(&p, rowx + hint.width() / 2 - penw - w - hpad, rowy, false, QRect(), str);

        if ((ix % 2) == 0)
            rowx = hint.width() / 2 + hpad;
    }
}

void KanjiToolTipWidget::recalculateSize()
{
    KanjiEntry *k = ZKanji::kanjis[index];

    // Default tooltip font used for meaning and some values.
    QFont f = QApplication::font("QTipLabel");
    QFontMetrics fm(f);

    // Bold version of the tooltip font for the reference names.
    QFont fb = f;
    fb.setBold(true);
    QFontMetrics fbm(fb);

    // Large font for the stroke count.
    QFont fxl = f;
    fxl.setPointSize(f.pointSize() * 2.5);
    QFontMetrics fxlm(fxl);

    // Font for the radical.
    QFont rf = Settings::radicalFont();
    rf.setPointSize(f.pointSize() * 2);
    QFontMetrics rfm(rf);

    // Font for kanji.
    QFont kf = Settings::kanjiFont();
    kf.setPointSize(f.pointSize() * 2.5);
    QFontMetrics kfm(kf);

    // Font for readings.
    QFont kkf = Settings::kanaFont();
    kkf.setPointSize(f.pointSize() * 0.9);
    QFontMetrics kkfm(kkf);

    // The width of the tooltip is limited to maxwidth width.
    int maxwidth = fm.averageCharWidth() * 40;
    int spacew = fm.width(QChar(' '));

    // The minimal width of the tooltip widget, which is the width of the first row. It's made
    // up from the kanji, the stroke count and radical labels.
    int minwidth = kfm.width(k->ch)/*.width()*/ + hpad + penw + hpad + fbm.boundingRect(tr("Strokes:")).width() + hpad * 3 + fxlm.boundingRect(QString::number((int)k->strokes)).width() + hpad + penw + hpad + fbm.boundingRect(tr("Radical:")).width() + hpad * 3 + rfm.boundingRect(ZKanji::radsymbols[k->rad - 1]).width() + hpad * 4;

    // The height of the tooltip widget, which includes the rows of kanji, meaning, readings,
    // any extra references. The only way to get a reasonable size for the kanji is by using
    // the overlinePos() and underlinePos() as a lot of fonts have strange heights. The height
    // of the meaning and reading lines are added when their widths are calculated.
    int height = (kfm.overlinePos() + kfm.underlinePos()) * 1.0 + vpad + penw + vpad + fm.lineSpacing() + vpad + penw + vpad + std::max(kkfm.ascent(), fbm.ascent()) + std::max(kkfm.lineSpacing(), fbm.lineSpacing()) + 1 /*+ vpad*/ + std::max(kkfm.descent(), fbm.descent());

    int refcnt = (Settings::kanji.mainref1 != 0) + (Settings::kanji.mainref2 != 0) + (Settings::kanji.mainref3 != 0) + (Settings::kanji.mainref4 != 0);
    height += (refcnt > 2 ? vpad : 0) + ((refcnt + 1) / 2) * (penw + vpad + std::max(fm.height(), fbm.height())) + (refcnt ? vpad : 0);

    QString meaning = dict->kanjiMeaning(index);

    // Width of the longest meaning line, but not less than the minimum width of the widget.
    int maxlinew = minwidth;

    // Find the widest reference text, which also influences the minimum widget width.
    for (int ix = 0; ix != 4; ++ix)
    {
        int ref = 0;
        switch (ix)
        {
        case 0:
            ref = Settings::kanji.mainref1;
            break;
        case 1:
            ref = Settings::kanji.mainref2;
            break;
        case 2:
            ref = Settings::kanji.mainref3;
            break;
        case 3:
            ref = Settings::kanji.mainref4;
            break;
        }
        if (ref == 0)
            continue;

        int w = fbm.boundingRect(ZKanji::kanjiReferenceTitle(ref) % ":").width() + hpad + penw + hpad + fm.boundingRect(ZKanji::kanjiReference(k, ref)).width() + hpad;
        maxlinew = std::max(maxlinew, w * 2 + penw);
    }
    maxwidth = std::max(maxwidth, maxlinew);

    int linew = 0;
    QCharTokenizer tokens(meaning.constData(), -1, [](QChar ch) { if (ch == ' ') return QCharKind::Delimiter; return QCharKind::Normal; });
    while (tokens.next())
    {
        int wordw = fm.boundingRect(QString(tokens.token(), tokens.tokenSize())).width();
        if (linew == 0 || linew + spacew + wordw < maxwidth)
            linew += (linew == 0 ? 0 : spacew) + wordw;
        else
        {
            maxlinew = std::max(maxlinew, linew);
            linew = wordw;
            height += fm.lineSpacing();
        }
    }
    maxlinew = std::max(maxlinew, linew);

    // Find the maximum line width of the ON and Kun readings.
    int rleft = std::max(fbm.boundingRect(tr("ON:")).width(), fbm.boundingRect(tr("Kun:")).width()) + hpad + penw + hpad;

    linew = rleft;

    for (int ix = 0, siz = k->on.size(); ix != siz; ++ix)
    {
        int wordw = kkfm.width(k->on[ix].toQString() % (ix == siz - 1 ? QChar() : QChar(',')));
        if (linew == rleft || linew + spacew + wordw < maxwidth)
            linew += (linew == rleft ? 0 : spacew) + wordw;
        else
        {
            maxlinew = std::max(maxlinew, linew);
            linew = rleft + wordw;
            height += kkfm.lineSpacing();
        }
    }
    maxlinew = std::max(maxlinew, linew);

    linew = rleft;

    for (int ix = 0, siz = k->kun.size(); ix != siz; ++ix)
    {
        int wordw = kkfm.width(k->kun[ix].toQString() % (ix == siz - 1 ? QChar() : QChar(',')));
        if (linew == rleft || linew + spacew + wordw < maxwidth)
            linew += (linew == rleft ? 0 : spacew) + wordw;
        else
        {
            maxlinew = std::max(maxlinew, linew);
            linew = rleft + wordw;
            height += kkfm.lineSpacing();
        }
    }
    maxlinew = std::max(maxlinew, linew);

    hint = QSize(maxlinew, height);

    updateGeometry();
}


//-------------------------------------------------------------

namespace ZKanji
{

    QWidget* kanjiTooltipWidget(Dictionary *d, int kindex)
    {
        KanjiToolTipWidget *w = new KanjiToolTipWidget();
        w->setKanji(d, kindex);

        return w;
    }

}