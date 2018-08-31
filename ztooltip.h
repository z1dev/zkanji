/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZTOOLTIP_H
#define ZTOOLTIP_H

#include <QFrame>
#include <QElapsedTimer>
#include <QVariant>
#include <QBasicTimer>

// Qt looks for a base class called QTipLabel which is inaccessable as it's private, when
// deciding whether to add a drop shadow to windows with the ToolTip window flag. This hack
// makes the moc mark ZToolTip as derived from Qt tooltips.
class QTipLabel : public QWidget
{
    Q_OBJECT
public:
    QTipLabel(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags) {}
    virtual ~QTipLabel() {};
};
    

class ZToolTip : public QTipLabel
{
    Q_OBJECT
public:
    virtual ~ZToolTip();
    // Shows the tooltip at the passed screen position, with the passed content widget to be
    // displayed. The displayer is the widget below the tooltip, and disprect is the rectangle
    // that holds the displayed information. The tooltip is hidden when the mouse is moved out
    // of disprect on displayer or the passed mshideafter milliseconds are over.
    // If msshowafter is -1, the tooltip is shown after the appropriate delay, otherwise the
    // specified delay is used. When this value is 0, the tooltip is shown without fade
    // animation, even if it's enabled in the system. To show the tooltip immediately but with
    // fade, pass 1 in msshowafter.
    static void show(const QPoint &screenpos, QWidget *content, QWidget *displayer, const QRect &disprect, int mshideafter = -1, int msshowafter = -1);
    // Shows the tooltip at the cursor's current screen position, with the passed content
    // widget to be displayed. The displayer is the widget below the tooltip, and disprect is
    // the rectangle that holds the displayed information. The tooltip is hidden when the
    // mouse is moved out of disprect on displayer or the passed mshideafter milliseconds are
    // over.
    // If msshowafter is -1, the tooltip is shown after the appropriate delay, otherwise the
    // specified delay is used. When this value is 0, the tooltip is shown without fade
    // animation, even if it's enabled in the system. To show the tooltip immediately but with
    // fade, pass 1 in msshowafter.
    static void show(QWidget *content, QWidget *displayer, const QRect &disprect, int mshideafter = -1, int msshowafter = -1);
    // Hides the tooltip, showing the fade out animation and removing the tooltip when the
    // animation ends.
    static void startHide();
    // Hides and destroyes the tooltip without showing the animation.
    // Whether a tooltip instance is visible on screan.
    static bool isShown();

    static void hideNow();
protected:
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;
    //virtual bool event(QEvent *e) override;
    virtual void timerEvent(QTimerEvent *e) override;
private:
    ZToolTip(const QPoint &screenpos, QWidget *content, QWidget *owner, const QRect &ownerrect, int mshideafter, int msshowafter, QWidget *parent = nullptr);

    // Shows or hides the tooltip depending on the argument. If the tooltip is to be hidden,
    // it's deleted after the fade ends.
    void startFade(bool show);
    // Sets the position of the widget to be shown at from screenpos passed to the constructor.
    void updatePosition();

    // Only valid to call when already visible. Moves the tooltip to new screen position,
    // deletes the old content and replaces it with the new one.
    void update(const QPoint &screenpos, QWidget *content, QWidget *displayer, const QRect &disprect, int mshideafter);


    static ZToolTip *instance;

    bool animate;

    QWidget *owner;
    QRect ownerrect;
    QPoint showpos;
    int mshideafter;

    QElapsedTimer time;

    QBasicTimer fadetimer;
    QBasicTimer waittimer;
    qint64 fadestart;
    bool fadein;

    typedef QTipLabel base;
};


#endif // ZTOOLTIP_H
