/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZITEMSCROLLER_H
#define ZITEMSCROLLER_H

#include <QStylePainter>
#include <QStyleOption>
//#include <QBasicTimer>
#include "zscrollarea.h"

// Displays kanji elements and kanji drawn with the element painter or kanji font.
// Pass a list to setItems() to set the items shown in the scroller. Negative values are
// indexes to a kanji element obtained with the following formula:
// -1 * ([element index] + 1)
// When painting an element, if the element has kanji or unicode character set, it's drawn
// as text.
class KanjiScrollerModel : public QObject
{
    Q_OBJECT
signals:
    // Emitted by the derived class when the items listed are replaced.
    void changed();
public:
    typedef size_t  size_type;

    KanjiScrollerModel(QObject *parent = nullptr);
    virtual ~KanjiScrollerModel();

    int items(int index) const;
    void setItems(const std::vector<int> &items);

    // Sets a background color for the scroller that is only used when derived classes don't
    // provide their own version of bgColor().
    void setBgColor(QColor col);

    // Background color for each item. When called with -1, should return background color for
    // the rest of the item scroller.
    virtual QColor bgColor(int index) const;
    virtual QColor textColor(int index) const;

    size_type size() const;
private:
    std::vector<int> list;

    QColor bgcolor;

    typedef QObject  base;
};

class SimilarKanjiScrollerModel : public KanjiScrollerModel
{
    Q_OBJECT
public:
    SimilarKanjiScrollerModel(QObject *parent = nullptr);
    virtual ~SimilarKanjiScrollerModel();

    void setItems(const std::vector<int> &items, int categlimit);
    virtual QColor textColor(int index) const override;
private:
    int limit;

    typedef KanjiScrollerModel  base;
};

class CandidateKanjiScrollerModel : public KanjiScrollerModel
{
    Q_OBJECT
public:
    CandidateKanjiScrollerModel(QObject *parent = nullptr);
    virtual ~CandidateKanjiScrollerModel();

    virtual QColor bgColor(int index) const override;
private:
    typedef KanjiScrollerModel  base;
};

class Dictionary;

class ZItemScroller : public ZScrollArea
{
    Q_OBJECT

public:
    ZItemScroller(QWidget *parent = nullptr);
    ~ZItemScroller();

    KanjiScrollerModel* model() const;
    void setModel(KanjiScrollerModel *newmodel);

    Dictionary* dictionary() const;
    void setDictionary(Dictionary *d);

    int cellSize() const;
    void setCellSize(int newsiz);

    virtual int scrollMin() const override;
    virtual int scrollMax() const override;
    virtual int scrollPage() const override;
    virtual int scrollStep() const override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
signals:
    void itemClicked(int index);
protected:
    virtual void scrolled(int oldpos, int &newpos) override;

    virtual void paintEvent(QPaintEvent *e) override;
    //virtual void resizeEvent(QResizeEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    //virtual void timerEvent(QTimerEvent *e) override;

    virtual void contextMenuEvent(QContextMenuEvent *e) override;
private slots:
    void modelChanged();
private:
    // Paints the left or right button, depending on the value of left.
    //void paintButton(QStylePainter &p, QStyleOptionViewItem &opts, bool left);
    // Number of items in model, if model is not null. Otherwise returns 0.
    //int modelSize() const;
    // Width of a single item in the scroller in pixels.
    //int itemWidth() const;
    // Returns the rectangle for the left or right buttons.
    //QRect buttonRect(bool left);

    // Returns the rectangle of the item at index.
    QRect itemRect(int index);
    // Returns the index of the item at the given position, or -1 if no item
    // was found there.
    int itemAt(const QPoint &pt);
    // Returns whether the left or right button is enabled or not.
    //bool buttonEnabled(bool left);
    // Changes the value of mouseitem to the new index and updates the widget.
    // If forceupdate is set to true, the rectangle of the new index is
    // updated even if the mouseitem was the same before.
    void setMouseItem(int index, bool forceupdate = false);

    // Paints a single cell at rect with painter p. Index is the Nth item to paint.
    void paintCell(QPainter &p, const QRect &rect, bool selected, int index) const;

    // Dictionary for the kanji tooltip or kanji info.
    Dictionary *dict;

    // Model serving the number of items to be displayed and drawing them on
    // request.
    KanjiScrollerModel *m;

    // Width and height of a single cell in the scroller.
    int cellsize;

    // True if items are listed and scrolled vertically.
    //bool vert;
    // Current scroll position in scroller in pixels.
    //int scrollpos;
    // Maximum scroll position in scroller in pixels.
    //int maxscrollpos;
    // Position of the mouse when it's over the area of the scroller or (-1,-1).
    //QPoint mousepos;

    // Index of item the mouse is over on or -1.
    int mouseitem;

    // Kanji character in the global kanjis list which was shown in the last tooltip.
    int kanjitipkanji;

    // Position of the mouse when the button was pressed, or (-1, -1) if the
    // mouse button is not pressed.
    //QPoint downpos;

    //QBasicTimer timer;

    typedef ZScrollArea base;
};

#endif // ZITEMSCROLLER_H
