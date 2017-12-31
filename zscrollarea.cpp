/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QStylePainter>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include "zscrollarea.h"
#include "zevents.h"
#include "colorsettings.h"
#include "zui.h"

struct ScrollAreaScrolledEvent : public EventTBase<ScrollAreaScrolledEvent>
{
private:
    typedef EventTBase<ScrollAreaScrolledEvent>  base;
public:
    ScrollAreaScrolledEvent(QWheelEvent *e) : base(), e(e) { ; }
    QWheelEvent* wheelEvent() { return e; }
private:
    QWheelEvent *e;
};

//-------------------------------------------------------------


ZScrollArea::ZScrollArea(QWidget *parent) : base(parent), widget(/*new QFrame(this)*/), scrollpos(-1), scrollsize(6), type(Scrollbar), ori(Horizontal), dragging(false), hovering(false), hoverscroll(true)//, btndown(false)
{
    setAttribute(Qt::WA_NoMousePropagation);
    setAttribute(Qt::WA_MouseTracking);
    setAttribute(Qt::WA_OpaquePaintEvent);

    setFrameShape(QFrame::StyledPanel);
    //widget->setFrameShape(QFrame::StyledPanel);
}

ZScrollArea::~ZScrollArea()
{

}

void ZScrollArea::reset()
{
    contentsReset();
}

ZScrollArea::ScrollerType ZScrollArea::scrollerType() const
{
    return type;
}

void ZScrollArea::setScrollerType(ScrollerType newtype)
{
    if (type == newtype)
        return;

    type = newtype;

    updateFrameRect();
    contentsReset();
}

bool ZScrollArea::hoverScroll() const
{
    return hoverscroll;
}

void ZScrollArea::setHoverScroll(bool val)
{
    hoverscroll = val;
}

int ZScrollArea::scrollPos() const
{
    return scrollpos;
}

void ZScrollArea::setScrollPos(int newpos)
{
    setScrollPos(newpos, nullptr);
}

int ZScrollArea::scrollerSize() const
{
    return scrollsize;
}

bool ZScrollArea::event(QEvent *e)
{
    if (e->type() == ScrollAreaScrolledEvent::Type())
    {
        ScrollAreaScrolledEvent *se = (ScrollAreaScrolledEvent*)e;
        if (underMouse())
        {
            if (se->wheelEvent() != nullptr)
                qApp->postEvent(this, new QMouseEvent(QEvent::MouseMove, se->wheelEvent()->pos(), Qt::NoButton, se->wheelEvent()->buttons(), se->wheelEvent()->modifiers()));
            else
            {
                QPoint p = mapFromGlobal(QCursor::pos());
                qApp->postEvent(this, new QMouseEvent(QEvent::MouseMove, p, Qt::NoButton, Qt::MouseButtons(), Qt::KeyboardModifiers()));
            }
        }

        return true;
    }

    return base::event(e);
}

void ZScrollArea::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    QStylePainter painter(this);

    QRect r = rect();
    QPalette pal = qApp->palette();
    bool wndactive = isActiveWindow();

    if (type == Buttons)
    {
        QStyleOptionToolButton opt;
        opt.initFrom(this);
        opt.icon = QIcon();
        opt.arrowType = Qt::NoArrow;
        opt.features = QStyleOptionToolButton::None;
        opt.state |= QStyle::State_Enabled | QStyle::State_AutoRaise | QStyle::State_Raised;
        opt.subControls = QStyle::SC_ToolButton;
        opt.activeSubControls = QStyle::SC_None;
        opt.toolButtonStyle = Qt::ToolButtonIconOnly;

        QPalette::ColorGroup grp = scrollpos == scrollMin() ? QPalette::Disabled : isActiveWindow() ? QPalette::Active : QPalette::Inactive;
        QColor textcol = pal.color(grp, QPalette::Text);
        if (ori == Horizontal)
        {
            painter.fillRect(QRect(r.left(), r.top(), scrollsize, r.height()), pal.color(isActiveWindow() ? QPalette::Active : QPalette::Inactive, QPalette::Window));
            if (grp != QPalette::Disabled && hovering && dragpos < scrollsize)
            {
                opt.rect = QRect(r.left(), r.top(), scrollsize, r.height());
                opt.pos = opt.rect.topLeft();
                painter.drawComplexControl(QStyle::CC_ToolButton, opt);
            }

            painter.setBrush(textcol);
            drawButton(painter, QRect(r.left(), r.top(), scrollsize, r.height()), scrollsize * 0.2, ori, true);

            grp = scrollpos < scrollMax() - scrollPage() ? QPalette::Active : QPalette::Disabled;
            textcol = pal.color(grp, QPalette::Text);

            painter.fillRect(QRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height()), pal.color(isActiveWindow() ? QPalette::Active : QPalette::Inactive, QPalette::Window));
            if (grp != QPalette::Disabled && hovering && dragpos >= scrollsize)
            {
                opt.rect = QRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height());
                opt.pos = opt.rect.topLeft();
                painter.drawComplexControl(QStyle::CC_ToolButton, opt);
            }

            painter.setBrush(textcol);
            drawButton(painter, QRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height()), scrollsize * 0.2, ori, false);
        }
        else
        {
            painter.fillRect(QRect(r.left(), r.top(), r.width(), scrollsize), pal.color(isActiveWindow() ? QPalette::Active : QPalette::Inactive, QPalette::Window));
            if (grp != QPalette::Disabled && hovering && dragpos < scrollsize)
            {
                opt.rect = QRect(r.left(), r.top(), r.width(), scrollsize);
                opt.pos = opt.rect.topLeft();
                painter.drawComplexControl(QStyle::CC_ToolButton, opt);
            }

            painter.setBrush(textcol);
            drawButton(painter, QRect(r.left(), r.top(), r.width(), scrollsize), scrollsize * 0.15, ori, true);

            grp = scrollpos < scrollMax() - scrollPage() ? QPalette::Active : QPalette::Disabled;
            textcol = pal.color(grp, QPalette::Text);

            painter.fillRect(QRect(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize), pal.color(isActiveWindow() ? QPalette::Active : QPalette::Inactive, QPalette::Window));
            if (grp != QPalette::Disabled && hovering && dragpos >= scrollsize)
            {
                opt.rect = QRect(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize);
                opt.pos = opt.rect.topLeft();
                painter.drawComplexControl(QStyle::CC_ToolButton, opt);
            }

            painter.setBrush(textcol);
            drawButton(painter, QRect(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize), scrollsize * 0.15, ori, false);
        }
        return;
    }

    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    r.adjust(mleft, mtop, -mright, -mbottom);

    std::pair<int, int> p = scrollPositions(r);
    QColor bgcol = Settings::textColor(wndactive, ColorSettings::ScrollBg);
    QColor sbcol = Settings::textColor(wndactive, ColorSettings::ScrollHandle);
    if (!dragging && !hovering)
        sbcol = mixColors(bgcol, sbcol, 0.4);

    if (ori == Horizontal)
    {
        painter.fillRect(r.left(), r.top() + r.height() - scrollsize, p.first, scrollsize, bgcol);
        painter.fillRect(r.left() + p.first, r.top() + r.height() - scrollsize, p.second - p.first, scrollsize, sbcol);
        painter.fillRect(r.left() + p.second, r.top() + r.height() - scrollsize, r.width() - p.second, scrollsize, bgcol);
    }
    else
    {
        painter.fillRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, p.first, bgcol);
        painter.fillRect(r.left() + r.width() - scrollsize, r.top() + p.first, scrollsize, p.second - p.first, sbcol);
        painter.fillRect(r.left() + r.width() - scrollsize, r.top() + p.second, scrollsize, r.height() - p.second, bgcol);
    }
}

void ZScrollArea::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);
    e->accept();

    if (drawArea().contains(e->pos()) || (type == Buttons && hoverscroll))
    {
        e->ignore();
        return;
    }

    QRect r = rect();
    if (type == Scrollbar)
    {
        int mleft, mtop, mright, mbottom;
        getContentsMargins(&mleft, &mtop, &mright, &mbottom);
        r.adjust(mleft, mtop, -mright, -mbottom);
    }
    if (!r.contains(e->pos()))
        return;

    QPoint pos = e->pos();
    if (e->button() == Qt::LeftButton)
    {
        dragging = true;
        hovering = false;
        if (type == Buttons)
        {
            dragpos = ori == Horizontal ? pos.x() - r.left() : pos.y() - r.top();
            hovering = buttonArea().contains(pos);
            if (hovering)
            {
                updateScroller();
                buttonScroll();
                buttontimer.start(300, this);
            }
        }
        else
        {

            std::pair<int, int> p = scrollPositions(r);

            if (ori == Horizontal)
                dragpos = pos.x() - r.left() - p.first;
            else
                dragpos = pos.y() - r.top() - p.first;

            // Page up or page down
            if (dragpos < 0 || dragpos > p.second - p.first)
            {
                page();

                if (dragging)
                    pagetimer.start(300, this);
            }
        }
    }
}

void ZScrollArea::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);
    e->accept();

    if (!dragging || e->button() != Qt::LeftButton)
    {
        mouseMoveEvent(e);
        e->ignore();
        return;
    }

    dragging = false;
    hovering = true;

    pagetimer.stop();
    buttontimer.stop();

    mouseMoveEvent(e);
}

void ZScrollArea::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);
    e->accept();

    if (drawArea().contains(e->pos()) || (type == Buttons && hoverscroll))
    {
        e->ignore();
        return;
    }

    // Needed empty handler to avoid event propagation.
}

void ZScrollArea::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);
    e->accept();

    QRect r = rect();

    if (type == Buttons)
    {
        bool changed = false;

        // See if the mouse is still over the correct button, and update it
        // if the result is different from before.
        if (drawArea().contains(e->pos()) || !r.contains(e->pos()) || !buttonArea().contains(e->pos()))
        {
            changed = hovering;
            //btndown = false;
            hovering = false;
        }
        if (!hovering)
        {
            if (!dragging || hoverscroll)
                dragpos = ori == Horizontal ? e->pos().x() - r.left() : e->pos().y() - r.top();
            hovering = buttonArea().contains(e->pos());
            changed = changed || hovering;
            if (hoverscroll && changed && hovering)
            {
                buttonScroll();
                buttontimer.start(300, this);
            }
        }
        if (!hovering)
        {
            e->ignore();
            if (hoverscroll)
                buttontimer.stop();
        }
        if (changed)
            updateButtons();
        return;
    }

    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    r.adjust(mleft, mtop, -mright, -mbottom);

    std::pair<int, int> p = scrollPositions(r);

    // In the middle of page up page down.
    if (dragging && (dragpos < 0 || dragpos > p.second - p.first))
        return;

    int w = r.width();
    int h = r.height();
    int x = e->pos().x() - r.left();
    int y = e->pos().y() - r.top();

    if (ori != Horizontal)
    {
        std::swap(w, h);
        std::swap(x, y);
    }

    // Mouse button is not pressed. Only check if hovering state changed.
    if (!dragging)
    {
        bool shouldhover = x >= p.first && x < p.second && y >= h - scrollsize && y < h;
        if (x >= 0 && x < w && y >= 0 && y < h - scrollsize)
            e->ignore();
        if (hovering != shouldhover)
        {
            hovering = shouldhover;
            updateScroller();
        }
        return;
    }

    //int dragpos2 = x - p.first;

    //if (dragpos2 == dragpos)
    //    return;

    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    int tmpscrollpos = scrollpos;
    //scrollpos = std::max(smin, std::min(smax - std::max(0, spage), scrollpos - int(double(dragpos - dragpos2) / w * (p.second - p.first))));
    scrollpos = std::max(smin, std::min<int>(smax - std::max(0, spage), double(smax - smin + 1) / w * (x - dragpos) ));

    if (tmpscrollpos == scrollpos)
        return;

    scrolled(tmpscrollpos, scrollpos);
    scrollpos = std::max(smin, std::min(smax - std::max(0, spage), scrollpos));

    if (tmpscrollpos == scrollpos)
        return;

    updateScroller();
}

void ZScrollArea::wheelEvent(QWheelEvent *e)
{
    if (!drawArea().contains(e->pos()) && (type != Scrollbar || !scrollArea().contains(e->pos())))
        return;

    e->accept();
    static int delta = 0;
    delta += e->angleDelta().y();

    int steps = delta / 120;
    if (steps == 0)
        return;

    delta -= steps * 120;

    setScrollPos(scrollpos + scrollStep() * -steps * qApp->wheelScrollLines(), e);
}

void ZScrollArea::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);
    e->accept();

    if (!hovering)
    {
        e->ignore();
        return;
    }

    hovering = false;
    updateScroller();

}

void ZScrollArea::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    if (type == Buttons)
    {
        // ZScrollArea shouldn't be resized in the perpendicular direction to scrolling, or
        // the current scroll position will be incorrect when scrolling by pixels, because the
        // button size changes taking away from the useful area.
        if (ori == Horizontal)
            scrollsize = e->size().height() * 0.3;
        else
            scrollsize = e->size().width() * 0.3;

        updateFrameRect();
    }

    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    int tmpscrollpos = std::max(smin, std::min(scrollpos, smax - std::max(0, spage)));
    if (tmpscrollpos != scrollpos)
    {
        scrolled(tmpscrollpos, scrollpos);
        scrollpos = std::max(smin, std::min(scrollpos, smax - std::max(0, spage)));
    }

    int mleft, mtop, mright, mbottom;
    if (type == Buttons)
        mleft = mtop = mright = mbottom = 0;
    else
        getContentsMargins(&mleft, &mtop, &mright, &mbottom);

    int ow = e->oldSize().width() - mleft - mright;
    int oh = e->oldSize().height() - mtop - mbottom;

    int w = e->size().width() - mleft - mright;
    int h = e->size().height() - mtop - mbottom;

    if (type == Buttons)
    {
        if (ori == Horizontal)
        {
            // back button
            update(mleft, mtop, scrollsize, h);
            // next button
            int left = std::min(ow, w) - scrollsize;
            update(mleft + left, mtop, w - left, h);
        }
        else
        {
            // back button
            update(mleft, mtop, w, scrollsize);
            // next button
            int top = std::min(oh, h) - scrollsize;
            update(mleft, mtop + top, w, h - top);
        }
        return;
    }

    if (ori == Horizontal)
    {
        int top = std::min(h, oh) - scrollsize;
        update(mleft, mtop + top, w, h - top);
    }
    else
    {
        int left = std::min(w, ow) - scrollsize;
        update(mleft + left, mtop, w - left, h);
    }

}

void ZScrollArea::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == pagetimer.timerId())
    {
        e->accept();

        page();
        if (!dragging)
        {
            pagetimer.stop();
            return;
        }

        pagetimer.start(100, this);
        return;
    }
    else if (e->timerId() == buttontimer.timerId())
    {
        e->accept();

        if ((!hoverscroll && !dragging) || (hoverscroll && !hovering))
        {
            buttontimer.stop();
            return;
        }

        buttonScroll();

        buttontimer.start(75, this);
        return;
    }

    base::timerEvent(e);
}

QRect ZScrollArea::drawArea() const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    QRect r = rect().adjusted(left, top, -right, -bottom);
    if (type == Scrollbar)
    {
        if (ori == Horizontal)
            r.setBottom(r.bottom() - scrollsize);
        else
            r.setRight(r.right() - scrollsize);
    }
    //else
    //{
    //    if (ori == Horizontal)
    //        r.adjust(scrollsize, 0, -scrollsize, 0);
    //    else
    //        r.adjust(0, scrollsize, 0, -scrollsize);
    //}
    return r;
}

void ZScrollArea::contentsReset(int newpos)
{
    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    int tmpscrollpos = scrollpos;
    if (newpos != std::numeric_limits<int>::min())
        scrollpos = std::max(smin, std::min(newpos, smax - std::max(0, spage)));
    else
        scrollpos = smin;

    if (tmpscrollpos != scrollpos)
    {
        scrolled(tmpscrollpos, scrollpos);
        scrollpos = std::max(smin, std::min(scrollpos, smax - std::max(0, spage)));
    }

    update();

    //if (type == Buttons)
    //{
    //    updateButtons();
    //    return;
    //}

    //updateScroller();
}

void ZScrollArea::updateFrameRect()
{
    switch (type)
    {
    case ScrollerType::Scrollbar:
        setFrameRect(rect());
        break;
    case ScrollerType::Buttons:
        if (ori == ScrollerOrientation::Horizontal)
            setFrameRect(rect().adjusted(scrollsize, 0, -scrollsize, 0));
        else
            setFrameRect(rect().adjusted(0, scrollsize, 0, -scrollsize));
        break;
    }
}

void ZScrollArea::setScrollPos(int newpos, QWheelEvent *e)
{
    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    newpos = std::max(smin, std::min(newpos, smax - std::max(0, spage)));

    if (newpos == scrollpos)
        return;

    std::swap(scrollpos, newpos);
    scrolled(newpos, scrollpos);
    updateScroller();

    qApp->postEvent(this, new ScrollAreaScrolledEvent(e));
}

void ZScrollArea::drawButton(QPainter &p, QRect r, double padding, ScrollerOrientation ori, bool first)
{
    p.save();

    double hpadding = ori == ScrollerOrientation::Horizontal ? padding / 2 : padding;
    double vpadding = ori == ScrollerOrientation::Vertical ? padding / 2 : padding;
    r.adjust(hpadding, vpadding, -hpadding, -vpadding);

    double w = r.width();
    double h = r.height();

    // Rounded corner circle radius.
    const double rad = std::min(std::min(w, h) * 0.4, std::max(w, h) * 0.2) / 2.0;

    QPen pen(p.brush().color());
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidthF(rad);
    p.setPen(pen);

    QPointF pts[3];
    if (ori == Horizontal)
    {
        pts[0] = QPointF(r.left() + 0.5 + (first ? rad : r.width() - rad), r.top() + r.height() / 2.0 + 0.5);
        pts[1] = QPointF(r.left() + 0.5 + (!first ? rad : r.width() - rad), r.top() + rad + 0.5);
        pts[2] = QPointF(r.left() + 0.5 + (!first ? rad : r.width() - rad), r.top() + r.height() - rad - 0.5);
    }
    else
    {
        pts[0] = QPointF(r.left() + r.width() / 2.0 + 0.5, r.top() + 0.5 + (first ? rad : r.height() - rad));
        pts[1] = QPointF(r.left() + rad + 0.5, r.top() + 0.5 + (!first ? rad : r.height() - rad));
        pts[2] = QPointF(r.left() + r.width() - rad - 0.5, r.top() + 0.5 + (!first ? rad : r.height() - rad));
    }

    p.setRenderHint(QPainter::Antialiasing);
    p.drawPolygon(pts, 3);

    p.restore();
}

std::pair<int, int> ZScrollArea::scrollPositions(QRect contentsrect) const
{
    if (type != Scrollbar)
        throw "Only scroll bar scrollers have scroll area.";

    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    QRect r = contentsrect;

    std::pair<int, int> p;

    int w;
    if (ori == Horizontal)
        w = r.width();
    else
        w = r.height();

    p.first = smin == smax ? 0 : int(double(scrollpos - smin) / (smax - smin) * w);
    p.second = smin == smax ? 0 : std::min(w, int(double(scrollpos + spage - smin) / (smax - smin) * w));
    
    return p;
}

QRect ZScrollArea::scrollArea() const
{
    if (type != Scrollbar)
        return QRect();

    QRect r = rect();
    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    r.adjust(mleft, mtop, -mright, -mbottom);

    if (ori == Horizontal)
        return QRect(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize);
    else
        return QRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height());
}

void ZScrollArea::page()
{
    if (type != Scrollbar)
        throw "Only scroll bar scrollers can skip page.";

    if (!dragging)
        return;

    QRect r = rect();
    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    r.adjust(mleft, mtop, -mright, -mbottom);

    QPoint cpos = mapFromGlobal(QCursor::pos());
    std::pair<int, int> p1 = scrollPositions(r);


    int xy = cpos.x() - r.left();
    int yx = cpos.y() - r.top();

    int wh = r.width();
    int hw = r.height();

    if (ori != Horizontal)
    {
        std::swap(xy, yx);
        std::swap(wh, hw);
    }

    // Ignore page up / page down request when mouse is not over the needed
    // side of the scroll bar.
    if ((dragpos < 0 && xy >= p1.first) || (dragpos >= p1.second - p1.first && xy < p1.second) || yx > hw || yx < hw - scrollsize || xy < 0 || xy > wh)
        return;

    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();

    int tmpscrollpos = scrollpos;

    if (dragpos < 0)
        scrollpos = std::max(smin, std::min(smax - std::max(spage, 0), scrollpos - std::max(spage, 1)));
    else
        scrollpos = std::max(smin, std::min(smax - std::max(spage, 0), scrollpos + std::max(spage, 1)));

    std::pair<int, int> p2 = scrollPositions(r);

    if (p1 == p2)
    {
        dragging = false;
        return;
    }

    if (dragpos < 0)
        dragpos += p1.first - p2.first;
    else
        dragpos -= p2.second - p1.second;

    dragging = dragpos < 0 || dragpos > p2.second - p2.first;

    scrolled(tmpscrollpos, scrollpos);
    scrollpos = std::max(smin, std::min(smax - std::max(spage, 0), scrollpos));

    updateScroller();
}

void ZScrollArea::updateScroller()
{
    if (type == Buttons)
    {
        bool up = dragpos < scrollsize;
        update(buttonArea());

        return;
    }

    QRect r = rect();
    int mleft, mtop, mright, mbottom;
    getContentsMargins(&mleft, &mtop, &mright, &mbottom);
    r.adjust(mleft, mtop, -mright, -mbottom);

    int w = r.width();
    int h = r.height();
    if (ori == Horizontal)
        update(r.left(), r.top() + h - scrollsize, w, scrollsize);
    else
        update(r.left() + w - scrollsize, r.top(), w, h);
}

void ZScrollArea::updateButtons()
{
    if (type != Buttons)
        throw "Only call for button type scroller.";

    QRect r = rect();

    if (ori == Horizontal)
    {
        update(r.left(), r.top(), scrollsize, r.height());
        update(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height());
    }
    else
    {
        update(r.left(), r.top(), r.width(), scrollsize);
        update(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize);
    }
    return;

}

QRect ZScrollArea::buttonArea() const
{
    if (type != Buttons)
        throw "Only button scrollers can get the button area.";

    // Check whether it's the up/back or the down/forward button.

    bool up = dragpos < scrollsize;

    QRect r = rect();

    if (ori == Horizontal)
        return up ? QRect(r.left(), r.top(), scrollsize, r.height()) : QRect(r.left() + r.width() - scrollsize, r.top(), scrollsize, r.height());
    else
        return up ? QRect(r.left(), r.top(), r.width(), scrollsize) : QRect(r.left(), r.top() + r.height() - scrollsize, r.width(), scrollsize);

}

void ZScrollArea::buttonScroll()
{
    if (type != Buttons)
        throw "Only button scrollers can scroll with the buttons.";

    if ((!hoverscroll && !dragging) || (hoverscroll && !hovering))
        throw "No button specified when mouse is not pressed.";


    bool up = dragpos < scrollsize;
    int smin = scrollMin();
    int smax = scrollMax();
    int spage = scrollPage();
    int sstep = scrollStep();

    if ((up && scrollpos == smin) || (!up && scrollpos == smax))
        return;

    int tmpscrollpos = scrollpos;
    if (up)
        scrollpos = std::max(smin, scrollpos - std::max(sstep, 1));
    else
        scrollpos = std::max(smin, std::min(smax - std::max(0, spage), scrollpos + std::max(sstep, 1)));

    if (scrollpos != tmpscrollpos)
    {
        scrolled(tmpscrollpos, scrollpos);
        scrollpos = std::max(smin, std::min(smax - std::max(0, spage), scrollpos));
    }
}


//-------------------------------------------------------------

