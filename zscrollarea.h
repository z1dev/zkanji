/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZSCROLLAREA_H
#define ZSCROLLAREA_H

#include <QtCore>
#include <QFrame>
#include <QBasicTimer>

// Widget with a self painted scroll bar. Unlike the QAbstractScrollArea, it doesn't use a
// container widget. Derived classes must call the base painting and mouse/keyboard events,
// and should not paint over the scrollbar area or scroll buttons.
// Drawing in derived classes should be only done on the part of the widget returned by
// drawArea(). When handling mouse events, if the mouse button was not pressed over the
// drawArea() part of the widget, call the ZScrollArea implementation first, and check the
// event's isAccepted() flag. If the event was accepted, the scroll area handled the event.
// Otherwise this flag must be set to accepted in derived classes. When the contents of the
// derived class changes, it should call contentsReset, both to repaint itself and to notify
// the scroll area that the scroll bar limits might have changed.
// When the user scrolls the widget, the scrolled function is called. To manually set the
// scroll position, call setScrollPos().
class ZScrollArea : public QFrame
{
    Q_OBJECT
public:
    enum ScrollerType { Scrollbar, Buttons };
    enum ScrollerOrientation { Horizontal, Vertical };

    ZScrollArea(QWidget *parent = nullptr);
    ~ZScrollArea();

    // Recalculates the scroll area and updates the widget. Derived classes can use it to
    // update their state before the recalculation and paint.
    virtual void reset();

    ScrollerType scrollerType() const;
    void setScrollerType(ScrollerType newtype);

    // Whether to automatically scroll when the mouse hovers over a scroll button. Only works
    // when scrollerType() is Buttons.
    bool hoverScroll() const;
    // Set to automatically scroll when the mouse hovers over a scroll button. Only works when
    // scrollerType() is Buttons.
    void setHoverScroll(bool val);

    // Returns the current scroller position.
    int scrollPos() const;

    // Set the scroll position to newpos. The passed value is clamped between scrollMin() and
    // scrollMax() - scrollPage().
    void setScrollPos(int newpos);

    // Minimum scroll position. The scroll bar will be at the left side or top at this
    // position.
    virtual int scrollMin() const = 0;

    // Maximum scroll position. The scroll bar will be at the right side or bottom when it
    // reaches scrollMax() - scrollPage() position.
    virtual int scrollMax() const = 0;

    // Visible part of contents in the same units as scrollMin() and scrollMax().
    virtual int scrollPage() const = 0;

    // Amount to scroll when scrolling with buttons. Ignored when type is not Buttons.
    virtual int scrollStep() const = 0;

    // Height or width taken up by the scrollbar from the widget's rectangle.
    int scrollerSize() const;
protected:
    virtual bool event(QEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void wheelEvent(QWheelEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;

    virtual void timerEvent(QTimerEvent *e) override;

    // The part of the widget available for drawing in derived classes.
    QRect drawArea() const;

    // Call when the contents of the derived widget has changed. Repaints the widget, setting
    // the scrollpos to newpos if it's set. The position will be clamped between scrollMin()
    // and scrollMax() - scrollPage().
    void contentsReset(int newpos = std::numeric_limits<int>::min());

    // Called when the widget's scroll position changes. By default only the scroll area is
    // repainted. Newpos can be modified but it will be clamped between min and max.
    virtual void scrolled(int oldpos, int &newpos) = 0;
private:
    void updateFrameRect();

    // Set the scroll position to newpos. The passed value is clamped between scrollMin() and
    // scrollMax() - scrollPage(). If a QWheelEvent is found, its keyboard modifiers and
    // position are passed to the possibly invoked mouseMoveEvent().
    void setScrollPos(int newpos, QWheelEvent *e);

    // Draws a round triangle fitting inside r. Set first to true if the triangle should point
    // left or up, and false for right or down.
    void drawButton(QPainter &p, QRect r, double padding, ScrollerOrientation ori, bool first);

    // Returns the left/top and right/bottom side of the visible scrollbar in widget client
    // coordinates.
    std::pair<int, int> scrollPositions(QRect contentsrect) const;

    // Returns the rectangle where the scroll bar can be drawn when type is Scrollbar.
    QRect scrollArea() const;

    // Returns the widget coordinates for the button which is currently down or hovered.
    QRect buttonArea() const;

    // Reacts to user mouse clicks that result in page up or page down. Sets dragging to false
    // if the scroll area is now under the mouse coordinate where the click started.
    void page();

    // Redraws the scrollbar or current button area.
    void updateScroller();

    // Calls update on the area of the two scroller buttons.
    void updateButtons();

    // Checks whether the mouse pointer is over the button which started scrolling, and if yes,
    // modifies the scroll position and notifies the derived classes of it.
    void buttonScroll();

    QFrame *widget;

    // Current scroll position between scrollMin() and scrollMax() - scrollPage()
    int scrollpos;

    // Height or width of the sroll bar / scroll button.
    int scrollsize;

    ScrollerType type;

    ScrollerOrientation orientation;

    // The left mouse button was pressed above the scroll bar or button.
    bool dragging;

    // The mouse cursor is positioned above the scroll bar or scroll button.
    bool hovering;

    // Auto scroll area when in buttons mode and the cursor hovers above a button. User
    // settable.
    bool hoverscroll;

    // The point relative to the scroll bar's top or left when dragging started. For buttons,
    // the point is relative to the widget's left or top side where the buttons are. If the
    // position lies right or below the area, the forward button was pressed and not the back
    // button.
    int dragpos;

    QBasicTimer pagetimer;
    QBasicTimer buttontimer;

    typedef QFrame base;
};

#endif // ZSCROLLAREA_H
