/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZSTATUSBAR_H
#define ZSTATUSBAR_H

#include <QStatusBar>
#include <QLayout>
#include <vector>

// Type of status widgets added to the status bar.
// * TitleValue : A title label and a value label.
// * TitleDouble : A title label and two value labels next to each other.
// * DoubleValue : No title label and two value labels next to each other.
// * SingleValue : A single value label that can extend horizontally.
enum class StatusTypes { TitleValue, TitleDouble, DoubleValue, SingleValue };

class QSizeGrip;
class ZStatusLayout : public QLayout
{
    Q_OBJECT
public:
    ZStatusLayout(QWidget *parent = nullptr);
    virtual ~ZStatusLayout();

    void createSizeGrip();
    void deleteSizeGrip();
    // Returns true if a sizing grip is created or to be created for this layout.
    bool hasSizeGrip() const;

    // Adds a status widget item to the layout. If this is the top layout, adds it to the
    // child layout instead.
    void add(QWidget *w);

    virtual void addItem(QLayoutItem *item) override;

    virtual QLayoutItem* itemAt(int index) const override;
    virtual QLayoutItem* takeAt(int index) override;
    virtual int count() const override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSize() const override;

    virtual void invalidate() override;
    virtual void setGeometry(const QRect &r) override;

    QSize minimumSize();
private:
    ZStatusLayout(ZStatusLayout *parent);

    // Helper function for recompute(), for computing the vertical and horizontal spacing
    // between items.
    void computeSpacing(QLayoutItem *first, QLayoutItem *next, int &spacew, int hs, const QStyle *style);

    // Moves the widgets to their correct positions on the parent widget.
    void realign(const QRect &r);

    void showTheGrip();

    QList<QLayoutItem*> list;

    // Widget only created for top level layout in the status bar, that holds the status
    // widgets added with add().
    QWidget *contents;

    // A sizing grip when shown. The value is left null if the status bar is not directly on a
    // main window.
    QSizeGrip *grip;
    // The sizing grip hides itself on maximize and needs to be explicitly notified that it
    // should show itself when the status bar is shown in general. When this is set to true,
    // the grip will be notified about it on status bar show.
    bool showthegrip;

    mutable int cacheheight;

    using QLayout::addWidget;

    typedef QLayout base;
};

class ZStatusBar : public QStatusBar
{
    Q_OBJECT
signals:
    void assigned() const;
public:
    typedef size_t  size_type;

    ZStatusBar(QWidget *parent = nullptr);
    ~ZStatusBar();

    // Emits the assigned() signal and clears the status bar. Call if the bar was already used
    // for another widget, and that widget needs to be notified.
    void assignTo(QObject *newbuddy);

    // Number of widgets added to the status bar.
    size_type size() const;

    // Returns true if the size() is 0 and no widgets were added to the status bar.
    bool empty() const;

    // Deletes all previously added widgets from the status bar.
    void clear();

    // Adds a widget to the status bar with a single value label. The label's size is
    // restricted to the passed number of characters. Pass 0 to vsiz to make it extening
    // (mainly when adding the last label that can have a generic long text.)
    // Returns the index of the widget that can be used in latter calls of setValue().
    int add(QString value, int vsiz);

    // Adds a widget to the status bar with a title and a value label. The labels' sizes are
    // restricted to the passed number of characters. Pass 0 to sizes to make them fixed size
    // which depends on the label texts. Set alignright to true to align the text in the value
    // label to the right.
    // Returns the index of the widget that can be used in latter calls of setValue().
    int add(QString title, int lsiz, QString value, int vsiz, bool alignright = false);

    // Changes the text of a value label in the widget added at index position when there was
    // a single value label.
    void setValue(int index, QString str);
    // Returns the text set for the value of the widget at index position when there was a
    // single value label.
    QString value(int index) const;

    // Adds a widget to the status bar with a title, and two value labels. The labels' sizes
    // are restricted to the passed number of characters. Pass 0 to sizes to make them fixed
    // size which depends on the label texts.
    // The first of the two values is always aligned right while the second is left aligned.
    // The spacing between them is smaller than between the title and the first value.
    // Returns the index of the widget that can be used in latter calls of setValues().
    // When passing an empty string for title, no label will be added in its place.
    int add(QString title, int lsiz, QString value1, int vsiz1, QString value2, int vsiz2);

    // Changes the text of the two value labels on the widget at the index position. Only
    // valid if the widget was added by the two values override of add().
    void setValues(int index, QString val1, QString val2);

    void setSizeGripEnabled(bool showing/*, int dummy = 0*/);
protected:
    virtual bool event(QEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
private:
    void addWidget(QWidget *w);

    // Widgets added to the status bar.
    std::vector<std::pair<StatusTypes, QWidget*>> list;

    QObject *buddy;

    typedef QStatusBar  base;
};


#endif // ZSTATUSBAR_H
