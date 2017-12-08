/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZABSTRACTSTATMODEL_H
#define ZABSTRACTSTATMODEL_H

#include <QObject>

class ZStatView;

// Type of statistics shown in the ZStatView
// BarScroll: (Default) Bar chart inside a scrolling area.
// BarStretch: Bar chart, with all bars visible and stretched out to the width of the view.
enum class ZStatType { BarScroll, BarStretch };

class ZAbstractStatModel : public QObject
{
    Q_OBJECT
public:
    ZAbstractStatModel(QObject *parent = nullptr) : base(parent) { ; }
    virtual ~ZAbstractStatModel() {}

    // Type of stats this model represents.
    virtual ZStatType type() const { return ZStatType::BarScroll; }

    // Number of data columns in the model.
    virtual int count() const = 0;
    // Pixel width of a bar at column col. This should be enough to draw the text under this
    // bar column. This value is not used when the ZStatView displays all bars with equal
    // width, stretching out to the full width of the view.
    virtual int barWidth(ZStatView *view, int col) const { return -1; }

    // The highest value on the vertical axis.
    virtual int maxValue() const = 0;

    // The lable to be drawn on the axis of the given orientation. If an empty string is
    // returned, the space for the label won't be reserved.
    virtual QString axisLabel(Qt::Orientation ori) const = 0;

    // The label to draw below a specific bar.
    virtual QString barLabel(int col) const = 0;

    // Number of values to be shown at a single bar position.
    virtual int valueCount() const = 0;
    // A value at the passed column, at the given value position. The value should refer to
    // the size of the individual bar parts.
    virtual int value(int col, int valpos) const = 0;

    // Tooltip text to be shown for the passed column. If this is an empty string, no tooltip
    // will be shown.
    virtual QString tooltip(int col) const { return QString(); }
private:
    typedef QObject base;
};

#endif // ZABSTRACTSTATMODEL_H
