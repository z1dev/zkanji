/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QStylePainter>
#include <QStyleOption>
#include <QToolButton>
#include <QBoxLayout>
#include <QtEvents>
#include <QLayout>

#include "generalsettings.h"

#ifdef Q_OS_WIN
#define min min
#define max max
#pragma comment( lib, "dwmapi.lib" )
#include "dwmapi.h"
#undef min
#undef max
#endif

#ifdef Q_OS_LINUX
#include "stayontop_x11.h"
#endif

#include "zwindow.h"
#include "zevents.h"


//-------------------------------------------------------------

#ifdef Q_OS_LINUX
namespace {
    class ZWindowFilter : public QObject
    {
    public:
        ZWindowFilter() : QObject(qApp) { qApp->installEventFilter(this); }

        static void install()
        {
            if (filter == nullptr)
            {
                zwindowcnt = 1;
                filter = new ZWindowFilter();
            }
            else
                ++zwindowcnt;
        }

        static void uninstall()
        {
            if (zwindowcnt == 0)
                return;

            if (--zwindowcnt == 0)
            {
                delete filter;
                filter = nullptr;
            }
        }
    protected:
        virtual bool eventFilter(QObject *o, QEvent *e) override
        {
            if (o->isWidgetType() && e->type() == QEvent::MouseButtonPress && dynamic_cast<ZWindow*>(((QWidget*)o)->window()) != nullptr)
            {
                ZWindow *w = ((ZWindow*)((QWidget*)o)->window());
                if (!w->isActiveWindow())
                {
                    w->activateWindow();
                    w->raise();
                    w->update();
                }
            }

            return false;
        }
    private:
        static int zwindowcnt;
        static ZWindowFilter *filter;

        typedef QObject base;
    };
    int ZWindowFilter::zwindowcnt = 0;
    ZWindowFilter* ZWindowFilter::filter = nullptr;
}
#endif

//-------------------------------------------------------------

const int POPUP_RESIZE_BORDER_SIZE = 4;


ZWindow::ZWindow(QWidget *parent, Qt::WindowFlags f) : base(parent, Qt::Dialog |
    /*
    #ifdef Q_OS_LINUX
    Qt::X11BypassWindowManagerHint |
    #endif
    */
    Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | f), inited(false), grabside((int)GrabSide::None), grabbing(false), border(BorderStyle::Resizable)
{
    setMouseTracking(true);


#ifdef Q_OS_LINUX
    ZWindowFilter::install();
#endif
}

ZWindow::~ZWindow()
{
#ifdef Q_OS_LINUX
    ZWindowFilter::uninstall();
#endif
}

void ZWindow::setParent(QWidget *newparent)
{
    base::setParent(newparent, windowFlags());
}

void ZWindow::setStayOnTop(bool val)
{
    auto flags = windowFlags();
    bool wasvisible = isVisible();

#ifdef Q_OS_WIN
    if (wasvisible)
        SetWindowPos(reinterpret_cast<HWND>(winId()), val ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    else
#endif

#ifdef Q_OS_LINUX
        if (testAttribute(Qt::WA_NativeWindow))
            x11_window_set_on_top(this, val);
        else
            setWindowFlags(val ? (flags /*| Qt::X11BypassWindowManagerHint*/ | Qt::WindowStaysOnTopHint) : (flags & ~(/*Qt::X11BypassWindowManagerHint |*/ Qt::WindowStaysOnTopHint)));
#else
        setWindowFlags(val ? (flags | Qt::WindowStaysOnTopHint) : (flags & ~Qt::WindowStaysOnTopHint));
#endif

#ifndef Q_OS_WIN
    if (wasvisible)
        show();
#endif
}

void ZWindow::windowInit()
{
    if (inited)
        return;

    inited = true;

    centralWidget()->setMouseTracking(true);

    if (captionWidget() != nullptr)
        captionWidget()->installEventFilter(this);
    switch (border)
    {
    case BorderStyle::Resizable:
        centralWidget()->layout()->setContentsMargins(POPUP_RESIZE_BORDER_SIZE, POPUP_RESIZE_BORDER_SIZE, POPUP_RESIZE_BORDER_SIZE, POPUP_RESIZE_BORDER_SIZE);
        break;
    case BorderStyle::Docked:
        centralWidget()->layout()->setContentsMargins(POPUP_RESIZE_BORDER_SIZE, POPUP_RESIZE_BORDER_SIZE, 2, 2);
        break;
    }
}

bool ZWindow::beingResized() const
{
    return grabside != (int)GrabSide::None;
}

ZWindow::BorderStyle ZWindow::borderStyle() const
{
    return border;
}

void ZWindow::setBorderStyle(BorderStyle style)
{
    if (border == style)
        return;

    border = style;

    switch (border)
    {
    case BorderStyle::Resizable:
        centralWidget()->layout()->setContentsMargins(Settings::scaled(POPUP_RESIZE_BORDER_SIZE), Settings::scaled(POPUP_RESIZE_BORDER_SIZE), Settings::scaled(POPUP_RESIZE_BORDER_SIZE), Settings::scaled(POPUP_RESIZE_BORDER_SIZE));
        if (captionWidget() != nullptr)
            captionWidget()->show();
        break;
    case BorderStyle::Docked:
        centralWidget()->layout()->setContentsMargins(Settings::scaled(POPUP_RESIZE_BORDER_SIZE), Settings::scaled(POPUP_RESIZE_BORDER_SIZE), 2, 2);
        if (captionWidget() != nullptr)
            captionWidget()->hide();
        break;
    }
}

QRect ZWindow::resizing(int /*side*/, QRect r)
{
    return r;
}

QAbstractButton* ZWindow::addCloseButton(QBoxLayout *layout)
{
    QToolButton *closebtn = new QToolButton(this);
    closebtn->setIconSize(QSize(Settings::scaled(18), Settings::scaled(18)));
    closebtn->setIcon(QIcon(":/closex.svg"));
    closebtn->setAutoRaise(true);
    connect(closebtn, &QAbstractButton::clicked, this, &ZWindow::close);
    QBoxLayout *l = layout != nullptr ? layout : captionWidget() != nullptr ? dynamic_cast<QBoxLayout*>(captionWidget()->layout()) : nullptr;
    if (l != nullptr)
    {
        QBoxLayout *bl = new QBoxLayout(QBoxLayout::Direction::LeftToRight);
        QFrame *line = new QFrame(this);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setFrameShape(QFrame::VLine);

        bl->setSpacing(0);
#ifdef Q_OS_MAC
        bl->addWidget(closebtn);
        bl->addWidget(line);
        l->insertLayout(0, bl);
#else
        bl->addWidget(line);
        bl->addWidget(closebtn);
        l->addLayout(bl);
#endif
        l->activate();
        layout->activate();
    }

    return closebtn;
}

void ZWindow::installGrabWidget(QWidget *widget)
{
    if (widget == nullptr || widget == captionWidget() || std::find(grabbers.begin(), grabbers.end(), widget) != grabbers.end())
        return;

    widget->installEventFilter(this);
    widget->setMouseTracking(true);
    grabbers.push_back(widget);
}

void ZWindow::showEvent(QShowEvent *e)
{
    base::showEvent(e);
    if (!inited)
        throw "Call init() in the constructor of the first derived class that returns a valid centralWidget().";
}

void ZWindow::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    opt.rect = rect();
    opt.lineWidth = 1;

    p.drawPrimitive(QStyle::PE_FrameWindow, opt);
}

void ZWindow::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);
    e->accept();

    if (grabside != (int)GrabSide::None || grabbing || e->button() != Qt::LeftButton)
        return;

    QRect r = rect();
    if (e->pos().x() >= Settings::scaled(POPUP_RESIZE_BORDER_SIZE) && e->pos().y() >= Settings::scaled(POPUP_RESIZE_BORDER_SIZE) &&
        (border == BorderStyle::Docked || (e->pos().x() <= r.width() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) && e->pos().y() <= r.height() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE))))
        return;

    int grabbed = 0;
    if (e->pos().x() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5)
        grabbed |= (int)GrabSide::Left;
    if (e->pos().y() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5)
        grabbed |= (int)GrabSide::Top;
    if (border != BorderStyle::Docked && e->pos().x() > r.width() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5)
        grabbed |= (int)GrabSide::Right;
    if (border != BorderStyle::Docked && e->pos().y() > r.height() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5)
        grabbed |= (int)GrabSide::Bottom;

    grabside = grabbed;
    grabpos = e->pos();
    if (grabbed & (int)GrabSide::Right)
        grabpos.setX(r.width() - e->pos().x());
    if (grabbed & (int)GrabSide::Bottom)
        grabpos.setY(r.height() - e->pos().y());
}

void ZWindow::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);
    e->accept();

    if (grabside == (int)GrabSide::None)
        return;

    grabside = (int)GrabSide::None;
}

void ZWindow::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
    QRect r = geometry();
    bool floating = border != BorderStyle::Docked;

    if (!grabbing && grabside == (int)GrabSide::None)
    {
        if ((e->pos().x() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5 && e->pos().y() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5) ||
            (floating && e->pos().x() > r.width() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5 && e->pos().y() > r.height() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5))

        {
            setCursor(Qt::SizeFDiagCursor);

            // The only way to know that the mouse is moved out from the window into a widget
            // is by a global filter.
            qApp->removeEventFilter(this);
            qApp->installEventFilter(this);
        }
        else if (floating && ((e->pos().x() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5 && e->pos().y() > r.height() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5) ||
            (e->pos().x() > r.width() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5 && e->pos().y() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) * 1.5)))

        {
            setCursor(Qt::SizeBDiagCursor);

            // The only way to know that the mouse is moved out from the window into a widget
            // is by a global filter.
            qApp->removeEventFilter(this);
            qApp->installEventFilter(this);
        }
        else if (e->pos().x() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) || (floating && e->pos().x() > r.width() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE)))
        {
            setCursor(Qt::SizeHorCursor);

            // The only way to know that the mouse is moved out from the window into a widget
            // is by a global filter.
            qApp->removeEventFilter(this);
            qApp->installEventFilter(this);
        }
        else if (e->pos().y() < Settings::scaled(POPUP_RESIZE_BORDER_SIZE) || (floating && e->pos().y() > r.height() - Settings::scaled(POPUP_RESIZE_BORDER_SIZE)))
        {
            setCursor(Qt::SizeVerCursor);

            // The only way to know that the mouse is moved out from the window into a widget
            // is by a global filter.
            qApp->removeEventFilter(this);
            qApp->installEventFilter(this);
        }
        else
            unsetCursor();
        //QApplication::processEvents();
        return;
    }

    if (grabside != (int)GrabSide::None)
    {
        int difx = 0;
        int dify = 0;

        if ((int)grabside & (int)GrabSide::Top)
            dify = e->pos().y() - grabpos.y();
        else if ((int)grabside & (int)GrabSide::Bottom)
            dify = e->pos().y() - (r.height() - grabpos.y());

        if ((int)grabside & (int)GrabSide::Left)
            difx = e->pos().x() - grabpos.x();
        else if ((int)grabside & (int)GrabSide::Right)
            difx = e->pos().x() - (r.width() - grabpos.x());

        QSize minsiz = minimumSizeHint();
        if (difx && ((int)grabside & (int)GrabSide::Left))
            r.setLeft(std::min(r.right() - minsiz.width(), r.left() + difx));
        if (difx && ((int)grabside & (int)GrabSide::Right))
            r.setRight(std::max(r.left() + minsiz.width(), r.right() + difx));
        if (dify && ((int)grabside & (int)GrabSide::Top))
            r.setTop(std::min(r.bottom() - minsiz.height(), r.top() + dify));
        if (dify && ((int)grabside & (int)GrabSide::Bottom))
            r.setBottom(std::max(r.top() + minsiz.height(), r.bottom() + dify));

        setGeometry(resizing(grabside, r));
    }
}

void ZWindow::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    /*centralWidget()->*/unsetCursor();
}

bool ZWindow::event(QEvent *e)
{
#ifdef Q_OS_WIN
    // TODO: (later) check for win xp and skip this then.
    if (e->type() == QEvent::WinIdChange)
    {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        DWMNCRENDERINGPOLICY val = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &val, sizeof(DWMNCRENDERINGPOLICY));

        MARGINS m = { -1 };
        /*HRESULT hr =*/ DwmExtendFrameIntoClientArea(hwnd, &m);
    }
#endif

#ifdef Q_OS_LINUX
    if (e->type() == QEvent::WinIdChange)
    {
        bool r = base::event(e);
        if (windowFlags().testFlag(Qt::WindowStaysOnTopHint))
            x11_window_set_on_top(this, true);
        return r;
    }
#endif

    return base::event(e);
}

bool ZWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (grabside == (int)GrabSide::None && !beingResized() && (e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease) && isGrabWidget(obj))
    {
        QMouseEvent *me = (QMouseEvent*)e;
        switch (me->type())
        {
        case QEvent::MouseMove:
            if (grabbing)
            {
                QRect r = geometry();
                move(r.left() + me->pos().x() - grabpos.x(), r.top() + me->pos().y() - grabpos.y());
            }
            break;
        case QEvent::MouseButtonPress:
            if (grabbing || me->button() != Qt::LeftButton)
                break;
            grabbing = true;
            grabpos = me->pos();
            break;
        case QEvent::MouseButtonRelease:
            if (!grabbing || me->button() != Qt::LeftButton)
                break;
            grabbing = false;
            break;
        default:
            break;
        }
    }

    if (obj == this || e->type() != QEvent::MouseMove || grabside != (int)GrabSide::None || isGrabWidget(obj))
        return base::eventFilter(obj, e);

    // The mouse cursor is not restored by itself from the sizing arrow cursor, if the cursor
    // is moved into a widget on the popup window. It must be restored manually by a global
    // filter here.

    qApp->removeEventFilter(this);
    /*centralWidget()->*/unsetCursor();

    return base::eventFilter(obj, e);
}

bool ZWindow::isGrabWidget(QObject *obj) const
{
    return obj != nullptr && (obj == captionWidget() || std::find(grabbers.begin(), grabbers.end(), obj) != grabbers.end());
}


//-------------------------------------------------------------

