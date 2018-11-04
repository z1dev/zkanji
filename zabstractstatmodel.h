/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZABSTRACTSTATMODEL_H
#define ZABSTRACTSTATMODEL_H

#include <QObject>

class ZStatView;

// Type of statistics shown in the ZStatView
// Bar: (Default) Bar chart.
// Area: Area chart.
enum class ZStatType { Bar, Area };

class ZAbstractStatModel : public QObject
{
    Q_OBJECT
signals:
    void modelChanged();
public:
    ZAbstractStatModel(QObject *parent = nullptr) : base(parent) { ; }
    virtual ~ZAbstractStatModel() {}

    // Number of data columns in the model.
    virtual int count() const = 0;

    // The highest value on the vertical axis.
    virtual int maxValue() const = 0;

    // Number of values to be shown at a single bar position.
    virtual int valueCount() const = 0;
    // A value at the passed column, at the given value position. The value should refer to
    // the size of the individual bar parts.
    virtual int value(int col, int valpos) const = 0;

    // The lable to be drawn on the axis of the given orientation. If an empty string is
    // returned, the space for the label won't be reserved.
    virtual QString axisLabel(Qt::Orientation ori) const = 0;

    // Tooltip text to be shown for the passed column. If this is an empty string, no tooltip
    // will be shown.
    virtual QString tooltip(int /*col*/) const { return QString(); }

protected:
    // Type of stats this model represents.
    virtual ZStatType type() const = 0;
private:
    friend class ZStatView;
    typedef QObject base;
};

class ZAbstractBarStatModel : public ZAbstractStatModel
{
    Q_OBJECT
public:
    ZAbstractBarStatModel(QObject *parent = nullptr) : base(parent) { ; }
    virtual ~ZAbstractBarStatModel() {}

    // Pixel width of a bar at column col. This should be enough to draw the text under this
    // bar column. Should return -1 for 0th value when the ZStatView must display all bars
    // with equal width, stretching out to the full width of the view.
    virtual int barWidth(ZStatView * /*view*/, int /*col*/) const { return -1; }

    // The label to draw below a specific bar.
    virtual QString barLabel(int col) const = 0;
protected:
    // Type of stats this model represents.
    virtual ZStatType type() const { return ZStatType::Bar; }
private:
    typedef ZAbstractStatModel base;
};

class ZAbstractAreaStatModel : public ZAbstractStatModel
{
    Q_OBJECT
private:
    typedef ZAbstractStatModel base;
public:
    ZAbstractAreaStatModel(QObject *parent = nullptr) : base(parent) { ; }
    virtual ~ZAbstractAreaStatModel() {}

    // Date on the left end of the graph, in millisecs since epoch.
    virtual qint64 firstDate() const = 0;
    // Date on the right end of the graph, in millisecs since epoch.
    virtual qint64 lastDate() const = 0;

    // Date at the given column position. In millisecs since epoch.
    virtual qint64 valueDate(int col) const = 0;
protected:
    // Type of stats this model represents.
    virtual ZStatType type() const { return ZStatType::Area; }
};

#endif // ZABSTRACTSTATMODEL_H
