/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QMenu>
#include <QApplication>
#include "zitemscroller.h"
#include "zui.h"
#include "kanjistrokes.h"
#include "kanji.h"
#include "colorsettings.h"
#include "fontsettings.h"
#include "zkanjimain.h"
#include "ztooltip.h"
#include "kanjisettings.h"
#include "globalui.h"
#include "generalsettings.h"

// Bad solution for detecting button down repeats by hardcoding the delay in
// a define, but this is how Qt does it for QAbstractButton with these same
// values.

#define AUTO_REPEAT_DELAY 300
#define AUTO_REPEAT_INTERVAL 30

//-------------------------------------------------------------


//ZItemScrollerModel::ZItemScrollerModel(QObject *parent) : base(parent)
//{
//}
//
//ZItemScrollerModel::~ZItemScrollerModel()
//{
//
//}


//-------------------------------------------------------------


KanjiScrollerModel::KanjiScrollerModel(QObject *parent) : base(parent)
{

}

KanjiScrollerModel::~KanjiScrollerModel()
{

}

int KanjiScrollerModel::items(int index) const
{
    return list[index];
}

void KanjiScrollerModel::setItems(const std::vector<int> &items)
{
    list = items;
    emit changed();
}

void KanjiScrollerModel::setBgColor(QColor col)
{
    bgcolor = col;
}

QColor KanjiScrollerModel::bgColor(int index) const
{
    return bgcolor;
}

QColor KanjiScrollerModel::textColor(int index) const
{
    return QColor();
}

int KanjiScrollerModel::size() const
{
    return list.size();
}


//-------------------------------------------------------------


SimilarKanjiScrollerModel::SimilarKanjiScrollerModel(QObject *parent) : base(parent)
{

}

SimilarKanjiScrollerModel::~SimilarKanjiScrollerModel()
{

}

void SimilarKanjiScrollerModel::setItems(const std::vector<int> &items, int categlimit)
{
    limit = categlimit;
    base::setItems(items);
}

QColor SimilarKanjiScrollerModel::textColor(int index) const
{
    if (index < limit)
        return base::textColor(index);
    return Settings::uiColor(ColorSettings::SimilarText);
}


//-------------------------------------------------------------


CandidateKanjiScrollerModel::CandidateKanjiScrollerModel(QObject *parent) : base(parent)
{

}

CandidateKanjiScrollerModel::~CandidateKanjiScrollerModel()
{

}

QColor CandidateKanjiScrollerModel::bgColor(int index) const
{
    if (index != -1)
    {
        int val = items(index);

        QChar ch = val < 0 ? ZKanji::elements()->itemUnicode(-val - 1) : ZKanji::kanjis[val]->ch;

        if (HIRAGANA(ch.unicode()))
            return Settings::uiColor(ColorSettings::HiraBg);
        else if (!KANJI(ch.unicode()))
            return Settings::uiColor(ColorSettings::KataBg);
    }

    return base::bgColor(index);
}


//-------------------------------------------------------------


ZItemScroller::ZItemScroller(QWidget *parent) : base(parent), dict(nullptr), m(nullptr), cellsize(Settings::scaled(Settings::fonts.kanjifontsize)), mouseitem(-1), kanjitipkanji(-1)
{
    //setBackgroundRole(QPalette::Base);
    setAutoFillBackground(false);

    setMouseTracking(true);
}

ZItemScroller::~ZItemScroller()
{

}

KanjiScrollerModel* ZItemScroller::model() const
{
    return m;
}

void ZItemScroller::setModel(KanjiScrollerModel *newmodel)
{
    if (m == newmodel)
        return;

    if (m != nullptr)
        disconnect(m, &KanjiScrollerModel::changed, this, 0);
    m = newmodel;

    if (m != nullptr)
        connect(m, &KanjiScrollerModel::changed, this, &ZItemScroller::modelChanged);

    contentsReset(0);
    //scrollpos = 0;
    //maxscrollpos = m == nullptr ? 0 : std::max(0, m->size() * itemWidth() - (size().width() - buttonRect(true).width() - buttonRect(false).width()));
    //mouseitem = downpos == QPoint(-1, -1) ? itemAt(mousepos) : -1;
    //update();
}

Dictionary* ZItemScroller::dictionary() const
{
    return dict;
}

void ZItemScroller::setDictionary(Dictionary *d)
{
    dict = d;
}

int ZItemScroller::cellSize() const
{
    return cellsize;
}

void ZItemScroller::setCellSize(int newsiz)
{
    if (cellsize == newsiz)
        return;
    cellsize = newsiz;
    updateGeometry();
}

int ZItemScroller::scrollMin() const
{
    return 0;
}

int ZItemScroller::scrollMax() const
{
    if (m == nullptr)
        return 0;
    if (scrollerType() == Scrollbar)
        return m->size() * cellsize;
    return m->size();
}

int ZItemScroller::scrollPage() const
{
    if (scrollerType() == Scrollbar)
        return drawArea().width();
    else
        return drawArea().width() / cellsize;
}

int ZItemScroller::scrollStep() const
{
    return 1;
}

void ZItemScroller::scrolled(int oldpos, int &newpos)
{
    update();
}

QSize ZItemScroller::minimumSizeHint() const
{
    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);

    return QSize(cellsize + mleft + mright, cellsize + mtop + mbottom);
}

void ZItemScroller::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (!m)
        return;

    QStylePainter p(this);
    QRect r = drawArea();

    QRect cr = r.intersected(e->rect());
    if (cr.isEmpty())
        return;

    p.setClipRect(cr);

    QColor gridcolor = Settings::uiColor(ColorSettings::Grid);

    // Get position of leftmost visible item.
    QSize s = size();
    int siz = m ? m->size() : 0;
    int pos = scrollerType() == Scrollbar ? scrollPos() / cellsize : scrollPos();
    int left = scrollerType() == Scrollbar ? r.left() - (scrollPos() % cellsize) : r.left();

    while (left <= cr.right() && pos != siz)
    {
        if (left + cellsize > cr.left())
        {
            QRect cellr = QRect(left, r.top(), cellsize, r.height());

            paintCell(p, cellr, pos == mouseitem, pos);

            p.setPen(gridcolor);
            p.drawLine(left + cellsize - 1, r.top(), left + cellsize - 1, r.bottom());
        }

        ++pos;
        left += cellsize;
    }

    QColor bg = m->bgColor(-1);
    p.fillRect(QRect(left, r.top(), r.width() - (left - r.left()) + 1, r.height()), bg.isValid() ? bg : Settings::textColor(this, ColorSettings::Bg));
}

//void ZItemScroller::resizeEvent(QResizeEvent *e)
//{
//    QSize os = e->oldSize();
//    QSize ns = size();
//    if (os.isEmpty() || ns.isEmpty())
//        return;
//    int maxwidth = modelSize() * itemWidth();
//    int mpos = m == nullptr ? 0 : std::max(0, m->size() * itemWidth() - (ns.width() - buttonRect(true).width() - buttonRect(false).width()));
//    if (scrollpos > mpos)
//    {
//        maxscrollpos = mpos;
//        scrollpos = maxscrollpos;
//        update();
//        return;
//    }
//
//    // The button pointing right should be repainted if it should be
//    // enabled/disabled as the size changed.
//    if ((mpos - scrollpos == 0) != (maxscrollpos - scrollpos == 0))
//    {
//        QRect br = buttonRect(false);
//        if (ns.width() > os.width())
//            br.setLeft(os.width() - br.width());
//        update(br);
//    }
//    maxscrollpos = mpos;
//}

void ZItemScroller::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);

    if (e->isAccepted() || e->button() != Qt::LeftButton)
    {
        e->accept();
        return;
    }

    QRect r = drawArea();
    QPoint p = e->pos();
    int item = -1;
    if (r.contains(p))
        item = itemAt(p);

    setMouseItem(item);

    //downpos = e->pos();
    //setMouseItem(itemAt(downpos));
    if (item != -1)
    {
    //    update(itemRect(mouseitem));
        emit itemClicked(mouseitem);
        return;
    }

    //bool left = buttonEnabled(true) && buttonRect(true).contains(downpos);
    //if (!left && !(buttonEnabled(false) && buttonRect(false).contains(downpos)))
    //    return;

    //update(buttonRect(left));
    //scrollpos = std::max(0, scrollpos + (left ? -1 : 1) * itemWidth() / 2);
    //update();
    //timer.start(AUTO_REPEAT_DELAY, this);
}

void ZItemScroller::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);
    e->accept();
}

void ZItemScroller::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);

    if (e->isAccepted() || e->button() != Qt::LeftButton)
        return;

    //if (buttonEnabled(true) && buttonRect(true).contains(downpos))
    //{
    //    timer.stop();
    //    update(buttonRect(true));
    //}
    //else if (buttonEnabled(false) && buttonRect(false).contains(downpos))
    //{
    //    timer.stop();
    //    update(buttonRect(false));
    //}
    //downpos = QPoint(-1, -1);
    //mousepos = e->pos();
    //setMouseItem(itemAt(mousepos), true);

    e->accept();
}

void ZItemScroller::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);

    QRect r = drawArea();
    QPoint p = e->pos();
    int item = -1;
    if (r.contains(p))
        item = itemAt(p);

    setMouseItem(item);

    int kanji = item == -1 ? -1 : m->items(item);
    if (item != -1 && kanji < 0)
        kanji = ZKanji::elements()->itemKanjiIndex(-kanji - 1);

    if (kanji < 0 || dict == nullptr)
    {
        ZToolTip::startHide();
        kanjitipkanji = -1;
        return;
    }

    if (kanjitipkanji != kanji)
    {
        kanjitipkanji = kanji;
        ZToolTip::show(ZKanji::kanjiTooltipWidget(dict, kanjitipkanji), this, itemRect(item).adjusted(0, 0, 1, 1), Settings::kanji.hidetooltip ? Settings::kanji.tooltipdelay * 1000 : 2000000000);
    }

    //if (mousepos != QPoint())
    //{
    //    if (buttonEnabled(true) && buttonRect(true).contains(mousepos) && !buttonRect(true).contains(pos))
    //        update(buttonRect(true));
    //    else if (buttonEnabled(false) && buttonRect(false).contains(mousepos) && !buttonRect(false).contains(pos))
    //        update(buttonRect(false));
    //}

    //std::swap(mousepos, pos);

    //if (mousepos != QPoint(-1, -1))
    //{
    //    if (buttonEnabled(true) && buttonRect(true).contains(mousepos) && !buttonRect(true).contains(pos))
    //        update(buttonRect(true));
    //    else if (buttonEnabled(false) && buttonRect(false).contains(mousepos) && !buttonRect(false).contains(pos))
    //        update(buttonRect(false));
    //}

    //if (downpos == QPoint(-1, -1))
    //    setMouseItem(itemAt(mousepos));
}

void ZItemScroller::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    QSize s = size();

    //if (buttonEnabled(true) && buttonRect(true).contains(mousepos) )
    //    update(buttonRect(true));
    //else if (buttonEnabled(false) && buttonRect(false).contains(mousepos))
    //    update(buttonRect(false));

    //mousepos = QPoint(-1, -1);
    setMouseItem(-1);

    ZToolTip::startHide();
    kanjitipkanji = -1;
}

void ZItemScroller::contextMenuEvent(QContextMenuEvent *e)
{
    QPoint p = e->pos();
    if (m == nullptr || !drawArea().contains(p))
        return;

    int item = itemAt(p);
    int kanji = item == -1 ? -1 : m->items(item);
    if (item != -1 && kanji < 0)
        kanji = ZKanji::elements()->itemKanjiIndex(-kanji - 1);

    if (kanji < 0)
        return;

    QMenu *menu = new QMenu(window());

    QAction *a = menu->addAction(tr("Kanji information"));
    connect(a, &QAction::triggered, [this, kanji]() { gUI->showKanjiInfo(dict, kanji); });
    menu->addSeparator();

    a = menu->addAction(tr("Copy kanji"));
    connect(a, &QAction::triggered, [kanji]() { gUI->clipCopy(ZKanji::kanjis[kanji]->ch); });
    a = menu->addAction(tr("Append kanji"));
    connect(a, &QAction::triggered, [kanji]() { gUI->clipAppend(ZKanji::kanjis[kanji]->ch); });

    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(e->globalPos());
    e->accept();
}

void ZItemScroller::modelChanged()
{
    contentsReset();
    //scrollpos = 0;
    //maxscrollpos = m == nullptr ? 0 : std::max(0, m->size() * itemWidth() - (size().width() - buttonRect(true).width() - buttonRect(false).width()));
    //mouseitem = itemAt(mousepos);
    //update();
}

//void ZItemScroller::paintButton(QStylePainter &p, QStyleOptionViewItem &opts, bool left)
//{
//    QRect rect = buttonRect(left);
//    bool enabled = buttonEnabled(left);
//    bool hovered = rect.contains(mousepos) && enabled;
//    bool down = rect.contains(downpos) && enabled;
//
//    p.save();
//    p.setPen(QPen(QBrush(enabled ? QPalette::Text : QPalette::Disabled), rect.width() / 10));
//
//    if (hovered)
//        p.fillRect(rect, Qt::blue);
//
//    double l, r, t, d;
//    // Gap between the lines.
//    d = 0.2;
//
//    if (rect.left() == 0)
//    {
//        l = 0.3;
//        r = 0.55;
//    }
//    else
//    {
//        l = 0.7 - d;
//        r = 0.45 - d;
//    }
//    t = 0.3;
//
//    p.drawLine(QLineF(rect.left() + rect.width() * r, rect.top() + rect.height() * t, rect.left() + rect.width() * l, rect.top() + rect.height() * 0.5));
//    p.drawLine(QLineF(rect.left() + rect.width() * l, rect.top() + rect.height() * 0.5, rect.left() + rect.width() * r, rect.top() + rect.height() * (1 - t)));
//
//    l += d;
//    r += d;
//    p.drawLine(QLineF(rect.left() + rect.width() * r, rect.top() + rect.height() * t, rect.left() + rect.width() * l, rect.top() + rect.height() * 0.5));
//    p.drawLine(QLineF(rect.left() + rect.width() * l, rect.top() + rect.height() * 0.5, rect.left() + rect.width() * r, rect.top() + rect.height() * (1 - t)));
//
//    p.restore();
//}
//
//int ZItemScroller::modelSize() const
//{
//    if (m)
//        return m->size();
//    return 0;
//}
//
//int ZItemScroller::itemWidth() const
//{
//    QSize s = size();
//    return s.height();
//}
//
//QRect ZItemScroller::buttonRect(bool left)
//{
//    const int btnsize = 22;
//
//    QSize s = size();
//    if (left)
//        return QRect(0, 0, btnsize, s.height());
//    return QRect(s.width() - btnsize, 0, btnsize, s.height());
//}

QRect ZItemScroller::itemRect(int index)
{
    if (!m || index < 0 || index >= m->size())
        return QRect();

    QRect r = drawArea();
    return QRect(r.left() + (index - scrollPos()) * cellsize, r.top(), cellsize, r.bottom()).intersected(r);
}

int ZItemScroller::itemAt(const QPoint &pt)
{
    QRect r = drawArea();
    if (!m || !r.contains(pt))
        return -1;

    //if (!m || pt.x() < buttonRect(true).right() + 1 || pt.x() >= buttonRect(false).left())
    //    return -1;
    
    int x;
    if (scrollerType() == Scrollbar)
        x = (pt.x() - r.left()) + scrollPos(); /*(buttonRect(true).right() + 1) + scrollpos;*/
    else
        x = scrollPos() * cellsize + (pt.x() - r.left());
    if (x / cellsize >= m->size())
        return -1;
    return x / cellsize;
}

//bool ZItemScroller::buttonEnabled(bool left)
//{
//    if (!m)
//        return false;
//
//    if (left)
//        return scrollpos != 0;
//    
//    return maxscrollpos - scrollpos > 0;
//}

void ZItemScroller::setMouseItem(int index, bool forceupdate)
{
    if (mouseitem == index)
    {
        if (forceupdate && index != -1)
            update(itemRect(index));
        return;
    }
    if (mouseitem != -1)
        update(itemRect(mouseitem));
    mouseitem = index;
    if (mouseitem != -1)
        update(itemRect(mouseitem));
}

void ZItemScroller::paintCell(QPainter &p, const QRect &rect, bool selected, int index) const
{
    int val = m->items(index);

    QChar ch = val < 0 ? ZKanji::elements()->itemUnicode(-val - 1) : ZKanji::kanjis[val]->ch;

    QColor bg = m->bgColor(index);
    QColor col = m->textColor(index);

    if (selected)
    {
        QColor c = Settings::textColor(this, ColorSettings::SelBg);

        p.setBrush(!bg.isValid() ? c : colorFromBase(Settings::textColor(this, ColorSettings::Bg), c, bg));
        c = Settings::textColor(this, ColorSettings::SelText);
        p.setPen(!col.isValid() ? c : colorFromBase(Settings::textColor(this, ColorSettings::Text), c, col));
    }
    else
    {
        p.setBrush(bg.isValid() ? bg : Settings::textColor(this, ColorSettings::Bg));
        p.setPen(col.isValid() ? col : Settings::textColor(this, ColorSettings::Text));
    }

    if (ch.unicode() == 0)
        p.setBrush(colorFromBase(Settings::textColor(this, ColorSettings::Bg), p.brush().color(), Settings::uiColor(ColorSettings::KanjiUnsorted)));

    p.fillRect(rect, p.brush());

    p.save();
    if (ch.unicode() != 0)
    {
        QFont f = Settings::kanjiFont();
        f.setPointSize(rect.height() * 0.7);

        p.setFont(f);
        //p.setPen(qApp->pal.color(QPalette::Text));
        p.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, ch);
    }
    else
    {
        p.setBrush(p.pen().color());
        p.setPen(Qt::transparent);
        ZKanji::elements()->drawElement(p, -val - 1, 0, rect, false);
    }

    p.restore();
}

//void ZItemScroller::timerEvent(QTimerEvent *e)
//{
//    if (e->timerId() != timer.timerId())
//        return;
//    timer.start(AUTO_REPEAT_INTERVAL, this);
//    bool left = buttonRect(true).contains(downpos);
//    if ((!left && !buttonRect(false).contains(downpos)) || !buttonEnabled(left))
//    {
//        timer.stop();
//        return;
//    }
//
//    if (!buttonRect(left).contains(mousepos))
//        return;
//
//    if (left)
//        scrollpos = std::max(0, scrollpos - itemWidth() / 2);
//    else
//        scrollpos = std::min(maxscrollpos, scrollpos + itemWidth() / 2);
//    update();
//    if (!buttonEnabled(left))
//        timer.stop();
//}


//-------------------------------------------------------------

