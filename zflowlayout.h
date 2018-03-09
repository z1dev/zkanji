/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZFLOWLAYOUT_H
#define ZFLOWLAYOUT_H

#include <QLayout>
#include <QtWidgets/QStyle>
#include "smartvector.h"

// Layout that places its items in a flowing manner. Items are placed one after the other in
// the same row as long as they fit. When no more item can fit, the next ones are placed in
// the next row. Fixed policy items never grow. Items with the other policies may grow wide
// (currently ignored too), but items with "Minimal" horizontal policy behave differently.
// "Minimal" policy items only grow in width if they are not on the last row and other items
// that grow were not found.
// Width of growing items can be limited by calling restrictWidth() with the widget managed by
// the item. These items won't grow larger than the set width, unless space is left for them
// after everything was expanded.
// Distribution of the remaining space in rows are equal between items on top of their
// original width.
class ZFlowLayout : public QLayout
{
    Q_OBJECT
public:
    ZFlowLayout(QWidget *parent = nullptr);
    ~ZFlowLayout();

    // Add a fake item at the top of the layout of the passed size for the mode change widget
    // used in the main zkanji windows.
    //void makeModeSpace(const QSize &size);

    // Adds existing item to the flow layout.
    virtual void addItem(QLayoutItem *item) override;

    virtual QLayoutItem* itemAt(int index) const override;
    virtual QLayoutItem* takeAt(int index) override;
    virtual int count() const override;

    virtual Qt::Orientations expandingDirections() const override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSize() const override;
    virtual void setGeometry(const QRect &r) override;

    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int w) const override;

    // Cleares the cached size of the layout.
    virtual void invalidate() override;

    // Restricts the growth of the layout item holding the w widget to width. This restriction
    // is ignored if only restricted width items are found in a row, unless it's the last row.
    // To clear the restriction, pass QWIDGETSIZE_MAX to restrictWidth().
    void restrictWidth(QWidget *w, uint width);
    // Returns the currently set restricted width, or QWIDGETSIZE_MAX if no restriction is
    // set. To clear the restriction, pass QWIDGETSIZE_MAX to restrictWidth().
    uint restrictedWidth(QWidget *w) const;

    int horizontalSpacing() const;
    void setHorizontalSpacing(int spac);
    int verticalSpacing() const;
    void setVerticalSpacing(int spac);

    // Spacing used after specific widgets. If the widget's spacing is not specified,
    // QWIDGETSIZE_MAX is returned.
    uint spacingAfter(QWidget *w) const;
    // Changes the spacing used after the passed widget to val, that can be different than the
    // spacing specified in setHorizontalSpacing(). Setting val to QWIDGETSIZE_MAX will erase
    // the widget's spacing.
    void setSpacingAfter(QWidget *w, uint val);

    // Returns how widgets not as tall as a row are placed vertically. Default alignment is
    // Qt::AlignTop.
    Qt::Alignment alignment() const;
    // Set how widgets not as tall as a row should be placed vertically. Default alignment is
    // Qt::AlignTop. Horizontal alignments and Qt::Baseline are ignored.
    void setAlignment(Qt::Alignment val);
private:
    int smartSpacing(QStyle::PixelMetric pm) const;

    // Computes the size of the layout including the margins for a given r rectangle. The
    // width of the returned size is at most the width of the r rectangle. The height is the
    // necessary size for the width. Caches the result and if the width of the passed
    // rectangle stays the same and no items were modified
    void recompute(const QRect &r, bool update);

    // Helper function for recompute(), for computing the vertical and horizontal spacing
    // between items.
    void computeSpacing(QLayoutItem *first, QLayoutItem *next, int &spacew, int &spaceh, int hs, int vs, const QStyle *style);

    // Helper for recompute(). Returns the iterator of the layout item after 'it' which holds
    // a visible widget. The passed 'it' iterator must point to a valid item and not endit.
    smartvector_iterator<QLayoutItem> nextVisibleIt(smartvector_iterator<QLayoutItem> it, smartvector_iterator<QLayoutItem> endit);

    smartvector<QLayoutItem> list;
    mutable QSize cached;

    // Widths set with restrictWidth(). This map is never cleared unless QWIDGETSIZE_MAX is
    // passed for every previously restricted widget.
    std::map<QWidget*, uint> restrictions;
    // Spacing used after widgets, ignoring the horizontal spacing set for the layout. Only
    // widgets in this map have different spacing.
    std::map<QWidget*, uint> spacingafter;

    // See makeModeSpace().
    //QSize modeSpace;

    bool justify;

    // Spacing between items laid out horizontally.
    int hspac;
    // Spacing between rows of items.
    int vspac;

    Qt::Alignment align;

    typedef QLayout base;
};

#endif // ZFLOWLAYOUT_H
