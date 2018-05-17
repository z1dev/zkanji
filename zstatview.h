/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZSTATVIEW_H
#define ZSTATVIEW_H

#include <QAbstractScrollArea>
#include <vector>
#include "zkanjimain.h"

class ZAbstractStatModel;
class ZStatView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    ZStatView(QWidget *parent = nullptr);
    virtual ~ZStatView();

    ZAbstractStatModel* model() const;
    void setModel(ZAbstractStatModel *model);
    void setMargins(int leftmargin, int topmargin, int rightmargin, int bottommargin);

    // Minimum spacing between two ticks on the vertical axis.
    int tickSpacing() const;
    // Set the minimum spacing between two ticks on the vertical axis. Can't be smaller than
    // the font metrics line height.
    void setTickSpacing(int val);

    // Scroll the statistics view to display the passed column at the center of the view.
    void scrollTo(int column);

    // Returns the index of the column drawn at the passed x coordinate or -1 when it is not
    // inside the drawn area or no column is found there.
    int columnAt(int x) const;
protected:
    virtual bool event(QEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

    virtual void changeEvent(QEvent *e) override;
    virtual bool viewportEvent(QEvent *e) override;
    virtual void scrollContentsBy(int dx, int dy) override;
    virtual void paintEvent(QPaintEvent *e) override;
private slots:
    void onModelChanged();
private:
    void paintBar(QPainter &p, int col, ZRect r);

    // Rectangle used for displaying the statistics bars.
    QRect statRect() const;

    void updateView();

    ZAbstractStatModel *m;

    // Draw the model stretched out to the full width of the view without scrollbars.
    bool stretched;

    // Left, top, right and bottom margins left unpainted.
    int lm, tm, rm, bm;

    // Minimum space between two ticks on the vertical axis.
    int tickspacing;

    // Header text width left of the statistics area.
    int hwidth;

    // Text written on the vertical axis left to the statistics area.
    QString vlabel;
    // Text written on the horizontal axis above the statistics area.
    QString hlabel;

    // X coordinate of each column inside the view.
    std::vector<int> colpos;

    // Tick values and their pixel positions. The last item is the highest.
    std::vector<std::pair<int, int>> ticks;

    typedef QAbstractScrollArea base;
};

#endif // ZSTATVIEW_H
