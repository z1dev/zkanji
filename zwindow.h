/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZWINDOW_H
#define ZWINDOW_H

#include <QMainWindow>

enum class GrabSide { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8, TopLeft = Top | Left, TopRight = Top | Right, BottomLeft = Bottom | Left, BottomRight = Bottom | Right };

class QAbstractButton;
class QBoxLayout;
class ZWindow : public QMainWindow
{
    Q_OBJECT
public:
    ZWindow(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~ZWindow();

    void setParent(QWidget *newparent);
    void setStayOnTop(bool val);
protected:
    enum class BorderStyle { Resizable, Docked };

    // Call at the end of the constructor of the derived class that returns a valid value for
    // centralWidget(). Calls to scaleWidget() must be used after this, because it sets border
    // sizes.
    virtual void windowInit();

    bool beingResized() const;

    BorderStyle borderStyle() const;
    void setBorderStyle(BorderStyle style);

    // Specifies the screen rectangle where the window will be placed while being resized on
    // side. Side is a combination of values from GrabSide. The default implementation returns
    // r unchanged.
    virtual QRect resizing(int side, QRect r);

    // Return the widget used for grabbing the window around. Returning nullptr is valid when
    // there is no such widget. While docked, the caption widget is hidden. This shouldn't
    // be the central widget, as it disables resizing.
    virtual QWidget* captionWidget() const = 0;

    // Creates a button for closing the window and returns it. If captionWidget() is not null
    // and has a box layout, or a layout is passed (that takes priority) adds the button to
    // the layout as the first or last widget. (First on Mac.)
    QAbstractButton* addCloseButton(QBoxLayout *layout = nullptr);

    // Set another widget beside the captionWidget() which can be used to grab the window.
    // This widget is not hidden when the window is docked.
    void installGrabWidget(QWidget *widget);

    // Should return the widget directly on the form. Usually ui->centralwidget.
    //virtual QWidget* centralWidget() const = 0;

    virtual void showEvent(QShowEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *obj, QEvent *e) override;
private:
    bool isGrabWidget(QObject *obj) const;

    // Set to true when the windowInit() function has been called in a base class. To make
    // sure the init function is called everywhere, showEvent() throws if inited is false.
    bool inited;

    // While the popup window is docked to the window size, the user pressed the mouse button
    // over its resizable edge. Combination of values in GrabSide.
    int grabside;
    // The point relative to the popup window where the user pressed the mouse button for
    // resize.
    QPoint grabpos;

    // Whether the window is being grabbed by its caption.
    bool grabbing;

    BorderStyle border;

    std::vector<QWidget*> grabbers;


    typedef QMainWindow base;
};


#endif // ZWINDOW_H

